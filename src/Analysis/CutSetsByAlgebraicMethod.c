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
 Module : CutSetsByAlgebraicMethod
 Author : FSC Limited
 Date   : 5/8/96
 
 SccsId : @(#)CutSetsByAlgebraicMethod.c	1.3 02/04/97
 
 Origin : FTA project,
          FSC Limited Private Venture.
 
 This module allows the determination of cut sets by the reduction
 of the logical structure of the fault tree into the minimal
 logical structure to represent the tree. This is known as the
 Algebraic method.

 The Algebraic method recursively determines the cut sets, using the
 Expr module (see expr.[ch]).
***************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "CutSetsByAlgebraicMethod.h" 
#include "fta.h"
#include "event_list.h"
#include "CutSetsCommon.h"
#include "NormalisedBooleanExpressions.h"
#include "item.h"
#include "StringUtilities.h"
#include "util.h"

#include "NativeCutSetsDialog.h"

/*---------------------------------------------------------------
 Routine : eval_cut_sets
 Purpose : Evaluate cut sets of the supplied item (algebraically),
 limiting the cut set determination to the given order.

 This routine works recursively.

 At present, we free each Expr after using it.
 In future, should record it under the intermediate event.
---------------------------------------------------------------*/
static Expr
eval_cut_sets(ITEM *ip, int limit)
{
    Expr       etmp1, etmp2, e1, expr = NULL;
    ITEM      *process_child;

/* clock_t start; */
/* float end; */

    if (ip == NULL) {
        printf("\n*** eval_cut_sets : NULL item pointer ***\n");
        exit(1);
    }

    switch(ip->type){

    case BASIC:
    case UNDEVELOP:
    case EXTERNAL:
    case COND_ANAL:
            expr = ExprCopy(ip->event->expr);       /* make a copy, because the
                                                     * returned value will be
                                                     * be freed, and we need
                                                     * event->expr for use
                                                     * elsewhere in the tree
                                                     */
			/* update working dialog progress bar */
			GenerateCutSetsProgressBarInc();
            break;

    case COND_NOT_ANAL:
            expr = NULL;   /* should never happen */
            printf("eval_cut_sets - ERROR : COND_NOT_ANAL found\n");
            break;

    case OR:
    case XOR:

            process_child = ip->process_child;

         /* If first child is an analysed condition,
          * we must AND it with other children, so
          * store value and save in until the end.
          */
            if (process_child->type == COND_ANAL) {
                e1 = eval_cut_sets(process_child, limit);
                process_child = process_child->process_sibling;
            } else {
                e1 = NULL;
            }

         /* if first child is a non-analysed condition,
          * ignore it.
          */
            if (process_child->type == COND_NOT_ANAL) {
                process_child = process_child->process_sibling;
            }

         /* set expr to first non-NULL child */
            while ( process_child != NULL && expr == NULL) {
                expr = eval_cut_sets(process_child, limit);
                process_child = process_child->process_sibling;
            }

         /* do OR of other children */
            while (process_child != NULL){
                if ( (etmp1 = eval_cut_sets(process_child, limit)) != NULL ) {
                    etmp2 = ExprOR(expr, etmp1);
                    ExprDestroy(expr);
               /**/ ExprDestroy(etmp1);
                    expr = etmp2;
                }
                process_child = process_child->process_sibling;
            }

         /* AND with any COND_ANAL condition */
            if ( e1 != NULL ) {
                etmp2 = ExprAND(expr, e1, limit);
                ExprDestroy(expr);
           /**/ ExprDestroy(e1);
                expr = etmp2;
            }

            break;

    case AND:
    case INHIBIT:
    case PRIORITY_AND:
            process_child = ip->process_child;

         /* if first child is a non-analysed condition,
          * ignore it.
          */
            if (process_child->type == COND_NOT_ANAL) {
                process_child = process_child->process_sibling;
            }

         /* set expr to first non-NULL child */
            while ( process_child != NULL && expr == NULL) {
                expr = eval_cut_sets(process_child, limit);
                process_child = process_child->process_sibling;
            }

         /* do AND of other children */
            while (process_child != NULL){
                if ( (etmp1 = eval_cut_sets(process_child, limit)) != NULL ) {
                    etmp2 = ExprAND(expr, etmp1, limit);
                    ExprDestroy(expr);
              /**/  ExprDestroy(etmp1);
                    expr = etmp2;
                }
                process_child = process_child->process_sibling;
            }

            break;

    case TRANSIN:
            expr = eval_cut_sets(ip->process_child, limit);
            break;

    case TRANSOUT:
      /* IGNORE ??? MPA 21/6/96 */
      expr = NULL; 
      break;

    default:
            printf("\n*** eval_cut_sets : Invalid item type ***\n\n");
            exit(1);
    }

 /* ought to set the expr field of intermediate events
  *    ip->event->expr = expr;
  * but we don't know where these are.  process_child and process_sibling
  * miss them out.
  * when we do this, we must NOT free the etmp1 expressions.
  */

 /* printf("%10s  ", ip->id); ExprPrint(expr); */

    return expr;

} /* eval_cut_sets */

/*---------------------------------------------------------------
 Routine : algebraic_cut_sets
 Purpose : Recursively evaluate the Expr associated with each 
 event in the tree. This process starts at the process_top_item
 and so the processing pointers must have been calculated prior
 to this routine being called.
---------------------------------------------------------------*/
static void
algebraic_cut_sets(TREE *tree, int num_order)
{
    clock_t time1, time2;

	/* initialise working dialog progress bar */
	GenerateCutSetsSetProgressBarMax(tree->num_bas);

 /* start clock */
    time1 = clock();

 /* initialise primary event list */
    initialise_exprs();

 /* evaluate cut sets */
    tree->mcs_expr  = eval_cut_sets(tree->process_top_item, num_order);
    tree->max_order = num_order;

 /* free primary evenmt list expressions */
    free_exprs();

    time2 = clock();

    printf("algebraic_cut_sets : time = %f\n",
           (time2-time1)/(float)CLOCKS_PER_SEC);

} /* algebraic_cut_sets */

/*---------------------------------------------------------------
 Routine : mcs_algebraic
 Purpose : Generates the minimum cut sets for the given tree
 by the Algebraic method. The order variable allows the cut sets
 to be limited to those whose number of primary events is less
 than or equal to the order.
---------------------------------------------------------------*/
BOOL
mcs_algebraic(char *filename, TREE *tree, int order)
{
    FILE *output;
    time_t tp;
    BOOL success = TRUE;
    char *tree_filename;

/* print_item( tree->top_item );  */

 /* if necessary, expand the tree */
    expand_tree(tree);

/* print_process_item( tree->top_item ); */

 if (GenerateCutSetsCheckForInterrupt()) {
   success = FALSE;
   return success;
 }
 /* calculate minimal cut sets by ALGEBRAIC method */
    algebraic_cut_sets(tree, order);

    /* Temporarily print expressions */
/*     ExprPrint(tree->mcs_expr);  */

  if (GenerateCutSetsCheckForInterrupt()) {
   success = FALSE;
   return success;
  }

/* store cut sets on file (only if we have any new ones) */
  /* generate absolute path to fta file */
  tree_filename = SU_Join(tree->path, tree->name);
  file_put_mcs(tree_filename, tree->mcs_expr, tree->max_order);
  strfree(tree_filename);

 /* produce cut sets report file */
    if ( (output = fopen(filename, "w")) == NULL ) {
            printf("Algebraic Cut Sets: unable to open file\n");
    } else {

     /* print header  */
        fprintf(output,
                    "Minimal Cut Sets\n"
                    "================\n\n");

        fprintf(output, "Tree   : %s\n", tree->name);
        time(&tp);
        fprintf(output, "Time   : %s\n", ctime(&tp));
        fprintf(output, "Method : Algebraic\n\n");
        fprintf(output, "No. of primary events = %d\n", tree->num_bas);
        fprintf(output, "Minimal cut set order = 1 to %d\n", tree->max_order);

  if (GenerateCutSetsCheckForInterrupt()) {
   success = FALSE;
   return success;
  }
    /* produce output */
        mcs_print(output, tree->mcs_expr, tree->max_order);
        fclose(output);
    }

  if (GenerateCutSetsCheckForInterrupt()) {
   success = FALSE;
   return success;
  } 
/* get rid of cut sets */
    ExprDestroy(tree->mcs_expr);
    tree->mcs_expr  = NULL;
    tree->mcs_end   = NULL;
    tree->max_order = 0;

  return success;
} /* mcs_algebraic */

/*---------------------------------------------------------------
 Routine : algebraic_time_estimate
 Purpose : Provides a rough estimate of the time that the
 determination of the minimum cut sets by the Algebraic method
 will take.
 
 At present no deterministic method of how long the Algebraic
 method will take for any given tree has not been found.
---------------------------------------------------------------*/
float
  algebraic_time_estimate(
    TREE *tree,
    int  nbas,
    int  nmin,
    int  nmax )
{
  static  float time_est = 0.0;
  clock_t start;

  /* Time how long it takes to traverse the process tree */
  /* Multiply by a factor to calculate how long the algorithm 
     will take */

  if ( nmin > nmax )
    return time_est;
  else {
    if ( !tree->timed ) {
      start = clock();

/*************************
 Start processing to determine how long it will take to
 algebraically process the tree */

/*       traverse_process_item( tree->process_top_item ); */
/*       printf("\n");  */
/* printf("\nGates = %d\n", traverse_process_item( tree->process_top_item ) ); */

      time_est = (float)( ( clock() - start ) / CLOCKS_PER_SEC );
    }
  }

  time_est = time_est;

/*   printf("alg_time_est: time = %.10f\n", time_est ); */

  /* In absence of any means of determining how long it will take to
     calculate for this tree, return max value observed that a tree
     will take for this method */
  time_est = 20.0; 

  tree->timed = TRUE;

  return time_est;

} /* algebraic_time_estimate */

