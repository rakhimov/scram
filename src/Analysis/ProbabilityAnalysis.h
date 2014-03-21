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
 
 SccsId : @(#)ProbabilityAnalysis.h	1.1 08/22/96
 
 Module for performing Quantative analysis of the fault tree. This
 involves generating probabilities for each cut set and the
 probability with which the failure of an event will cause the
 top level event to fail.
*****************************************************************/

#ifndef PROBS_H
#define PROBS_H

#include "treeP.h"
#include "boolean.h"

/*---------------------------------------------------------------
 Routine : calculate_probs
 Purpose : Calculate the probabilities for the tree, based upon
 the minimal cut sets.
---------------------------------------------------------------*/
extern BOOL 
  calculate_probs(
    char *filename,       /* IN - name of report file          */
    TREE *tree,           /* IN - tree                         */
    int   max_order,      /* IN - max order of cut sets to use */
    int   prob_n_terms,   /* IN - number of terms in expansion */
    float unit_time );    /* IN - unit time to be applied      */

/*---------------------------------------------------------------
 Routine : probs_estimate
 Purpose : Estimate time to perform probabilities calculation.
---------------------------------------------------------------*/
extern float            /* RET - time (seconds)                   */
  probs_estimate(
    TREE *tree,         /* IN  - tree                             */
    int num_mcs,        /* IN  - number of cut sets               */
    int min_term,       /* IN  - min number of terms to evaluate  */
    int max_term);      /* IN  - max number of terms to evaluate  */

#endif
