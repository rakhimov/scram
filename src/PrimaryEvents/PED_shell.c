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
 Module : PED_shell.h
 Author : FSC Limited
 Date   : 29/7/96
 SccsId : @(#)PED_shell.c	1.6 01/28/97
 
 This module implements the shell handler for the Primary Events
 database.
 A new shell is created if no shell already exists for the supplied
 database name (including NULL).
 Each shell creates a new instance of the Primary Events Database
 application, and a list of shells is maintained. Each application
 holds it's current database name in user data attached to the shell
 widget.
 
****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "PED_shell.h"

#include "FileUtilities.h"
#include "util.h"
#include "list_suu.h"
#include "primary_events_database.h"
#include "MemoryUtil.h"

#include "AssertUtil.h"

#include "TopLevelData.h"

ASSERTFILE(__FILE__)

static List shell_list = NULL;

/*--------------------------------------------------------------
 Routine : ped_shell_primary_event_exists
 Purpose : Returns whether the primary event Id exists in the
 supplied database.
---------------------------------------------------------------*/
BOOL 
  ped_shell_primary_event_exists(
    char *id ,
    char *database_name)
{
	List tmp;
	PRIMARY_EVENT *pe;
	PRIMARY_EVENTS_DATABASE *ped_state;
	char *s, *s1;

	tmp = shell_list;
	while ( tmp != NULL ) {
		ped_state = (PRIMARY_EVENTS_DATABASE*)Head_Of( tmp );

		ASSERT( ped_state != NULL ); 

		if ( ( database_name == NULL ) && 
			( ped_state->current_primary_events_database == NULL ) ) {
			pe = ped_state->primary_event_get( id, ped_state );
			return ( pe != NULL );
		} else {
			if ( ( database_name != NULL ) &&
				( ped_state->current_primary_events_database != NULL ) ) {
				if ( strcmp( 
					( s = (char *)filename_from_pathname(
					ped_state->current_primary_events_database ) ),
					( s1 = (char *)filename_from_pathname(database_name ) ) ) == 0 ) {
					strfree( s ); 
					strfree( s1 ); 
					pe = ped_state->primary_event_get( id, ped_state );
					return ( pe != NULL );
				} else {  
					strfree( s );  
					strfree( s1 );  
				}  
			}
			tmp = Tail_Of( tmp );
		}
	} 

	return FALSE;
} /* ped_shell_primary_event_exists */



/*--------------------------------------------------------------
 Routine : ped_shell_create
 Purpose : Routine creates a shell and adds the necessary callback
           functions to it.
---------------------------------------------------------------*/
PRIMARY_EVENTS_DATABASE *ped_shell_create()
{
  	PRIMARY_EVENTS_DATABASE *ped_state;

	if ( !fNewMemory( (void **)&ped_state, sizeof( PRIMARY_EVENTS_DATABASE ) ) )
	{
		exit( 1 );
	}

	ped_state->shell = TRUE;
	ped_state->current_primary_events_database = NULL;
	ped_state->primary_events_list = NULL; 
	ped_state->ped_change_since_saved = FALSE;
	ped_state->primary_event_get_selected =
    primary_events_database__primary_event_get_selected;
	ped_state->primary_event_get =
    primary_events_database__primary_event_get;

	ped_state->modify_callback = 0;
	ped_state->user_data = NULL;

	ped_shell_add( ped_state );
	return(ped_state);

} /* ped_shell_create */



/*--------------------------------------------------------------
 Routine : ped_shell_add
 Purpose : Routine makes a new Shell_Item
           Point to first shell in list
           Make this shell the first in list
---------------------------------------------------------------*/ 
void
  ped_shell_add(
    PRIMARY_EVENTS_DATABASE *ped_state)
{

	Append(ped_state, &shell_list);
  //Construct( ped_state, &shell_list );

} /* ped_shell_add */




/*--------------------------------------------------------------
 Routine : ped_shell_get_primary_event
 Purpose :
---------------------------------------------------------------*/
PRIMARY_EVENT *
  ped_shell_get_primary_event(
    char *id ,
    char *database_name )
{
  List tmp;
  PRIMARY_EVENT *pe;
  PRIMARY_EVENTS_DATABASE *ped_state;
  char *s, *s1;

  ASSERT( ( id != NULL ) );
  tmp = shell_list;

  while ( tmp != NULL ) {
    ped_state = (PRIMARY_EVENTS_DATABASE*)Head_Of( tmp );

    ASSERT( ped_state != NULL ); 

    if ( ( database_name == NULL ) && 
         ( ped_state->current_primary_events_database == NULL ) ) {
      pe = ped_state->primary_event_get( id, ped_state ); 
      return pe;
    }
    else {
      if ( ( database_name != NULL ) &&
           ( ped_state->current_primary_events_database != NULL ) ) {
        if ( strcmp( 
               ( s = (char *)filename_from_pathname(
                 ped_state->current_primary_events_database ) ),
               ( s1 = (char *)filename_from_pathname(
                 database_name ) ) ) == 0 ) {
          strfree( s ); 
          strfree( s1 ); 
          pe = ped_state->primary_event_get( id, ped_state );
          return pe;
        } 
        else  
        {  
          strfree( s );  
          strfree( s1 );  
        }  
      }
        tmp = Tail_Of( tmp );
    }
  } 

  return NULL;

} /* ped_shell_get_primary_event */




/*--------------------------------------------------------------
 Routine : ped_shell_exists
 Purpose : Return whether the named database is opened in an
 application shell.
---------------------------------------------------------------*/
BOOL
  ped_shell_exists(
    char *database_name ) 
{
  List tmp;
  PRIMARY_EVENTS_DATABASE *ped_state;

  /* for each shell in list
       if database_name found in shell
       then
         return TRUE;
       end if;
     end for;

     return FALSE;
  */

  tmp = shell_list;
  while ( tmp != NULL ) {
    ped_state = (PRIMARY_EVENTS_DATABASE*)Head_Of( tmp );

    ASSERT( ped_state != NULL ); 

    if ( ( database_name == NULL ) && 
      ( ped_state->current_primary_events_database == NULL ) ) {
      return TRUE;
    }
    else {
      if ( ( database_name != NULL ) &&
           ( ped_state->current_primary_events_database != NULL ) ) {
        if ( strcmp( 
               (char *)filename_from_pathname(
                 ped_state->current_primary_events_database ), 
               (char *)filename_from_pathname(
                 database_name ) ) == 0 ) {
          return TRUE;
        } 
      }
        tmp = Tail_Of( tmp );
    }
  } 

  return FALSE;

} /* ped_shell_exists */



/*--------------------------------------------------------------
 Routine : ped_shell_any_exist
 Purpose : Routine returns TRUE if any shells exist
---------------------------------------------------------------*/ 
BOOL
  ped_shell_any_exist()
{

  return ( Length_Of( shell_list ) > 0 );

} /* ped_shell_any_exist */



/*--------------------------------------------------------------
 Routine : ped_shell_open
 Purpose : Returns whether the named database is open in an
 application shell.
---------------------------------------------------------------*/
BOOL
  ped_shell_open(
    char* database_name )
{
	List tmp;
	PRIMARY_EVENTS_DATABASE *ped_state;

	tmp = shell_list;
	while ( tmp != NULL ) {
		ped_state = (PRIMARY_EVENTS_DATABASE*)Head_Of( tmp ) ;

		ASSERT( ped_state != NULL ); 

		if ( database_name != NULL ) {
			return(primary_events_database_open_cb(database_name));
			return FALSE;
		} else {
			if ( ped_state->current_primary_events_database == NULL ) 
				return TRUE;
		}
		tmp = Tail_Of( tmp );
	} 

	return FALSE;

} /* ped_shell_open */


/*--------------------------------------------------------------
 Routine : ped_shell_add_modify_callback
 Purpose : To perform any required actions in the application that
 spawned the Database shell.
---------------------------------------------------------------*/
void
  ped_shell_add_modify_callback(
    char *database_name,
    void (*modify_callback)(void *),
    void *user_data )
{
  List tmp;
  PRIMARY_EVENTS_DATABASE *ped_state;

  /* for each shell in list
       if database_name found in shell
       then
         add callback function and user data to ped_state;
       end if;
     end for;
  */

  tmp = shell_list;
  while ( tmp != NULL ) {
    ped_state = (PRIMARY_EVENTS_DATABASE*)Head_Of( tmp );

    ASSERT( ped_state != NULL ); 

    if ( ( database_name == NULL ) && 
      ( ped_state->current_primary_events_database == NULL ) ) {
      primary_events_database_add_modify_callback(
        ped_state,
        modify_callback,
        user_data );
      return;
    }
    else {
      if ( ( database_name != NULL ) &&
           ( ped_state->current_primary_events_database != NULL ) ) {
        if ( strcmp( 
               (char *)filename_from_pathname(
                 ped_state->current_primary_events_database ), 
               (char *)filename_from_pathname(
                 database_name ) ) == 0 ) {
          primary_events_database_add_modify_callback(
            ped_state,
            modify_callback,
            user_data );
	  return;
        } 
      }
        tmp = Tail_Of( tmp );
    }
  } 

} /* ped_shell_add_modify_callback */



/*--------------------------------------------------------------
 Routine : ped_shell_first
 Purpose : Return the name of the first database in the list.
---------------------------------------------------------------*/
char *
  ped_shell_first()
{
/*  PRIMARY_EVENTS_DATABASE *ped_state; */

/*   ASSERT( ped_state != NULL );  */
/*
  if ( shell_list != NULL ) {
    ped_state = ped_shell_get_state(
      *(Shell_Item)Head_Of( shell_list ) );

    return ped_state->current_primary_events_database; 
  }
  else
    return NULL;
*/

    /* supposed to get first in list, but we only
       have one at the moment */
  return current_database->current_primary_events_database;

} /* ped_shell_first */


/*--------------------------------------------------------------
 Routine : ped_shell_last
 Purpose : Return the last database in the list.
---------------------------------------------------------------*/
PRIMARY_EVENTS_DATABASE *
  ped_shell_last()
{
    unsigned length;
    List tmp;
	PRIMARY_EVENTS_DATABASE *ped_state;
	int i;

    length = Length_Of(shell_list);

    tmp = shell_list;

    for(i=0; i < length-1; i++) {
        tmp = Tail_Of( tmp );
    }
		
    ped_state = (PRIMARY_EVENTS_DATABASE*)Head_Of( tmp );

    return ped_state;

} /* ped_shell_last */


/*--------------------------------------------------------------
 Routine : ped_shell_delete
 Purpose : Delete the entire shell list.
---------------------------------------------------------------*/
void
  ped_shell_delete()
{
    List cur;
    List del;
	PRIMARY_EVENTS_DATABASE *ped_state;

	cur = shell_list;
	while ( cur != NULL ) {
		ped_state = (PRIMARY_EVENTS_DATABASE*)Head_Of( cur ) ;
	    ped_new( ped_state );
		del = cur;
    	cur = Tail_Of( cur );
		FreeMemory(del);
	}
	
	shell_list = NULL;

} /* ped_shell_last */



/*--------------------------------------------------------------
 Routine : ped_shell_delete_expanded
 Purpose : Delete all but the first ped shell.
---------------------------------------------------------------*/
void
  ped_shell_delete_expanded()
{
    List cur;
    List del;
	PRIMARY_EVENTS_DATABASE *ped_state;

	if(shell_list && Tail_Of(shell_list)){

	    cur = Tail_Of(shell_list);
	    while ( cur != NULL ) {
		    ped_state = (PRIMARY_EVENTS_DATABASE*)Head_Of( cur ) ;
	        ped_new( ped_state );
		    del = cur;
    	    cur = Tail_Of( cur );
		    FreeMemory(del);
		}
	
    	shell_list->Next = NULL;
	}

} /* ped_shell_last */


