///////////////////////////////////////////////////////////////////////////////
// 
// OpenFTA - Fault Tree Analysis
// Copyright (C) 2005 FSC Limited
// 
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your 
// option) any later version.
// 
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for 
// more details.
//
// You should have received a copy of the GNU General Public License along 
// with this program; if not, write to the Free Software Foundation, Inc., 
// 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
//
// To contact FSC, please send an email to support@fsc.co.uk or write to 
// FSC Ltd., Cardiff Business Technology Centre, Senghenydd Road, Cardiff,
// CF24 4AY.
//
///////////////////////////////////////////////////////////////////////////////

/*****************************************************************
 Module : MonteCarloSimulation
 Author : FSC Limited
 Date   : 24 July 1996
 Origin : FTA project,
          FSC Limited Private Venture.

 SccsId : @(#)MonteCarloSimulation.c	1.4 02/04/97

 Implement the Monte-Carlo analysis.
*****************************************************************/

#include <stdlib.h>
#include <math.h>
#include <time.h>

#include "MonteCarloSimulation.h"
#include "CutSetsByComboMethod.h"
#include "fta.h"
#include "basic.h"
#include "event_list.h"
#include "util.h"
#include "treeUtil.h"
#include "bits.h"
#include "FailureStatistics.h"
#include "RandomNumbers.h"

#include "NativeMonteCarloDialog.h"

#include "BaseTimeEstimate.h" 
/* #include "TimeEstimate.h" */

#include "MemoryUtil.h"

#include "AssertUtil.h"

ASSERTFILE( __FILE__ )

#define MONTE_MAX_F 0        /* unlimited */

/*---------------------------------------------------------------
 Routine : MonteFileOpen
 Purpose :
---------------------------------------------------------------*/
static BOOL
MonteFileOpen(
			  char *filename,
              FILE ** mcarlo_temp_file )
{
  if ( ( *mcarlo_temp_file = fopen( filename, "w" ) ) == NULL )
  {
    printf( "*** generate_monte_carlo_simulation : error opening file\n" );
    return FALSE;
  }
  else
    return TRUE;

} /* MonteFileOpen */

/*---------------------------------------------------------------
 Routine : MonteFileWriteFailures
 Purpose :
---------------------------------------------------------------*/
static void
MonteFileWriteFailures( 
                       FILE * mcarlo_temp_file,
                       double bas_prob,
                       Fnode * fail,
                       int test_num,
                       int fault_num )
{
  int  i;
  int  j;

  fprintf( mcarlo_temp_file, "Rank   "
          "Failure mode         "
          "Failures  "
          "Estimated Probability         "
          "Importance\n\n" );

  i = 0;
  while ( fail != NULL )
  {
    char **fp = BitPara( fail->b, 20 );

/*         printf( "%3d %s %-20s - %4d  :  %E\n",  */
/*                i,  */
/*                BitString( fail->b ),  */
/*                fp[ 0 ],  */
/*                fail->n,  */
/*                bas_prob * fail->n / test_num );  */

/*         for ( j = 1; fp[ j ] != NULL; j++ ) {  */
/*             printf( "       %-20s\n", fp[ j ] );  */
/*         }  */

    fprintf( mcarlo_temp_file,
            "%3d   %-20s  %-8d  %E ( +/- %E )  %5.2f%%\n",
            i + 1,
            fp[ 0 ],
            fail->n,
            bas_prob * fail->n / test_num,
            bas_prob * sqrt( ( double ) ( fail->n ) ) / test_num,
            100.0 * fail->n / fault_num );

    for ( j = 1; fp[ j ] != NULL; j++ )
    {
      fprintf( mcarlo_temp_file,
              "      %-20s\n", fp[ j ] );
    }

    ParaDestroy( fp );
    fail = fail->prev;
    i++;
  }

} /* MonteFileWriteFailures */

