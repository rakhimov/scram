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
 
 SccsId : @(#)CutSetsByAlgebraicMethod.h	1.1 08/22/96
 
 Origin : FTA project,
          FSC Limited Private Venture.
 
 This module allows the determination of cut sets by the reduction
 of the logical structure of the fault tree into the minimal
 logical structure to represent the tree. This is known as the
 Algebraic method.
***************************************************************/

#ifndef ALGEBRAIC_H
#define ALGEBRAIC_H

#include "treeUtil.h"
#include "boolean.h"
/*---------------------------------------------------------------
 Routine : mcs_algebraic
 Purpose : Generates the minimum cut sets for the given tree
 by the Algebraic method. The order variable allows the cut sets
 to be limited to those whose number of primary events is less
 than or equal to the order.
---------------------------------------------------------------*/
extern BOOL
mcs_algebraic(
    char *filename,
    TREE *tree, 
    int order);

/*---------------------------------------------------------------
 Routine : algebraic_time_estimate
 Purpose : Provides a rough estimate of the time that the
 determination of the minimum cut sets by the Algebraic method
 will take.

 At present no deterministic method of how long the Algebraic
 method will take for any given tree has not been found.
---------------------------------------------------------------*/
extern float
  algebraic_time_estimate(
    TREE *tree,
    int  nbas,
    int  nmin,
    int  nmax );

#endif
