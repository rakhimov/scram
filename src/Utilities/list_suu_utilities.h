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
 SCCS   :@(#)list_suu_utilities.h	1.1 08/22/96
 
 C version of Booch List_Single_Unbounded_Unmanaged
 
****************************************************************/

#ifndef LIST_SU_H
#define LIST_SU_H

#include "boolean.h"
#include "list_suu.h"


/*--------------------------------------------------------------
 Routine : lsu_Construct 
 Purpose : Concatenates the second list to the end of the first list.
---------------------------------------------------------------*/
extern void
  lsu_Construct(
    List *The_List,
    List *And_The_List );

/*--------------------------------------------------------------
 Routine : lsu_Split
 Purpose : Splits The_List into two separate lists At_The_Position
           and puts the list after At_The_Position Into_The_List
---------------------------------------------------------------*/
extern void
  lsu_Split(
          List     *The_List,
    const unsigned At_The_Position,
          List     *Into_The_List );

/*--------------------------------------------------------------
 Routine : lsu_InsertItemByPosition 
 Purpose : Inserts The_Item into In_The_List After_The_Position
---------------------------------------------------------------*/
extern void
  lsu_InsertItemByPosition(
    const Item     The_Item,
          List     *In_The_List,
    const unsigned After_The_Position );

/*--------------------------------------------------------------
 Routine : lsu_InsertListByPosition
 Purpose : Inserts The_List After_The_Position In_The_list
---------------------------------------------------------------*/
extern void
  lsu_InsertListByPosition(
          List     *The_List,
          List     *In_The_List,
    const unsigned After_The_Position );

/* extern void */
/*   lsu_InsertItem( */
/*     const Item The_Item, */
/*           List *After_The_List ); */
/*  */
/* extern void */
/*   lsu_InsertList( */
/*     const List The_List, */
/*           List *After_The_List ); */
/*  */

/*--------------------------------------------------------------
 Routine : lsu_Remove_Item
 Purpose : Removes an item from the list.
---------------------------------------------------------------*/
extern void 
  lsu_Remove_Item( 
    List     *In_The_List, 
    unsigned At_The_Position );

/*--------------------------------------------------------------
 Routine : lsu_Retreive_Item
 Purpose : Retreives an item from the. Returns NULL if item not
           found
---------------------------------------------------------------*/
extern Item 
  lsu_Retreive_Item( 
    List     In_The_List, 
    unsigned At_The_Position );

/*--------------------------------------------------------------
 Routine : Managed_Remove_Item
 Purpose : Removes an item from the list and frees it's memory.
---------------------------------------------------------------*/
extern void 
  Managed_Remove_Item( 
    List     *In_The_List, 
    unsigned At_The_Position );

/*--------------------------------------------------------------
 Routine : lsu_Foot_Of 
 Purpose : Returns a pointer (of type List) to the last node of a list
---------------------------------------------------------------*/
extern List
  lsu_Foot_Of(
    const List The_List );

/* extern BOOL */
/*   lsu_Is_Head( */
/*     const List The_List ); */

#endif