/*---------------------------------------------------------------
 Routine : MonteFileWriteCompressedFailures
 Purpose :
---------------------------------------------------------------*/
static void
MonteFileWriteCompressedFailures( 
                                 FILE * mcarlo_temp_file,
                                 double bas_prob,
                                 Fnode * fail,
                                 int test_num,
                                 int fault_num )
{
  int  i;
  int  j;
  Fnode *p;

  fprintf( mcarlo_temp_file, "\n\nCompressed:\n\n" );

  fprintf( mcarlo_temp_file, "Rank   "
          "Failure mode         "
          "Failures  "
          "Estimated Probability    "
          "Importance\n\n" );

  i = 0;
  p = fail;
  while ( p != NULL )
  {
    char **fp = BitPara( p->b, 20 );

/*         printf( "%3d %s %-20s - %4d  :  %E\n",  */
/*                i,  */
/*                BitString( p->b ),  */
/*                fp[ 0 ],  */
/*                p->n,  */
/*                bas_prob * p->n / test_num );  */

/*         for ( j = 1; fp[ j ] != NULL; j++ ) {  */
/*             printf( "       %-20s\n", fp[ j ] );  */
/*         }  */

    fprintf( mcarlo_temp_file,
            "%3d   %-20s  %-8d  %E ( +/- %E )  %5.2f%%\n",
            i + 1,
            fp[ 0 ],
            p->n,
            bas_prob * p->n / test_num,
            bas_prob * sqrt( ( double ) ( p->n ) ) / test_num,
            100.0 * p->n / fault_num );

    for ( j = 1; fp[ j ] != NULL; j++ )
    {
      fprintf( mcarlo_temp_file,
              "      %-20s\n", fp[ j ] );
    }

    ParaDestroy( fp );
    p = p->prev;
    i++;
  }

} /* MonteFileWriteCompressedFailures */

/*---------------------------------------------------------------
 Routine : MonteFileWriteImportance
 Purpose :
---------------------------------------------------------------*/
static void
MonteFileWriteImportance( 
                         FILE * mcarlo_temp_file,
                         double bas_prob,
                         Fnode * fail,
                         int num_bas,
                         int test_num,
                         float monte_carlo_prob )
{
  int  i;
  int  j;
  float *imp;
  Fnode *p;

  /* calculate importances of individual events */
  if ( !fNewMemory( ( void * ) &imp, ( num_bas * sizeof( float ) ) ) )
  {
    printf( "\n*** calculate_probs : malloc failed ***\n" );
    exit( 1 );
  }

  for ( j = 0; j < num_bas; j++ )
  {
    imp[ j ] = 0;
  }

  p = fail;
  while ( p != NULL )
  {
    for ( j = 0; j < p->b->n; j++ )
    {
      if ( BitGet( p->b, ( p->b->n - 1 ) - j ) )
      {
        imp[ j ] += bas_prob * p->n / test_num;
      }
    }
    p = p->prev;
  }

  fprintf( mcarlo_temp_file, "\n\nPrimary Event Analysis:\n\n" );

  fprintf( mcarlo_temp_file, " Event          "
          "Failure contrib.    "
          "Importance\n\n" );


  for ( i = 0; i < num_bas; i++ )
  {
    char *fs = BasicString( num_bas, i );
    fprintf( mcarlo_temp_file, "%-15s %E            %5.2f%%\n",
            fs, imp[ i ], 100 * imp[ i ] / monte_carlo_prob );
    strfree( fs );
  }

  FreeMemory( imp );

} /* MonteFileWriteImportance */

/*---------------------------------------------------------------
 Routine : CleanUpOperations
 Purpose :
---------------------------------------------------------------*/
static void
  CleanUpOperations(
    FILE *monte_temp_file,
    double *rel,
    EVENT **index,
    BitArray *fault_vec)
{
	if(monte_temp_file) {
	    fclose( monte_temp_file );
	}
    initialise_failures( 0 );
    FreeMemory( rel );
    FreeMemory( index );
    BitDestroy(fault_vec);
} /* CleanUpOperations  */

