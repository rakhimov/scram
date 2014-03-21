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
 
 SccsId : @(#)FileUtilities.h	1.1 08/22/96
 
 Origin : FTA project,
          FSC Limited Private Venture, Level 2 Prototype.
 
 This module provides general purpose file handling routines.
***************************************************************/
#ifndef FILE_UTILS_H
#define FILE_UTILS_H
 
#include <time.h>
#include "boolean.h"
 
#ifdef WIN32
#define R_OK 4
#define W_OK 2
#define F_OK 0
#endif /* WIN32 */


/*---------------------------------------------------------------
 Routine : generate_filename
 Purpose : Take file name (possibly with suffix) and generate
 name with different suffix
 e.g.   generate_filename("thing.x", ".uvw") -> "thing.uvw"
 Returns pointer to heap.
 
 Presumes that filename is simple (i.e., not with a pathname)
 
 This routine will work if the filename is not simple as long as
 the pathname doesnot contain any '.'
---------------------------------------------------------------*/
extern char *
  generate_filename(
    char *fname,    /* in: Filename string */
    char *suffix ); /* in: Suffix string e.g., "*", "*.tla", ".tla" */
                    /* return: filename.suffix string */
 
/*---------------------------------------------------------------
 Routine : filename_from_pathname
 Purpose : Strips the filename from the end of the pathname
 supplied and returns a pointer to the filename.
---------------------------------------------------------------*/
extern char *
  filename_from_pathname(
    char *pathname );
 
/*---------------------------------------------------------------
 Routine : path_from_pathname
 Purpose : Strips the path from the start of the pathname
 supplied and returns a pointer to the path.
---------------------------------------------------------------*/
extern char *
  path_from_pathname(
    char *pathname );
 

/*---------------------------------------------------------------
 Routine : file_copy
 Purpose : Copies the file in from to the file in to.
 If the copy fails it returns non-zero
---------------------------------------------------------------*/
extern int
  file_copy(
    char *from,
    char *to );
 
/*---------------------------------------------------------------
 Routine : date_of
 Purpose : Return date stamp of a file
---------------------------------------------------------------*/
extern time_t
  date_of( char *filename );
 
/*---------------------------------------------------------------
 Routine : file_exists
 Purpose : Return indication that a file exists
---------------------------------------------------------------*/
extern BOOL
  file_exists(
    char *file );

/*---------------------------------------------------------------
 Routine : file_is_valid
 Purpose : Returns an indication of whether the file supplied can
 be accessed for the mode supplied.
---------------------------------------------------------------*/
extern BOOL
  file_is_valid(
    char *filename,
    int mode );

/*---------------------------------------------------------------
 Routine : file_has_suffix
 Purpose : Returns the suffix of the given filename, or NULL if
 no suffix exists.
---------------------------------------------------------------*/
extern char *
  file_has_suffix(
    char *filename );


/*---------------------------------------------------------------
 Routine : number_of_folders
 Purpose : returns the number of folders contained in a path
---------------------------------------------------------------*/
extern int
  num_folders_in_pathname(
    char *path );


/*---------------------------------------------------------------
 Routine : file_separator
 Purpose : returns the value of the platform specific file
           separator
---------------------------------------------------------------*/
extern char
  file_separator();



#endif
