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
 Module : File Utilities
 Author : FSC Limited
 Date   : 5/8/96
 
 SccsId : @(#)FileUtilities.c	1.3 12/16/96
 
 Origin : FTA project,
          FSC Limited Private Venture, Level 2 Prototype.
 
 This module provides general purpose file handling routines.
***************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef WIN32
#include <unistd.h>
#else
#include <Io.h>
#endif /* WIN32 */

#include <sys/stat.h> 

#include "FileUtilities.h"
#include "StringUtilities.h"
#include "util.h"

#include "MemoryUtil.h"

#include "AssertUtil.h"

ASSERTFILE(__FILE__)

#define FILE_EXISTS 0



/*---------------------------------------------------------------
 Routine : generate_filename
 Purpose : Take file name (possibly with suffix) and generate
 name with different suffix
 e.g.   generate_filename("thing.x", ".uvw") -> "thing.uvw"
 Returns pointer to heap.
 
 This routine will work if the filename is not simple as long as
 the pathname does not contain any '.'
---------------------------------------------------------------*/
char *
  generate_filename(
    char *fname,    /* in: Filename string */
    char *suffix )  /* in: Suffix string e.g., "*", "*.tla", ".tla" */
                    /* return: filename.suffix string */
{
  char *s = NULL;
  char *dot = strrchr(fname, '.');
  int len_dot    = (dot    == NULL) ? 0 : strlen(dot);
  int curr_len_dot    = 0;
  int len_fname  = (fname  == NULL) ? 0 : strlen(fname);
/*   int len_suffix = (suffix == NULL) ? 0 : strlen(suffix); */
/*   int n = len_fname - len_dot; */
/*   int len = n + len_suffix; */
 
  ASSERT( fname != NULL);
/*   ASSERT( SU_Number_Of_Char( '.', fname, FALSE ) <= 1 ); */
 
/*   if ( len_dot == 0 ) {  */
/*     if ( fNewMemory( (void *)&s, len_fname + 1 + len_suffix + 1 ) )  */
/*     {  */
/*       strcpy( s, fname );  */
/*       strcpy( s + len_fname, "." );  */
/*       strcpy( s + len_fname + 1, suffix );  */
/*     }  */
/*     else  */
/*       return NULL;  */
/*   } else {  */
/*   */
/*     if ( n > 0 ) {  */
/*       if (   */
/*         fNewMemory(   */
/*           (void *)&s,   */
/*           ( len_fname - len_dot + 1 ) + len_suffix + 1 ) )  */
/*       {  */
/*         strcpy( s, fname );  */
/*         strcpy( s + ( len_fname - len_dot + 1 ), suffix );  */
/*       }  */
/*       else  */
/*         return NULL;  */
/*     }  */
/*   }  */
/*   return s;  */
  
/* Alternatively... */

  /* find position of LAST '.' in filename */  
  len_dot = 0;
  curr_len_dot = 0;
  do {
    curr_len_dot = SU_Location_Of( '.', &(fname[len_dot]), FALSE, TRUE );
    len_dot += curr_len_dot + 1;
    curr_len_dot++;
  } while(curr_len_dot != 0 && (len_dot + curr_len_dot) < len_fname);

  len_dot--;

  if ( len_dot == 0 )
    len_fname = (int)strlen( fname );  
  else
    len_fname = len_dot;

  if ( fNewMemory( (void **)&s, ( len_fname + 1 + (int)strlen( suffix ) + 1 ) ) )
  { 
    strcpy( s, fname ); 
    strcpy( s + len_fname, "." ); 
    strcpy( s + len_fname + 1, suffix ); 
    return s; 
  } 
  else
  {    
    return strsave( NULL );  
  }
 
} /* generate_filename */
 
/*---------------------------------------------------------------
 Routine : filename_from_pathname
 Purpose : Strips the filename from the end of the pathname
 supplied and returns a pointer to the filename.

 returns a pointer to the Heap.

 find last '/'
 if there is a last '/'
 then
   if last '/' not at end of pathname
   then
     allocate memory for string after last '/'
     copy pathname + position of '/' + 1 to new string
     return new string
   else
     return NULL
   end if;
 else
   return strsave( pathname );
 end if;

---------------------------------------------------------------*/
char *filename_from_pathname(char *pathname )
{
	char *s = NULL;
	/* slash points to the last occurance of '/' with the pathname,
		so cannot be freed by this routine */
	char *slash;
	/*   int pos_slash; */
 
	if ( pathname == NULL )
		return NULL;

#ifdef WIN32
	if ( SU_Number_Of_Char( '\\', pathname, FALSE ) == 0 )
#else
	if ( SU_Number_Of_Char( '/', pathname, FALSE ) == 0 )
#endif
	{
		return strsave( pathname );
	}
	else
	{
#ifdef WIN32
		slash = strrchr( pathname, '\\' );
#else
		slash = strrchr( pathname, '/' );
#endif
		if ( fNewMemory( (void **)&s, strlen( slash ) ) )
		{
			strcpy( s, slash + 1 );
			return s;
		}
	}
	return strsave( NULL );

} /* filename_from_pathname */
 

/*---------------------------------------------------------------
 Routine : path_from_pathname
 Purpose : Strips the path from the start of the pathname
 supplied and returns a pointer to the path.

 returns a pointer to the Heap.

 find last '/'
 if there is a last '/'
 then
   if last '/' not at end of pathname
   then
     allocate memory for before last '/'
     copy pathname from 0 to position of '/' to new string
     return new string
   else
     return NULL
   end if;
 else
   return strsave( "" );
 end if;

---------------------------------------------------------------*/
char *path_from_pathname(char *pathname )
{
	char *s = NULL;
	/* slash points to the last occurance of '/' with the pathname,
		so cannot be freed by this routine */
	char *slash;
	/*   int pos_slash; */
 
	if ( pathname == NULL )
		return NULL;

#ifdef WIN32
	if ( SU_Number_Of_Char( '\\', pathname, FALSE ) == 0 )
#else
	if ( SU_Number_Of_Char( '/', pathname, FALSE ) == 0 )
#endif
	{
		return strsave( NULL );
	}
	else
	{
#ifdef WIN32
		slash = strrchr( pathname, '\\' );
#else
		slash = strrchr( pathname, '/' );
#endif
		if ( fNewMemory( (void **)&s, strlen(pathname) - strlen(slash+1) + 1) )
		{
            /* copy relevant part of string and terminate with NULL char */
			strncpy(s, pathname, strlen(pathname) - strlen(slash+1));
            s[strlen(pathname) - strlen(slash+1)] = '\0';
			return s;
		}
	}
	return strsave( NULL );

} /* path_from_pathname */


/*---------------------------------------------------------------
 Routine : file_copy
 Purpose : Copies the file in from to the file in to.
 If the copy fails it returns non-zero
---------------------------------------------------------------*/
int
  file_copy(
    char *from,
    char *to )
{
#ifdef WIN32
	char *cmd = (char *)strsave( "copy \"" );
#else
	char *cmd = (char *)strsave( "cp \"" );
#endif
	
	int i;
 
ASSERT( ( from != NULL ) && ( to != NULL ) );
 
  cmd = (char *)strgrow( cmd, from );
  cmd = (char *)strgrow( cmd, "\" \"" );
  cmd = (char *)strgrow( cmd, to );
  cmd = (char *)strgrow( cmd, "\"" );
 
  i = system( cmd );
 
  strfree( cmd );
 
  return i;
 
} /* file_copy */
 
/*---------------------------------------------------------------
 Routine : date_of
 Purpose : Return date stamp of a file
---------------------------------------------------------------*/
time_t
  date_of( char *filename )
{
  struct stat stat_info;
  time_t date;
 
ASSERT( filename != NULL );
 
 /* check if file exists and get date stamp */
  if (  stat(filename, &stat_info) == -1 ) {
/*     printf("date_of: file %s not found\n", filename);  */
    return 0;
  } else {
    date = stat_info.st_mtime;
   /* printf("date_of: %20s : %s", filename, ctime( &date )); */
  }
 
  return date;
 
} /* date_of */
 
/*---------------------------------------------------------------
 Routine : file_exists
 Purpose : Return indication that a file exists
---------------------------------------------------------------*/
BOOL
  file_exists(
    char *filename )
{
  return file_is_valid( filename, F_OK );

} /* file_exists */

/*---------------------------------------------------------------
 Routine : file_is_valid
 Purpose : Returns an indication of whether the file supplied can
 be accessed for the mode supplied.

 if (mode is read and file exists for read) or
    (mode is write and file does not exist) or
    (mode is write and file exists for write) or
    (mode is exists and file exists) then
   return True;
 else
   return False;
 end if;

---------------------------------------------------------------*/
BOOL
  file_is_valid(
    char *filename,
    int mode )
{
  char *s;
 
/* ASSERT( filename != NULL ); */
 
  if (
    ( ( s = filename_from_pathname( filename ) ) != NULL ) &&
    ( ( mode == R_OK ) && ( access( filename, mode ) == FILE_EXISTS ) ) ||
    ( ( mode == W_OK ) && !( access( filename, F_OK ) == FILE_EXISTS ) ) ||
    ( ( mode == W_OK ) && ( access( filename, mode ) == FILE_EXISTS ) ) ||
    ( ( mode == F_OK ) && ( access( filename, mode ) == FILE_EXISTS ) ) ) {
    FreeMemory( s );
    return TRUE;
  } else {
    FreeMemory( s );
    return FALSE;
  }
} /* file_is_valid */

/*---------------------------------------------------------------
 Routine : file_has_suffix
 Purpose : Returns the suffix of the given filename, or NULL if
 no suffix exists.

 if filename has a dot then
   return characters after dot
 else
   return NULL;
 end if;

---------------------------------------------------------------*/
char *
  file_has_suffix(
    char *filename )
{
  char *dot_and_suffix = strrchr( filename, '.' );
  int len_suffix =
    ( dot_and_suffix == NULL ) ? 0 : strlen( dot_and_suffix ) - 1;
 
ASSERT( filename != NULL );
 
  if ( len_suffix > 0 ) {
    return dot_and_suffix + 1;
  }
  return NULL;
} /* file_has_suffix */



/*---------------------------------------------------------------
 Routine : number_of_folders
 Purpose : returns the number of folders contained in a path
---------------------------------------------------------------*/
int
  num_folders_in_pathname(
    char *path )
{
    int num_chars;

#ifdef WIN32
	num_chars = SU_Number_Of_Char( '\\', path, FALSE );
#else
	num_chars = SU_Number_Of_Char( '/', path, FALSE );
#endif

    return num_chars;

}


/*---------------------------------------------------------------
 Routine : file_separator
 Purpose : returns the value of the platform specific file
           separator
---------------------------------------------------------------*/
extern char
  file_separator()
{
#ifdef WIN32
	return '\\';
#else
	return '/';
#endif
}

