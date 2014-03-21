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

#ifndef HAVE_BLOCK_H
#define HAVE_BLOCK_H

#ifdef DEBUG

#include <stddef.h> /* for size_t */

/*  EXERCISE extension:  see page 226
 *
 *  blocktag is a list of all of the types of allocated
 *  blocks maintained by the application.
 */

typedef enum
{
	tagNone,           /* ClearMemoryRefs sets all blocks to tagNone. */
	tagSymName,
	tagSymStruct,
	tagListNode,       /* These blocks must have two references. */
	
	tagLast
} blocktag;



/*----------------------------------------------------------------------
 *  blockinfo is a structure that contains the memory log
 *  information for one allocated memory block.  Every allocated
 *  memory block has a corresponding blockinfo structure in
 *  the memory log.
 */

typedef struct BLOCKINFO
{
    struct BLOCKINFO *pbiNext;
    byte    *pb;                /*  Start of block      */
    size_t  size;               /*  Length of block     */
    flag    fReferenced;        /*  Ever referenced?    */

    /* EXERCISE FIELDS BELOW */
    unsigned nReferenced;       /*  How many references */
    blocktag tag;		/*  What type of block? */

} blockinfo;                    /*  Naming:  bi, *pbi.  */


flag fCreateBlockInfo(byte *pbNew, size_t sizeNew);
void FreeBlockInfo(byte *pbToFree);
void UpdateBlockInfo(byte *pbOld, byte *pbNew, size_t sizeNew);
size_t sizeofBlock(byte *pb);


void ClearMemoryRefs(void);
void NoteMemoryRef(void *pv);
void CheckMemoryRefs(void);
flag fValidPointer(void *pv, size_t size);



/*----------------------------------------------------------------------*/

/*  The functions prototypes below are for the modified functions
//  that implement EXERCISEs 4 and 5 on pages 225 and 226.
*/

void ClearMemoryRefsEx(void);
void NoteMemoryRefEx(void *pv, blocktag tag);
void CheckMemoryRefsEx(void);


/*  NoteMemoryRefRange(pv, size)
 *
 *  NoteMemoryRefRange marks the block that pv points into as being
 *  referenced.  Note: pv does *not* have to point to the start of
 *  a block; it may point anywhere within an allocated block, but
 *  it must point to "size" allocated bytes.  Note: if possible,
 *  use NoteMemoryBlock--it provides stronger validation
 */
void NoteMemoryRefRange(void *pv, size_t size);


/*  NoteMemoryBlock(pv, size)
 *
 *  NoteMemoryBlock marks the block that pv points into as being
 *  referenced.  Note: pv *must* point to the start of the allocated
 *  block and the allocated block *must* be exactly "size" bytes long
 *  or you will get an assertion failure.
 */
void NoteMemoryBlock(void *pv, size_t size);



/*----------------------------------------------------------------------*/

/*  The structure and functions prototypes below implement
//  EXERCISE 6 described on pages 228 throught 231.
*/

/* Simulated error stuff */

typedef struct
{
	unsigned nSucceed;    /* # of times to succeed before failing */
	unsigned nFail;       /* # of times to fail */
	unsigned nTries;      /* # of times already called  */
	int      lock;        /* if lock is > 0, disable mechanism */
} failureinfo;


void SetFailures(failureinfo *pfi, unsigned nSucceed, unsigned nFail);
void EnableFailures(failureinfo *pfi);
void DisableFailures(failureinfo *pfi);
flag fFakeFailure(failureinfo *pfi);

extern failureinfo fiMemory;

#endif



/*----------------------------------------------------------------------*/

/*  EXERCISE 2 on pages 222 through 224.
 *
 *  bDebugByte is a magic value that is stored at the
 *  tail of every allocated memory block in DEBUG
 *  versions of the program.  sizeofDebugByte is added
 *  to the sizes passed to malloc and realloc so that
 *  the correct amount of space is allocated.
 */

#define bDebugByte 0xE1

#ifdef DEBUG
	#define sizeofDebugByte 1
#else
	#define sizeofDebugByte 0
#endif

#endif /* HAVE_BLOCK_H */
