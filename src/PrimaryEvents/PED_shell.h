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
 SccsId : @(#)PED_shell.h	1.2 10/31/96

 This module implements the shell handler for the Primary Events
 database.
 A new shell is created if no shell already exists for the supplied
 database name (including NULL).
 Each shell creates a new instance of the Primary Events Database
 application, and a list of shells is maintained. Each application
 holds it's current database name in user data attached to the shell
 widget.

****************************************************************/

#ifndef PED_SHELL_H
#define PED_SHELL_H

#include "boolean.h"

#include "primary_event.h"
#include "primary_events_database.h"


/*--------------------------------------------------------------
 Routine : ped_shell_primary_event_exists
 Purpose : Returns whether the primary event Id exists in the
 supplied database.
---------------------------------------------------------------*/
extern BOOL
  ped_shell_primary_event_exists(
    char *id ,
    char *database_name);



/*--------------------------------------------------------------
 Routine : ped_shell_get_primary_event
 Purpose : Returns the primary event with the Id in the supplied
 database.
---------------------------------------------------------------*/
extern PRIMARY_EVENT *
  ped_shell_get_primary_event(
    char *id ,
    char *database_name);


/*--------------------------------------------------------------
 Routine : ped_shell_add
 Purpose : Adds a created database application shell to the
 shell_list.
---------------------------------------------------------------*/
extern void
  ped_shell_add( PRIMARY_EVENTS_DATABASE *ped_state );


/*--------------------------------------------------------------
 Routine : ped_shell_create
 Purpose : Creates a new database application shell.
---------------------------------------------------------------*/
extern PRIMARY_EVENTS_DATABASE *
  ped_shell_create();


/*--------------------------------------------------------------
 Routine : ped_shell_exists
 Purpose : Return whether the named database is opened in an
 application shell.
---------------------------------------------------------------*/
extern BOOL
  ped_shell_exists(
    char *database_name );


/*--------------------------------------------------------------
 Routine : ped_shell_any_exist
 Purpose : Returns the number of shells in the shell_list 
---------------------------------------------------------------*/
extern BOOL
  ped_shell_any_exist();


/*--------------------------------------------------------------
 Routine : ped_shell_open
 Purpose : Returns whether the named database is open in an
 application shell.
---------------------------------------------------------------*/
extern BOOL
  ped_shell_open(
    char *database_name );


/*--------------------------------------------------------------
 Routine : ped_shell_add_modify_callback
 Purpose : To perform any required actions in the application that
 spawned the Database shell.
---------------------------------------------------------------*/
extern void
  ped_shell_add_modify_callback(
    char *database_name,
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
  primary_events_database_open(
    char *primary_events_database_name);


/*--------------------------------------------------------------
 Routine : ped_shell_first
 Purpose : Return the name of the first database in the list.
---------------------------------------------------------------*/
extern char *
  ped_shell_first();


/*--------------------------------------------------------------
 Routine : ped_shell_last
 Purpose : Return the last database in the list.
---------------------------------------------------------------*/
PRIMARY_EVENTS_DATABASE *
  ped_shell_last();

/*--------------------------------------------------------------
 Routine : ped_shell_delete
 Purpose : Delete the entire shell list.
---------------------------------------------------------------*/
extern void
  ped_shell_delete();


/*--------------------------------------------------------------
 Routine : ped_shell_delete_expanded
 Purpose : Delete all but the first ped shell.
---------------------------------------------------------------*/
extern void
  ped_shell_delete_expanded();

#endif
