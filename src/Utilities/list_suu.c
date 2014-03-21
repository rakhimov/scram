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
 SccsId : @(#)list_suu.c	1.2 09/26/96
 
 C version of Booch List_Single_Unbounded_Unmanaged
 
****************************************************************/

#include <stdio.h>
/* #include <stdlib.h> */

#include "list_suu.h"

#include "MemoryUtil.h"

#include "AssertUtil.h"

ASSERTFILE(__FILE__)

/*--------------------------------------------------------------
 Routine : Copy
 Purpose : Creates a copy of a list.
---------------------------------------------------------------*/
void
  Copy(
    const List From_The_List,
          List *To_The_List )
{
  List From_Index = From_The_List;
  List To_Index;

  if ( From_The_List == NULL )
  {
    To_The_List = NULL;
  } else {
/*     *To_The_List = (Node *)malloc( sizeof( Node ) ); */
    if ( fNewMemory( (void **)To_The_List, sizeof( Node ) ) )
    {

      (*To_The_List)->The_Item = From_Index->The_Item;
      (*To_The_List)->Next = NULL;

      To_Index = *To_The_List;
      From_Index = From_Index->Next;

      while ( From_Index != NULL ) {
/*         To_Index->Next = (Node *)malloc( sizeof( Node ) ); */
        if ( fNewMemory( (void **)(To_Index->Next), sizeof( Node ) ) )
        {
          To_Index->Next->The_Item = From_Index->The_Item;
          To_Index->Next->Next = NULL;
          To_Index = To_Index->Next;
          From_Index = From_Index->Next;
        }
        else
          From_Index = NULL;
      }

    }
    else
      *To_The_List = NULL;
  }

} /* Copy */

/*--------------------------------------------------------------
 Routine : Clear
 Purpose : Routine iteratively traverses The_List deleting each
           node and deallocating memory, until the list is empty.
---------------------------------------------------------------*/
void
  Clear(
    List *The_List )
{
  The_List = NULL;
} /* Clear */

/*--------------------------------------------------------------
 Routine : Managed_Clear
 Purpose : Routine iteratively traverses The_List deleting each
           node and deallocating memory, until the list is empty.
---------------------------------------------------------------*/
void
  Managed_Clear(
    List *The_List )
{
  List Temporary_Node;
/*   List TempList; */
 
  while((*The_List)!= NULL){
    Temporary_Node = (*The_List);
    (*The_List) = (*The_List)->Next;
    FreeMemory(Temporary_Node);
  }

} /* Managed_Clear */



/*--------------------------------------------------------------
 Routine : Append
 Purpose : Adds a new node to the bottom of the list.
---------------------------------------------------------------*/
void
  Append(
    const Item The_Item,
          List *The_List )
{
  List Temporary_Node;
  List Last_Node;

  /* create new node */
  if (fNewMemory( (void **)&Temporary_Node, sizeof( Node ) ) ) {

	Temporary_Node->The_Item = The_Item;
    Temporary_Node->Next = NULL;

	if(*The_List == NULL) {
	  /* list empty so point list to one and only node */
      *The_List = Temporary_Node;
	} else {
	  /* find end of list and append new node */
      Last_Node = *The_List;
      while ( Last_Node->Next != NULL ) {
        Last_Node = Last_Node->Next;
	  }
    
      Last_Node->Next = Temporary_Node;
	}
  } else {
	/* node creation failed */
    *The_List = NULL;
  }

} /* Append */


/*--------------------------------------------------------------
 Routine : Construct
 Purpose : Adds a new node to the top of the list.
---------------------------------------------------------------*/
void
  Construct(
    const Item The_Item,
          List *And_The_List )
{
  List Temporary_Node;

/*   Temporary_Node = (List)malloc( sizeof( Node ) ); */
  if (fNewMemory( (void **)&Temporary_Node, sizeof( Node ) ) )
  {
    Temporary_Node->The_Item = The_Item;
    Temporary_Node->Next = *And_The_List;
    *And_The_List = Temporary_Node;
  }
  else
    *And_The_List = NULL;

} /* Construct */

/*--------------------------------------------------------------
 Routine : Set_Head
 Purpose : Sets the value of The_Item of the top node in the list
           to that passed in as the second parameter.
---------------------------------------------------------------*/
void
  Set_Head(
          List *Of_The_List,
    const Item To_The_Item )
{
  ASSERT( Of_The_List != NULL );

  (*Of_The_List)->The_Item = To_The_Item;

} /* Set_Head */

/*--------------------------------------------------------------
 Routine : Swap_Tail
 Purpose : Routine swps the tails of two lists.
---------------------------------------------------------------*/
void
  Swap_Tail(
    List *Of_The_List,
    List *And_The_List )
{
  List Temporary_Node;

  ASSERT( Of_The_List != NULL );

  Temporary_Node = (*Of_The_List)->Next;
  (*Of_The_List)->Next = *And_The_List;
  *And_The_List = Temporary_Node;

} /* Swap_Tail */

/*--------------------------------------------------------------
 Routine : Is_Equal
 Purpose : Compares two lists node by node and returns true if
           all the nodes contain equivalent The_Item's.
---------------------------------------------------------------*/
BOOL
  Is_Equal(
    const List Left,
    const List Right )
{
  List Left_Index = Left;
  List Right_Index = Right;

  while ( Left_Index != NULL ) {
    if ( Left_Index->The_Item != Right_Index->The_Item )
    {
      return FALSE;
    } 
    Left_Index = Left_Index->Next;
    Right_Index = Right_Index->Next;
  }

  return ( Right_Index == NULL );

} /* Is_Equal */

/*--------------------------------------------------------------
 Routine : Length_Of
 Purpose : Routine returns the number of nodes in a list as an
           unsigned int.
---------------------------------------------------------------*/
unsigned
  Length_Of(
    const List The_List )
{
  unsigned Count = 0;
  List Index = The_List;

  while ( Index != NULL ) {
    Count = Count + 1;
    Index = Index->Next;
  }
  return Count;

} /* Length_Of */

/*--------------------------------------------------------------
 Routine : Is_Null
 Purpose : Returns TRUE if the list passed in is a NULL list.
---------------------------------------------------------------*/
BOOL
  Is_Null(
    const List The_List )
{
  return ( The_List == NULL );
} /* Is_Null */

/*--------------------------------------------------------------
 Routine : Head_Of
 Purpose : Returns The_Item at the top of the list.
---------------------------------------------------------------*/
Item
  Head_Of(
    const List The_List )

{
  ASSERT( The_List != NULL );

  return The_List->The_Item;

} /* Head_Of */

/*--------------------------------------------------------------
 Routine : Tail_Of
 Purpose : Returns the tail of a list i.e. the input list minus
           the top node.
---------------------------------------------------------------*/
List
  Tail_Of(
    const List The_List )
{
  ASSERT( The_List != NULL );

  return The_List->Next;

} /* Tail_Of */

