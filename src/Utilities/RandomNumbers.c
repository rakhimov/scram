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
 Module : RandomNumbers
 Author : FSC Limited
 Date   : 24 July 1996
 Origin : FTA project,
          FSC Limited Private Venture.
 
 SccsId : @(#)RandomNumbers.c	1.3 11/08/96
 
 Module for generating random numbers, based on the ANSI C
 library routine rand() or internal software routine ran0()
*****************************************************************/

#define RAND_BASE (float)(RAND_MAX + 1.0)
#define RAND rand

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
/* #include "RandomNumbers.h"  */
#include "MathMacros.h"

static int reset = 1;

/*---------------------------------------------------------------
 Routine : reset_ran0
 Purpose : To reset the static global variable used by ran0
---------------------------------------------------------------*/
static void 
  reset_ran0()
{
    reset = 1;

} /* reset_ran0 */

/*---------------------------------------------------------------
 Routine : ran0
 Purpose : To get better random numbers from a poor generator 
 such as rand()

 See Numerical Recipies in C p 274-346.
 
 For some reason this is VERY SLOW - uses bags of system 
 time - investigate
---------------------------------------------------------------*/
static int 
  ran0()
{
    static int v[42];
    int j;

 /* fill the vector with numbers from rand() */
    if ( reset ) {
        printf("resetting ran0\n");
        reset = 0;
        for (j = 0; j < 42; j++) { rand(); }      /* exercise rand() */
        for (j = 0; j < 42; j++) {
            v[j] = rand();
        }
    }

 /* use v[0] as an index on the rest of the array */
    j = 1 + (int)( 42.0 * v[0] / RAND_BASE );

    if ( j > 42 || j < 1 ) printf("error in ran0");

 /*  return the chosen number : but first, put it in v[0] as the next index,
  *  and fill its place with a new random number
  */
    v[0] = v[j];
    v[j] = rand();
    return v[0];

} /* ran0 */

/*---------------------------------------------------------------
 Routine : frand
 Purpose : To generate a floating point number between 0 and 1

 Call rand() once : get 2**15 possible numbers
            twice : get 2**30 ( OK for 1,000,000,000 calls to rand_exp() )
            four times : get 2**60
---------------------------------------------------------------*/
float 
  frand( void )
{
/* first order - no good for the exponential generator */
/*     return RAND()/RAND_BASE; */

/* second order - good enough for most purposes */
    return (RAND() + RAND()/RAND_BASE)/RAND_BASE; 

 /*
  * fourth order - can generate very small numbers, if you ever need them
  */
/*     return (RAND() + (RAND() + (RAND() + RAND()  */
/*            / RAND_BASE)/RAND_BASE)/RAND_BASE)/RAND_BASE ;  */

} /* frand */

/*---------------------------------------------------------------
 Routine : nrand
 Purpose : To generate a random integer between 1 and n inclusive

 0 <= frand() < 1, but the greatest frand() is 1-1E-9. As a float
 this comes out as 1.0000000. Thus, if you think you have got
 n + 1, you should actually round down. This is the reason for the
 fiddle at the end.
---------------------------------------------------------------*/
int 
  nrand( int n )
{
    float r = frand();
    int m = (int)(r * n) + 1;
    if ( m > n ) {              /* caused by precision failure ! */
        m = n;
    }
    return m;

} /* nrand */

/*---------------------------------------------------------------
 Routine : rnd
 Purpose : To generate a random integer between n1 and n2 inclusive
---------------------------------------------------------------*/
int rnd(int n1, int n2)
{
    return n1 - 1 + nrand( n2 - n1 + 1 );

} /* rnd */

/*---------------------------------------------------------------
 Routine : rand_exp
 Purpose : To generate a random number disributed as y -> g(y)
 from a variable x distributed uniformly x -> u(x) = 1 : 0 < x < 1

 To get this y = f(x) where x = f~(y) = integral g(y) dy 
 
 e.g      g(y) = exp(-y) :: x = f~(y) = 1 - exp(-y) :: y = f(x) = -ln(1-x)
          or equivalently f(x) = -ln(x) as (1-x) is distributed as x

  y -> k exp( -ky )  (this distribution has mean value 1/k )
---------------------------------------------------------------*/
float 
  rand_exp( float k )
{
/*     double drand48(); */
    return -log( frand() )/k;
 /* return -log( drand48() )/k; */

} /* rand_exp */

/*---------------------------------------------------------------
 Routine : rand_binomial
 Purpose : To generate a random number distributed as Bin(n, p)
 
 (see Crawshaw + Chambers p200)
   n = number of trials
   p = prob. success on each trial  ( 0 <= p <= 1 )
 
 NOTE : mean = n * p

 This routine uses the definition of the binomial distribution, ie
 the number of successes in n trials, probability of success p
 with each trial. This uses n calls to frand(). For large n there
 are better methods. See Numerical Recipies in C p 223.
 
 for n large, p ~= 0.5, you can use rand_disc_norm( np, np(1-p) ).
---------------------------------------------------------------*/
int 
  rand_binomial( int n, float p )
{
   int b = 0;  /* result, ie number of successes */
   int i;

   for (i = 0; i < n; i++) {  /* for n trials   */
       if ( frand() < p ) {   /* if success ... */
           b++;               /* increment b    */
       }
   }
   return b;

} /* rand_binomial */

/*---------------------------------------------------------------
 Routine : rand_norm
 Purpose : To generate a random number from normal distribution,
 mean m, standard deviation s
 
 Box-Muller method. Numerical Recipies in C p 216.
 Generates two normally distributed variables at once.
---------------------------------------------------------------*/
float 
  rand_norm(float m, float s)
{
    static int iset = 0;
    static float gset;
    float fac, r, v1, v2, nrv;

    if (iset == 0) {                   /* generate two new variables        */
        do {
            v1 = 2.0 * frand() - 1.0;      /* two uniform variables         */
            v2 = 2.0 * frand() - 1.0;
            r = v1*v1 + v2*v2;             /* square on the hypotenuse      */
        } while ( r >= 1.0 );              /* reject if outside unit circle */
        fac = sqrt(-2.0*log(r)/r);         /* fiendishly clever Box-Muller
                                            * transformation
                                            */
        gset = v1*fac;                     /* store first normal variable   */
        iset = 1;                          /* set flag                      */
        nrv =  v2*fac;                     /* use other normal variable     */
    } else {                           /* we've already got one !           */
        iset = 0;                          /* set flag                      */
        nrv =  gset;                       /* use it                        */
    }
    return nrv * s + m;           /* scale for mean and variance and return */

} /* rand_norm */

/*---------------------------------------------------------------
 Routine : rand_disc_norm
 Purpose : To generate a random number from discrete normal
 distribution.

 NOTE : this procedure appears to give correct mean, but s.d. slightly
        too large. Beware of this if using to approximate binomial
        distribution.
---------------------------------------------------------------*/
int 
  rand_disc_norm( float m, float s)
{
    int i = (int)floor( rand_norm( m, s ) + 0.5 );
    return i;

} /* rand_disc_norm */

/*---------------------------------------------------------------
 Routine : rand_disc
 Purpose : To generate a discrete random distribution 1 to n.
 The probabilities are returned in p[]
---------------------------------------------------------------*/
int 
  rand_disc( int n, double *p )
{
    int i;
/*     float c = 0.0, r = frand(); */
    double c = 0.0, r = (double)frand();

    for( i = 0; i < n; i++ ) {
        c += p[i];
        if (r <= c) {
            return i+1;
        }
    }
/*
 * r is greater than the sum of the probabilities.
 * if (sum of the probabilities) much different from 1, issue a
 * warning. In any case, return n
   The implication is that if this is true, then the probability with
   which the last event in the list is taken to have failed will
   be skewed, i.e., it is not quite so random. Especially a problem
   if the last n events have very small probabilites, which then
   tend to be less than r for much of the time. This appears to
   be the case with large trees.
   
   Instead of reporting the warning, wouldn't it be better to repeat
   the process until r <= c ?? MPA 17/7/96
 */
    if ( ! EQUAL( c, 1.0 ) ) {
        printf("*** rand_disc warning : \n"
               "    r = %g, probabilities do not add up to 1.0 : c = %f\n", 
               r, c);
    }
    return n;

} /* rand_disc */


/*---------------------------------------------------------------
 FOR TEST PURPOSES ONLY vvvvvvvvvvvvvvvvvvvvvvv
---------------------------------------------------------------*/

/*
 *  main to test rand_norm
 */

#ifdef NORM

#define M   500.0
#define SD  15.811
#define R   10000
 
main()
{
    float sum_r = 0, sum_r2 = 0;
    float mean, sd;
    float r;
    int i;
 
    for (i = 0; i < R; i++) {
        r = rand_norm( M, SD );
        sum_r  += r;
        sum_r2 += r*r;
    }
 
    mean = sum_r / R;
    sd   = sqrt( sum_r2 / R - mean * mean );
    printf("R: %d mean: %f s.d.: %f\n", R, mean, sd);
 
}

#endif

/*
 *  main to test rand_disc_norm
 */

#ifdef DISC_NORM

#define M   500.0
#define SD  15.811
#define R   10000

main()
{
    float sum_d = 0, sum_d2 = 0;
    float mean, sd;
    int d;
    int i;

    for (i = 0; i < R; i++) {
        d = rand_disc_norm( M, SD );
        sum_d  += d;
        sum_d2 += d*d;
    }

    mean = sum_d / R;
    sd   = sqrt( sum_d2 / R - mean * mean );
    printf("R: %d mean: %f s.d.: %f\n", R, mean, sd);

}

#endif

/*
 *  main to test rand_disc
 */

#ifdef DISC

#define N   5
#define R   10000

main()
{
    float p[]      = { 0.2, 0.1, 0.2, 0.45, 0.05 };
    int   result[] = { 0, 0, 0, 0, 0 };
    int   n        = N;
    int r, i;

    for (i = 0; i < R; i++) {
        r = rand_disc( n, p );
        result[r - 1]++;
    }

    printf("\nR = %d\n\n", R);
    for (i = 0; i < N; i++) {
        printf("%d expected: %f observed: %f\n", i+1, p[i], (1.0*result[i])/R);
    }

}

#endif

/*
 *  main to test rnd
 */

#ifdef RND

#define NUMGOES 10

main( void )
{
    long i, total, r;
    int t[15] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    total = 0;
    printf("RAND_MAX = %d\n", RAND_MAX);
    for ( i = 0; i < NUMGOES; i++){
        total += (r = rnd(5, 10));
        t[r]++;
        printf("%d\n", r);
    }
    printf("mean = %f\n", total/(float)NUMGOES );
    for ( i = 0; i < 15; i++){
        printf("%d : %d\n", i, t[i]);
    }
}

#endif

/*
 * main to test rand_binomial(n, p)
 *
 * call rand_binomial(3, 1.7/3) lots of times should give mean 1.7
 *
 */

#ifdef BINOMIAL

main()
{
# if 0
    int n = 3;
    float mean = 1.7;
    float p = mean/n;
    int sample = 100000;
    int c      = 1;
    int lim    = n;
#else
    int n      = 10000;
    float p    = 0.001;
    float mean = n * p;
    int sample = 1000;
    float c    = 1.0;
    int lim    = mean * 3.0;
#endif
    float expected[50], r, rf, pf;
    int observed[50];
    int total = 0;
    int i;

    r  = 1.0 - p;
    rf = pow( r, (float)n );
    pf = 1.0;
#if 0
    for(i = 0; i < lim + 1; i++ ) {
        printf("c = %d  rf = %f  pf = %f\n", c, rf, pf);
        observed[i] = 0;
        expected[i] = c * rf * pf;
        c = c * (n - i) / (i + 1);
        rf = rf / r;
        pf = pf * p;
    }
#else
    expected[0] = rf;
    observed[0] = 0;
    for(i = 1; i < lim + 1; i++ ) {
        observed[i] = 0;
        expected[i] = expected[i-1] * p * (n - i + 1) / ( r * i );
    }
#endif

#if 0
    for(i = 0; i < sample; i++ ) {
        observed[ rand_binomial(n, p) ]++;
    }
#endif

    for(i = 0; i < lim + 1; i++ ) {
        total += i * observed[i];
        printf("%d : observed  %d  expected  %f\n", i, observed[i],
                                                       expected[i] * sample);
    }
    printf("\n mean = %f\n", total / (float) sample);

}

#endif