/*---------------------------------------------------------------
 Routine : MonteFileWrite
 Purpose :
---------------------------------------------------------------*/
static BOOL
MonteFileWrite( 
               FILE * mcarlo_temp_file,
               TREE * tree,
               double bas_prob,
               Fnode * fail,
               int num_bas,
               int test_num,
               int fault_num,
               float unit_time,
               double *rel,
               EVENT ** index,
               int other )
{

  int  nfm;
  time_t tp;
  float monte_carlo_prob;

  monte_carlo_prob = ( bas_prob * fault_num ) / test_num;

  fprintf( mcarlo_temp_file,
          "Monte Carlo Simulation\n"
          "======================\n\n" );

  fprintf( mcarlo_temp_file, "Tree   : %s\n", tree->name );
  time( &tp );
  fprintf( mcarlo_temp_file, "Time   : %s\n", ctime( &tp ) );

  if ( bas_prob == 0.0 )
  {
    fprintf( mcarlo_temp_file,
    "All primary event probabilties are 0.0 - analysis abandoned\n\n" );
    return FALSE; 
  }

  fprintf( mcarlo_temp_file, "Note: Only runs with at least one "
          "component failure are simulated\n\n" );

  fprintf( mcarlo_temp_file, "Number of primary events  = %d\n", num_bas );
  fprintf( mcarlo_temp_file, "Number of tests           = %d\n", test_num );
  fprintf( mcarlo_temp_file, "Unit Time span used       = %f\n\n", unit_time );
  fprintf( mcarlo_temp_file, "Number of system failures = %d\n\n", fault_num );

  /* assume sqrt( n ) uncertainty */
  fprintf( mcarlo_temp_file, "Probability of at least   = %E  ( exact )\n",
          bas_prob );
  fprintf( mcarlo_temp_file, "one component failure\n\n" );

  fprintf( mcarlo_temp_file, "Probability of top event  = %E  ( +/- %E )\n\n",
          monte_carlo_prob,
          monte_carlo_prob / sqrt( ( double ) fault_num ) );

  MonteFileWriteFailures( 
                         mcarlo_temp_file,
                         bas_prob,
                         fail,
                         test_num,
                         fault_num );

/*     printf( "other failures ( mode not recorded ) = %d\n", other );  */

  if ( other > 0 )
  {
    fprintf( mcarlo_temp_file,
            "other failures ( mode not recorded ) %-8d  %E\n",
            other, bas_prob * other / test_num );
  }

  /* Compress the failure modes */
  compress_fail_data2( &fail, &nfm, &other );

  MonteFileWriteCompressedFailures( 
                                   mcarlo_temp_file,
                                   bas_prob,
                                   fail,
                                   test_num,
                                   fault_num );

  MonteFileWriteImportance( 
                           mcarlo_temp_file,
                           bas_prob,
                           fail,
                           num_bas,
                           test_num,
                           monte_carlo_prob );

  return TRUE;

} /* MonteFileWrite */

