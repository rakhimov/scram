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
 
 SccsId : @(#)StringUtilities.h	1.1 08/22/96
 
 Origin : FTA project,
          FSC Limited Private Venture, Level 2 Prototype.
 
This module provides a C implementation of the set of String 
Utilities as specified in Software Components in Ada by Booch.

***************************************************************/

#ifndef StrUtil
#define StrUtil

#include "boolean.h"

extern void
  SU_Make_Uppercase(
    char *The_String );

extern void
  SU_Make_Lowercase(
    char *The_String );

extern void
  SU_Capitalize(
    char *The_String );

extern void
  SU_Uncapitalize(
    char *The_String );

extern void
  SU_Replace(
    char The_Character,
    char With_The_Character,
    char *In_The_String,
    BOOL Case_Sensitive );

extern char *
  SU_Uppercase(
    char *The_String );

extern char *
  SU_Lowercase(
    char *The_String );
 
extern char *
  SU_Capitalized(
    char *The_String );
 
extern char *
  SU_Uncapitalized(
    char *The_String );
 
extern char *
  SU_Replaced(
    char The_Character,
    char With_The_Character,
    char *In_The_String,
    BOOL Case_Sensitive );

extern BOOL
  SU_Is_Null(
    char *The_String );

extern BOOL
  SU_Is_Control(
    char *The_String );
 
extern BOOL
  SU_Is_Graphic(
    char *The_String );
 
extern BOOL
  SU_Is_Uppercase(
    char *The_String );
 
extern BOOL
  SU_Is_Lowercase(
    char *The_String );
 
extern BOOL
  SU_Is_Digit(
    char *The_String );
 
extern BOOL
  SU_Is_Alphabetic(
    char *The_String );
 
extern BOOL
  SU_Is_Alphanumeric(
    char *The_String );
 
extern BOOL
  SU_Is_Special(
    char *The_String );
 
extern char *
  SU_Centered(
    char *The_String,
    int  In_The_Width,
    char With_The_Filler );
  
extern char *
  SU_Left_Justified(
    char *The_String,
    int  In_The_Width,
    char With_The_Filler );

extern char *
  SU_Right_Justified(
    char *The_String,
    int  In_The_Width,
    char With_The_Filler );

extern char *
  SU_Stripped(
    char The_Character,
    char *From_The_String,
    BOOL Case_Sensitive );

extern char *
  SU_Stripped_Leading(
    char The_Character,
    char *From_The_String,
    BOOL Case_Sensitive );

extern char *
  SU_Stripped_Trailing(
    char The_Character,
    char *From_The_String,
    BOOL Case_Sensitive );

extern int
  SU_Number_Of_Char(
    char The_Character,
    char *In_The_String,
    BOOL Case_Sensitive );

extern int
  SU_Number_Of_Str(
    char *The_String,
    char *In_The_String,
    BOOL Case_Sensitive );
 
extern int
  SU_Location_Of(
    char The_Character,
    char *In_The_String,
    BOOL Case_Sensitive,
    BOOL Forward );

extern BOOL
  SU_Is_Equal(
    char *Left,
    char *Right,
    BOOL Case_Sensitive );

extern BOOL
  SU_Is_Less_Than(
    char *Left,
    char *Right,
    BOOL Case_Sensitive );

extern BOOL
  SU_Is_Greater_Than(
    char *Left,
    char *Right,
    BOOL Case_Sensitive );

extern int
  SU_Num_Same_Chars(
    char *string1,
    char *string2 );

char *
  SU_Join(
    char *string1,
    char *string2 );

char *
  SU_Copy(
    char *string );


#endif
