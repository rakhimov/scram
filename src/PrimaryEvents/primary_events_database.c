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
 Module : primary_events_database.c
 Author : FSC Limited
 Date   : 30/7/96

 SccsId : @(#)primary_events_database.c	1.15 01/28/97
 
 This module defines the separate Primary Event Database, originally
 developed for Formal-FTA.
 The primary_events_database is structured as a separate application,
 and is created below a supplied parent widget.
 
 The intention is to allow a user to double click on an Event symbol.
 If no ID is associated with that symbol, the currently selected Event
 in the currently associated database is used. If no event is selected,
 a warning is displayed. If no database is associated, a new database
 shell is spawned off, and the user is expected to select an event and
 double click on the Event symbol again.
****************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "primary_events_database.h"
#include "fta.h"
#include "NativePEDFrame.h"
#include "file_pd.h"

#include "PED_shell.h"
#include "primary_event.h"
#include "Dialogs.h"
#include "FileDialogs.h"

#include "util.h"
#include "FileUtilities.h"
#include "list_suu_utilities.h"

#include "MemoryUtil.h"

#include "AssertUtil.h" 

ASSERTFILE(__FILE__)

typedef struct update_info{
  PRIMARY_EVENTS_DATABASE *ped_state;
  primary_event event;
} UPDATE_PACKAGE;


#define LOAD_PED_WARNING \
"WARNING: Database not saved.\n\n\
The database has changed since it was last saved.\n\
Opening a saved database will lose any changes that you made.\n\n\
Open saved database and lose any changes?"


#define NEW_PED_WARNING \
"WARNING: Database not saved.\n\n\
The database has changed since it was last saved.\n\
Creating a new database will lose any changes that you made.\n\n\
Create new database and lose any changes?"


/*--------------------------------------------------------------
 Routine : Update_PrimEv_Type_Ok
 Purpose : Updates the PED with the entry passed in.
---------------------------------------------------------------*/
static void
  Update_PrimEv_Type_Ok(UPDATE_PACKAGE *update_pack);


/*--------------------------------------------------------------
 Routine : primary_events_database_delete
 Purpose : Deletes the events list of the currently open database.
---------------------------------------------------------------*/
static void
  primary_events_database_delete(
    PRIMARY_EVENTS_DATABASE *ped_state );


/*--------------------------------------------------------------
 Routine : ped_notify_changed
 Purpose : Sets whether the database that is open has been
 changed since it was last saved or not.
---------------------------------------------------------------*/
static void
  ped_notify_changed( 
    BOOL value,
    PRIMARY_EVENTS_DATABASE *ped_state )
{
    ped_state->ped_change_since_saved = value;
    if ( ped_state->modify_callback )
	{
	    ped_state->modify_callback( ped_state->user_data );
	}

} /* ped_notify_changed */


/*--------------------------------------------------------------
 Routine : primary_events_database_delete
 Purpose : Deletes the events list of the currently open database.
---------------------------------------------------------------*/
void
  primary_events_database_delete(
    PRIMARY_EVENTS_DATABASE *ped_state )
{
  List pel;
  PRIMARY_EVENT *event;
  pel = ped_state->primary_events_list;


  while ( pel != NULL ) {
    event = (PRIMARY_EVENT*)lsu_Retreive_Item( pel, 1 );
	if(event) {

	  primary_event_free_internal(event);

	}
    lsu_Remove_Item( &pel, 1 );
  }
  ped_state->primary_events_list = pel;

  ped_notify_changed( FALSE, ped_state );
 
} /* primary_events_database_delete */


/*--------------------------------------------------------------
 Routine : ped_new
 Purpose : delete old primary_events_database and set name
---------------------------------------------------------------*/
void
  ped_new(
    PRIMARY_EVENTS_DATABASE *ped_state )
{
 
    primary_events_database_delete( ped_state );
    ped_state->current_primary_events_database = NULL;

    /* set_main_window_title( ped_state );  */

} /* ped_new */




/*--------------------------------------------------------------
 Routine : verify_event
 Purpose : Routine actions the apply button for the PED, passes
           control to Update_PrimEv_Type_OkCb if the type of 
           event is changed.
---------------------------------------------------------------*/
void
  verify_event(
	PRIMARY_EVENTS_DATABASE *ped_state,
    primary_event event,
    PrimaryEventType original_type )
{
	List pel;
	PRIMARY_EVENT    *cp;
	BOOL event_found;

	/* If event->id exists in the list then
		modify event by calling Update_PrimEv_Type_OkCb
		 else
		add event
	     end if;
	*/
	ASSERT( ped_state != NULL );

	event_found = FALSE;

	pel = ped_state->primary_events_list;
    while ( pel != NULL ) {
      
		if ( strcmp( ( (PRIMARY_EVENT *)Head_Of( pel ) )->id,
            event->id ) == 0 ) {
 
			event_found = TRUE;
		  
			if (event->type == original_type){
				primary_event_copy(event, (PRIMARY_EVENT *)Head_Of( pel ) );
   				ped_notify_changed( TRUE,ped_state );
				return;
			} else {
				UPDATE_PACKAGE *update_pack;
				if ( !fNewMemory( (void **)&update_pack, sizeof(UPDATE_PACKAGE) ) )
				{
					exit( 1 );
				}

				update_pack->ped_state = ped_state;
				update_pack->event = event;
				if(PEDFrameAskQuestion("Changing this event will cause any trees\n"
                                        "that depend on this event to change symbols.\n"
                                        "Do you want to continue ?", 
										FTA_QUESTION_TITLE)) {
					Update_PrimEv_Type_Ok(update_pack);

				}
				return;
			}
		}
		pel = Tail_Of( pel );
	} 
 
	if(!event_found) {
		pel = ped_state->primary_events_list;
		cp = primary_event_new( event->id );
		primary_event_copy( event, cp ); 
		Construct( cp, &pel );
		ped_state->primary_events_list = pel;

		ped_notify_changed( TRUE, ped_state );

	}
} /* verify_event */



/*--------------------------------------------------------------
 Routine : delete event
 Purpose : routine to remove the selected primary event
 from the Primary events list.
---------------------------------------------------------------*/
void
  delete_event(
	PRIMARY_EVENTS_DATABASE *ped_state,
    const char *event_name
	)
{
  List pel;
  int i;

  if ( event_name != NULL ) {

    /* Remove from database list */
    pel = ped_state->primary_events_list;
    i = 1; 
    while ( ( pel != NULL ) && 
      ( ( strcmp(    
            ((primary_event)Head_Of( pel ))->id,  
            event_name ) != 0 ) ) ) {  
      i++; 
      pel = Tail_Of( pel ); 
    } 

    if ( i > 0 ) {
      lsu_Remove_Item( &(ped_state->primary_events_list), i );
    }

    ped_notify_changed( TRUE, ped_state );


  }

} /* primary_event_delete_cb */


/*--------------------------------------------------------------
 Routine : Update_PrimEv_Type_Ok
 Purpose : Updates the PED with the altered event and type passed in.
---------------------------------------------------------------*/
void
  Update_PrimEv_Type_Ok(
    UPDATE_PACKAGE *update_pack
	)
{
  List pel;
  PRIMARY_EVENTS_DATABASE *ped_state;
  primary_event event;

  pel = update_pack->ped_state->primary_events_list;
  event = update_pack->event;
  ped_state = update_pack->ped_state;
 
  while ( pel != NULL ) {
    if ( strcmp( ( (PRIMARY_EVENT *)Head_Of( pel ) )->id,
          event->id ) == 0 ) {
      primary_event_copy(event, (PRIMARY_EVENT *)Head_Of( pel ) );     
      
	  ped_notify_changed( TRUE,ped_state );

      FreeMemory(update_pack);
      return;
    }
    pel = Tail_Of( pel );
  } 
} /* Update_PrimEv_Type_Ok */



/*--------------------------------------------------------------
 Routine : primary_events_database__primary_event_get_selected
 Purpose : Returns the primary event selected in the primary
 events list.
---------------------------------------------------------------*/
PRIMARY_EVENT *
  primary_events_database__primary_event_get_selected(
    void *ped_state )
{
	char *id;

	ASSERT( ped_state != NULL );

	id = PEDFrameGetSelectedId();

	if(id) {

		/* Get primary event with this id */
		return primary_events_database__primary_event_open(
							id, (PRIMARY_EVENTS_DATABASE *)ped_state );
	} else {
		return NULL;
	}

} /* primary_events_database__primary_event_get_selected */



/*--------------------------------------------------------------
 Routine : primary_events_database__primary_event_get
 Purpose : Returns the primary event with the selected Id.
---------------------------------------------------------------*/
PRIMARY_EVENT *
  primary_events_database__primary_event_get(
    char *id,
    void *ped_state )
{

  ASSERT( ped_state != NULL );

  /* Get primary event with this id */
  return 
    primary_events_database__primary_event_open(
      id, (PRIMARY_EVENTS_DATABASE *)ped_state );

} /* primary_events_database__primary_event_get*/



/*--------------------------------------------------------------
 Routine : primary_events_database__primary_event_open
 Purpose : Open the Primary Event with the supplied Id.
           If no Id is supplied a new Primary Event is created.
           If the Id exists, the Primary Event is opened for modification.
           If the Primary Event cannot be opened returns False else returns True.
---------------------------------------------------------------*/
PRIMARY_EVENT *
  primary_events_database__primary_event_open(
    char *id,
    PRIMARY_EVENTS_DATABASE *ped_state
  )
{
  List pel;
  PRIMARY_EVENT *cp;

  ASSERT( ped_state != NULL );

  pel = ped_state->primary_events_list; 
  while ( pel != NULL ) { 
    cp = (PRIMARY_EVENT *)Head_Of(pel); 
    if ( strcmp( id, cp->id ) == 0 ) {
      return cp;
    }
    pel = Tail_Of(pel); 
  } 

  return NULL;

} /* primary_events_database__primary_event_open */



/*--------------------------------------------------------------
 Routine : primary_events_database_add_modify_callback
 Purpose : Adds the function to call when the database is modified.
---------------------------------------------------------------*/
void
  primary_events_database_add_modify_callback(
    PRIMARY_EVENTS_DATABASE *ped_state,
    void (*modify_callback)(void *),
    void *user_data )
{

  ped_state->modify_callback = modify_callback;
  ped_state->user_data = user_data;

} /* primary_events_database_add_modify_callback */





/*--------------------------------------------------------------
 Routine : primary_events_database_read
 Purpose : Reads the database file of the supplied name from
 the current working directory. As the events are read in the
 PED widget is populated.
---------------------------------------------------------------*/
static BOOL
  primary_events_database_read(
    char *filename,
    PRIMARY_EVENTS_DATABASE *ped_state )
{
  FILE            *fp;
  char            id[ MAX_SYMBOL_ID_LENGTH ];
  PRIMARY_EVENT    *cp;
  int             ret;
  List pel;
 
printf("primary_events_database.c: In primary_events_database_read\n");

printf("primary_events_database.c: about to open ped file - %s\n", filename);


  if ( ( fp = fopen( filename, "r" ) ) == NULL ) {
    return FALSE;
  }
 
printf("primary_events_database.c: opened ped file\n");


/* Make new primary event
   while events exits in file
     Read from file into that event
     Add event to list
     Make new primary event
   end while;
*/
  pel = ped_state->primary_events_list;
  cp = primary_event_new( id );

printf("primary_events_database.c: read in events\n");

  while ( ( ret = primary_event_read( fp, cp ) ) == TRUE ) {
printf("primary_events_database.c: got new event\n");

    Construct( cp, &pel );

    cp = primary_event_new( id );
  }

printf("primary_events_database.c: finished reading in events\n");


  /* Because one more event is always created than is used, delete
     the unused one */
  primary_event_free( cp );

  ped_state->primary_events_list = pel;

  fclose(fp);
  ped_notify_changed(FALSE, ped_state);
 
/*   pel = ped_state->primary_events_list; */
/*   printf("The P.E.List is "); */
/*   while ( pel != NULL ) { */
/*     cp = (PRIMARY_EVENT *)Head_Of(pel); */
/*     printf("%s ", cp->id ); */
/*     pel = Tail_Of(pel); */
/*   } */
/*   printf("\n"); */

/*   if (ret == EOF) { */
  if (ret == FALSE) {
    return TRUE;
  } else {
    return FALSE;
  }
} /* primary_events_database_read */



/*--------------------------------------------------------------
 Routine : ped_load_proc
 Purpose : The callback routine for the Database Open thread to
 load the database from the selected file.
---------------------------------------------------------------*/
static int
  ped_load_proc(
    void *user_data,
    char *fname )
{
  PRIMARY_EVENTS_DATABASE *ped_state;

printf("primary_events_database.c: In ped_load_proc\n");

  //ped_state = (PRIMARY_EVENTS_DATABASE *)user_data;

	// delete current database
	//PEDFrameNewPEDFile(FALSE);

	// update ped_state
	ped_state = ped_shell_last();

/*   if ( !ped_shell_exists( filename_from_pathname( fname ) ) ) { */
    ped_state->current_primary_events_database = strsave( fname );
    primary_events_database_delete( ped_state );
    if ( !primary_events_database_read( fname, ped_state ) ) {
      return FILE_ERROR;
    } else {
/*       set_main_window_title( ped_state ); */
      ped_notify_changed( FALSE, ped_state );
      return FILE_OK;
    }
/*   } */
/*   else { */
    /* File already open so don't open again */
/*     return FILE_OK; */
/*   } */

} /* ped_load_proc */



/*--------------------------------------------------------------
 Routine : primary_events_database_open_cb
 Purpose : Attempts to open the primary_events_database with the
           supplied name.
           If the primary_events_database cannot be opened returns
           False, else returns True.
---------------------------------------------------------------*/
BOOL
  primary_events_database_open_cb(
    char *primary_events_database_name)
{
	char *warning = NULL;
	PRIMARY_EVENTS_DATABASE *ped_state;

/* 	ped_state = NativePEDCurrentPEDState(); */

    ped_state = ped_shell_last();

	if ( primary_events_database_changed_since_saved( ped_state ) ) {
		warning = LOAD_PED_WARNING;
	}
	load_from_file(
		primary_events_database_name, 
		PED_APPN_TITLE " : ",
		"Primary Event Database",
		"*.ped",
		warning,
		ped_state,
		&ped_load_proc,
		FD_PED);

	PEDFrameSetWindowTitle( primary_events_database_name );

	return TRUE; 

} /* primary_events_database_open_cb */


/*--------------------------------------------------------------
 Routine : primary_events_database_write
 Purpose : Writes the currently open database to the supplied
 file.
---------------------------------------------------------------*/
static BOOL
  primary_events_database_write(
    char *filename,
    PRIMARY_EVENTS_DATABASE *ped_state )
{
  FILE *fp;
  PRIMARY_EVENT *cp;
  List pel;
 
 /* test if file exists */
  if ( ( fp = fopen( filename, "w" ) ) == NULL ) return FALSE;
 
  pel = ped_state->primary_events_list;
  while ( pel != NULL ) {
  cp = (PRIMARY_EVENT *)Head_Of( pel );
    primary_event_write( fp, cp );
    pel = Tail_Of( pel );
  }
 
  fclose( fp );
  ped_notify_changed( FALSE, ped_state );
 
  return TRUE;
 
} /* primary_events_database_write */


/*--------------------------------------------------------------
 Routine : primary_events_database_save_to_file
 Purpose : Callback routine to save the currently open database
 to the selected file.
---------------------------------------------------------------*/
BOOL primary_events_database_save_to_file(
    PRIMARY_EVENTS_DATABASE *ped_state,
    char *filename)
{ 
/* printf("Saving %s\n", filename); */
  if ( primary_events_database_write( filename, ped_state ) ) {
    ped_state->current_primary_events_database = 
      (char *)strsave(filename);
/*     set_main_window_title( ped_state ); */

    ped_notify_changed( FALSE, ped_state );

    return TRUE;
  } else {
    return FALSE;
  }
 
} /* primary_events_database_save_to_file */



/*--------------------------------------------------------------
 Routine : primary_events_database_changed_since_saved
 Purpose : Returns whether the database that is open has changed
 since it was last saved.
---------------------------------------------------------------*/
BOOL
  primary_events_database_changed_since_saved(
    PRIMARY_EVENTS_DATABASE *ped_state )
{
    return ped_state->ped_change_since_saved;

} /* primary_events_database_changed_since_saved */




/*--------------------------------------------------------------
 Routine : primary_events_database_new_cb
 Purpose : The routine that actually creates a new 
 primary_events_database. Prompts for confirmation if a 
 primary_events_database is currently open and not saved.

 Confirmation can be skipped by setting the confirm flag to FALSE.

 Split out to allow calling from a toolbar button. Returns Boolean
 value stating whether the new command was carried out. 
---------------------------------------------------------------*/
BOOL primary_events_database_new_cb(PRIMARY_EVENTS_DATABASE *ped_state, BOOL confirm)
{
	BOOL accepted = TRUE;
	
	if ( confirm && primary_events_database_changed_since_saved( ped_state ) ) {
		if(PEDFrameAskQuestion(NEW_PED_WARNING, FTA_NEW_TITLE " Primary Event Database")) {
/* 			ped_new(ped_state); */
			ped_shell_delete();
		} else {
			accepted = FALSE;
		}
	} else {
/* 		ped_new( ped_state ); */
     	ped_shell_delete();
		ped_notify_changed( FALSE, ped_state );
	}

	return accepted;

} /* primary_events_database_new_cb */


