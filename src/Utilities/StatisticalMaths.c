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
 
 SccsId : @(#)StatisticalMaths.c	1.2 11/08/96
 
 Module for generating permuatations and combinations, and providing
 other statistical maths routines.
 
 perms and combs generate permutations and combinations, respectively,
 of the integers in the input array, and call a user-provided function
 for each one. All perms/combs of the specified order are generated,
 in no guaranteed order.

 A permuation is a selection of r objects from a set of n objects,
 where the order of objects chosen IS considered significant.
 eg the PERMUTATIONS of 2 elements taken from {1,2,3} are
 
      {1,2}, {1,3}, {2,1}, {2,3}, {3,2}, {3,1}
  
 A combination is a selection of r objects from a set of n objects,
 where the order of objects chosen IS NOT considered significant.
 eg the COMBINATIONS of 2 elements taken from {1,2,3} are
 
      {1,2}, {1,3}, {2,3}

 The number of permuations (r objects from n) is given by the
 function nPr, which takes the value n! / (n-r)!

 The number of combinations (r objects from n) is given by the
 function nCr, which takes the value n! / (n-r)!r!

 The sum of nCi, i = 0 to r is sometime useful, and is here denoted by
 nKr. Note the nCr are the terms of a binomial expansion, and that
 nKn = 2^n.

 In fact, we calculate these using a recurrence relation, because the
 factorials get too big.

 The purpose of comb and perm is to generate ALL combinations and
 permutations (of given order) of the integers in the input array.
 For each one generated, a function provided by the user is called.
 There is no control over the sequence in which perms/combs are
 generated.

 In typical usage, the array generated might be used to index into some
 other array, so that some operation may be performed on all
 permutations or combinations of some set of objects. Both work
 recursively, by choosing an element and then generating a perm/comb
 of the remaining elements.
*****************************************************************/

#include <stdio.h>
#include "StatisticalMaths.h" 
#include "NativeNumericalProbabilityDialog.h"
#include "NativeMonteCarloDialog.h"

static float current_progress = 0.0f;
static float one_increment = 1.0f;

/*---------------------------------------------------------------
 Routine : swap
 Purpose : Swap elements of integer array
---------------------------------------------------------------*/
static void
  swap(
    int *a,    /* IN/OUT - array        */
    int  i,    /* IN     - first index  */
    int  j)    /* IN     - second index */
{
    int tmp = a[i];
    a[i] = a[j];
    a[j] = tmp;

} /* swap */
   
/*---------------------------------------------------------------
 Routine : vec_print
 Purpose : A test function - may be called by comb or perm
 to print each vector generated.
---------------------------------------------------------------*/
void
  vec_print(
    int  *z,         /* IN  - array containing combination */
    int   zi,        /* IN  - length of combination        */
    void *user_dat)  /* IN  - user data                    */
{
    int i;

    for(i=0; i<zi; i++) {
        printf("%2d ", z[i]);
    }
    printf("\n");

} /* vec_print */


/*---------------------------------------------------------------
 Routine : combs
 Purpose : Generate combinations, r objects from n.
 (objects are integers in array a, length n)
 f is called for each combination.
 The arguments for f are as for vec_print above.

 This routine is recursive.
---------------------------------------------------------------*/
void
  combs(
    int *a,     /* IN  - input array                              */
    int  n,     /* IN  - length of a                              */
    int  r,     /* IN  - length of combinations to take from a    */
    int *z,     /* OUT - array to put combination into            */
    int  zi,    /* IN  - index into z                             */
    void f( int *, int, void * ),   /* ?     function to call for each comb generated */
    void *p)    /* ?     user data for f()                        */
{
    int i;

    if ( r == 0 ) {
        f(z, zi, p);

		/* update progress bar (if required) */
		current_progress += 1.0f;
		while(current_progress > one_increment) {
		    GenerateNumericalProbabilityProgressBarInc();
            current_progress -= one_increment;
		}

        return;
    } else {
        for(i = 0; i < n-r+1 && !GenerateNumericalProbabilityCheckForInterrupt(); i++) {
            z[zi] = a[i];
            combs(a+i+1, n-i-1, r-1, z, zi+1, f, p);
        }
    }
} /* combs */

/*---------------------------------------------------------------
 Routine : perms
 Purpose : Generate permutations, r objects from n.
 (objects are integers in array a, length n)
 f is called for each permutation.
 The arguments for f are as for vec_print above.
 
 This routine is recursive.
---------------------------------------------------------------*/
void
  perms(
    int *a,     /* IN  - input array                              */
    int  n,     /* IN  - length of a                              */
    int  r,     /* IN  - length of permutations to take from a    */
    int *z,     /* OUT - array to put permutation into            */
    int  zi,    /* IN  - index into z                             */
    void f( int *, int, void * ),   /* ?     function to call for each perm generated */
    void *p)    /* ?     user data for f()                        */
{
    int i;

    if ( r == 0 ) {
        f(z, zi, p);
        return;
    } else {
        for(i = 0; i < n; i++) {
            z[zi] = a[i];
            swap(a, i, 0);
            perms(a+1, n-1, r-1, z, zi+1, f, p);
            swap(a, i, 0);
        }
    }

} /* perms */

/*---------------------------------------------------------------
 Routine : factorial
 Purpose : Calculate n factorial
---------------------------------------------------------------*/
float
  factorial( int n )
{
    float val = 1;

    while( n > 1 ) {
        val *= n--;
    }
    return val;

} /* factorial */

/*---------------------------------------------------------------
 Routine : nCr
 Purpose : Calculate number of combinations, r from n

 Recurrence relation
---------------------------------------------------------------*/
float
  nCr( int n, int r)
{

    float val = 1;
    int i;

    for(i=0; i<r; i++) {
        val = val * (n-i) / (i+1);
    }
    return val;

} /* nCr */

/*---------------------------------------------------------------
 Routine : nKr
 Purpose : Calculate sum of nCi, for i = 0 to r.

 Direct sum
---------------------------------------------------------------*/
float
  nKr( int n, int r)
{
    float result = 0;
    while( r >= 0 ) {
        result += nCr(n, r--);
    }
    return result;
} /* nKr */

/*---------------------------------------------------------------
 Routine : nPr
 Purpose : Calculate number of permutations, r from n.

 Recurrence relation
---------------------------------------------------------------*/
float
  nPr( int n, int r)
{

    float val = 1;
    int i;

    for(i=0; i<r; i++) {
        val = val * (n-i);
    }
    return val;

} /* nPr */


/*---------------------------------------------------------------
 Routine : set_one_increment
 Purpose : sets static one_increment to n and sets num_calls to 0
---------------------------------------------------------------*/
void
  set_one_increment(float n)
{
	one_increment = n;
	current_progress = 0.0f;
}



#ifdef TEST
/*---------------------------------------------------------------
 Routine : comb_count
 Purpose : A test function called by comb or perm for each
 vector generated. Print progress info to stdout
---------------------------------------------------------------*/
static void
  comb_count(
    int  *z,         /* IN  - array containing combination */
    int   zi,        /* IN  - length of combination        */
    int  *pn)        /* IN  - pointer to number of combs   */
{
    static int count   = 0;
    static int percent = 0;
    float f = 100 * ++count / (float) *pn;

    while ( f  >= (float)(percent + 1) ) {
        percent++;
        if ( (percent % 10) == 0 ) {
            printf(". %d%%\n", percent);
        } else {
            printf(".");
        }
        fflush(stdout);
    }
}

main()
{
    int N = 50;
    int R = 5;
    int *x, *z;
    int n;
    int i;

    n = nCr(N, R);
    printf("%dC%d = %d\n", N, R, n);
/*
    n = nPr(N, R);
    printf("%dP%d = %d\n", N, R, n);
*/

    if ( (x = (int *)malloc( N * sizeof(int) )) == NULL ) {
        printf("*** malloc failed for x\n");
        exit(1);
    }
    for(i=0; i<N; i++) {
        x[i] = i+1;
    }
    if ( (z = (int *)malloc( N * sizeof(int) )) == NULL ) {
        printf("*** malloc failed for z\n");
        exit(1);
    }
    
    combs(x, N, R, z, 0, comb_count, &n);
 /* perms(x, N, R, z, 0, vec_print, NULL); */

}


#endif /* def TEST */
