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
 Module : FailureStatistics
 Author : FSC Limited
 Date   : 24 July 1996
 Origin : FTA project,
          FSC Limited Private Venture.
 
 SccsId : @(#)FailureStatistics.h	1.1 08/22/96
 
 Module for storing failure statistics

 Single-instance data structure.
 
   use initialise_failures() to initialise with a maximum number of
                             failure modes
 
   use record_failure()      to record each failure event

   use initialise_failures() again to free memory

*****************************************************************/

#ifndef FAIL_H
#define FAIL_H

#include "bits.h"

typedef struct _Fnode {
    BitArray      *b;      /* failure mode vector                  */
    int            n;      /* number of occurances                 */
    struct _Fnode *left;   /* pointer to left child                */
    struct _Fnode *right;  /* pointer to right child               */
    struct _Fnode *next;   /* pointer to next Fnode (ordered by n) */
    struct _Fnode *prev;   /* pointer to prev Fnode (ordered by n) */
} Fnode;

/*---------------------------------------------------------------
 Routine : initialise_failures
 Purpose : Initialise data structures for the recording of 
 failure information.
---------------------------------------------------------------*/
extern void
  initialise_failures( 
    int n ); /* IN - max No failure modes (0 means no limit) */

/*---------------------------------------------------------------
 Routine : record_failure
 Purpose : Record an instance of the failure represented by bit 
 array b
---------------------------------------------------------------*/
extern void
  record_failure(
    BitArray *b);  /* IN - bit array (failure mode) */

/*---------------------------------------------------------------
 Routine : get_fail_data
 Purpose : Return the failure data
---------------------------------------------------------------*/
extern void
  get_fail_data(
    Fnode **h,      /* OUT -  top of list                      */
    int    *n,      /* OUT -  number of failure modes recorded */
    int    *oth );  /* OUT -  number of "other" failures       */

/*---------------------------------------------------------------
 Routine : list_print
 Purpose : Print fail data as a list to stdout
---------------------------------------------------------------*/
extern void
  list_print();

/*---------------------------------------------------------------
 Routine : compress_fail_data
 Purpose : Compress the data to minimal sets
---------------------------------------------------------------*/
extern void
  compress_fail_data( 
    Fnode **h, 
    int *n, 
    int *oth );

#endif

