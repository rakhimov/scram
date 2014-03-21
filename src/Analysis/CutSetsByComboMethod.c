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

/***************************************************************
 Module : CutSetsByComboMethod
 Author : FSC Limited
 Date   : 5/8/96

 SccsId : @(#)CutSetsByComboMethod.c	1.5 02/04/97

 Origin : FTA project,
          FSC Limited Private Venture.

 This module allows the determination of cut sets by testing
 combinations of primary events to see if they are cut sets. For
 large trees and lots of primary events, this can take a considerable
 time.
***************************************************************/

#include <stdlib.h>

#include "CutSetsByComboMethod.h"
#include "event_list.h"
#include "CutSetsCommon.h"
#include "RandomNumbers.h"
#include "StatisticalMaths.h"
#include "fta.h"

#include "NativeMonteCarloDialog.h"

#include "BaseTimeEstimate.h" 
/* #include "TimeEstimate.h" */

#include "MemoryUtil.h"

/*
 * forward declarations
 */
static BOOL
combo_cut_sets( 
               TREE * tree,
               int order );

static void
eval_comb( 
          int *z,
          int zi,
          TREE * tree );

/* global variables used for progress reports */
static int ncombs;        /* number of combs to test                   */
static int count;        /* number of combs tested                    */
static int percent;        /* percentage complete                       */

/* #define this to use set_basic_vals2 - the 'fast' version
 * otherwise use set_basic_vals
 */
#undef SET_BASIC_VALS2

/*---------------------------------------------------------------
 Routine : mcs_combo
 Purpose : Generates the minimum cut sets for the given tree
 by the Combo method. The order variable allows the cut sets
 to be limited to those whose number of primary events is less
 than or equal to the order.
---------------------------------------------------------------*/
BOOL
mcs_combo( TREE * tree, int order )
{
  FILE *output;
  int  start_order = tree->max_order;
  time_t tp;
  BOOL success = TRUE;

  /* if necessary, expand the tree */
  expand_tree( tree );

  /* calculate minimal cut sets by COMBO method */
  if ( !combo_cut_sets( tree, order ) )
  {
    success = FALSE;
    return success;
  }

/*     ExprPrint( tree->mcs_expr ); */

  /* store cut sets on file ( only if we have any new ones ) */
  if ( order > start_order )
  {
    file_put_mcs( tree->name, tree->mcs_expr, tree->max_order );
  }

  /* produce cut sets report file */
  if ( ( output = fopen( MCS_REPORT_TEMPFILE, "w" ) ) == NULL )
  {
    printf( "Combo Cut Sets: unable to open file\n" );
  }
  else
  {

    /* print header  */
    fprintf( output,
            "Minimum Cut Sets\n"
            "================\n\n" );

    fprintf( output, "Tree   : %s\n", tree->name );
    time( &tp );
    fprintf( output, "Time   : %s\n", ctime( &tp ) );
    fprintf( output, "Method : Combo\n\n" );
    fprintf( output, "No. of primary events = %d\n", tree->num_bas );
    fprintf( output, "Cut set order         = 1 to %d\n", tree->max_order );

    /* produce output */
    mcs_print( output, tree->mcs_expr, tree->max_order );
    fclose( output );
  }

  /* get rid of cut sets */
  ExprDestroy( tree->mcs_expr );
  tree->mcs_expr = NULL;
  tree->mcs_end = NULL;
  tree->max_order = 0;

  return success;
} /* mcs_combo */

