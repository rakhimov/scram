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

/***************************************************************
 Module : Record_Io
 Author : FSC Limited
 Date   : 22/10/96
 
 SccsId : @(#)Record_Io.c	1.1 10/22/96
 
 Origin : FTA project,
          FSC Limited Private Venture.
 
 This module implements a more modular approach to reading a
 file of records with delimited fields.
 
 This module only implements a few of the interfaces provided,
 and performs no 'Output' although the name implies it does.
 This is a future enhancement.
***************************************************************/

#include <ctype.h>
#include <float.h>

#include "Record_Io.h"
#include "util.h"
#include "MemoryUtil.h"

#define MAX_INPUT_STRING_LENGTH 25

/*---------------------------------------------------------------
 Routine : GetStringField
 Purpose : Reads the next string field from the file.
 If no field exists, or EOF is encountered, returns NULL, else
 returns a pointer to a the string.
 
 Leaves the file pointer at the beginning of the next field (i.e.,
 skips all delimiters).
---------------------------------------------------------------*/
char *
  GetStringField(
    FILE *FromFile )
{
  char *InputString;
  char InputChar; 
  char c;
  char str[ 2 ]; 
/*   int  i = 0; */

/* Get characters, maintaining whitespace, up to the next delimiter */
/* records ending with a delimiter actually end ';'<LF> */
/* so should beware of this */

printf("Record_Io.c: In GetStringField\n");

  /* Get first character of stream... */
  InputChar = GetChar( FromFile, FALSE );

  if ( InputChar == EOF ) {
printf("Record_Io.c: First char is EOF\n");
	  
	  return NULL;
  }
  c = InputChar;

  /* If first character is delimiter, field must be empty... */
  if ( c == DELIMITER )
  {
printf("Record_Io.c: First char is DELIMITER\n");

    return NULL;
  }

  /* skip leading <LF>... */
  while ( c == '\n' )
  {
    InputChar = GetChar( FromFile, FALSE );
    if ( InputChar == EOF ) {
printf("Record_Io.c: Skipping leading <LFs> First, but char is EOF\n");

      return NULL;
	}
    c = InputChar;
  }

  /* Read string field... */
  InputString = strsave( NULL );

  str[ 1 ] = '\0';

  while ( c != DELIMITER )
  {
    str[ 0 ] = c;
    InputString = strgrow( InputString, str );
    InputChar = GetChar( FromFile, FALSE );
    if ( InputChar == EOF )
    {
      InputString = strgrow( InputString, '\0' );
  if(InputString == NULL) {
printf("Record_Io.c: Returned string = NULL\n");
  } else {
printf("Record_Io.c: Returned string = %s\n", InputString);
}

      return InputString;
    }
    c = InputChar;
  }
  InputString = strgrow( InputString, '\0' );

  if(InputString == NULL) {
printf("Record_Io.c: Returned string = NULL\n");
  } else {
printf("Record_Io.c: Returned string = %s\n", InputString);
}
  return InputString;

} /* GetStringField */

/*---------------------------------------------------------------
 Routine : GetChar
 Purpose : Reads the next character from the file.
 If no character exists, or EOF is encountered, returns EOF, else
 returns a pointer to a character.
---------------------------------------------------------------*/
char 
  GetChar(
    FILE *FromFile,
    BOOL IgnoreWhiteSpace )
{
/* get char. If char not EOL or EOF, and not field delimiter, return char
   else skip and get next char */

  char c;

  c = fgetc( FromFile );

/*  printf("Record_Io.c: In GetChar, current char = '%c'\n", c); */

  if ( c == EOF )
    return EOF;

  if ( IgnoreWhiteSpace == TRUE )
  {

    while ( isspace( c ) )
    {
      c = fgetc( FromFile );
      if ( c == EOF ) 
      {
        return EOF;
      }
    }

    return c;

  }
  else
  {
    if ( c == EOF ) 
    {
        return EOF;
    }

    return c;

  }

  return EOF;

} /* GetChar */

int *
  get_int(
    FILE *fp )
{
  return NULL;
}

/*---------------------------------------------------------------
 Routine : GetFloatField
 Purpose : Reads the next float field from the file.
 If no field exists, or EOF is encountered, then Success will be returned FALSE, else
 returns a pointer to the float and Success will be returned TRUE.

 Leaves the file pointer at the beginning of the next field (i.e.,
 skips all delimiters).
---------------------------------------------------------------*/
float
  GetFloatField(
    FILE *FromFile,
	BOOL *Success )
{
  char *str;
  float theFloat;

  str = GetStringField( FromFile );
  if ( str == NULL )
  {
    theFloat = FLT_MAX;
	*Success = FALSE;
    return theFloat;
  }

  if ( !sscanf( str, "%g", &theFloat ) )
  {
    theFloat = FLT_MAX;
	*Success = FALSE;
  }

  FreeMemory(str);
  *Success = TRUE;
  return theFloat;

} /* GetFloatField */

void
  skip_field(
    FILE *fp )
{
}

void
  skip_entry(
    FILE *fp )
{
}

