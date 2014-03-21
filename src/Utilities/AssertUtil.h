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
 Module : Assert
 Author : FSC Limited
 Date   : 5/8/96
 
 SccsId : @(#)Assert.h	1.2 09/26/96
 
 Origin : FTA project,
          FSC Limited Private Venture, Level 2 Prototype.
 
Assert macro defined in Writing Solid Code, p17 
Usage: ASSERT( Boolean_Expresion ); 

***************************************************************/
#ifndef HAVE_ASSERT_H
#define HAVE_ASSERT_H

#ifdef DEBUG

  void _Assert( char *, unsigned); /* prototype */

  #define ASSERTFILE( str ) \
    static char strAssertFile[] = str;

  #define ASSERT( f )   \
    if ( f )            \
      {}                \
    else                \
      _Assert( strAssertFile, __LINE__ )

#else

  #define ASSERTFILE( str )
  #define ASSERT( f )

#endif

#ifdef DEBUG
 
  void _AssertMsg( char *strMessage ); /* prototype */
 
  #define ASSERTMSG( f, str ) \
    if ( f )            \
      {}                \
    else                \
      _AssertMsg( str ) 

#else
 
  #define ASSERTMSG( f, str ) 

#endif

#endif /* HAVE_ASSERT_H */

