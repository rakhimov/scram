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
 
 SccsId : @(#)NormalisedBooleanExpressions.c	1.3 11/08/96
 
 Module for manipulating boolean expressions in 'normal form',
 i.e. the "OR of the ANDS". Based upon the prototype "bool".
 
 An Expression is a list of groups. A group is a bit array
 representing the AND of the events which are set to fail.
 Each group is a minimal cut set.
 
 This version uses the BitArray type in the module "bits"
 to represent the AND of basic variables. There are
 as many bits as basic variables.
 
 NOT is not handled.

 The method is bottom-up. Sub expressions are always represented in
 normal form. The problem then reduces to being able to OR and AND
 normal form expressions to produce another normal-form expression.

 An expression is a list of groups which
 are understood to be ORed together. A group is a number of variables
 which are understood to be ANDed together. Groups are represented by
 BitArrays

 An expression is represented by a linked list of groups, which are
 kept in numerical order. A "sentinal" group is used to mark the end of
 the list. This contains a zero BitArray.
 Note that no group will be repeated.

 To OR two expressions, we take the first and OR in turn with each group
 of the other. For each new group we must take into account the law of
 ABSORPTION which states that A.B + A = A. This means that if any
 group in an expression is a subset of any other, we must strike out the
 LONGER of the two. e.g ABCD + AC -> AC. Hence we check each new group
 against all existing groups in the expression. If the new group is a
 subset of an old group, we eliminate the old group, and continue.
 if an old group is a subset of the new group, we need not bother to add
 the new group. To do this test we form the AND of the two groups. A small
 speed improvement could be gained by writing a dedicated function.
 Having struck out any redundant old groups, we add the
 new group in its numeric position.

 To AND two expressions we must 'expand the brackets', eg

  (A + BC) . ( AB + BC + CD ) = A.AB  + A.BC  + A.CD +
                                BC.AB + BC.BC + BC.CD

 To AND each pair of groups we use the AND provided for bit arrays.
 We than OR these successively into a new expression. The absorption
 rules ensure we obtain :

        AB + ACD + BC

*****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "boolean.h"
#include "NormalisedBooleanExpressions.h" 
#include "StatisticalMaths.h"

#include "MemoryUtil.h"
#include "util.h"

static BIN_TYPE  zero_byte     = 0;
static BitArray  zero_bitarray = {1, &zero_byte};
static BitArray *stop = &zero_bitarray;             /* a 1-bit zero */

/*---------------------------------------------------------------
 Routine : GroupCreate
 Purpose : Create and initialise a Group, with the given bit array.

 get memory for group
 create the BitArray
 initialise the BitArray (with copy)
 initialise list pointers
 return pointer
---------------------------------------------------------------*/
Group *
  GroupCreate( BitArray *val )   /* IN  - initial BitArray or NULL */
{
/*     Group *g = (Group *)malloc( sizeof(Group) ); */
    Group *g;

/*     if (g == NULL)  */
    if ( !fNewMemory( (void *)&g, sizeof(Group) ) ) 
    {
        printf("\n*** GroupCreate: malloc failed ***\n");
        exit(1);
    }

    g->b     = val == NULL ? NULL : BitClone(val);
    g->next  = g->prev = NULL;
    return g;
} /* GroupCreate */

/*---------------------------------------------------------------
 Routine : GroupCreateN
 Purpose : Create and initialise a Group, with the given size.

 get memory for group
 create the BitArray
 initialise the BitArray
 initialise list pointers
 return pointer
---------------------------------------------------------------*/
Group *
  GroupCreateN( int n )   /* IN  - size (bits) */
{
/*     Group *g = (Group *)malloc( sizeof(Group) ); */
    Group *g;

/*     if (g == NULL)  */
    if ( !fNewMemory( (void *)&g, sizeof(Group) ) ) 
    {
        printf("\n*** GroupCreateN: malloc failed ***\n");
        exit(1);
    }

    g->b     = BitCreate(n);
    g->next  = g->prev = NULL;
    return g;
} /* GroupCreateN */

/*---------------------------------------------------------------
 Routine : GroupSet
 Purpose : Set a Group to the value of the given bit array.
---------------------------------------------------------------*/
void
  GroupSet( Group *g, BitArray *val )
{
    g->b = val == NULL ? NULL : BitClone(val);
} /* GroupSet */

/*---------------------------------------------------------------
 Routine : GroupDestroy
 Purpose : Destroy a Group.

 free string part of g
 free g
---------------------------------------------------------------*/
void
  GroupDestroy( Group *g )
{
    if (g->b != NULL) {
        BitDestroy(g->b);
    }
    FreeMemory(g);
} /* GroupDestroy */

/*---------------------------------------------------------------
 Routine : GroupAND
 Purpose : Return the AND of two groups.

 The condition (g1 AND g2) is represented by the bit-wise OR
 of the component bit arrays. Careful!
---------------------------------------------------------------*/
Group *
  GroupAND( Group *g1, Group *g2 )
{
    BitArray *b = BitOR(g1->b, g2->b);
#if 0
    Group *g = GroupCreate( b );         /* unnecessary copy */
    BitDestroy(b);
#endif
    Group *g = GroupCreate( NULL );      /* avoid unnecessary copy */
    g->b = b;

    return g;
} /* GroupAND */

/*---------------------------------------------------------------
 Routine : GroupEqual
 Purpose : Test for equality between groups. Groups are equal if
 all their elements are equal.
---------------------------------------------------------------*/
int
  GroupEqual( Group *g1, Group *g2 )
{
    return BitEquals( g1->b, g2->b );
} /* GroupEqual */

/*---------------------------------------------------------------
 Routine : GroupComp
 Purpose : Compare groups, first by their order, then by comparing
 their bit values.
 
 g1 > g2 : return  1
 g1 < g2 : return -1
 g1 = g2 : return  0
---------------------------------------------------------------*/
int
  GroupComp( Group *g1, Group *g2 )
{
    BitArray *zero = stop;   /* a 1-bit zero */
    int order1, order2;

    if ( BitComp(g1->b, zero) == 0 ) {
        if ( BitComp(g2->b, zero) == 0 ) {
            return 0;
        } else {
            return 1;
        }
    }

    if ( BitComp(g2->b, zero) == 0 ) {
        return -1;
    }

    order1 = GroupOrder(g1);
    order2 = GroupOrder(g2);

    if (order1 > order2) {
        return  1;
    } else if (order1 < order2) {
        return -1;
    } else {
        return BitComp( g2->b, g1->b );
    }
} /* GroupComp */

/*---------------------------------------------------------------
 Routine : GroupSubset
 Purpose : Check subset status between two groups
 
 Returns  SUBSET    if g1 is a subset of g2
          SUPERSET  if g2 is a subset of g1
          NEITHER   if g1 and g2 disjoint
 BEGIN GroupEqual
   calculate g = g1 AND g2
   IF (g == g2) THEN
      SUBSET
   ELSE IF (g == g1) THEN
      SUPERSET
   ELSE
      NEITHER 
   END IF
 END GroupEqual
---------------------------------------------------------------*/
int
  GroupSubset( Group *g1, Group *g2 )
{
    int    result;
    Group *g = GroupAND(g1, g2);

    if ( GroupEqual(g, g2) ) {
        result =  SUBSET;
    } else if ( GroupEqual(g, g1) ) {
        result = SUPERSET;
    } else {
        result = NEITHER ;
    }
    GroupDestroy(g);
    return result;
} /* GroupSubset */

/*---------------------------------------------------------------
 Routine : GroupOrder
 Purpose : Return order of a group (number of bits set).
---------------------------------------------------------------*/
int
  GroupOrder( Group *g )
{
    return BitCount( g->b );
} /* GroupOrder */


/*---------------------------------------------------------------
 Routine : ExprCreate
 Purpose : Create an Expression

 Create a 'sentinal' group to mark the end of the Expression
---------------------------------------------------------------*/
Expr
  ExprCreate()
{
    Group *g = GroupCreate(stop);
    return g;
} /* ExprCreate */

/*---------------------------------------------------------------
 Routine : ExprDestroy
 Purpose : Destroy an Expr

 This routine is recursive.
---------------------------------------------------------------*/
void
  ExprDestroy( Expr e )
{
    if ( e != NULL ) {
        ExprDestroy(e->next);
        GroupDestroy(e);
    }
} /* ExprDestroy */

/*---------------------------------------------------------------
 Routine : ExprCopy
 Purpose : Return a copy of an expression.

 Create new
 Step through e, copying each group and inserting in new expr
---------------------------------------------------------------*/
Expr
  ExprCopy( Expr e )
{
 /* BitArray *stop = BitCreate(1);   /* a 1-bit zero */
    Expr enew = ExprCreate();
    Group *g, *end = enew;

    while ( BitComp(e->b, stop) != 0 ) {
        g = GroupCreate(e->b);
        if (end->prev == NULL) { /* copying the first group */
            enew = g;
        } else {
            end->prev->next = g;
        }
        g->prev = end->prev;
        g->next = end;
        end->prev = g;
        e = e->next;
    }
    return enew;
} /* ExprCopy */

/*---------------------------------------------------------------
 Routine : ExprORGroup
 Purpose : OR of an Expr and a Group [this is the basic "append"
 operation for expressions ]

 BEGIN ExprORGroup
    IF ( g is NULL ) THEN
       return expression unchanged
    END IF
 
    FOR (each group p in the expression) DO
       determine if g is a subset of p or vice versa
       CASE ( g is a subset of p ) :
          eliminate p from the expression
       CASE ( p is a subset of g ) :
          return without changing expression (g is redundant)
       DEFAULT :
          continue to next group
       END CASE
    END FOR

    find position in list for g
    add g to the list
    return e
 END ExprORGroup
---------------------------------------------------------------*/
Expr
  ExprORGroup(
    Expr   e,      /* IN  - expression                            */
    Group *g,      /* IN  - group                                 */
    int   *done )  /* OUT - flag, set  if g added, else FALSE */
{
 /* BitArray *stop = BitCreate(1);   /* a 1-bit zero */
    Group    *p, *tmp;

    if (g == NULL) {
        *done = 0;
        return e;
    }

 /*
  * Go through expression, eliminating any groups that are "absorbed"
  * by g, or quitting if g is absorbed
  */
    p = e;
   	while(BitComp(p->b, stop) != 0) {
        switch( GroupSubset(g, p) ) {

        case SUBSET :               /* eliminate p from the expression        */
            tmp = p->next;
            if (p->prev == NULL) {  /* we are eliminating the first element   */
                e = p->next;
                p->next->prev = NULL;
            } else {
                p->prev->next = p->next;
                p->next->prev = p->prev;
            }
            GroupDestroy(p);
            p = tmp;                /* next group */
            break;

        case SUPERSET :             /* return without changing the expression */
            *done = 0;
            return e;
            break;

        default :                   /* continue to the next group             */
            p = p->next;
            break;
        }
    }

 /*
  * if we are still here, add the new Group to the Expr, in numeric order
  */
#if 1
    for( p = e; GroupComp(p, g) < 0; p = p->next )
        ;

    if (p->prev == NULL) {  /* we are adding at the start of the list */
        e = g;
    } else {
        p->prev->next = g;
    }
#else
    p = e;
    e = g;
#endif

    g->prev = p->prev;
    g->next = p;
    p->prev = g;

    *done = 1;
    return e;

} /* ExprORGroup */

/*---------------------------------------------------------------
 Routine : ExprAND
 Purpose : Create the AND of two Exprs.
 Order of result is limited by 'limit' - zero means unlimited

 BEGIN ExprAND
    Create new expression e
    FOR ( all p1 in e1 ) DO
       FOR ( all p2 in e2 ) DO
          g = p1 AND p2
          IF (order of g > limit)
             destroy g
          ELSE
             e = e OR g
             IF (g not added [because redundant]) THEN
                 destroy g
             END IF
          END IF
       END FOR
    END FOR
    return e
 END ExprAND
---------------------------------------------------------------*/
Expr
  ExprAND( Expr e1, Expr e2, int limit )
{
 /* BitArray *stop = BitCreate(1);   /* a 1-bit zero */
    Expr   e = ExprCreate();
    Group *g, *p1, *p2;
    int    done;
/* clock_t start; */
/* float end; */

/* start = clock(); */
    for(p1 = e1; BitComp(p1->b, stop) != 0; p1 = p1->next) {
        for(p2 = e2; BitComp(p2->b, stop) != 0; p2 = p2->next) { 
            g = GroupAND(p1, p2); 
            if ( (limit > 0) && (GroupOrder(g) > limit) ) { 
                GroupDestroy(g); 
            } else { 
                e = ExprORGroup(e, g, &done); 
                if ( !done ) { 
                    GroupDestroy(g); 
                } 
            } 
        } 
    }
/* end = (float)( ( clock() - start ) / CLOCKS_PER_SEC ); */
/* if ( end > 0.000001 ) {  */
/*   printf("ExprAND = %g\n", end); */
/* }  */
    return e;

} /* ExprAND */

/*---------------------------------------------------------------
 Routine : ExprOR
 Purpose : Calculate the OR of two expressions.

 BEGIN ExprOR
    copy e1 into e
    FOR ( all p in e2 ) DO
       tmp = copy of p
       e = e OR tmp
       IF (tmp not used) THEN
          destroy it
       END IF
    END FOR
    return e
 END ExprOR
---------------------------------------------------------------*/
Expr
  ExprOR( Expr e1, Expr e2 )
{
 /* BitArray *stop = BitCreate(1);   /* a 1-bit zero */
    Expr   e = ExprCopy(e1);
    Group *p, *tmp;
    int    done;

    for(p = e2; BitComp(p->b, stop) != 0; p = p->next) {
        tmp = GroupCreate(p->b);
        e = ExprORGroup(e, tmp, &done);
        if ( !done ) {
            GroupDestroy(tmp);
        }
    }
    return e;

} /* ExprOR */

/*---------------------------------------------------------------
 Routine : ExprCount
 Purpose : Count number of groups in the expression
---------------------------------------------------------------*/
int
  ExprCount( Expr e )
{
    int i;

    for(i = 0; BitComp(e->b, stop) != 0; e=e->next, i++ )
        ;

    return i;
} /* ExprCount */

/*---------------------------------------------------------------
 Routine : ExprCountOrder
 Purpose : Count number of groups in the expression of order less
 than n.
 
 Note: Assumes that groups are ordered with low order cut sets
 first in list.
---------------------------------------------------------------*/
int
  ExprCountOrder( Expr e, int n )
{
    Group *p;
    int i;

    for( p=e, i=0; BitComp(p->b, stop) != 0; p=p->next, i++ ) {
        if ( GroupOrder(p) > n ) break;
    }

    return i;

} /* ExprCountOrder */

/*---------------------------------------------------------------
 Routine : ExprPrint
 Purpose : Print an Expression to stdout.
---------------------------------------------------------------*/
void
  ExprPrint( Expr e )
{
    int first = 1;
    char *s;

    if ( e->b == NULL || BitComp(e->b, stop) == 0 ) {
        printf("(NULL Expr)\n");
        return;
    }

printf("ExprPrint: ");

    while( BitComp(e->b, stop) != 0 ) {
        if (!first) {
            printf(" + ");
        } else {
            first = 0;
        }
        printf("%s", s = BitString(e->b));
        free(s);
        e = e->next;
    }
    printf("\n");

} /* ExprPrint */


/*---------------------------------------------------------------
 Routine : ExprAppend
 Purpose : Append a group to end of an expression (before seninel) 
 - no checking
---------------------------------------------------------------*/
static Expr
  ExprAppend(
    Expr    e,      /* IN  - expression                               */
    Group  *z,      /* IN  - pointer to last group (sentinel) in expr */
    Group  *g,      /* IN  - group                                    */
    int    *done )  /* OUT - flag, set  if g added, else FALSE        */
{
    Group    *p;
/*     int n = BITS_BYTES(g->b->n); */

    if ((p = z->prev) == NULL) {
        e = g;
    } else {
        p->next    = g;
        g->prev    = p;
    }
    g->next = z;
    z->prev = g;

    *done = 1;
    return e;

} /* ExprAppend */

/*---------------------------------------------------------------
 Routine : ExprCutSet
 Purpose : Optimised version of ExprOR for adding a cut set to an
 Expression.

 Relies upon  # all groups being of same size
              # cut sets being added low order first
              # g not being NULL
 
 Only checks for this group being absorbed by existing groups
---------------------------------------------------------------*/
Expr
  ExprCutSet(
    Expr    e,      /* IN  - expression                               */
    Group  *z,      /* IN  - pointer to last group (sentinel) in expr */
    Group  *g,      /* IN  - group                                    */
    int    *done )  /* OUT - flag, set  if g added, else FALSE        */
{
    Group    *p;
    int n = BITS_BYTES(g->b->n);
    int superset;                 /* flag: g is a superset of p */
    int i;

 /*
  * Go through expression, quitting if g is a superset of another cut set
  */
   	for(p = e; (p->b->n != 1) || p->b->a[0] != 0; p = p->next) {

        superset = TRUE;

        for(i = 0; i < n; i++) {
            if ( (p->b->a)[i] & ~(g->b->a)[i] ) {
               superset = FALSE;
               break;
            }
        }

        if (superset) {            /* return without changing the expression */
            *done = 0;
            return e;
        }
    }

 /*
  * if we are still here, add the new Group to the Expr
  */
#if 0
 /*
  * add in numeric order
  */
    for( p = e; GroupComp(p, g) < 0; p = p->next )
        ;

    if (p->prev == NULL) {  /* we are adding at the start of the list */
        e = g;
    } else {
        p->prev->next = g;
    }
    g->prev = p->prev;
    g->next = p;
    p->prev = g;
#endif

/*
 * add at start of list
 *
#if 0
    p = e;
    e = g;
    g->prev = p->prev;
    g->next = p;
    p->prev = g;
#endif

/*
 * add at end of list (just before the sentinel)
 */
#if 1
    if ((p = z->prev) == NULL) {
        e = g;
    } else {
        p->next    = g;
        g->prev    = p;
    }
    g->next = z;
    z->prev = g;
#endif

    *done = 1;
    return e;

} /* ExprCutSet */

/*---------------------------------------------------------------
 Routine : ExprEnd
 Purpose : Return pointer to last group in expression
---------------------------------------------------------------*/
Group *
  ExprEnd(Expr e) 
{
    Group *p;

    if (e == NULL) {
        return NULL;
    }

    for(p = e; p->next != NULL; p = p->next)
        ;

    return p;

} /* ExprEnd */

/*---------------------------------------------------------------
 Routine : ExprWrite
 Purpose : Write an Expression to a given file

 Write each group as string of '0' and '1' chars.
 Separate groups by '\n'
---------------------------------------------------------------*/
void
  ExprWrite(FILE *fp, Expr e)
{
/*     int first = 1; */
    char *s;

    if ( e->b == NULL || BitComp(e->b, stop) == 0 ) {
        fprintf(fp, "\n");
        return;
    }

    while( BitComp(e->b, stop) != 0 ) {
        fprintf(fp, "%s\n", s = BitString(e->b));
        strfree(s);
        e = e->next;
    }
} /* ExprWrite */

/*---------------------------------------------------------------
 Routine : ExprLineLength
 Purpose : Determine length of current line of steam fp, without 
 reading it
---------------------------------------------------------------*/
static int
  ExprLineLength(FILE *fp)
{
    int i = 0;
    long offset = ftell(fp);
    int c;

    while ((c = fgetc(fp)) != '\n') {
        if (c == EOF) {
            return -1;
        }
        if(c == '0' || c == '1')
        {
            i++;
        }
    }
    fseek(fp, offset, SEEK_SET);

    return i;

} /* ExprLineLength */


/*---------------------------------------------------------------
 Routine : ExprRead
 Purpose : Read an Expression from a given file.

 Assume format of ExprWrite
---------------------------------------------------------------*/
Expr
  ExprRead(FILE *fp)
{
    Expr e = ExprCreate();
    Expr z = e;
    int len = ExprLineLength(fp);
    char *s;
    int done;

    if (len < 0) {  /* error */
        printf("ExprRead: incorrect file format\n");
        return NULL;
    }

    if (len == 0) {  /* NULL expr */
        printf("ExprRead: NULL expr\n");
        return e;
    }

/*     s = (char *)malloc( (len + 1) * sizeof( char ) ); */
    if ( !fNewMemory( (void *)&s, ( (len + 1) * sizeof( char ) ) ) )
    {
      exit( 1 );
    }

    while ( fscanf(fp, "%s", s) == 1 ) {
        Group *g = GroupCreateN(len);

        if (BitSetString(g->b, s) == -1) {
            printf("ExprRead: incorrect file format\n");
            return NULL;
        }
     /* e = ExprORGroup(e, g, &done); */
        e = ExprAppend(e, z, g, &done);
        if ( !done ) {
            GroupDestroy(g);
        }
    }
    FreeMemory(s);
    return e;

} /* ExprRead */

/*---------------------------------------------------------------
 Routine : GroupSentinel
 Purpose : Test if group is the sentinal at the end of an
 Expression.
---------------------------------------------------------------*/
int
  GroupSentinel( Group *g )
{
    return BitEquals(g->b, stop);
} /* GroupSentinel */

#ifndef TEST

/*
 * calculate probability of an expression from probs of basic events.
 */

static float  prob_term;
static float *basic_prob = NULL;
static int    basic_n    = 0;

/*---------------------------------------------------------------
 Routine : set_prob_term
 Purpose : sets static prob_term to n
---------------------------------------------------------------*/
void
  set_prob_term(float n)
{
	prob_term = n;
}

/*---------------------------------------------------------------
 Routine : set_basic_n
 Purpose : sets static basic_n to n
---------------------------------------------------------------*/
void
  set_basic_n(int n)
{
	basic_n = n;
}

/*---------------------------------------------------------------
 Routine : set_basic_prob
 Purpose : sets static basic_prob to n_ptr
---------------------------------------------------------------*/
void
  set_basic_prob(float *n_ptr)
{
	basic_prob = n_ptr;
}


/*---------------------------------------------------------------
 Routine : calc_sub_term
 Purpose : Calculate the probability of a combination of m.c.s.
 and add to the current term.

 Form the AND of the m.c.s. given in z
 Multiply the probs as given in static vector basic_prob
 Add it to prob_term
---------------------------------------------------------------*/
void
  calc_sub_term(
    int    *z,      /* IN  - array containing combination */
    int     zi,     /* IN  - length of combination        */
    Group **index)  /* IN  - user data index to m.c.s.    */
{
    Group    *g = GroupCreateN(basic_n), *tmp;
/*     char     *s; */
    float     p;
    int       i;

    for( i=0; i<zi; i++ ) {
        tmp = GroupAND(g, index[z[i]]);
        GroupDestroy(g);
        g = tmp;
    }

    p = 1.0;
    for(i = 0; i < g->b->n; i++) {
        if (BitGet(g->b, (g->b->n-1) - i)) {
            p *= basic_prob[i];
        }
    }

 /* printf("[%s : %f] ", s=BitString(g->b), p);
  * free(s);
  */

    GroupDestroy(g);
    prob_term += p;

} /* calc_sub_term */

/*---------------------------------------------------------------
 Routine : ExprCutsetProbs
 Purpose : Calculate probability of individual cut sets in an
 expression.

 Call calc_sub_term() for "combinations" consisting of just
 one cut set. Put values in term array.
---------------------------------------------------------------*/
float *                     /* RET - probability array                       */
  ExprCutsetProbs(
    Expr   e,   /* IN  - expression                              */
    float *pr)  /* IN  - array of probabilities  of basic events */
{
    float  *term;  /* array of terms        */
    int n = 0;     /* number of groups      */
    Group **index; /* index to the groups   */
    Group  *p;     /* pointer               */
    int     z;     /* cut set prob to evaluate */
    int     i;
    float  *error_value;

    if ( !fNewMemory( (void *)&error_value, sizeof( float ) ) )
    {
      exit( 1 );
    }

    if ( BitComp(e->b, stop) == 0 ) {
        printf("(NULL Expr)\n");
        *error_value = 0.0;
        return error_value;
    }

 /* set static variables */
    basic_prob = pr;
    basic_n    = e->b->n;

 /* count the groups */
    n = ExprCount(e);

 /* malloc arrays */
/*     index = (Group**)malloc( n * sizeof(Group *) ); */
    if ( !fNewMemory( (void *)&index, ( n * sizeof(Group *) ) ) )
    {
      exit( 1 );
    }
 
/*     term  = (float *)malloc( n * sizeof(float) ); */
    if ( !fNewMemory( (void *)&term, ( n * sizeof(float) ) ) )
    {
      exit( 1 );
    }

 /* create index to groups */
    for( i=0, p=e; BitComp(p->b, stop) != 0; i++, p=p->next ) {
        index[i] = p;
    }

    for (z = 0; z < n; z++) {
        prob_term = 0;
        calc_sub_term(&z, 1, index);
        term[z] = prob_term;
    }

    FreeMemory(index);
	FreeMemory(error_value);
    return term;

} /* ExprCutsetProbs */

/*---------------------------------------------------------------
 Routine : ExprProb
 Purpose : Calculate probability of an expression

 See Lewis p367 & 369
---------------------------------------------------------------*/
float                       /* RET - probability                             */
  ExprProb( 
    Expr   e,         /* IN  - expression                              */
    float *pr,        /* IN  - array of probabilities  of basic events */
    int    max_order, /* IN  - max order of cut sets to use            */
    int    nt)        /* IN  - term (0 means all)                      */
{
    float   prob;  /* total probability     */
    int n = 0;     /* number of groups      */
    Group **index; /* index to the groups   */
    Group  *p;     /* pointer               */
    int    *x;     /* array to permute      */
    int    *z;     /* array of permutations */
    int     c;     /* number of combs       */
    char   *sign;
    clock_t time1, time2;
    float   time;
    int     i, order;

 /* start clock */
    time1 = clock();

    if ( BitComp(e->b, stop) == 0 ) {
        printf("(NULL Expr)\n");
        return 0.0;
    }

 /* set static variables */
    basic_prob = pr;
    basic_n    = e->b->n; 

 /* count the groups, use only up to max_order */
    for( p=e, n=0; BitComp(p->b, stop) != 0; p=p->next, n++ ) {
        if ( GroupOrder(p) > max_order ) break;
    }
/*     printf("ExprProb: n = %d\n", n); */

 /* malloc arrays */
/*     index = (Group**)malloc( n * sizeof(Group *) ); */
    if ( !fNewMemory( (void *)&index, ( n * sizeof(Group *) ) ) )
    {
      exit( 1 );
    }

/*     x     = (int  * )malloc( n * sizeof(int) ); */
    if ( !fNewMemory( (void *)&x, ( n * sizeof(int) ) ) )
    {
      exit( 1 );
    }

/*     z     = (int  * )malloc( n * sizeof(int) ); */
    if ( !fNewMemory( (void *)&z, ( n * sizeof(int) ) ) )
    {
      exit( 1 );
    }

 /* create index to groups and array to permute */
    for( i=0, p=e; i < n; i++, p=p->next ) {
        index[i] = p;
        x[i] = i;
    }

 /* calculate prob */
    prob = 0;

    if (nt == 0) {  /* all terms */

        for ( order = 1; order <= n; order++ ) {
            c = nCr(n, order);
/*             printf("\norder = %d : c = %d\n", order, c); */
            prob_term = 0;
            combs(x, n, order, z, 0, calc_sub_term, index);
            prob_term *= ((order % 2) ? 1.0 : -1.0 );
            sign       = ((order % 2) ? "+" :  "" );
            prob += prob_term;
/*             printf("%d order : prob= %f (%s%f)\n", */
/*                    order, prob, sign, prob_term); */
        }
    } else {        /* just term nt */

     /* c = nCr(n, nt);
      * printf("\nterm = %d : c = %d\n", nt, c);
      */

        prob_term = 0;
        combs(x, n, nt, z, 0, calc_sub_term, index);
        prob_term *= ((nt % 2) ? 1.0 : -1.0 );
        prob += prob_term;
    }
    
    FreeMemory(index);
    FreeMemory(x);
    FreeMemory(z);

    time2 = clock();

    time = (time2-time1)/(float)CLOCKS_PER_SEC;

/*     printf("ExprProb : nt = %d, max_order = %d : time = %f, time/N = %f\n", */
/*            nt, max_order, time, time/(nCr(n, nt)) ); */

    return prob;

} /* ExprProb */

#endif /* ndef TEST */

/*---------------------------------------------------------------
 FOR TESTING PURPOSES ONLY vvvvvvvvvvvvvvvvvv
---------------------------------------------------------------*/

#ifdef TEST
main()
{
    BitArray *bit[20];
    Group *g1, *g2, *g3;
    Expr  e1, e2, e3, e4, e5;
    int   flg, i;
    FILE *fp;

    for (i=0; i<20; i++) {
        bit[i] = BitCreate(10);
        BitSetInt(bit[i], i);
     /* printf("BitComp(bit[%d], stop) = %d\n", i, BitComp(bit[i], stop) ); */
    }
    g1 = GroupCreate(bit[5]);
    g2 = GroupCreate(bit[14]);
    g3 = GroupAND(g1, g2);

    printf("g1        = %s\n", BitString(g1->b));
    printf("g2        = %s\n", BitString(g2->b));
    printf("g1 AND g2 = %s\n", BitString(g3->b));

    printf("\ne1 :\n");
    e1 = ExprCreate();
    ExprPrint(e1);
    e1 = ExprORGroup(e1, g1, &flg);
    ExprPrint(e1);
    e1 = ExprORGroup(e1, g3, &flg);
    ExprPrint(e1);
    e1 = ExprORGroup(e1, g2, &flg);
    ExprPrint(e1);

    printf("bit[12] = %s\n", BitString(bit[12]) );
    e1 = ExprORGroup(e1, GroupCreate(bit[12]), &flg);
    printf("flg = %d\n", flg);
    ExprPrint(e1);

    e1 = ExprORGroup(e1, GroupCreate(bit[7]), &flg);
    ExprPrint(e1);

    e1 = ExprORGroup(e1, GroupCreate(bit[9]), &flg);
    ExprPrint(e1);

    printf("\ne2 :\n");
    e2 = ExprCreate();
    ExprPrint(e2);
    e2 = ExprORGroup(e2, GroupCreate(bit[6]), &flg);
    ExprPrint(e2);
    e2 = ExprORGroup(e2, GroupCreate(bit[14]), &flg);
    ExprPrint(e2);
    e2 = ExprORGroup(e2, GroupCreate(bit[13]), &flg);
    ExprPrint(e2);

    printf("\ne3 :\n");
    e3 = ExprCopy(e2);
    ExprPrint(e3);

    printf("\ne4 :\n");
    e4 = ExprAND(e1, e2, 0);
    ExprPrint(e4);

    printf("\ne5 :\n");
    e5 = ExprOR(e1, e2);
    ExprPrint(e5);

    fp = fopen("exprtest", "w");
    ExprWrite(fp, e5);
    fclose(fp);

    fp = fopen("exprtest", "r");
    e4 = ExprRead(fp);
    fclose(fp);
    ExprPrint(e4);

}
#endif /* def TEST */