/*---------------------------------------------------------------
 Routine : generate_monte_carlo_simulation
 Purpose : Do a monte-carlo simulation to obtain an estimate of
 1 ) top event probability
 2 ) individual failure mode probabilities

 In the SLOW method, for each run we simply fail each primary event
 randomly with appropriate probability. For each vector evaluate the tree
 and record the result.

 In the fast method, we wish to avoid considering all the cases where
 there are no primary failure events at all, ie to generate only those
 in which there is at least one failure. We have to be careful. You CAN'T
 generate a "first" event according to their relative probabilities, and
 then independently test for all other failures. The method employed
 is to consider which failure occurs FIRST in the fault vector.
 ( Not first in time, just first in the list ).
 Pick the first one at random ( using the correct probability distribution! )
 and then independently test for the failure of all events occuring AFTER
 it in the vector, using for each its own failure probability.

 An optimisation would be to record the values obtained when evaluating
 the tree for first ... nth order failures. It is probably only worth doing
 for first order.
---------------------------------------------------------------*/
BOOL
generate_monte_carlo_simulation(BOOL testing,
								time_t *test_time,
								char *filename,
                                TREE * tree,
                                int monte_carlo_n,
                                float unit_time )
{
  int  num_bas;
  int  i, j;
  int  test_num = 0, fault_num = 0;
  double *rel;        /* array rel probabilities of being "first"     */
  EVENT **index;        /* index to primary events                        */
  EVENT *bp;        /* pointer to a primary event                     */
  double bas_prob;        /* probability at least one primary event
                           failure */
  FILE *mcarlo_temp_file;
  BitArray *fault_vec;
  /* pointers to fail list   */
  Fnode *fail;
  int  other;        /* 'other' failures        */
  int  nfm;        /* number of failure modes */
  /* importances of primary events */
  float f;
  int  first;
  clock_t time1, time2;
  BOOL success = TRUE;
  float one_increment;
  float current_progress;

/*   TimeEstimate TimeCalculation; */

  ASSERT( ( tree != NULL ) && ( monte_carlo_n > 0 ) && ( unit_time > 0.0 ) );

  /* start clock */
/*   time1 = clock(); */
/*   TimeCalculation.StartTimingPeriod(); */
  StartTimingPeriod();

  if(!testing) {
	if ( !MonteFileOpen( filename, &mcarlo_temp_file ) )
	{
		printf( "*** generate_monte_carlo_simulation : error opening file\n" );
		return FALSE;
	}
  }

  /* if necessary, expand the tree */
  expand_tree( tree );

  if (!testing && GenerateMonteCarloCheckForInterrupt() )
  {
    success = FALSE;
fclose( mcarlo_temp_file );
    return success;
  }

  /* set the probabilities in the primary event list */
  set_bas_prob( unit_time );

  /* get number of primary events */
  if ( ( num_bas = tree->num_bas ) == 0 )
  {
    return FALSE;
  }

  /* malloc arrays */
  if ( !fNewMemory( ( void * ) &rel, ( num_bas * sizeof( double ) ) ) )
  {
    printf( "\n*** generate_monte_carlo_simulation 1 : malloc failed ***\n" );
    exit( 1 );
  }

  if ( !fNewMemory( ( void * ) &index, ( num_bas * sizeof( EVENT * ) ) ) )
  {
    printf( "\n*** generate_monte_carlo_simulation 2 : malloc failed ***\n" );
    exit( 1 );
  }

  fault_vec = BitCreate( num_bas );
  initialise_failures( MONTE_MAX_F );

  if (!testing && GenerateMonteCarloCheckForInterrupt() )
  {
    success = FALSE;
    CleanUpOperations(
        mcarlo_temp_file,
        rel,
        index,
		fault_vec);
    return success;
  }

  /* calculate primary event parameters */
  calc_bas_parms( &bas_prob, rel, index );
/*     printf( "bas_prob = %E\n", bas_prob ); */

#undef SLOW

#ifdef SLOW
  bas_prob = 1.0;
#endif

  if (!testing &&  GenerateMonteCarloCheckForInterrupt() )
  {
    success = FALSE;
    CleanUpOperations(
        mcarlo_temp_file,
        rel,
        index,
		fault_vec);
    return success;
  }

  if (!testing) {
	  /* set up progress bar */
	one_increment = monte_carlo_n;
	one_increment /= 100.0;
	GenerateMonteCarloSetProgressBarMax(100);
	current_progress = 0.0;

  }

  if(testing) {
	time1 = clock();
  }

  for ( i = 0; i < monte_carlo_n; i++ )
  {

    test_num++;
    reset_basic_vals();
    BitSetAll( fault_vec, 0 );

#ifdef SLOW
    first = -1;
#else
    /* generate "first" failure */
    bp = index[ first = ( rand_disc( num_bas, rel ) - 1 ) ];
    bp->val = TRUE;
    BitSet( fault_vec, ( num_bas - 1 ) - first, 1 );
    /* printf( "%s ", bp->id ); */
#endif

	
    if (!testing && GenerateMonteCarloCheckForInterrupt()) {
		
        success = FALSE;
        CleanUpOperations(
            mcarlo_temp_file,
            rel,
            index,
            fault_vec);
        return success;
	}

    /* see if any others fail also */
    for ( j = first + 1; j < num_bas; j++ )
    {
      bp = index[ j ];
      if ( ( f = frand() ) <= bp->prob )
      {
        bp->val = TRUE;
        BitSet( fault_vec, ( num_bas - 1 ) - j, 1 );
        /* printf( "%s ", bp->id ); */
      }
    }
    /* printf( "\n" ); */

    /* evaluate the tree */
    if ( eval_tree( tree->process_top_item ) )
    {
      fault_num++;
      record_failure( fault_vec );
    }
  
    if (!testing) {

		/* update progress bar (if required) */
		current_progress += 1.0f;
		while(current_progress > one_increment) {
		    GenerateMonteCarloProgressBarInc();
            current_progress -= one_increment;
		}
	}
  }

  /* rank_print(); */

  if(testing) {
	  time2 = clock();

	  *test_time = time2 - time1;
	  
	  mcarlo_temp_file = NULL;

	  CleanUpOperations(
	    mcarlo_temp_file,
        rel,
        index,
        fault_vec);

	  return TRUE;
  }


  get_fail_data( &fail, &nfm, &other );

  if ( 
      MonteFileWrite( 
                     mcarlo_temp_file,
                     tree,
                     bas_prob,
                     fail,
                     num_bas,
                     test_num,
                     fault_num,
                     unit_time,
                     rel,
                     index,
                     other ) )
  {

  CleanUpOperations(
      mcarlo_temp_file,
      rel,
      index,
      fault_vec);

/*     fclose( mcarlo_temp_file ); */

    if ( GenerateMonteCarloCheckForInterrupt() )
    {
      success = FALSE;
      return success;
    }

    /* free memory */
/*     initialise_failures( 0 ); */
/*     FreeMemory( rel ); */
/*     FreeMemory( index ); */

/*     time2 = clock(); */

/*     printf( "generate_monte_carlo_simulation : n = %d : time = %f\n", */
/*            monte_carlo_n, ( time2 - time1 ) / ( float ) CLOCKS_PER_SEC ); */
/* printf( "generate_monte_carlo_simulation : n = %d : time = %f\n", */
/*   monte_carlo_n, TimeCalculation.TimeElapsed() ); */

    return ( TRUE );
  }
  else
    return FALSE;

} /* generate_monte_carlo_simulation */

