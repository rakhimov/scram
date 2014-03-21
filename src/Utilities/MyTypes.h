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

/*
//	Writing Solid Code
//	Appendix B
//
//	Memory Logging Routines
//
//
//	The code in this appendix implements a simple linked-list
//	version of the memory logging routines that are discussed in
//	Chapter 3.  The code is intentionally simple so that it can
//	be easily understood -- it is not meant to be used in any
//	application that makes heavy use of the memory manager.  But
//	before you spend time rewriting the routines to use an AVL-tree,
//	a B-tree, or any other data structure that provides fast
//	searches, first try the code to verify that it is indeed too
//	slow for practical use in your application.  You may find that
//	the code works well for you as is, particularly if you don't
//	maintain many globally allocated memory blocks.
//
//	The implementation in this file is straightforward:  For every
//	allocated memory block, these routines allocate an extra bit of
//	memory to hold a "blockinfo" structure that contains the log
//	information.  See its definition below.  When a new "blockinfo"
//	structure is created, it is filled in and placed at the head
//	of the linked-list structure -- there is no attempt to maintain
//	any particular ordering for the list.  Again, this implementation
//	was chosen because it is simple and easy to understand.
//
//
//	Note:  This code differs slightly from what appears in the
//	book -- the code includes many of the modifications suggested
//	in the book's exercises.  These changes are highlighted with the
//	comment "EXERCISE" or the notation "Ex."
//
*/


#ifndef MyTypes
#define MyTypes

/*----------------------------  File: MyTypes.h ----------------------------*/


/*----------------------------------------------------
//  byte is the smallest unsigned unit of storage.
//
//  Naming convention:  b
*/
typedef unsigned char byte;


/*----------------------------------------------------
//  "flag" is a logical true/false value that is either
//  0 (false) or non-zero (true).
//
//  Example:  if (f)      or     if (!f)
//               ...                ...
//
//  Note:  DON'T compare flags to constants or other
//  flags.  Always use the logical operators:  !, &&, ||.
//
//  Naming convention:  f
*/

typedef unsigned char flag;

#endif 
