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

/*****************************************************************
 Module : CutSetsCommon
 Author : FSC Limited
 Date   : 24 July 1996 

 SccsId : @(#)CutSetsCommon.c	1.2 09/26/96

 This module encapsulates the common routines associated with
 minimum cut sets. These are used by both Combo and Algebraic
 methods of cut set production.

******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "CutSetsCommon.h"
#include "util.h"
#include "FileUtilities.h"
#include "fta.h"
#include "treeUtil.h"
#include "event_list.h"

#include "MemoryUtil.h"

/*---------------------------------------------------------------
 Routine : youngest_fta
 Purpose : Return date stamp of a the given file. If it is a tree
 file, find the youngest of any included tree and the given tree,
 for all included trees.

 IF suffix is ".fta" THEN
    look for all included trees and return date of most recent
 ELSE
    just return date of this file
 ENDIF
 
 If an error occurs, returns (time_t)NULL. This may not be 
 satisfactory.  MPA 1/8/96
---------------------------------------------------------------*/
static time_t
  youngest_fta( char *filename )
{
  FILE *fp;
  char *suffix;
/*     struct stat stat_info; */
  time_t date;
  char id[13];
  char line[100];
/*   char c; */
  char type[3];
  int n;
/*   int i; */
  char *text;
 
  date = date_of( filename );
 
 /* if ".fta" file, read file and check transfered-in trees */
  if ( ( suffix = strrchr( filename, '.' ) ) != NULL &&
         strcmp( suffix, ".fta") == 0 ) {
 
    fp = fopen(filename, "r");
 
     /* discard first line (database name) */
    if (fscanf(fp, "%[^\n]\n", line) != 1) {
      printf("youngest_fta(1): error reading file %s\n", filename);
      return 0;
    }
 
    text = (char *)load_post_it_note( fp );
    FreeMemory( text );

    if (fscanf(fp, "%s %s %d\n", type, id, &n) < 3) {
      printf("ERROR READING EVENT TYPE AND ID FROM FILE\n");
      return (time_t)NULL;
    }
    if ( type[ 0 ] == 'I' ) {         /* a transfer-in event   */
      date = MAX(date, date_of(id));
    } else if ( type[ 0 ] == 'M' ) {  /* an intermediate event */
 
      /* discard (text) */
      if (fscanf(fp, "%d", &n) < 1) {
        printf("ERROR READING EVENT DESCRIPTION TEXT LENGTH FROM FILE\n");
        return (time_t)NULL;
      } else {
/*         if ((text = (char *)malloc(n+1)) == NULL) */
        if ( !fNewMemory( (void *)&text, ( n + 1) ) )
        {
          printf("\n*** load_item : malloc failed ***\n");
          exit(1);
        }
        if (n) {
          if (fgetc(fp) != ' ') {
            printf("ERROR READING FIELD SEPARATOR FROM FILE\n");
            return (time_t)NULL;
          }

	  if (!fgets(text, n+1, fp)) {
	    printf("ERROR READING EVENT DESCRIPTION TEXT FROM FILE\n");
	    return (time_t)NULL;
	  }

        }

	if (fscanf(fp, "\n") == EOF) {
	  printf("ITEM: ERROR READING EVENT TERMINATOR FROM FILE\n");
	  return (time_t)NULL;
	} 

	/*
        if (!fgets(text, n+1, fp)) {
          printf("ERROR READING EVENT DESCRIPTION TEXT FROM FILE\n");
          return (time_t)NULL;
        }
        FreeMemory( text );
        if (fgetc(fp) != '\n') {
          printf("ERROR READING EVENT TERMINATOR FROM FILE\n");
          return (time_t)NULL;
        }
	*/
      }
  
    }
    fclose(fp);
  }
 
  return date;
 
} /* youngest_fta */

