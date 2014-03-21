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
 Module : StringUtilities
 Author : FSC Limited
 Date   : 5/8/96
 
 SccsId : @(#)StringUtilities.c	1.2 09/26/96
 
 Origin : FTA project,
          FSC Limited Private Venture, Level 2 Prototype.
 
This module provides a C implementation of the set of String
Utilities as specified in Software Components in Ada by Booch.
 
***************************************************************/

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "StringUtilities.h"
#include "util.h"

#include "MemoryUtil.h"

#include "AssertUtil.h"

ASSERTFILE(__FILE__)

/**************************************************************************/

void
  SU_Make_Uppercase(
    char *The_String )
{
  int i;

  for ( i = 0; i < (int)strlen( The_String ); i++ ) {
    The_String[ i ] = toupper( The_String[ i ] );
  }

} /* SU_Make_Uppercase */

/**************************************************************************/

void
  SU_Make_Lowercase(
    char *The_String )
{
  int i;

  for ( i = 0; i < (int)strlen( The_String ); i++ ) {
    The_String[ i ] = tolower( The_String[ i ] );
  }

} /* SU_Make_Lowercase */

/**************************************************************************/

void
  SU_Capitalize(
    char *The_String )
{
  int i;

  for ( i = ( (int)strlen( The_String ) - 1 ); i > 0; i-- ) {
    if ( ( isspace( The_String[ i - 1 ] ) ) ||
         ( The_String[ i - 1 ] == '_' ) ) {
      The_String[ i ] = toupper( The_String [ i ] );
    }
  }
} /* SU_Capitalize */

/**************************************************************************/

void
  SU_Uncapitalize(
    char *The_String )
{
  int i;

  for ( i = ( (int)strlen( The_String ) - 1 ); i > 0; i-- ) {
    if ( ( isspace( The_String[ i - 1 ] ) ) ||
         ( The_String[ i - 1 ] == '_' ) ) {
      The_String[ i ] = tolower( The_String [ i ] );
    }
  }
} /* SU_Uncapitalize */

/**************************************************************************/

void
  SU_Replace(
    char The_Character,
    char With_The_Character,
    char *In_The_String,
    BOOL Case_Sensitive )
{
  int i;

  if ( Case_Sensitive == TRUE ) {
    for ( i = 0; i < (int)strlen( In_The_String ); i++ ) {
      if ( In_The_String[ i ] == The_Character )
        In_The_String[ i ] = With_The_Character;
    }
  }
  else {
    for ( i = 0; i < (int)strlen( In_The_String ); i++ ) {
      if ( tolower( In_The_String[ i ] ) == tolower( The_Character ) )
        In_The_String[ i ] = With_The_Character;
    }
  }

} /* SU_Replace */

/**************************************************************************/

char *
  SU_Uppercase(
    char *The_String )
{
  SU_Make_Uppercase( The_String );
  return The_String;

} /* SU_Uppercase */

/**************************************************************************/

char *
  SU_Lowercase(
    char *The_String )
{
  SU_Make_Lowercase( The_String );
  return The_String;

} /* SU_Lowercase */
 
/**************************************************************************/

char *
  SU_Capitalized(
    char *The_String )
{
  SU_Capitalize( The_String );
  return The_String;
} /* SU_Capitalized */
 
/**************************************************************************/

char *
  SU_Uncapitalized(
    char *The_String )
{
  SU_Uncapitalize( The_String );
  return The_String;
} /* SU_Uncapitalized */
 
/**************************************************************************/

char *
  SU_Replaced(
    char The_Character,
    char With_The_Character,
    char *In_The_String,
    BOOL Case_Sensitive )
{
  SU_Replace(
    The_Character,
    With_The_Character,
    In_The_String,
    Case_Sensitive );

  return In_The_String;

} /* SU_Replaced */

/**************************************************************************/

BOOL
  SU_Is_Null(
    char *The_String )
{
  if ( The_String[ 0 ] == '\0' )
    return TRUE;
  else
    return FALSE;

} /* SU_Is_Null */

/**************************************************************************/

BOOL
  SU_Is_Control(
    char *The_String )
{
  int i;

  for ( i = 0; i < (int)strlen( The_String ); i++ )
    if ( !iscntrl( The_String[ i ] ) )
      return FALSE;
  return TRUE;

} /* SU_Is_Control */
 
/**************************************************************************/

BOOL
  SU_Is_Graphic(
    char *The_String )
{
  int i;

  for ( i = 0; i < (int)strlen( The_String ); i++ )
    if ( !isgraph( The_String[ i ] ) )
      return FALSE;
  return TRUE;

} /* SU_Is_Graphic */
 
/**************************************************************************/

BOOL
  SU_Is_Uppercase(
    char *The_String )
{
  int i;

  for ( i = 0; i < (int)strlen( The_String ); i++ )
    if ( !isupper( The_String[ i ] ) )
      return FALSE;
  return TRUE;

} /* SU_Is_Uppercase */
 
/**************************************************************************/

BOOL
  SU_Is_Lowercase(
    char *The_String )
{
  int i;

  for ( i = 0; i < (int)strlen( The_String ); i++ )
    if ( !islower( The_String[ i ] ) )
      return FALSE;
  return TRUE;

} /* SU_Is_Lowercase */
 
/**************************************************************************/

BOOL
  SU_Is_Digit(
    char *The_String )
{
  int i;

  for ( i = 0; i < (int)strlen( The_String ); i++ )
    if ( !isdigit( The_String[ i ] ) )
      return FALSE;
  return TRUE;

} /* SU_Is_Digit */
 
/**************************************************************************/

BOOL
  SU_Is_Alphabetic(
    char *The_String )
{
  int i;

  for ( i = 0; i < (int)strlen( The_String ); i++ )
    if ( !isalpha( The_String[ i ] ) )
      return FALSE;
  return TRUE;

} /* SU_Is_Alphabetic */
 
/**************************************************************************/

BOOL
  SU_Is_Alphanumeric(
    char *The_String )
{
  int i;

  for ( i = 0; i < (int)strlen( The_String ); i++ )
    if ( !isalnum( The_String[ i ] ) )
      return FALSE;
  return TRUE;

} /* SU_Is_Alphanumeric */
 
/**************************************************************************/

BOOL
  SU_Is_Special(
    char *The_String )
{
  return FALSE;
} /* SU_Is_Special */
 
/**************************************************************************/

char *
  SU_Centered(
    char *The_String,
    int  In_The_Width,
    char With_The_Filler )
{
  int Left_Margin;
  int Right_Margin; 
  int i;
  int j;

/*   char *Centered_String = (char *)malloc( In_The_Width + 1 ); */
  char *Centered_String;

  ASSERT( In_The_Width >= (int)strlen( The_String ) );

  if ( fNewMemory( (void **)&Centered_String, In_The_Width + 1 ) )
  { 

    Left_Margin = ( In_The_Width - (int)strlen( The_String ) ) / 2;
    Right_Margin = (int)strlen( The_String ) + Left_Margin; 

    for ( i = 0; i < Left_Margin; i++ ) {
      Centered_String[ i ] = With_The_Filler;
    }

    j = 0;
    for ( i = i; i < Right_Margin; i++ ) {
      Centered_String[ i ] = The_String[ j++ ];
    }

    for ( i = i; i < In_The_Width; i++ ) {
      Centered_String[ i ] = With_The_Filler;
    }

    Centered_String[ i ] = '\0';
  
    return Centered_String;

  }
  else
    return NULL;

} /* SU_Centered */
  
/**************************************************************************/

char *
  SU_Left_Justified(
    char *The_String,
    int  In_The_Width,
    char With_The_Filler )
{
  int i;
/*   char *Justified_String = (char *)malloc( In_The_Width + 1 ); */
  char *Justified_String;

  ASSERT( In_The_Width >= (int)strlen( The_String ) );

  if( fNewMemory( (void **)&Justified_String, In_The_Width + 1 ) )
  {
    for ( i = 0; i < (int)strlen( The_String ); i++ )
      Justified_String[ i ] = The_String[ i ];

    for ( i = i; i < In_The_Width; i++ ) 
      Justified_String[ i ] = With_The_Filler;

    Justified_String[ i ] = '\0';

    return Justified_String;

  }
  else
    return NULL;

} /* SU_Left_Justified */

/**************************************************************************/

char *
  SU_Right_Justified(
    char *The_String,
    int  In_The_Width,
    char With_The_Filler )
{
  int i;
  int j;
  int Right_Margin;
/*   char *Justified_String = (char *)malloc( In_The_Width + 1 ); */
  char *Justified_String;

  ASSERT( In_The_Width >= (int)strlen( The_String ) );

  if ( fNewMemory( (void **)&Justified_String, In_The_Width + 1 ) )
  {
    Right_Margin = In_The_Width - (int)strlen( The_String );
 
    for ( i = 0; i < Right_Margin; i++ )
      Justified_String[ i ] = With_The_Filler;

    j = 0;
    for ( i = i; i < In_The_Width; i++ )
      Justified_String[ i ] = The_String[ j ];
 
    Justified_String[ i ] = '\0';

    return Justified_String;

  }
  else
    return NULL;

} /* SU_Right_Justified */

/**************************************************************************/

char *
  SU_Stripped(
    char The_Character,
    char *From_The_String,
    BOOL Case_Sensitive )
{
  int i;
  int j;
/*   char *Stripped_String = (char *)malloc( (int)strlen( From_The_String ) + 1 ); */
  char *Stripped_String;

  if ( fNewMemory( (void **)&Stripped_String, (int)strlen( From_The_String ) + 1 ) )
  {

    j = 0;
    if ( Case_Sensitive == TRUE ) {
      for ( i = 0; i < (int)strlen( From_The_String ); i++ ) {
        if ( From_The_String[ i ] != The_Character )
          Stripped_String[ j++ ] = From_The_String[ i ];
      }
    }
    else {
      for ( i = 0; i < (int)strlen( From_The_String ); i++ ) {
        if ( tolower( From_The_String[ i ] ) != tolower( The_Character ) )
          Stripped_String[ j++ ] = From_The_String[ i ];
      }
    }

    Stripped_String[ j ] = '\0';

    return Stripped_String;

  }
  else
    return NULL;

} /* SU_Stripped */

/**************************************************************************/

char *
  SU_Stripped_Leading(
    char The_Character,
    char *From_The_String,
    BOOL Case_Sensitive )
{
  int i;
  int j;
/*   char *Stripped_String = (char *)malloc( (int)strlen( From_The_String ) + 1 ); */
  char *Stripped_String;

  if ( fNewMemory( (void **)&Stripped_String, (int)strlen( From_The_String ) + 1 ) )
  { 

    j = 0;
    i = 0;
    if ( Case_Sensitive == TRUE ) {
      while ( ( i < (int)strlen( From_The_String ) ) &&
              ( From_The_String[ i ] == The_Character ) ) {
        i++;
      }
      for ( i = i; i < (int)strlen( From_The_String ); i++ ) {
        Stripped_String[ j++ ] = From_The_String[ i ];
      }
    }
    else {
      while ( ( i < (int)strlen( From_The_String ) ) &&
              ( tolower( From_The_String[ i ] ) == 
                tolower( The_Character ) ) ) {
        i++;
      }
      for ( i = i; i < (int)strlen( From_The_String ); i++ ) {
        Stripped_String[ j++ ] = From_The_String[ i ];
      }
    }

    Stripped_String[ j ] = '\0';

    return Stripped_String;

  }
  else
    return NULL;

} /* SU_Stripped_Leading */

/**************************************************************************/

char *
  SU_Stripped_Trailing(
    char The_Character,
    char *From_The_String,
    BOOL Case_Sensitive )
{
  int i;
  int j;
/*   char *Stripped_String = (char *)malloc( (int)strlen( From_The_String ) + 1 ); */
  char *Stripped_String;

  if ( fNewMemory( (void **)&Stripped_String, (int)strlen( From_The_String ) + 1 ) )
  {

    j = 0;
    i = (int)strlen( From_The_String ) - 1;
    if ( Case_Sensitive == TRUE ) {
      while ( ( i >= 0 ) &&
              ( From_The_String[ i ] == The_Character ) ) {
        i--;
      }
      j = i + 1;
      for ( i = 0; i < j; i++ ) {
        Stripped_String[ i ] = From_The_String[ i ];
      }
    }
    else {
      while ( ( i >= 0 ) &&
              ( tolower( From_The_String[ i ] ) == 
                  tolower( The_Character ) ) ) {
        i--;
      }
      j = i + 1;
      for ( i = 0; i < j; i++ ) {
        Stripped_String[ i ] = From_The_String[ i ];
      }
    }

    Stripped_String[ i ] = '\0';

    return Stripped_String;

  }
  else
    return NULL;

} /* SU_Stripped_Trailing */

/**************************************************************************/

int
  SU_Number_Of_Char(
    char The_Character,
    char *In_The_String,
    BOOL Case_Sensitive )
{
  int count;
  int i;

  count = 0;
  if ( Case_Sensitive == TRUE ) {
    for ( i = 0; i < (int)strlen( In_The_String ); i++ )
      if ( In_The_String[ i ] == The_Character )
        count++;
  }
  else {
    for ( i = 0; i < (int)strlen( In_The_String ); i++ )
      if ( tolower( In_The_String[ i ] ) == tolower( The_Character ) )
        count++;
  }

  return count;

} /* SU_Number_Of_Char */

/**************************************************************************/

int
  SU_Number_Of_Str(
    char *The_String,
    char *In_The_String,
    BOOL Case_Sensitive )
{
  int count;
/*   int i; */
  char *s;
  char *ss;

  ASSERT( In_The_String != NULL );

  count = 0;

  if ( (int)strlen( The_String ) > (int)strlen( In_The_String ) )
    return count;

/*   s = (char *)malloc( (int)strlen( In_The_String ) + 1 ); */
  if ( fNewMemory( (void **)&s, (int)strlen( In_The_String ) + 1 ) )
  {
    if ( Case_Sensitive == TRUE ) {
      s = strcpy( s, In_The_String );
      while ( ( s = strstr( s, The_String ) ) != NULL ) {
        s = s + (int)strlen( The_String );
        count ++;
      } 
      FreeMemory( s );

      return count;
    }
    else {
/*       ss = (char *)malloc( (int)strlen( The_String ) + 1 ); */
      if ( fNewMemory( (void **)&ss, (int)strlen( The_String ) + 1 ) )
      {

        s = strcpy( s, In_The_String );
        ss = strcpy( ss, The_String );
        SU_Make_Lowercase( s );
        SU_Make_Lowercase( ss );
        while ( ( s = strstr( s, ss ) ) != NULL ) {
          s = s + (int)strlen( ss );
          count++;
        }
        FreeMemory( ss );
      }
      FreeMemory( s );

      return count;
    }
  }
  else
    return count;

} /* SU_Number_Of_Str */
 
/**************************************************************************/

int
  SU_Location_Of(
    char The_Character,
    char *In_The_String,
    BOOL Case_Sensitive,
    BOOL Forward )
{
  int i;

  if ( Case_Sensitive == TRUE ) {
    if ( Forward == TRUE ) {
      for ( i = 0; i < (int)strlen( In_The_String ); i++ ) {
        if ( In_The_String[ i ] == The_Character ) {
          return i;
        }
      }
    }
    else {
      for ( i = (int)strlen( In_The_String ) - 1; i >= 0; i-- ) {
        if ( In_The_String[ i ] == The_Character ) {
          return i;
        }
      }
    }
  }
  else {
    if ( Forward == TRUE ) {
      for ( i = 0; i < (int)strlen( In_The_String ); i++ ) {
        if ( tolower( In_The_String[ i ] ) == tolower( The_Character ) ) {
          return i;
        }
      }
    }
    else {
      for ( i = (int)strlen( In_The_String ) - 1; i >= 0; i-- ) {
        if ( tolower( In_The_String[ i ] ) == tolower( The_Character ) ) {
          return i;
        }
      }
    }
  }

  return 0; /* Not found in string? */

} /* SU_Location_Of */

/**************************************************************************/

BOOL
  SU_Is_Equal(
    char *Left,
    char *Right,
    BOOL Case_Sensitive )
{
  int i;

  if ( (int)strlen( Left ) != (int)strlen( Right ) )
    return FALSE;

  if ( Case_Sensitive == TRUE ) {
    for ( i = 0; i < MAX( (int)strlen( Left ), (int)strlen( Right ) ); i++ ) {
      if ( Left[ i ] != Right [ i ] )
        return FALSE;
    }
    return TRUE;
  }
  else {
    for ( i = 0; i < MAX( (int)strlen( Left ), (int)strlen( Right ) ); i++ ) {
      if ( tolower( Left[ i ] ) != tolower( Right [ i ] ) )
        return FALSE;
    }
    return TRUE;
  }

} /* SU_Is_Equal */

/**************************************************************************/

BOOL
  SU_Is_Less_Than(
    char *Left,
    char *Right,
    BOOL Case_Sensitive )
{
  int i;

  if ( (int)strlen( Left ) > (int)strlen( Right ) )
    return FALSE;

  if ( Case_Sensitive == TRUE ) {
    for ( i = 0; i < MAX( (int)strlen( Left ), (int)strlen( Right ) ); i++ ) {
      if ( Left[ i ] > Right [ i ] )
        return FALSE;
    }
    return TRUE;
  }
  else {
    for ( i = 0; i < MAX( (int)strlen( Left ), (int)strlen( Right ) ); i++ ) {
      if ( tolower( Left[ i ] ) > tolower( Right [ i ] ) )
        return FALSE;
    }
    return TRUE;
  }

} /* SU_Is_Less_Than */

/**************************************************************************/

BOOL
  SU_Is_Greater_Than(
    char *Left,
    char *Right,
    BOOL Case_Sensitive )
{
  int i;

  if ( (int)strlen( Left ) < (int)strlen( Right ) )
    return FALSE;

  if ( Case_Sensitive == TRUE ) {
    for ( i = 0; i < MAX( (int)strlen( Left ), (int)strlen( Right ) ); i++ ) {
      if ( Left[ i ] < Right [ i ] )
        return FALSE;
    }
    return TRUE;
  }
  else {
    for ( i = 0; i < MAX( (int)strlen( Left ), (int)strlen( Right ) ); i++ ) {
      if ( tolower( Left[ i ] ) < tolower( Right [ i ] ) )
        return FALSE;
    }
    return TRUE;
  }

} /* SU_Is_Greater_Than */


int
  SU_Num_Same_Chars(
    char *string1,
    char *string2 )
{
    int num_chars = 0;
    int len1;
    int len2;

    len1 = strlen(string1);
    len2 = strlen(string2);

    while(string1[num_chars] == string2[num_chars] && num_chars < len1 && num_chars < len2) {
        num_chars++;
    }

    return num_chars;
} /* SU_Num_Same_Chars */
    

char *
  SU_Join(
    char *string1,
    char *string2 )
{
    char *result;

    /* are both strings empty? */
    if(!string1 && !string2) {
        result = NULL;

    } else if(string1 && !string2) {
		fNewMemory( (void **)&result, strlen(string1) + 1 );
        /* result = (char*)malloc((strlen(string1) + 1) * sizeof(char)); */
        if(result) {
            strcpy(result, string1);
        }
    } else if(!string1 && string2) {
		fNewMemory( (void **)&result, strlen(string2) + 1 );
        /* result = (char*)malloc((strlen(string2) + 1) * sizeof(char));*/
        if(result) {
            strcpy(result, string2);
        }

    } else if(string1 && string2) {
		fNewMemory( (void **)&result, strlen(string1) + strlen(string2) + 1 );
        /* result = (char*)malloc((strlen(string1) + strlen(string2) + 1) * sizeof(char)); */
        if(result) {
            strcpy(result, string1);
            strcat(result, string2);
        }
    }

    return result;

} /* SU_Join */
    

char *
  SU_Copy(
    char *string )
{
    char *result;

    /* are both strings empty? */
    if(!string) {
        result = NULL;

    } else {
		fNewMemory( (void **)&result, strlen(string) + 1 );
        /* result = (char*)malloc((strlen(string) + 1) * sizeof(char)); */
        if(result) {
            strcpy(result, string);
        }
    }

    return result;

} /* SU_Copy */
