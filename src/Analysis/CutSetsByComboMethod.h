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
 
 SccsId : @(#)CutSetsByComboMethod.h	1.1 08/22/96
 
 Origin : FTA project,
          FSC Limited Private Venture.
 
 This module allows the determination of cut sets by testing 
 combinations of basic events to see if they are cut sets. For
 large trees and lots of basic events, this can take a considerable
 time.
***************************************************************/

#ifndef COMBO_H
#define COMBO_H

/* was tree_pic.h but imports TREE from tree.h */
#include "treeP.h"
#include "boolean.h"
/*---------------------------------------------------------------
 Routine : mcs_combo
 Purpose : Generates the minimum cut sets for the given tree
 by the Combo method. The order variable allows the cut sets
 to be limited to those whose number of primary events is less
 than or equal to the order.
---------------------------------------------------------------*/
extern BOOL 
  mcs_combo(
    TREE *tree, 
    int order);

/*---------------------------------------------------------------
 Routine : combo_time_estimate
 Purpose : Provides a rough estimate of the time that the
 determination of the minimum cut sets by the Combo method
 will take.
---------------------------------------------------------------*/
extern float            /* RET - time (seconds)                    */
  combo_time_estimate(
    TREE *tree,         /* IN  - tree                              */
    int nbas,           /* IN  - number of basic events            */
    int nmin,           /* IN  - min order of cut sets to evaluate */
    int nmax );         /* IN  - max order of cut sets to evaluate */

/*---------------------------------------------------------------
 Routine : mcs_time_est
 Purpose : Estimate time to evaluate the tree once (seconds)
 Also used in Monte Carlo time estimating.
---------------------------------------------------------------*/
extern double
  mcs_time_est(
    TREE *tree);

#endif