/*---------------------------------------------------------------
 Routine : file_get_mcs
 Purpose : Read cut sets from file.
 
 fta_file is the pathname of the file in which the Tree is saved
 and must not be NULL
 
 This routine attempts to get the cut sets from the file 
 {Tree name}.mcs
 
 If no cut sets file exists, or the mcs file is older than the tree
 file, the returned parameters are all set to Null values.

 see if ".fta" file exists, and get date stamp
 see if ".mcs" file exists, and get date stamp
 IF ".mcs" file more recent than ".fta" file THEN
    read the cut sets
 ENDIF
---------------------------------------------------------------*/
void
  file_get_mcs(
    char   *fta_file,     /* IN  - name of tree file          */
    char  **mcs_file,     /* OUT - name of file               */
    int    *num_mcs,      /* OUT - number of cut sets in file */
    int    *order,        /* OUT - order of cut sets in file  */
    time_t *mcs_date,     /* OUT - date stamp of file         */
    Expr   *e)            /* OUT - cut sets found             */
{
    FILE *fp;
    time_t fta_date;

 /* check if .fta file exists and get date stamp */
    if (  (fta_date = youngest_fta(fta_file)) == 0 ) {
      printf("file_get_mcs: file %s does not exist\n", fta_file);
      *mcs_file = NULL; 
      *mcs_date = (time_t)NULL; 
      *e = NULL; 
      *order = 0;
      return;
    }
 /* printf("time last modified : %s\n", ctime( &fta_date )); */

 /* generate .mcs file name */
    *mcs_file = generate_filename(fta_file, MCS_SUFFIX );

 /* check if .mcs file exists and get date stamp */
    if ( (*mcs_date = date_of(*mcs_file)) == 0 ) {
/*       printf("file_get_mcs: file %s does not exist\n", *mcs_file); */
      strfree(*mcs_file);
      *mcs_file = NULL; 
      *mcs_date = (time_t)NULL; 
      *e = NULL; 
      *num_mcs = 0, 
      *order = 0;
      return;
    }
 /* printf("time last modified : %s\n", ctime( &fta_date )); */

 /* if date stamps O.K. */
    if ( *mcs_date > fta_date ) {

     /* read order and cut sets */
      fp = fopen(*mcs_file, "r");
      if (fscanf(fp, "%d", order) == 1 ) {

        printf("Order = %d\n", *order);


        /*fgetc(fp);*/    /* swallow one '\n'. Note that
                       * fscanf(fp, "%d\n", order) does not work, because
                       * '\n' is whitespace. It swallows an indefinite
                       * number of '\n's looking for a '\n' !
                       */
        fscanf(fp, "\n");

        if ( (*e = ExprRead(fp)) != NULL ) {
          *num_mcs = ExprCount(*e);
/*           ExprPrint(*e); */
          fclose(fp);
          return;
        }
      }
      fclose(fp);
    }
    strfree(*mcs_file);
    *mcs_file = NULL; 
    *mcs_date = (time_t)NULL; 
    *e = NULL; 
    *num_mcs = 0, 
    *order = 0;
} /* file_get_mcs */



/*---------------------------------------------------------------
 Routine : file_put_mcs
 Purpose : Save mcs to file (tree.fta -> tree.mcs)
 
 fta_file is the pathname of the file in which the Tree is saved
 and must not be NULL
 
 This routine attempts to save the minimum cut sets to the file
 {Tree name}.mcs
---------------------------------------------------------------*/
void
  file_put_mcs(
    char *fta_file,   /* IN  - name of tree file  */
    Expr  e,          /* IN  - cut sets           */
    int order)        /* IN  - order              */
{
    FILE *fp;
    char *mcs_file;

 /* generate .mcs file name */
    mcs_file = generate_filename(fta_file, MCS_SUFFIX );

 /* save cut sets */
    if ( (fp = fopen(mcs_file, "w")) == NULL ) {
      printf("file_put_mcs: failed to save cut sets\n");
      strfree( mcs_file );
      return;
    }
    strfree( mcs_file );
    fprintf(fp, "%d\n", order);
    ExprWrite(fp, e);
    fclose(fp);

} /* file_put_mcs */
 
/*---------------------------------------------------------------
 Routine : mcs_print
 Purpose : Print the minimum cut set information to the supplied
 file.
---------------------------------------------------------------*/
void
  mcs_print( FILE *fp, Expr e, int max_order)
{
    Group *g = e;
    int order;
/*     int *nmcs = (int *)malloc( (max_order+1) * sizeof(int) ); */
    int *nmcs;
 
    if ( !fNewMemory( (void *)&nmcs, ( (max_order+1) * sizeof(int) ) ) )
    {
      exit( 1 );
    }

 /* print cut sets */
    for( order = 1; order <= max_order; order++ ) {
        nmcs[order] = 0;
        fprintf(fp, "\nOrder %d:\n", order);
        while ( GroupOrder( g ) == order && !GroupSentinel(g) ) {
            char **p = (char **)BitPara(g->b, 50);
            int i = 0;
 
            fprintf(fp, "  %3d) %-50s\n", ++(nmcs[order]), p[i++]);
            while (p[i] != NULL) {
                fprintf(fp, "       %-50s\n", p[i++]);
            }
            ParaDestroy( p );
            g = g->next;
        }
    }
 
 /* print qualitative importance analysis */
    fprintf(fp, "\n\nQualitative Importance Analysis:\n\n");
    fprintf(fp, "Order        Number\n");
    fprintf(fp, "-----        ------\n");
 
    nmcs[0] = 0;
    for( order = 1; order <= max_order; order++ ) {
        fprintf(fp, "%4d           %d\n", order, nmcs[order]);
        nmcs[0] += nmcs[order];
    }
    fprintf(fp, "  ALL          %d\n\n", nmcs[0]);
    FreeMemory(nmcs);

} /* mcs_print */