/*---------------------------------------------------------------
 Routine : combo_cut_sets
 Purpose : Determines cut sets by generating only those
 combinations that it needs to test - i.e. only those <= the order
 specified by the user.
---------------------------------------------------------------*/
BOOL
combo_cut_sets( TREE * tree, int num_order )
{
  int  start_order;        /* order to start from                          */
  int  order;        /* order of cut sets to generate                */
  int *index;        /* array of indices of primary events             */
  int *combo;        /* generated combination of primary event indices */
  clock_t time1, time2;
  int  i;
  BOOL success = TRUE;
/*   TimeEstimate Timer; */

  /* start clock */
/*   time1 = clock(); */
/*   Timer.StartTimingPeriod(); */
  StartTimingPeriod();

#ifdef SET_BASIC_VALS2
  /* create the bv_index */
  if ( ( nbas = create_basic_vals_index() ) == 0 )
  {
    return;
  }
#else
  if ( tree->num_bas == 0 )
  {
    return FALSE;
  }
#endif

  if ( GenerateMonteCarloCheckForInterrupt() )
  {
    success = FALSE;
    return success;
  }

  /* work out where to start from */
  start_order = tree->max_order + 1;
  if ( tree->mcs_expr == NULL )
  {
    tree->mcs_expr = tree->mcs_end = ExprCreate();
  }

  /* initialise index */
  if ( !fNewMemory( ( void * ) &index, ( tree->num_bas * sizeof( int ) ) ) )
  {
    printf( "\n*** combo_cut_sets : malloc failed ***\n" );
    exit( 1 );
  }
  for ( i = 0; i < tree->num_bas; i++ )
  {
    index[ i ] = i;
  }

  if ( GenerateMonteCarloCheckForInterrupt() )
  {
    success = FALSE;
    return success;
  }

  /* create combo array */
  if ( !fNewMemory( ( void * ) &combo, ( tree->num_bas * sizeof( int ) ) ) )
  {
    printf( "\n*** combo_cut_sets : malloc failed ***\n" );
    exit( 1 );
  }

  if ( GenerateMonteCarloCheckForInterrupt() )
  {
    success = FALSE;
    return success;
  }

  /* generate and test events of all orders <= num_order */
  for ( order = start_order; order <= num_order; order++ )
  {
    ncombs = nCr( tree->num_bas, order );
    count = 0;
    percent = 0;        /* used for progress reports */

/*         printf( "Order %d ( %8d combinations ) Start Time %s :\n", */
/*                 order, ncombs, time_string()  ); */

    /* do eval_comb() for all combinations of %order% elements of
       %index% */
    combs( index, tree->num_bas, order, combo, 0, eval_comb, tree );
  }
  FreeMemory( index );
  FreeMemory( combo );

  if ( GenerateMonteCarloCheckForInterrupt() )
  {
    success = FALSE;
    return success;
  }
  tree->max_order = num_order;

/*   time2 = clock(); */

/*   printf( "combo_cut_sets : num_order = %d : time = %f\n", */
/*          num_order, ( time2 - time1 ) / ( float ) CLOCKS_PER_SEC ); */
/*          num_order, Timer.TimeElapsed() ); */

  return ( TRUE );

} /* combo_cut_sets */

/*---------------------------------------------------------------
 Routine : eval_comb
 Purpose : Evaluates the tree for the given combination. Evaluation
 involves failing a combination of events and evaluating the tree
 to see if the combination will cause the tree to fail, and if the
 cut set is minimal.

 evaluate tree
 IF ( TRUE ) THEN
   IF ( cut set is minimal ) THEN
     add it to the mcs list
   END IF
 END IF
 note:
 This function is time-critical for the evaluation of cut-sets,
 and has been optimised.
---------------------------------------------------------------*/
void
eval_comb( 
          int *z,        /* IN  - array containing combination */
          int zi,        /* IN  - length of combination        */
          TREE * tree )        /* IN  - user data - tree             */
{
  static Group *g = NULL;
  static int n = 0;
  int  done;
  int  i = 0;

  /* evaluate this combination */

#ifdef SET_BASIC_VALS2
  set_basic_vals2( z, zi );
#else
  set_basic_vals( z );
#endif

  if ( eval_tree( tree->process_top_item ) )
  {

    /* create a bit array corresponding to this fault vector - reuse
       old g if the same size */

    if ( n != tree->num_bas )
    {
      n = tree->num_bas;
      if ( g != NULL )
      {
        GroupDestroy( g );
        g = NULL;
      }
    }
    if ( g == NULL )
    {
      g = GroupCreateN( n );
    }
    else
    {
      BitSetAll( g->b, 0 );
    }
    for ( i = 0; i < zi; i++ )
    {
      BitSet( g->b, ( tree->num_bas - 1 ) - z[ i ], 1 );
    }

    /* add to expression */

    /* this could be done with ExprORGroup. ExprCutSet is an
       optimised version */
    tree->mcs_expr =
      ExprCutSet( tree->mcs_expr, tree->mcs_end, g, &done );
    if ( done )
    {
      g = NULL;        /* we need a new one */
    }
  }

  /* report progress */
/*     f = 100 * ++count / ( float ) ncombs; */
/*  */
/*     while (  f  >= ( float )( percent + 1 )  ) { */
/*         percent++; */
/*         if (  ( percent % 10 ) == 0  ) { */
/*             printf( ". %3d%%             -              %s\n", */
/*                    percent, time_string() ); */
/*         } else { */
/*             printf( "." ); */
/*         } */
/*         fflush( stdout ); */
/*     } */

} /* eval_comb */

