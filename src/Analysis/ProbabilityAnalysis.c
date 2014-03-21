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
 Module : ProbabilityAnalysis
 Author : FSC Limited
 Date   : 24 July 1996
 Origin : FTA project,
          FSC Limited Private Venture.
 
 SccsId : @(#)ProbabilityAnalysis.c	1.4 02/04/97
 
 Module for performing Quantative analysis of the fault tree. This
 involves generating probabilities for each cut set and the
 probability with which the failure of an event will cause the
 top level event to fail.
*****************************************************************/

#include <stdlib.h>
#include <math.h>
#include <time.h>

#include "ProbabilityAnalysis.h"
#include "StatisticalMaths.h"
#include "fta.h" 
#include "basic.h"
#include "event_list.h"
#include "util.h"
#include "bits.h"
#include "NormalisedBooleanExpressions.h"

#include "NativeNumericalProbabilityDialog.h"

#include "BaseTimeEstimate.h" 
/* #include "TimeEstimate.h" */

#include "MemoryUtil.h"

static void
  CleanUpOperations(
    FILE *file,
    float *probs,
    float *cp,
    float *imp,
	BitArray *stop)
{
    fclose(file);
    FreeMemory(probs);
    FreeMemory(cp);
    FreeMemory(imp);
	BitDestroy(stop);

} /* CleanUpOperations */


/*---------------------------------------------------------------
 Routine : calculate_probs
 Purpose : Calculate the probabilities for the tree, based upon
 the minimal cut sets.
---------------------------------------------------------------*/
BOOL
  calculate_probs(
    char *filename,       /* IN - filename to write report to */
    TREE *tree,           /* IN - tree                         */
    int   max_order,      /* IN - max order of cut sets to use */
    int   prob_n_terms,   /* IN - number of terms in expansion */
    float unit_time )     /* IN - unit time factor to be applied */
{
    BitArray *stop = BitCreate(1);  /* 1-bit zero */
    FILE  *file;
    Expr   e;
    Group *g;
    float *probs, *cp, *imp;
    float  p;
    int    num_bas, num_mcs, i, j;
/*     char  *mcs_file; */
/*     int    order; */
    clock_t time1, time2;
    time_t tp;
    BOOL success = TRUE;
	float one_increment /* value for one increment of the progress bar */;

 /* start clock */
    time1 = clock();

    if ( (file = fopen(filename, "w")) == NULL) {
        printf("*** calculate_probs : error opening file\n");
        return FALSE;
    }

/*     printf("calculate_probs()\n"); */

 /* include transfered-in trees and build the primary event list
  *
  * We need to do something different to deal with Common Cause Analysis
  * We don't need the tree, but we do need the primary event list.
  * Need to add the common cause events into the primary event list.
  */
 /* if necessary, expand tree */
    expand_tree(tree);

 /* set probs in BASLIST from the events database */
    set_bas_prob( unit_time );

 /* get number of primary events */
    if ((num_bas = tree->num_bas) == 0) {
fclose( file );
        return FALSE;
    }

  if (GenerateNumericalProbabilityCheckForInterrupt()) {
    success = FALSE;
fclose( file );
    return success;
  }

 /* create array of probabilities of primary events */
    if ( !fNewMemory( (void *)&probs, ( num_bas * sizeof(float) ) ) ) 
    {
        printf("\n*** calculate_probs 1 : malloc failed ***\n");
        exit(1);
    }

    if ( !fNewMemory( (void *)&imp, ( num_bas * sizeof(float) ) ) ) 
    {
        printf("\n*** calculate_probs : malloc failed ***\n");
        exit(1);
    }

 /* fill array */
    get_probs( probs );

 /* get mcs list */
    e = tree->mcs_expr;

    /* num_mcs =  ExprCount(e); */

    /* how many mcs are actually used? */
	num_mcs = ExprCountOrder(tree->mcs_expr, max_order);

	/* make sure that max_term does not exceed number of mcs */
	/* if number of mcs is zero then return FALSE */
	if(num_mcs == 0) {
		return FALSE;
	} else if (prob_n_terms > num_mcs) {
		prob_n_terms = num_mcs;
	}

	/* initialise Working dialog */
	/* most of the cpu time is taken up in the ExprProb() function */
	/* the working dialog is incremented in the combs() function */
	one_increment = 0.0;
	for(i = 1; i <= prob_n_terms; i++) {
		one_increment += nCr(num_mcs, i);
	}

	/* set up progress bar */
	one_increment /= 100.0;
    set_one_increment(one_increment);
	GenerateNumericalProbabilitySetProgressBarMax(100);


/*  ExprPrint(e);   */

 /* print header */
    fprintf(file,
        "Probabilities Analysis\n"
        "======================\n\n");
    fprintf(file, "Tree   : %s\n", tree->name);
    time(&tp);
    fprintf(file, "Time   : %s\n", ctime(&tp));

    fprintf(file, "Number of primary events   = %d\n",   num_bas);
    fprintf(file, "Number of minimal cut sets = %d\n",   num_mcs);
    fprintf(file, "Order of minimal cut sets  = %d\n",   tree->max_order);
    if (max_order < tree->max_order) {
        fprintf(file, "               (order <= %d used)\n\n", max_order);
    } else {
        fprintf(file, "\n");
    }

    fprintf(file, "Unit time span         = %f\n\n",   unit_time);

 /* calculate cut set probabilities - use ALL the cut sets */
    cp = ExprCutsetProbs(e, probs);

    fprintf(file, "Minimal cut set probabilities :\n\n");

    i = 0;
    for(g=e; !BitEquals(g->b, stop); g=g->next) {
        char **fp = BitPara( g->b, 30 );

/*         printf("(%3d) %s %-20s - %E\n", */
/*                i+1, */
/*                BitString(g->b), */
/*                fp[0], */
/*                cp[i]); */
/*  */
/*         for (j = 1; fp[j] != NULL; j++) { */
/*             printf("       %-20s\n", fp[j]); */
/*         } */

        if (GenerateNumericalProbabilityCheckForInterrupt()) {
            success = FALSE;
            CleanUpOperations(
                file,
                probs,
                cp,
                imp,
				stop);
			ParaDestroy(fp);
            return success;
		}

        fprintf(file,
               "%3d   %-30s   %E\n",
               i+1,
               fp[0],
               cp[i]);

        for (j = 1; fp[j] != NULL; j++) {
            fprintf(file,
                   "      %-20s\n", fp[j]);
        }

        ParaDestroy(fp);
        i++;
    }

 /* calculate top level probability  - use only up to max_order cut sets */
    fprintf(file, "\n\n"
                  "Probability of top level event "
                  "(minimal cut sets up to order %d used):\n\n", max_order);

    p = 0;

	

    for (i = 1; i <= prob_n_terms && i <= num_mcs && !GenerateNumericalProbabilityCheckForInterrupt(); i++) {
        float term;
        char *sign, *s, *bound;

        p += (term = ExprProb(e, probs, max_order, i));
        sign       = ((i % 2) ? "+" :  "-" );
        s          = ((i > 1) ? "s" :  " " );
        bound      = ((i % 2) ? "upper" :  "lower" );

        fprintf(file, "%2d term%s   %s%E   = %E (%s bound)\n",
                i, s, sign, fabs(term), p, bound);
    }

    if (prob_n_terms >= num_mcs) {
        fprintf(file, "\nExact value : %E\n", p);
    }

    if (GenerateNumericalProbabilityCheckForInterrupt()) {
        success = FALSE;
        CleanUpOperations(
            file,
            probs,
            cp,
            imp,
			stop);
        return success;
	}

 /* calculate importances of individual events */

    for (j = 0; j < num_bas; j++) {
        imp[j] = 0;
    }

    i = 0;
    for(g=e; !BitEquals(g->b, stop); g=g->next) {
        for (j = 0; j < g->b->n; j++) {
            if ( BitGet(g->b, (g->b->n-1) - j) ) {
               imp[j] += cp[i];
            }
        }
        i++;
    }

    if (GenerateNumericalProbabilityCheckForInterrupt()) {
        success = FALSE;
        CleanUpOperations(
            file,
            probs,
            cp,
            imp,
			stop);
        return success;
	}

    fprintf(file, "\n\nPrimary Event Analysis:\n\n");

    fprintf(file, " Event          "
                  "Failure contrib.    "
                  "Importance\n\n");

    for (i = 0; i < num_bas; i++) {
        char *fs = BasicString(num_bas, i);
        fprintf(file, "%-15s %E            %5.2f%%\n",
                fs, imp[i], 100 * imp[i] / p);
        strfree(fs);
    }

    time2 = clock();

/*     printf("calculate_probs : num_terms = %d : time = %f\n",   */
/*            prob_n_terms, (time2-time1)/(float)CLOCKS_PER_SEC);   */

    CleanUpOperations(
        file,
        probs,
        cp,
        imp,
		stop);
/*     fclose(file); */
/*     FreeMemory(probs); */
/*     free(cp); */
/*     FreeMemory(imp); */

    return ( TRUE );

} /* calculate_probs */

/*---------------------------------------------------------------
 Routine : probs_estimate
 Purpose : Estimate time to perform probabilities calculation.

 FOR each term DO
    estimate += (no. of sub-terms to test) * (time for 1 sub-term)
 END FOR
 
 Time for each sub-term is found to be (CONST_A + CONST_B * i)

 This is all very well, but the constants will be different on
 other machines, or with a heavy load. We need to do some sort of
 timing.
---------------------------------------------------------------*/
#define CONST   0.000009
#define CONST_A 0.0000158
#define CONST_B 0.0000055
float                              /* RET - time (seconds)                   */
  old_probs_estimate(
    TREE *tree,         /* IN  - tree                             */
    int max_order,      /* IN  - number of cut sets               */
    int min_term,       /* IN  - min number of terms to evaluate  */
    int max_term)       /* IN  - max number of terms to evaluate  */
{
    int num_mcs;   /* number of cut sets used */
    float t = 0;
    int i;
/*     TimeEstimate Base; */

 /* find out how many cut sets are actually used */
    num_mcs = ExprCountOrder(tree->mcs_expr, max_order);

    for (i = min_term; i <= max_term; i++) {
/*         t += (CONST_A + CONST_B * i) * nCr(num_mcs, i); */
        t += nCr(num_mcs, i);
    }
/*     return t * tree->num_bas; */
/*     return t * tree->num_bas * ( BaseTimeEstimate() / 2500.0 ); */
/*     return t * tree->num_bas * ( Base.ReferenceEstimate() / 2500.0 ); */
    return t * tree->num_bas * ( ReferenceEstimate() / 2500.0 );

} /* probs_estimate */


/*---------------------------------------------------------------
 Routine : probs_estimate
 Purpose : Estimate time to perform probabilities calculation.

 FOR each term DO
    estimate += (no. of sub-terms to test) * (time for 1 sub-term)
 END FOR
 
 Time for each sub-term is found to be (CONST_A + CONST_B * i)

 This is all very well, but the constants will be different on
 other machines, or with a heavy load. We need to do some sort of
 timing.
---------------------------------------------------------------*/
#define CONST   0.000009
#define CONST_A 0.0000158
#define CONST_B 0.0000055
float                              /* RET - time (seconds)                   */
  probs_estimate(
    TREE *tree,         /* IN  - tree                             */
    int max_order,      /* IN  - number of cut sets               */
    int min_term,       /* IN  - min number of terms to evaluate  */
    int max_term)       /* IN  - max number of terms to evaluate  */
{
    int num_mcs;   /* number of cut sets used */
    float t = 0;
    int i,j;
    Group **index; /* index to the groups   */
	int *z;
    Group  *p;     /* pointer               */
    clock_t time1, time2;
    BitArray *stop = BitCreate(1);  /* 1-bit zero */
	float *probs;
	
	/*     TimeEstimate Base; */

    /* find out how many cut sets are actually used */
    num_mcs = ExprCountOrder(tree->mcs_expr, max_order);

	/* make sure that max_term does not exceed number of mcs */
	/* if number of mcs is zero then return 0 */
	if(num_mcs == 0) {
		return 0.0f;
	} else if (max_term > num_mcs) {
		max_term = num_mcs;
	}

	/* allocate memory required for testing */
    if ( !fNewMemory( (void *)&probs, ( tree->num_bas * sizeof(float) ) ) ) 
    {
        exit(1);
    }
    if ( !fNewMemory( (void *)&index, ( num_mcs * sizeof(Group *) ) ) )
    {
      exit( 1 );
    }
    if ( !fNewMemory( (void *)&z, ( num_mcs * sizeof(int) ) ) )
    {
      exit( 1 );
    }

	/* populate the arrays with default data */
	for(i=0, p=tree->mcs_expr; i<num_mcs; i++, p=p->next) {
		index[i] = p;
		z[i] = i;
	}	

	/* fill the probs array */
    get_probs( probs );


	/* set the static variables to sensible values */
	set_basic_n(tree->num_bas);
	set_prob_term(0.0);	
	set_basic_prob(probs);

	/* The function that takes most of the time is calc_sub_term().
	   Run this function for each number of terms required. Run it
	   enough times for the CPU clock to change. */
    for (i = min_term; i <= max_term; i++) {
		time1 = clock();
		j = 0;
		do {
			calc_sub_term(z, i, index);
			j++;
			time2 = clock();
		} while(time1 == time2);

        t += nCr(num_mcs, i) * (time2 - time1) / j;
    }

    FreeMemory(index);
	FreeMemory(z);
	FreeMemory(probs);

    return t/CLOCKS_PER_SEC;

} /* probs_estimate */


