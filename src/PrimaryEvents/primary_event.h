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
 Module : primary_event.h
 Author : FSC Limited
 Date   : 30/7/96
 SccsId : @(#)primary_event.h	1.4 11/22/96

****************************************************************/

#ifndef PE_P_H
#define PE_P_H

#include <stdio.h>

#include "basic.h"  

#include "boolean.h"

#define PRIM_MAX_DESC_LENGTH    255

typedef enum _PrimaryEventType {
    PRIM_BASIC = 0, 
    PRIM_EXTERN = 1, 
    PRIM_UNDEVELOPED = 2, 
    PRIM_COND_NOTANAL = 3,
    PRIM_COND_ANAL = 4,
    PRIM_NONE = 5 /* Always the last (invalid) */
} PrimaryEventType;
 
typedef struct _primary_event {
    char                id[ MAX_SYMBOL_ID_LENGTH ];

/* not used - should be removed */
    char                group[ MAX_SYMBOL_ID_LENGTH ];    

    PrimaryEventType     type;
/*     int                 dormant; */
    char                dormant;
    char                lambda;
    char                *desc;
    float               prob; 
    struct _primary_event *next;
} PRIMARY_EVENT;
 
typedef struct _primary_event                *primary_event;

static char type_strings[ PRIM_NONE ] =
  { 'B', 'E', 'U', 'C', 'N' };


/*--------------------------------------------------------------
 Routine : primary_event_new
 Purpose : Create a new Primary event object
---------------------------------------------------------------*/
extern PRIMARY_EVENT *
  primary_event_new(
    char *id );

/*--------------------------------------------------------------
 Routine : primary_event_read
 Purpose : Read the contents of a Primary event from the supplied file
           This should really return a better indication of success or
           failure than 'int'
---------------------------------------------------------------*/
extern int
  primary_event_read(
    FILE          *fp,
    PRIMARY_EVENT *cp );

/*--------------------------------------------------------------
 Routine : primary_event_add_description
 Purpose : Add the supplied descriptive text to the supplied
           Primary Event
---------------------------------------------------------------*/
extern BOOL
  primary_event_add_description(
    PRIMARY_EVENT *cp,
    char      *desc );


/*--------------------------------------------------------------
 Routine : primary_event_free_internal
 Purpose : Free the internal resources of the supplied Primary Event.
---------------------------------------------------------------*/
extern PRIMARY_EVENT *
  primary_event_free_internal(
    PRIMARY_EVENT *cp );

/*--------------------------------------------------------------
 Routine : primary_event_free
 Purpose : Free the resources of the supplied Primary Event.
---------------------------------------------------------------*/
extern PRIMARY_EVENT *
  primary_event_free(
    PRIMARY_EVENT *cp );

/*--------------------------------------------------------------
 Routine : primary_event_null 
 Purpose : Returns a null Primary Event. Should only be copied, not edited
---------------------------------------------------------------*/
extern PRIMARY_EVENT *
  primary_event_null();

/*--------------------------------------------------------------
 Routine : primary_event_write
 Purpose : Write the supplied primary event to the supplied
 file.
---------------------------------------------------------------*/
extern int
  primary_event_write(
    FILE          *fp,
    PRIMARY_EVENT *cp );

/*--------------------------------------------------------------
 Routine : primary_event_item_type
 Purpose : Return the type of the event in terms that the
 tree application understands.
---------------------------------------------------------------*/
extern int
  primary_event_item_type(
    PrimaryEventType type );

/*--------------------------------------------------------------
 Routine : primary_event_copy
 Purpose : Copy from one event to another. Both events are 
 presumed to be created.
---------------------------------------------------------------*/
extern void
  primary_event_copy(
    PRIMARY_EVENT *from,
    PRIMARY_EVENT *to );



#endif
