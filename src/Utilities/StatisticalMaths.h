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
 Module : StatisticalMaths
 Author : FSC Limited
 Date   : 24 July 1996
 Origin : FTA project,
          FSC Limited Private Venture.
 
 SccsId : @(#)StatisticalMaths.h	1.2 11/08/96
 
 Module for generating permuatations and combinations, and providing
 other statistical maths routines.

 perms and combs generate permutations and combinations, respectively,
 of the integers in the input array, and call a user-provided function
 for each one. All perms/combs of the specified order are generated,
 in no guaranteed order. 
*****************************************************************/

#ifndef PERM_H
#define PERM_H

/*---------------------------------------------------------------
 Routine : vec_print
 Purpose : A test function - may be called by comb or perm
 to print each vector generated.
---------------------------------------------------------------*/
extern void
  vec_print(
    int  *z,         /* IN  - array containing combination */
    int   zi,        /* IN  - length of combination        */
    void *user_dat); /* IN  - user data - not used         */

/*---------------------------------------------------------------
 Routine : combs
 Purpose : Generate combinations, r objects from n.
 (objects are integers in array a, length n)
 f is called for each combination.
 The arguments for f are as for vec_print above.

 This routine is recursive.
---------------------------------------------------------------*/
extern void
  combs(
    int *a,     /* IN  - input array                              */
    int  n,     /* IN  - length of a                              */
    int  r,     /* IN  - length of combinations to take from a    */
    int *z,     /* OUT - array to put combination into            */
    int  zi,    /* IN  - index into z                             */
    void f( int *, int, void * ),   /* ?     function to call for each comb generated */
    void *p);   /* ?     user data for f()                        */

/*---------------------------------------------------------------
 Routine : perms
 Purpose : Generate permutations, r objects from n.
 (objects are integers in array a, length n)
 f is called for each permutation.
 The arguments for f are as for vec_print above.
 
 This routine is recursive.
---------------------------------------------------------------*/
extern void
  perms(
    int *a,     /* IN  - input array                              */
    int  n,     /* IN  - length of a                              */
    int  r,     /* IN  - length of permutations to take from a    */
    int *z,     /* OUT - array to put permutation into            */
    int  zi,    /* IN  - index into z                             */
    void f( int *, int, void * ),   /* ?     function to call for each perm generated */
    void *p);   /* ?     user data for f()                        */

/*---------------------------------------------------------------
 Routine : factorial
 Purpose : Calculate n factorial
---------------------------------------------------------------*/
extern float
  factorial( 
    int n );

/*---------------------------------------------------------------
 Routine : nCr
 Purpose : Calculate number of combinations, r from n
---------------------------------------------------------------*/
extern float
  nCr( 
    int n, 
    int r);

/*---------------------------------------------------------------
 Routine : nPr
 Purpose : Calculate number of permutations, r from n.
---------------------------------------------------------------*/
extern float
  nPr( 
    int n, 
    int r);

/*---------------------------------------------------------------
 Routine : nKr
 Purpose : Calculate sum of nCi, for i = 0 to r.
---------------------------------------------------------------*/
extern float
  nKr( 
    int n, 
    int r);

/*---------------------------------------------------------------
 Routine : set_one_increment
 Purpose : sets static one_increment to n and sets num_calls to 0
---------------------------------------------------------------*/
extern void
  set_one_increment(float n);


#endif
