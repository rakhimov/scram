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

/****************************************************************
 Module : list_suu.h
 Author : FSC Limited
 Date   : 30/7/96
 SccsId : @(#)list_suu.h	1.2 11/08/96

 C version of Booch List_Single_Unbounded_Unmanaged

****************************************************************/

#ifndef LIST_SUU
#define LIST_SUU

#include "boolean.h"

typedef void *Item;

typedef struct node {
  Item The_Item;
  struct node *Next;
} Node;

typedef Node *List;

extern const List Null_List;

/*--------------------------------------------------------------
 Routine : Copy
 Purpose : Creates a copy of a list.
---------------------------------------------------------------*/
extern void
  Copy(
    const List From_The_List,
          List *To_The_List );

/*--------------------------------------------------------------
 Routine : Clear
 Purpose : Routine clears The_List without freeing any allocated
 space.
---------------------------------------------------------------*/
extern void
  Clear(
    List *The_List );

/*--------------------------------------------------------------
 Routine : Managed_Clear
 Purpose : Routine iteratively traverses The_List deleting each
           node and deallocating memory, until the list is empty.
---------------------------------------------------------------*/
extern void
  Managed_Clear(
    List *The_List );


/*--------------------------------------------------------------
 Routine : Append
 Purpose : Adds a new node to the bottom of the list.
---------------------------------------------------------------*/
void
  Append(
    const Item The_Item,
          List *The_List );

/*--------------------------------------------------------------
 Routine : Construct
 Purpose : Adds a new node to the top of the list.
---------------------------------------------------------------*/
extern void
  Construct(
    const Item The_Item,
          List *And_The_List );

/*--------------------------------------------------------------
 Routine : Set_Head
 Purpose : Sets the value of The_Item of the top node in the list
           to that passed in as the second parameter.
---------------------------------------------------------------*/
extern void
  Set_Head(
          List *Of_The_List,
    const Item To_The_Item );

/*--------------------------------------------------------------
 Routine : Swap_Tail
 Purpose : Routine swps the tails of two lists.
---------------------------------------------------------------*/
extern void
  Swap_Tail(
    List *Of_The_List,
    List *And_The_List );

/*--------------------------------------------------------------
 Routine : Is_Equal
 Purpose : Compares two lists node by node and returns true if
           all the nodes contain equivalent The_Item's.
---------------------------------------------------------------*/
extern BOOL
  Is_Equal(
    const List Left,
    const List Right );

/*--------------------------------------------------------------
 Routine : Length_Of
 Purpose : Routine returns the number of nodes in a list as an
           unsigned int.
---------------------------------------------------------------*/
extern unsigned
  Length_Of(
    const List The_List );

/*--------------------------------------------------------------
 Routine : Is_Null
 Purpose : Returns TRUE if the list passed in is a NULL list.
---------------------------------------------------------------*/
extern BOOL
  Is_Null(
    const List The_List );

/*--------------------------------------------------------------
 Routine : Head_Of
 Purpose : Returns The_Item at the top of the list.
---------------------------------------------------------------*/
extern Item
  Head_Of(
    const List The_List );

/*--------------------------------------------------------------
 Routine : Tail_Of
 Purpose : Returns the tail of a list i.e. the input list minus
           the top node.
---------------------------------------------------------------*/
extern List
  Tail_Of(
    const List The_List );

#endif
