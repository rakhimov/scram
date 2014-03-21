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
 Module : primary_events_database.h
 Author : FSC Limited
 Date   : 30/7/96

 SccsId :@(#)primary_events_database.h	1.3 10/31/96

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

#ifndef PRIMARY_EVENTS_DATABASE_H
#define PRIMARY_EVENTS_DATABASE_H

#include "boolean.h"
#include "list_suu.h"
#include "primary_event.h"

#define PED_APPN_TITLE "Formal-PED"

typedef struct ped_state_data{
  BOOL        shell;
  char        *current_primary_events_database;
  List        primary_events_list;
  BOOL        ped_change_since_saved;
  PRIMARY_EVENT *(*primary_event_get_selected)( void * );
  PRIMARY_EVENT *(*primary_event_get)( char *, void * );
  void        (*modify_callback)(void *);
  void        *user_data;
}PRIMARY_EVENTS_DATABASE;


extern void
  ped_new(
    PRIMARY_EVENTS_DATABASE *ped_state );


/*--------------------------------------------------------------
 Routine : delete event
 Purpose : routine to remove the selected primary event
 from the Primary events list.
---------------------------------------------------------------*/
extern void
  delete_event(
	PRIMARY_EVENTS_DATABASE *ped_state,
    const char *event_name
	);

/*--------------------------------------------------------------
 Routine : verify_event
 Purpose : Routine actions the apply button for the PED, passes
           control to Update_PrimEv_Type_OkCb if the type of
           event is changed.
---------------------------------------------------------------*/
extern void
  verify_event(
	PRIMARY_EVENTS_DATABASE *ped_state,
    primary_event event,
    PrimaryEventType original_type );


/*--------------------------------------------------------------
 Routine : primary_events_database__primary_event_get_selected
 Purpose : Returns the primary event selected in the primary
 events list.
---------------------------------------------------------------*/
extern PRIMARY_EVENT *
  primary_events_database__primary_event_get_selected(
    void *ped_state );


/*--------------------------------------------------------------
 Routine : primary_events_database__primary_event_get
 Purpose : Returns the primary event with the selected Id.
---------------------------------------------------------------*/
extern PRIMARY_EVENT *
  primary_events_database__primary_event_get(
    char *id, 
    void *ped_state );


/*--------------------------------------------------------------
 Routine : primary_events_database__primary_event_open
 Purpose : Open the Primary Event with the supplied Id.
	   If no Id is supplied a new Primary Event is created.
	   If the Id exists, the Primary Event is opened for 
           modification.
	   If the Primary Event cannot be opened returns False 
           else returns True.
---------------------------------------------------------------*/
extern PRIMARY_EVENT *
  primary_events_database__primary_event_open(
    char *id,
    PRIMARY_EVENTS_DATABASE *database_state
  );


/*--------------------------------------------------------------
 Routine : primary_events_database_add_modify_callback
 Purpose : Adds the function to call when the database is modified.
---------------------------------------------------------------*/
extern void
  primary_events_database_add_modify_callback(
    PRIMARY_EVENTS_DATABASE *ped_state,
    void (*modify_callback)(void *),
    void *user_data );


/*--------------------------------------------------------------
 Routine : primary_events_database_open
 Purpose : Attempts to open the primary_events_database with the
           supplied name.
	   If the primary_events_database cannot be opened returns
	   False, else returns True.
---------------------------------------------------------------*/
extern BOOL
  primary_events_database_open_cb(
    char *primary_events_database_name);



/*--------------------------------------------------------------
 Routine : primary_events_database_new_cb
 Purpose : The routine that actually creates a new 
 primary_events_database. Prompts for confirmation if a 
 primary_events_database is currently open and not saved.

 Confirmation can be skipped by setting the confirm flag to FALSE.

 Split out to allow calling from a toolbar button. Returns Boolean
 value stating whether the new command was carried out. 
---------------------------------------------------------------*/
extern BOOL primary_events_database_new_cb(
    PRIMARY_EVENTS_DATABASE *ped_state,
    BOOL confirm);


/*--------------------------------------------------------------
 Routine : primary_events_database_save_to_file
 Purpose : Callback routine to save the currently open database
 to the selected file.
---------------------------------------------------------------*/
extern BOOL primary_events_database_save_to_file(
    PRIMARY_EVENTS_DATABASE *ped_state,
    char *filename);


/*--------------------------------------------------------------
 Routine : primary_events_database_changed_since_saved
 Purpose : Returns whether the database that is open has changed
 since it was last saved.
---------------------------------------------------------------*/
extern BOOL
  primary_events_database_changed_since_saved(
    PRIMARY_EVENTS_DATABASE *ped_state );


#endif
