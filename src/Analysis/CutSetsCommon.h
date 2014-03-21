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
 Module : CutSetsCommon
 Author : FSC Limited
 Date   : 24 July 1996
 Origin : FTA project,
          FSC Limited Private Venture.

 SccsId : @(#)CutSetsCommon.h	1.1 08/22/96

 This module encapsulates the common routines associated with
 minimum cut sets. These are used by both Combo and Algebraic
 methods of cut set production.
*****************************************************************/	

#ifndef CUT_SETS_H
#define CUT_SETS_H

#include <time.h>
#include <stdio.h>

#include "NormalisedBooleanExpressions.h"

/*---------------------------------------------------------------
 Routine : file_get_mcs
 Purpose : Read cut sets from file.

 fta_file is the pathname of the file in which the Tree is saved
 and must not be NULL

 This routine attempts to get the cut sets from the file 
 {Tree name}.mcs

 If no cut sets file exists, or the mcs file is older than the tree
 file, the returned parameters are all set to Null values.
---------------------------------------------------------------*/
extern void
  file_get_mcs(
    char   *fta_file,     /* IN  - name of tree file          */
    char  **mcs_file,     /* OUT - name of file               */
    int    *num_mcs,      /* OUT - number of cut sets in file */
    int    *order,        /* OUT - order of cut sets in file  */
    time_t *mcs_date,     /* OUT - date stamp of file         */
    Expr   *e);           /* OUT - cut sets found             */


/*---------------------------------------------------------------
 Routine : file_put_mcs
 Purpose : Save mcs to file (tree.fta -> tree.mcs)

 fta_file is the pathname of the file in which the Tree is saved
 and must not be NULL
 
 This routine attempts to save the minimum cut sets to the file
 {Tree name}.mcs
---------------------------------------------------------------*/
extern void
  file_put_mcs(
    char *fta_file,   /* IN  - name of tree file  */
    Expr  e,          /* IN  - cut sets           */
    int order);       /* IN  - order              */

/*---------------------------------------------------------------
 Routine : mcs_print
 Purpose : Print the minimum cut set information to the supplied
 file.
---------------------------------------------------------------*/
extern void
  mcs_print( 
    FILE *fp, 
    Expr e, 
    int max_order );


#endif
