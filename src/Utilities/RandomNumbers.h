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
 
 SccsId : @(#)RandomNumbers.h	1.1 08/22/96
 
 Module for generating random numbers, based on the ANSI C
 library routine rand() or internal software routine ran0()
*****************************************************************/
 
#ifndef RAND_H
#define RAND_H

/*---------------------------------------------------------------
 Routine : frand
 Purpose : To generate a floating point number between 0 and 1
---------------------------------------------------------------*/
extern float 
  frand( void  );

/*---------------------------------------------------------------
 Routine : nrand
 Purpose : To generate a random integer between 1 and n inclusive
---------------------------------------------------------------*/
extern int   
  nrand( int );

/*---------------------------------------------------------------
 Routine : rnd
 Purpose : To generate a random integer between n1 and n2 inclusive
---------------------------------------------------------------*/
extern int     
  rnd( int, int);

/*---------------------------------------------------------------
 Routine : rand_exp
 Purpose : To generate a random number disributed as y -> g(y)
 from a variable x distributed uniformly x -> u(x) = 1 : 0 < x < 1
---------------------------------------------------------------*/
extern float 
  rand_exp( float );

/*---------------------------------------------------------------
 Routine : rand_binomial
 Purpose : To generate a random number distributed as Bin(n, p) 

 (see Crawshaw + Chambers p200)
   n = number of trials
   p = prob. success on each trial  ( 0 <= p <= 1 )
 
 NOTE : mean = n * p
---------------------------------------------------------------*/
extern int 
  rand_binomial( int, float );

/*---------------------------------------------------------------
 Routine : rand_norm
 Purpose : To generate a random number from normal distribution, 
 mean m, standard deviation s

 Box-Muller method. Numerical Recipies in C p 216.
---------------------------------------------------------------*/
extern float 
  rand_norm(float, float);

/*---------------------------------------------------------------
 Routine : rand_disc_norm
 Purpose : To generate a random number from discrete normal 
 distribution.
---------------------------------------------------------------*/
extern int 
  rand_disc_norm( float, float);

/*---------------------------------------------------------------
 Routine : rand_disc
 Purpose : To generate a discrete random distribution 1 to n.
 The probabilities are returned in p[]
---------------------------------------------------------------*/
extern int 
  rand_disc( int n, double *p );

#endif
