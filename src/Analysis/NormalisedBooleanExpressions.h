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
 Module : NormalisedBooleanExpressions
 Author : FSC Limited
 Date   : 24 July 1996
 Origin : FTA project,
          FSC Limited Private Venture.

 SccsId : @(#)NormalisedBooleanExpressions.h	1.1 08/22/96

 Module for manipulating boolean expressions in 'normal form',
 i.e. the "OR of the ANDS". Based upon the prototype "bool".
 
 An Expression is a list of groups. A group is a bit array 
 representing the AND of the events which are set to fail.
 Each group is a minimal cut set.

 This version uses the BitArray type in the module "bits"
 to represent the AND of basic variables. There are
 as many bits as basic variables.
 
*****************************************************************/

#ifndef EXPR_H
#define EXPR_H

#include <stdio.h>
#include "bits.h"

#define SUBSET        1
#define SUPERSET     -1
#define NEITHER       0

/* type to represent the AND of basic events */
typedef struct _Group {
    BitArray      *b;     /* bit array representing the AND of those
                           * variables which are set.
                           */

    struct _Group *next;  /* pointer to next Group in list     */
    struct _Group *prev;  /* pointer to previous Group in list */

} Group, *Expr;           /* an Expr is a linked list of groups, representing
                           * the OR of these groups. The Groups will be
                           * maintained in numerical order
                           */

/*---------------------------------------------------------------
 Routine : GroupCreate
 Purpose : Create and initialise a Group, with the given bit array.
---------------------------------------------------------------*/
extern Group *
  GroupCreate( 
    BitArray *val );  /* IN  - initial BitArray or NULL */

/*---------------------------------------------------------------
 Routine : GroupCreateN
 Purpose : Create and initialise a Group, with the given size.
---------------------------------------------------------------*/
extern Group *
  GroupCreateN( 
    int n );  /* IN  - size (bits) */

/*---------------------------------------------------------------
 Routine : GroupSet
 Purpose : Set a Group to the value of the given bit array.
---------------------------------------------------------------*/
extern void
  GroupSet( 
    Group *g, 
    BitArray *val );

/*---------------------------------------------------------------
 Routine : GroupDestroy
 Purpose : Destroy a Group.
---------------------------------------------------------------*/
extern void
  GroupDestroy( 
    Group *g );

/*---------------------------------------------------------------
 Routine : GroupAND
 Purpose : Return the AND of two groups.
---------------------------------------------------------------*/
extern Group *
  GroupAND( 
    Group *g1, 
    Group *g2 );

/*---------------------------------------------------------------
 Routine : GroupEqual
 Purpose : Test for equality between groups. Groups are equal if
 all their elements are equal.
---------------------------------------------------------------*/
extern int
  GroupEqual( 
    Group *g1, 
    Group *g2 );

/*---------------------------------------------------------------
 Routine : GroupComp
 Purpose : Compare groups, first by their order, then by comparing
 their bit values. 

 g1 > g2 : return  1
 g1 < g2 : return -1
 g1 = g2 : return  0
---------------------------------------------------------------*/
extern int
  GroupComp( 
    Group *g1, 
    Group *g2 );

/*---------------------------------------------------------------
 Routine : GroupSubset
 Purpose : Check subset status between two groups

 Returns  SUBSET    if g1 is a subset of g2
          SUPERSET  if g2 is a subset of g1
          NEITHER   if g1 and g2 disjoint
---------------------------------------------------------------*/
extern int
  GroupSubset( 
    Group *g1, 
    Group *g2 );

/*---------------------------------------------------------------
 Routine : GroupOrder
 Purpose : Return order of a group (number of bits set).
---------------------------------------------------------------*/
extern int
  GroupOrder( 
    Group *g );

/*---------------------------------------------------------------
 Routine : ExprCreate
 Purpose : Create an Expression
---------------------------------------------------------------*/
extern Expr
  ExprCreate();

/*---------------------------------------------------------------
 Routine : ExprDestroy
 Purpose : Destroy an Expr
---------------------------------------------------------------*/
extern void
  ExprDestroy( 
    Expr e );

/*---------------------------------------------------------------
 Routine : ExprCopy
 Purpose : Return a copy of an expression.
---------------------------------------------------------------*/
extern Expr
  ExprCopy( 
    Expr e );

/*---------------------------------------------------------------
 Routine : ExprORGroup
 Purpose : OR of an Expr and a Group
---------------------------------------------------------------*/
extern Expr
  ExprORGroup(
    Expr   e,      /* IN  - expression                            */
    Group *g,      /* IN  - group                                 */
    int   *done ); /* OUT - flag, set TRUE if g added, else FALSE */

/*---------------------------------------------------------------
 Routine : ExprAND
 Purpose : Create the AND of two Exprs.
 Order of result is limited by 'limit' - zero means unlimited
---------------------------------------------------------------*/
extern Expr
  ExprAND( 
    Expr e1, 
    Expr e2, 
    int limit );

/*---------------------------------------------------------------
 Routine : ExprOR
 Purpose : Calculate the OR of two expressions.
---------------------------------------------------------------*/
extern Expr
  ExprOR( 
    Expr e1, 
    Expr e2 );

/*---------------------------------------------------------------
 Routine : ExprCount
 Purpose : Count number of groups in the expression
---------------------------------------------------------------*/
extern int
  ExprCount( 
    Expr e );

/*---------------------------------------------------------------
 Routine : ExprCountOrder
 Purpose : Count number of groups in the expression of order less
 than n.

 Note: Assumes that groups are ordered with low order cut sets 
 first in list.
---------------------------------------------------------------*/
extern int
  ExprCountOrder( 
    Expr e, 
    int n );

/*---------------------------------------------------------------
 Routine : ExprPrint
 Purpose : Print an Expression to stdout.
---------------------------------------------------------------*/
extern void
  ExprPrint( 
    Expr e );

/*---------------------------------------------------------------
 Routine : ExprCutSet
 Purpose : Optimised version of ExprOR for adding a cut set to an
 Expression.
---------------------------------------------------------------*/
extern Expr
  ExprCutSet(
    Expr    e,      /* IN  - expression                               */
    Group  *z,      /* IN  - pointer to last group (sentinel) in expr */
    Group  *g,      /* IN  - group                                    */
    int    *done ); /* OUT - flag, set  if g added, else FALSE        */

/*---------------------------------------------------------------
 Routine : ExprCutsetProbs
 Purpose : Calculate probability of individual cut sets in an 
 expression.
---------------------------------------------------------------*/
extern float *  /* RET - probability array                      */
  ExprCutsetProbs(
    Expr   e,   /* IN  - expression                             */
    float *pr); /* IN  - array of probabilities of basic events */

/*---------------------------------------------------------------
 Routine : ExprProb
 Purpose : Calculate probability of an expression
---------------------------------------------------------------*/
extern float          /* RET - probability                             */
  ExprProb( 
    Expr   e,         /* IN  - expression                              */
    float *pr,        /* IN  - array of probabilities  of basic events */
    int    max_order, /* IN  - max order of cut sets to use            */
    int    nt);       /* IN  - term (0 means all)                      */

/*---------------------------------------------------------------
 Routine : ExprEnd
 Purpose : Return pointer to last group in expression
---------------------------------------------------------------*/
extern Group *
  ExprEnd(
    Expr expr);

/*---------------------------------------------------------------
 Routine : ExprWrite
 Purpose : Write an Expression to a given file
---------------------------------------------------------------*/
extern void
  ExprWrite(
    FILE *fp, 
    Expr e);

/*---------------------------------------------------------------
 Routine : ExprRead
 Purpose : Read an Expression from a given file.
---------------------------------------------------------------*/
extern Expr
  ExprRead(
    FILE *fp);

/*---------------------------------------------------------------
 Routine : GroupSentinel
 Purpose : Test if group is the sentinal at the end of an 
 Expression.
---------------------------------------------------------------*/
extern int
  GroupSentinel( 
  Group *g );

/*---------------------------------------------------------------
 Routine : calc_sub_term
 Purpose : Calculate the probability of a combination of m.c.s.
 and add to the current term.

 Form the AND of the m.c.s. given in z
 Multiply the probs as given in static vector basic_prob
 Add it to prob_term
---------------------------------------------------------------*/
extern void
  calc_sub_term(
    int    *z,      /* IN  - array containing combination */
    int     zi,     /* IN  - length of combination        */
    Group **index); /* IN  - user data index to m.c.s.    */

/*---------------------------------------------------------------
 Routine : set_prob_term
 Purpose : sets static prob_term to n
---------------------------------------------------------------*/
extern void
  set_prob_term(float n);

/*---------------------------------------------------------------
 Routine : set_basic_n
 Purpose : sets static basic_n to n
---------------------------------------------------------------*/
extern void
  set_basic_n(int n);

/*---------------------------------------------------------------
 Routine : set_basic_prob
 Purpose : sets static basic_prob to n_ptr
---------------------------------------------------------------*/
extern void
  set_basic_prob(float *n_ptr);


#endif