/*---------------------------------------------------------------
 Routine : monte_estimate
 Purpose : Estimate time for a monte-carlo simulation

 Uses the time estimate given by mcs_time_est().
 FIDDLE accounts for the time taken to store failure modes - this is
 not predictable.
---------------------------------------------------------------*/
#define FIDDLE 2.2
float        /* RET - time ( seconds )       */
old_monte_estimate( TREE * tree,        /* IN  - tree                 */
               long int n )        /* IN  - number of runs       */
{
/*   TimeEstimate Base; */
  float estimated_time;

/*     return n * mcs_time_est( tree ) * FIDDLE; */
/*     return n * mcs_time_est( tree ) * (  500 * BaseTimeEstimate()  ); */

  estimated_time = 
/*     n * mcs_time_est( tree ) * ( 1200 * Base.ReferenceEstimate() ); */
    (float)n * mcs_time_est( tree ) * ( (float)1200 * ReferenceEstimate() );

  return estimated_time;

} /* monte_estimate */


/*---------------------------------------------------------------
 Routine : monte_estimate
 Purpose : Estimate time for a monte-carlo simulation

 Uses the time estimate given by mcs_time_est().
 FIDDLE accounts for the time taken to store failure modes - this is
 not predictable.
---------------------------------------------------------------*/
float        /* RET - time ( seconds )       */
monte_estimate( TREE * tree,        /* IN  - tree                 */
               long int n )        /* IN  - number of runs       */
{
/*   TimeEstimate Base; */
  time_t run_time;
  float estimated_time = 0.0;
  int num_iterations, i;
  BOOL success = TRUE;

  num_iterations = 10;

  do {

      estimated_time = 0.0;
	  /* run 10 times and take an average */
      i = 0;
	  do {
	  
          success = generate_monte_carlo_simulation(TRUE,
								&run_time,
								NULL,
                                tree,
                                num_iterations,
                                1.0 );

		  estimated_time += run_time;
          i++;
	  } while (i < 10 && run_time > 0 && success);

      if(run_time <=0 ) {  
         /* if test not long enough then increase the number of iterations */
	     num_iterations *= 2;
	  }

  } while (success && run_time <= 0);
  
  if(success) {
    estimated_time *= n;
	estimated_time /= 10;
	estimated_time /= num_iterations;
	estimated_time /= CLOCKS_PER_SEC;
  } else {
	  estimated_time = 0.0;
  }

  return estimated_time;

} /* monte_estimate */
