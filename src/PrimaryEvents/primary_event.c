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
 Module : primary_event.c
 Author : FSC Limited
 Date   : 30/7/96
 SccsId : @(#)primary_event.c	1.6 11/15/96

****************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "primary_event.h"

#include "item.h"
#include "Record_Io.h"
#include "MemoryUtil.h"
#include "util.h"

/* #include "Assert.h"  */

static PRIMARY_EVENT *null_event;



/*--------------------------------------------------------------
 Routine : primary_event_initialise
 Purpose : Initialise the contents of the primary event.
---------------------------------------------------------------*/
static BOOL
  primary_event_initialise(
    PRIMARY_EVENT *cp )
{
    strcpy( cp->id, "" );
    strcpy( cp->group, "" );
/*     cp->type = BASIC; */
    cp->type = PRIM_NONE;
    cp->dormant = '\0';
    cp->desc = '\0';
    cp->prob = 0.0;
    cp->lambda = '\0';
    return TRUE;
} /* primary_event_initialise */


/*--------------------------------------------------------------
 Routine : primary_event_destroy
 Purpose : Destroy the primary event and release all memory
 allocated for it.
---------------------------------------------------------------*/
static PRIMARY_EVENT *
  primary_event_destroy( 
    PRIMARY_EVENT *cp )
{
  FreeMemory( cp );
  return NULL;

} /* primary_event_destroy */

/*--------------------------------------------------------------
 Routine : primary_event_new
 Purpose : Create a new Primary event object
---------------------------------------------------------------*/
PRIMARY_EVENT *
  primary_event_new(
    char *id )
{
  PRIMARY_EVENT *cp;
 
/*   if ( ( cp = (PRIMARY_EVENT *)malloc( sizeof( PRIMARY_EVENT ) ) ) == NULL )  */
  if ( !fNewMemory( (void **)&cp, sizeof( PRIMARY_EVENT ) ) )
  {
    printf("\n*** primary_event_new : malloc failed ***\n");
    exit(1);
  }
  primary_event_initialise( cp );
/*   strcpy( cp->id, id ); */
/*   return primary_event_append( cp ); */
  return cp;

} /* primary_event_new */

/*--------------------------------------------------------------
 Routine : primary_event_read
 Purpose : Read the contents of a Primary event from the supplied file
          This should really return a better indication of success or
          failure than 'int'
---------------------------------------------------------------*/
int
  primary_event_read(
    FILE          *theFile,
    PRIMARY_EVENT *cp )
{
  char c;
  char *str;
  float f;
  PrimaryEventType type; 
  int i;
  BOOL floatOK;

printf("primary_event.c: In primary event read\n");


printf("primary_event.c: get ID string\n");

  /* Get ID string (compulsory)... */
  str = GetStringField( theFile );
  if(str == NULL) {
     printf("  = NULL\n");
  } else {
     printf("  = %s\n",str);
  }

  if ( str != NULL )
  {
    strncpy( cp->id, str, MAX_SYMBOL_ID_LENGTH );
    FreeMemory(str);

printf("primary_event.c: get group string\n");

    /* Get Group string (if one exists)... */
    str = GetStringField( theFile );
    if (str == NULL) {
       printf("  = NULL\n");
    } else {
       printf("  = %s\n",str);
    }
    
    if ( str != NULL )
    {
      strcpy( cp->group, str );
      FreeMemory(str);
    }

printf("primary_event.c: get type char\n");

    /* Get Type character (compulsory)... */
    c = GetChar( theFile, TRUE );

    printf("  = %c\n",c);
    
    type = PRIM_NONE;
    for ( i = 0; i < PRIM_NONE; i++ ) {
      if ( c == type_strings[ i ] ) {
        type = (PrimaryEventType)i;
        break;
      }
    }

    if ( type == PRIM_NONE ) return FALSE;
 
    cp->type = type;

printf("primary_event.c: get dormant char\n");

    /* Get Dormant character (if one exists)... */
    c = GetChar( theFile, TRUE );
    if ( c != DELIMITER )
    {
      cp->dormant = c;
      while ( c != DELIMITER )
      {
        c = GetChar( theFile, TRUE );
      }
    }
    else
      cp->dormant = '\0';

printf("primary_event.c: get description string\n");
    
    /* Get Description string (if one exists)... */
    str = GetStringField( theFile );
    if ( str != NULL ) {
      primary_event_add_description( cp, str );
      FreeMemory(str);
    }

printf("primary_event.c: get prob value\n");

    /* Get Probability value (compulsory)... */
    f = GetFloatField( theFile, &floatOK );
    cp->prob = f;

printf("primary_event.c: get lambda indicator\n");
	
    /* Get lambda indicator (if one exists)... */
    str = GetStringField( theFile );
    if ( str != NULL )
      cp->lambda = str[ 0 ];
    else
      cp->lambda = '\0';

    if ( str != NULL ) {
      FreeMemory(str);
    }

    return TRUE;
  }

  return FALSE;

} /* primary_event_read */

/*--------------------------------------------------------------
 Routine : primary_event_add_description
 Purpose : Add the supplied descriptive text to the supplied
           Primary Event
---------------------------------------------------------------*/
BOOL
  primary_event_add_description(
    PRIMARY_EVENT *cp, 
    char      *desc )
{
  char *pStr;

/*   if ( ( cp->desc = (char *)malloc( strlen( desc ) + 1 ) ) == NULL )  */
  if ( !fNewMemory( (void **)&pStr, ( strlen( desc ) + 1 ) ) )
  {
    printf("\n*** primary_event_add_description : malloc failed ***\n");
    exit(1);
  }
  cp->desc = pStr;
  strcpy( cp->desc, desc );
  return TRUE;

} /* primary_event_add_description */


/*--------------------------------------------------------------
 Routine : primary_event_free_internal
 Purpose : Free the internal resources of the supplied Primary Event.
---------------------------------------------------------------*/
PRIMARY_EVENT *
  primary_event_free_internal(
    PRIMARY_EVENT *cp )
{
	if(cp->desc) {
	    FreeMemory( cp->desc );
	}
	return NULL;
} /* primary_event_free */



/*--------------------------------------------------------------
 Routine : primary_event_free
 Purpose : Free the resources of the supplied Primary Event.
---------------------------------------------------------------*/
PRIMARY_EVENT *
  primary_event_free(
    PRIMARY_EVENT *cp )
{
  primary_event_free_internal(cp);
  FreeMemory( cp );
  return NULL;

} /* primary_event_free */

/*--------------------------------------------------------------
 Routine : primary_event_null
 Purpose : Returns a null Primary Event. Should only be copied, not edited
---------------------------------------------------------------*/
PRIMARY_EVENT *
  primary_event_null()
{
  int i;

  if ( null_event == NULL ) {
/*     null_event = (PRIMARY_EVENT *)malloc( sizeof( PRIMARY_EVENT ) ); */
    if ( !fNewMemory( (void **)&null_event, sizeof( PRIMARY_EVENT ) ) )
    {
      exit( 1 );
    }

    for ( i = 0; i < MAX_SYMBOL_ID_LENGTH; i++ ) null_event->id[ i ] = ' ';
    null_event->id[ 0 ] = '\0';
    null_event->type = PRIM_NONE;
    null_event->dormant = '\0';
    null_event->lambda = '\0'; 
/*     null_event->lambda = 0; */
    null_event->desc = NULL;
    null_event->prob = 0.0;
    null_event->next = NULL;
  }
  return null_event;

} /* primary_event_null */

/*--------------------------------------------------------------
 Routine : primary_event_copy
 Purpose :
---------------------------------------------------------------*/
void
  primary_event_copy(
    PRIMARY_EVENT *from,
    PRIMARY_EVENT *to )
{
/*   char          desc[ MAX_SYMBOL_DESC_LENGTH ]; */
  char *pStr;

  strcpy( to->id, from->id );
  strcpy( to->group, "" ); 
  to->type = from->type; 
  to->dormant = from->dormant; 
/*   strcpy( desc, from->desc ); */

/*   to->desc = desc; */
/*   to->desc = (char *)malloc( strlen( from->desc ) ); */

  /* strlen( from->desc ) may be == zero (e.g., if description
     has been deleted, so don't allocate new pointer for it
     has fNewMemory doesn't like a zero size.
  */
  if ( (int)strlen( from->desc ) > 0 )
  {
    if ( !fNewMemory( (void **)&pStr, strlen( from->desc ) ) )
    {
      exit( 1 );
    }
    to->desc = pStr;
    to->desc = strcpy( to->desc, from->desc );
  }
  else
    to->desc = NULL;

  to->prob = from->prob; 
  to->lambda = from->lambda; 

} /* primary_event_copy */

/*--------------------------------------------------------------
 Routine : primary_event_write
 Purpose : Write the supplied primary event to the supplied
 file.
---------------------------------------------------------------*/
int
  primary_event_write(
    FILE           *fp,
    PRIMARY_EVENT *cp )
{

  char *str;

  /* The fact that cp->desc may be NULL, which can't be written out
     by fprintf, is a bit of a cludge, considering that by just
     strsave( NULL ) the problem goes away. It might be better to
     investigate making a null description strsave( NULL ) rather
     than NULL or '\0'
  */
  if ( cp->desc == NULL )
  {
    str = strsave( NULL );
    cp->desc = str;

    fprintf(
      fp,
      "%s;%s;%c%c;%s;%e;%c;\n",
      cp->id,
      cp->group,
      type_strings[ cp->type ],
      cp->dormant,
      cp->desc,
      cp->prob,
      cp->lambda );

    strfree( cp->desc );
    cp->desc = NULL;

  }
  else
    fprintf(
      fp,
      "%s;%s;%c%c;%s;%e;%c;\n",
      cp->id,
      cp->group,
      type_strings[ cp->type ],
      cp->dormant,
      cp->desc,
      cp->prob,
      cp->lambda );

  return 0;

} /* primary_event_write */

/*--------------------------------------------------------------
 Routine : primary_event_item_type
 Purpose : Return the type of the event in terms that the
 tree application understands.
---------------------------------------------------------------*/
int
  primary_event_item_type(
    PrimaryEventType type )
{
  switch(type) {
    case PRIM_BASIC:
      return BASIC;
      break;
    case PRIM_EXTERN:
      return EXTERNAL;
      break;
    case PRIM_UNDEVELOPED:
      return UNDEVELOP;
      break;
    case PRIM_COND_NOTANAL:
      return COND_NOT_ANAL;
      break;
    case PRIM_COND_ANAL:
      return COND_ANAL;
      break;
    case PRIM_NONE:
      return UNKNOWN;
      break;
    default:
      printf("\n*** primary_event_item_type : "
             "Error translating type ***\n\n");
      exit(1);
  }

  return 0;

} /* primary_event_item_type */


