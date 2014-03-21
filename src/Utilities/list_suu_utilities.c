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
 Module : list_suu_utilities.h
 Author : FSC Limited
 Date   : 30/7/96
 SccsId : @(#)list_suu_utilities.c	1.2 09/26/96
 
 C version of Booch List_Single_Unbounded_Unmanaged
 
****************************************************************/

#include <stdlib.h>
/* #include <stdio.h>    */

#include "list_suu_utilities.h"

#include "MemoryUtil.h"
#include "AssertUtil.h"

ASSERTFILE(__FILE__)

/*--------------------------------------------------------------
 Routine : lsu_Foot_Of
 Purpose : Returns a pointer (of type List) to the last node of a list 
---------------------------------------------------------------*/
List
  lsu_Foot_Of(
    const List The_List )
{
  List Index = The_List;

  while ( !Is_Null( Tail_Of( Index ) ) ) {
    Index = Tail_Of( Index );
  }
  return Index;

} /* Foot_Of */



/*--------------------------------------------------------------
 Routine : lsu_Construct
 Purpose : Concatenates the second list to the end of the first list.
---------------------------------------------------------------*/
void
  lsu_Construct(
    List *The_List,
    List *And_The_List )
{
  List Index = lsu_Foot_Of( *The_List );
  List Temporary_List = *And_The_List;

  Swap_Tail( &Index, &Temporary_List );

} /* Construct */


/*--------------------------------------------------------------
 Routine : lsu_Retreive_Item
 Purpose : Retreives an item from the. Returns NULL if item not
           found
---------------------------------------------------------------*/
Item 
  lsu_Retreive_Item( 
    List     In_The_List, 
    unsigned At_The_Position )
{
  List Temporary_List = NULL, Removed_List = NULL;  
  int i;

  ASSERT( At_The_Position > 0 );
  ASSERT( !Is_Null( In_The_List ) );

  for(i=1; i<At_The_Position && In_The_List; i++) {
	In_The_List = Tail_Of(In_The_List);
  }

  if(In_The_List) {
	return In_The_List->The_Item;
  } else {
	  return NULL;
  }

} /* Retreive_Item */ 



/*--------------------------------------------------------------
 Routine : Location_Of 
 Purpose : Returns the list starting at The_Position In_The_List
           Must not be passed The_Position==1 (use Head_Of instead)
           The_Position must be a legal value
           ie The_Position < Length_Of(In_The_List)
---------------------------------------------------------------*/
List
  Location_Of(
    const unsigned The_Position,
    const List     In_The_List )
{
  List Index;
  unsigned Count;

  ASSERT( The_Position > 0 );

/*   if Is_Null( In_The_List )  */
/*   { */
/*     return NULL; */
/*   } else { */
  ASSERT( !( Is_Null( In_The_List ) ) );

    Index = In_The_List;
    for ( Count=2; Count <= The_Position; Count++ ) {
      Index = Tail_Of( Index );
/*       if ( Is_Null( Index ) ) { */
/*         return NULL; */
/*       } */
  ASSERT( !( Is_Null( Index ) ) );

    } 
    return Index;
/*   } */
  
} /* Location_Of */



/*--------------------------------------------------------------
 Routine : lsu_Split
 Purpose : Splits The_List into two separate lists At_The_Position
           and puts the list after At_The_Position Into_The_List
---------------------------------------------------------------*/
void
  lsu_Split(
          List     *The_List,
    const unsigned At_The_Position,
          List     *Into_The_List )
{
  List Index;

  ASSERT( At_The_Position > 0 );
  ASSERT( !Is_Null( *The_List ) );
  ASSERT( Length_Of( *The_List ) > 1 );

  Index = Location_Of( ( At_The_Position - 1 ), *The_List );
  Clear( Into_The_List ); 
  Swap_Tail( &Index, Into_The_List );  

} /* Split */



/*--------------------------------------------------------------
 Routine : lsu_InsertItemByPosition
 Purpose : Inserts The_Item into In_The_List After_The_Position
---------------------------------------------------------------*/
void
  lsu_InsertItemByPosition(
    const Item     The_Item,
          List     *In_The_List,
    const unsigned After_The_Position )
{
  List Index = Location_Of( After_The_Position, *In_The_List );
  List Temporary_List;

  ASSERT( After_The_Position > 0 );

  Construct( The_Item, &Temporary_List );
  Swap_Tail( &Index, &Temporary_List );
  Index = Tail_Of( Index );
  Swap_Tail( &Index, &Temporary_List );

} /* Insert */



/*--------------------------------------------------------------
 Routine : lsu_InsertListByPosition
 Purpose : Inserts The_List After_The_Position In_The_list
---------------------------------------------------------------*/
void
  lsu_InsertListByPosition(
          List     *The_List,
          List     *In_The_List,
    const unsigned After_The_Position )
{
  List Index = Location_Of( After_The_Position, *In_The_List );
  List Temporary_List = *The_List;
  List Temporary_Tail = lsu_Foot_Of( *The_List );

  ASSERT( After_The_Position > 0 );

  Swap_Tail( &Index, &Temporary_List );
  Swap_Tail( &Temporary_Tail, &Temporary_List );

} /* Insert */



/*--------------------------------------------------------------
 Routine : lsu_Remove_Item
 Purpose : Removes an item from the list and frees it's memory.
---------------------------------------------------------------*/
void 
  lsu_Remove_Item( 
    List     *In_The_List, 
    unsigned At_The_Position )
{
  List Temporary_List = NULL, Removed_List = NULL;  

  ASSERT( At_The_Position > 0 );
  ASSERT( !Is_Null( *In_The_List ) );

  if ( At_The_Position > 1 ) {
    /* Split the list into two, one pointed to by In_The_List,
       the other by Temporary_List. The item to be removed is at
       the head of the Temporary_List. */
    lsu_Split( In_The_List, At_The_Position, &Temporary_List );  

    /* Discard unwanted item, and free the memory. */
    Removed_List = Temporary_List;
    Temporary_List = Tail_Of( Temporary_List );
	FreeMemory(Removed_List->The_Item);
    FreeMemory(Removed_List);

    /* Join the temporary list back to the given list */
    lsu_Construct( In_The_List, &Temporary_List );    

  } else {
    Removed_List = *In_The_List;
    *In_The_List = Tail_Of( *In_The_List );
	FreeMemory(Removed_List->The_Item);
    FreeMemory(Removed_List);  }

} /* Remove_Item */ 

/*--------------------------------------------------------------
 Routine : Managed_Remove_Item
 Purpose : Removes an item from the list and frees it's memory.
---------------------------------------------------------------*/
void 
  Managed_Remove_Item( 
    List     *In_The_List, 
    unsigned At_The_Position )
{
  List Temporary_List = NULL;
  List Removed_List = NULL;
 
  ASSERT( At_The_Position > 0 );
  ASSERT( !Is_Null( *In_The_List ) );
 
  if ( At_The_Position > 1 ) {
    /* Split the list into two, one pointed to by In_The_List,
       the other by Temporary_List. The item to be removed is at
       the head of the Temporary_List. */
    lsu_Split( In_The_List, At_The_Position, &Temporary_List );
 
    /* Discard unwanted item, and free the memory. */
    Removed_List = Temporary_List; /* freeing the deleted node */
    Temporary_List = Tail_Of( Temporary_List );
	FreeMemory(Removed_List->The_Item);
    FreeMemory(Removed_List);
 
    /* Join the temporary list back to the given list */
    lsu_Construct( In_The_List, &Temporary_List );
 
  } else {
    Removed_List = *In_The_List;
    *In_The_List = Tail_Of( *In_The_List ); /* freeing the deleted node */
	FreeMemory(Removed_List->The_Item);
    FreeMemory(Removed_List);
  }
 
} /* Managed_Remove_Item */