/*---------------------------------------------------------------
 Routine : mcs_time_est
 Purpose : Estimate time to evaluate the tree once ( seconds )

 Evaluate tree for at least NUM_TEST random inputs, and for
 at least TIME_TEST seconds.
 Remember the value - only do once for each tree
---------------------------------------------------------------*/
#define NUM_TEST 1000
#define TIME_TEST 0.5
double
mcs_time_est( TREE * tree )
{
  static double time_est = 0.0;        /* time estimate             */
  float start;
  int *z;        /* for using eval_comb() */
  float f;
  int  n_test = 0;
  int  i, j, tmp;
/*   TimeEstimate Timer; */

/*   if ( !tree->timed ) */
/*   { */

#ifdef SET_BASIC_VALS2
    /* create the bv_index */
    if ( ( nbas = create_basic_vals_index() ) == 0 )
    {
      return;
    }
#else
    if ( tree->num_bas == 0 )
    {
      return 0.0;
    }
#endif

    if ( !fNewMemory( ( void * ) &z, ( tree->num_bas * sizeof( int ) ) ) )
    {
      exit( 1 );
    }
    for ( j = 0; j < tree->num_bas; j++ )
    {
      if ( ( f = frand() ) <= 0.5 )
      {
        z[ j ] = TRUE;
      }
      else
      {
        z[ j ] = FALSE;
      }
    }
/*     start = ( float ) clock(); */
/*     Timer.StartTimingPeriod(); */
    StartTimingPeriod();

    while ( time_est < TIME_TEST )
    {
      for ( i = 0; i < NUM_TEST; i++ )
      {

        tmp = nrand( tree->num_bas ) - 1;
        z[ tmp ] = !z[ tmp ];

        /* evaluate the tree NOTE: FAST_WAY gives under-estimate by
           factor of 3. WHY ?? does this imply we could optimise
           COMBO by a factor of three ? can we generate combinations
           in Grey code order? ( Numerical Recipies ) */

#ifdef SET_BASIC_VALS2
        set_basic_vals2( z, tree->num_bas );
#else
        set_basic_vals( z );
#endif

        eval_tree( tree->process_top_item );
      }
/*       time_est = ( clock() - start ) / ( float ) CLOCKS_PER_SEC; */
/*       time_est = Timer.TimeElapsed(); */
      time_est = TimeElapsed();
      n_test += NUM_TEST;
    }

/*     printf( "mcs_time_est: time = %f, n_test = %d, time_est = %f\n",  */
/*            time_est, n_test, time_est / n_test );  */
    time_est = time_est / n_test;
    tree->timed = TRUE;
/*   } */

  /* return 0.00036; /* value for typical2.fta */
  return time_est;

} /* mcs_time_est */

/*---------------------------------------------------------------
 Routine : combo_time_estimate
 Purpose : Provides a rough estimate of the time that the
 determination of the minimum cut sets by the Combo method
 will take.

 Estimate = ( no. of combs to test ) * ( time for 1 combination )

 However this does not take into account the time to test new
 failure combinations against previously found cut sets, and add them
 to the list. This will depend upon how many cut sets there are and
 this can not be easily estimated. We take this into account with
 a fiddle factor.
---------------------------------------------------------------*/
/* #define FIDDLE 2.0 */
float        /* RET - time ( seconds )                    */
combo_time_estimate( 
                    TREE * tree, /* IN  - tree                              */
                    int nbas,    /* IN  - number of primary events            */
                    int nmin,    /* IN  - min order of cut sets to evaluate */
                    int nmax )   /* IN  - max order of cut sets to evaluate */
{
  float FIDDLE;
/*   FIDDLE = 40 * BaseTimeEstimate(); */
/*   TimeEstimate Base; */

/*   FIDDLE = 40 * Base.ReferenceEstimate(); */
  FIDDLE = 40 * ReferenceEstimate();

  if ( nmin > nmax )
  {
    return 0;
  }
  else
  {
    return ( nKr( nbas, nmax ) - nKr( nbas, nmin - 1 ) ) * FIDDLE *
      mcs_time_est( tree );
  }
} /* combo_time_estimate */
