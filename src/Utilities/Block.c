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

/*----------------------------------------------------------------------
 *  Debugging support routines for the memory manager.
 *  These routines maintain the debugging information
 *  using a simple linked list.
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

#include "MyTypes.h"
#include "AssertUtil.h"
#include "boolean.h"

#include "Block.h"

ASSERTFILE(__FILE__) 

#ifdef DEBUG

static int numBlocks = 0;
	
static blockinfo *pbiGetBlockInfo(byte *pbToFind);


/*----------------------------------------------------------------------
 *  The functions in this file must compare arbitrary pointers, an
 *  operation that the ANSI standard does not guarantee to be
 *  portable.
 *
 *  The macros below isolate the pointer comparisons needed in this
 *  file.  The implementations assume "flat" pointers where
 *  straightforward comparisons will always work.  The definitions
 *  below will *not* work for some of the common 80x86 memory models.
 */

#define fPtrLess(pLeft, pRight)    ((pLeft) <  (pRight))
#define fPtrGrtr(pLeft, pRight)    ((pLeft) >  (pRight))
#define fPtrEqual(pLeft, pRight)   ((pLeft) == (pRight))
#define fPtrLessEq(pLeft, pRight)  ((pLeft) <= (pRight))
#define fPtrGrtrEq(pLeft, pRight)  ((pLeft) >= (pRight))



/*----------------------------------------------------------------------
 *  bGarbage is a constant used to fill memory whenever
 *  the routines in this file create garbage.
 */

#ifndef bGarbage
	#define bGarbage 0xA3
#endif



/*--------------------------------------------------------------------*/
/*           * * * * *  Private data/functions  * * * * *             */
/*--------------------------------------------------------------------*/



/*----------------------------------------------------------------------
 *  pbiHead points to a singly-linked list of blockinfo structures
 *  that comprise the memory log.
 */

static blockinfo *pbiHead = NULL;



/*----------------------------------------------------------------------
 *  pbiGetBlockInfo(pb)
 *
 *  pbiGetBlockInfo searches the memory log to find the block
 *  that pb points into, and returns pointer to the corresponding
 *  blockinfo structure of the memory log.  Note: pb *must* point
 *  into an allocated block or you will get an assertion failure; the
 *  function either asserts or succeeds--it never returns an error.
 *
 *      blockinfo *pbi;
 *      ...
 *      pbi = pbiGetBlockInfo(pb);
 *      //  pbi->pb points to the start of pb's block
 *      //  pbi->size is the size of the block that pb points into
 */

static blockinfo *pbiGetBlockInfo(byte *pb)
{
	blockinfo *pbi;

	for (pbi = pbiHead; pbi != NULL; pbi = pbi->pbiNext)
	{
		byte *pbStart = pbi->pb;                    /* For readability */
		byte *pbEnd   = pbi->pb + pbi->size - 1;
		
		if (fPtrGrtrEq(pb, pbStart)  &&  fPtrLessEq(pb, pbEnd))
			break;
	}

	/*  Couldn't find pointer?  Is it  (a) garbage?  (b) pointing to
	 *  a block that was freed?  or (c) pointing to a block that
	 *  moved when it was resized by fResizeMemory?
	 */
	ASSERT(pbi != NULL);
	
	return (pbi);
}



/*--------------------------------------------------------------------*/
/*              * * * * *  Public functions  * * * * *                */
/*--------------------------------------------------------------------*/


int numAllocatedBlocks() {

	return numBlocks;
}


/*----------------------------------------------------------------------
 *  fCreateBlockInfo(pbNew, sizeNew)
 *
 *  This function creates a log entry for the memory block
 *  defined by pbNew:sizeNew.  The function returns TRUE if it
 *  successfully creates the log information; FALSE otherwise.
 *
 *      if (fCreateBlockInfo(pbNew, sizeNew))
 *          success -- the memory log has an entry for pbNew:sizeNew
 *      else
 *          failure -- you should free pbNew since there isn't an entry
 */

flag fCreateBlockInfo(byte *pbNew, size_t sizeNew)
{
	blockinfo *pbi;
	
	ASSERT(pbNew != NULL  &&  sizeNew != 0);

	pbi = (blockinfo *)malloc(sizeof(blockinfo));
	if (pbi != NULL)
	{
		pbi->pb = pbNew;
		pbi->size = sizeNew;
		pbi->pbiNext = pbiHead;
		pbiHead = pbi;

		numBlocks++;
	}

	return (flag)(pbi != NULL);
}



/*----------------------------------------------------------------------
 *  FreeBlockInfo(pbToFree)
 *
 *  This function clears a log entry for the memory block that
 *  pbToFree points to.  pbToFree *must* point to the start of an
 *  allocated block or you will get an assertion failure.
 */

void FreeBlockInfo(byte *pbToFree)
{
	blockinfo *pbi, *pbiPrev;
	
	pbiPrev = NULL;
	for (pbi = pbiHead; pbi != NULL; pbi = pbi->pbiNext)
	{
		if (fPtrEqual(pbi->pb, pbToFree))
		{
			if (pbiPrev == NULL)
				pbiHead = pbi->pbiNext;
			else
				pbiPrev->pbiNext = pbi->pbiNext;
			break;
		}
		pbiPrev = pbi;
	}
	
	/* if pbi is NULL then pbToFree is invalid */
	ASSERT(pbi != NULL);
	
	/* Make sure that nothing has written off the end of the block. */
	ASSERT(*(pbi->pb + pbi->size) == bDebugByte);         /* EXERCISE */
	
	/* Destroy the contents of *pbi before freeing them. */
	memset(pbi, bGarbage, sizeof(blockinfo));
	
	free(pbi);
	
	numBlocks--;

}



/*----------------------------------------------------------------------
 *  UpdateBlockInfo(pbOld, pbNew, sizeNew)
 *
 *  UpdateBlockInfo looks up the log information for the memory
 *  block that pbOld points to.  The function then updates the log
 *  information to reflect that the block now lives at pbNew and
 *  is sizeNew bytes long.  pbOld *must* point to the start of
 *  an allocated block or you will get an assertion failure.
 */

void UpdateBlockInfo(byte *pbOld, byte *pbNew, size_t sizeNew)
{
	blockinfo *pbi;
	
	ASSERT(pbNew !=NULL  &&  sizeNew != 0);
	
	pbi = pbiGetBlockInfo(pbOld);
	ASSERT(pbOld == pbi->pb);        /* must point to start of block */
	
	pbi->pb = pbNew;
	pbi->size = sizeNew;
}



/*----------------------------------------------------------------------
 *  sizeofBlock(pb)
 *
 *  sizeofBlock returns the size of the block that pb points to.
 *  pb *must* point to the start of an allocated block or you will
 *  get an assertion failure.
 */

size_t sizeofBlock(byte *pb)
{
	blockinfo *pbi;

	pbi = pbiGetBlockInfo(pb);
	ASSERT(pb == pbi->pb);           /* must point to start of block */
	
	/* Make sure that nothing has written off the end of the block. */
	ASSERT(*(pbi->pb + pbi->size) == bDebugByte);         /* EXERCISE */
	
	return (pbi->size);
}



/*--------------------------------------------------------------------*/
/*          The following routines are used to find lost              */
/*          memory blocks and dangling pointers.  See                 */
/*          Chapter 3 for a discussion of these routines.             */
/*--------------------------------------------------------------------*/



/*----------------------------------------------------------------------
 *  ClearMemoryRefs(void)
 *
 *  ClearMemoryRefs marks all blocks in the memory log as being
 *  unreferenced.
 */

void ClearMemoryRefs(void)
{
	blockinfo *pbi;

	for (pbi = pbiHead; pbi != NULL; pbi = pbi->pbiNext)
		pbi->fReferenced = FALSE;
}



/*----------------------------------------------------------------------
 *  NoteMemoryRef(pv)
 *
 *  NoteMemoryRef marks the block that pv points into as being
 *  referenced.  Note: pv does *not* have to point to the start of
 *  a block; it may point anywhere within an allocated block.
 */

void NoteMemoryRef(void *pv)
{
	blockinfo *pbi;

	pbi = pbiGetBlockInfo((byte *)pv);
	
	/* Make sure that nothing has written off the end of the block. */
	ASSERT(*(pbi->pb + pbi->size) == bDebugByte);         /* EXERCISE */
	
	pbi->fReferenced = TRUE;
}



/*----------------------------------------------------------------------
 *  CheckMemoryRefs(void)
 *
 *  CheckMemoryRefs scans the memory log looking for blocks that
 *  have not been marked with a call to NoteMemoryRef.  If this
 *  function finds an unmarked block, it asserts.
 */

void CheckMemoryRefs(void)
{
	blockinfo *pbi;

	for (pbi = pbiHead; pbi != NULL; pbi = pbi->pbiNext)
	{
		/*  A simple check for block integrity.  If this
		 *  assert fires, it means something is wrong with
		 *  the debug code that manages blockinfo or,
		 *  possibly a wild memory store has trashed the
		 *  data structure.  Either way, there's a bug.
		 */
		ASSERT(pbi->pb != NULL  &&  pbi->size != 0);
	
		/* Make sure that nothing has written off the end of the block. */
		ASSERT(*(pbi->pb + pbi->size) == bDebugByte);         /* EXERCISE */

		/*  Check for lost/leaky memory.  If this assert
		 *  fires, it means the app has either lost track
		 *  of this block, or not all global pointers
		 *  have been accounted for with NoteMemoryRef.
		 */
		ASSERT(pbi->fReferenced);		
	}
}



/*----------------------------------------------------------------------
 *  fValidPointer(pv, size)
 *
 *  fValidPointer verifies that pv points into an allocated memory
 *  block and that there are at least "size" allocated bytes from
 *  pv to the end of the block.  If either condition is not met,
 *  fValidPointer will assert; the function will never return FALSE.
 *  The reason fValidPointer returns a flag at all (always TRUE) is
 *  to allow you to call the function within an ASSERT macro.  While
 *  this isn't the most efficient method to use, using the ASSERT
 *  neatly handles the debug/ship version control without resorting
 *  to #ifdef DEBUG's or introducing other ASSERT-like macros.
 *
 *      ASSERT(fValidPointer(pb, size));
 */

flag fValidPointer(void *pv, size_t size)
{
	blockinfo *pbi;
	byte *pb = (byte *)pv;

	ASSERT(pv != NULL  &&  size != 0);
	
	pbi = pbiGetBlockInfo(pb);                   /* This validates pv */
	
	/* pv is valid, but is size? (not if pb+size overflows the block) */
	ASSERT(fPtrLessEq(pb + size, pbi->pb + pbi->size));
	
	/* Make sure that nothing has written off the end of the block. */
	ASSERT(*(pbi->pb + pbi->size) == bDebugByte);         /* EXERCISE */

	return (TRUE);
}



/*--------------------------------------------------------------------*/
/*          The following routines implement answers to the           */
/*          exercises in chapter 3.                                   */
/*--------------------------------------------------------------------*/



/*----------------------------------------------------------------------
 *  ClearMemoryRefsEx(void)
 *
 *  ClearMemoryRefsEx marks all blocks in the memory log as being
 *  unreferenced.
 */

void ClearMemoryRefsEx(void)
{
	blockinfo *pbi;

	for (pbi = pbiHead; pbi != NULL; pbi = pbi->pbiNext)
	{
		pbi->nReferenced = 0;
		pbi->tag = tagNone;
	}
}



/*----------------------------------------------------------------------
 *  NoteMemoryRefEx(pv)
 *
 *  NoteMemoryRefEx marks the block that pv points into as being
 *  referenced.  Note: pv does *not* have to point to the start of
 *  a block; it may point anywhere within an allocated block.
 */

void NoteMemoryRefEx(void *pv, blocktag tag)
{
	blockinfo *pbi;

	pbi = pbiGetBlockInfo((byte *)pv);
	
	/* Make sure that nothing has written off the end of the block. */
	ASSERT(*(pbi->pb + pbi->size) == bDebugByte);         /* EXERCISE */
		
	pbi->nReferenced++;
	
	ASSERT(pbi->tag == tagNone  ||  pbi->tag == tag);
	pbi->tag = tag;
}



/*----------------------------------------------------------------------
 *  CheckMemoryRefsEx(void)
 *
 *  CheckMemoryRefsEx scans the memory log looking for blocks that
 *  have not been marked with a call to NoteMemoryRefEx.  If this
 *  function finds an unmarked block, it asserts.
 */

void CheckMemoryRefsEx(void)
{
	blockinfo *pbi;

	for (pbi = pbiHead; pbi != NULL; pbi = pbi->pbiNext)
	{
		/*  A simple check for block integrity.  If this
		 *  assert fires, it means something is wrong with
		 *  the debug code that manages blockinfo or,
		 *  possibly a wild memory store has trashed the
		 *  data structure.  Either way, there's a bug.
		 */
		ASSERT(pbi->pb != NULL  &&  pbi->size != 0);
	
		/* Make sure that nothing has written off the end of the block. */
		ASSERT(*(pbi->pb + pbi->size) == bDebugByte);         /* EXERCISE */

		
		/*  Check for lost/leaky memory.  If there are no
		 *  references at all, it means the app has either
		 *  lost track of this block, or not all global
		 *  pointers are being accounted for with NoteMemoryRef.
		 *  Some types of blocks may have more than one
		 *  reference to them.
		 */
		switch (pbi->tag)
		{	
		default:
			ASSERT(FALSE);           /* Untagged block--it should be tagged. */
			break;
		
		case tagNone:
		case tagSymName:
			ASSERT(pbi->nReferenced == 1);
			break;
			
		case tagListNode:
			ASSERT(pbi->nReferenced == 2);
			break;

		case tagSymStruct:
			ASSERT(pbi->nReferenced == 3);
			break;
			
		}
	}
}





/*----------------------------------------------------------------------
 *  NoteMemoryRefRange(pv, size)
 *
 *  NoteMemoryRefRange marks the block that pv points into as being
 *  referenced.  Note: pv does *not* have to point to the start of a
 *  block; it may point anywhere within an allocated block, but there
 *  must be at least "size" bytes left in the block.  Note: if
 *  possible, use NoteMemoryBlock--it provides stronger validation.
 */

void NoteMemoryRefRange(void *pv, size_t size)
{
	blockinfo *pbi;
	byte *pb = (byte *)pv;

	pbi = pbiGetBlockInfo((byte *)pv);
	
	/* pv is valid, but is size? (not if pb+size overflows the block) */
	ASSERT(fPtrLessEq(pb + size, pbi->pb + pbi->size));
	
	/* Make sure that nothing has written off the end of the block. */
	ASSERT(*(pbi->pb + pbi->size) == bDebugByte);         /* EXERCISE */
	
	/* Note that the block has been referenced.  */
	pbi->fReferenced = TRUE;
}




/*----------------------------------------------------------------------
 *  NoteMemoryBlock(pv, size)
 *
 *  NoteMemoryBlock marks the block that pv points into as being
 *  referenced.  Note: pv *must* point to the start of the allocated
 *  block and the allocated block *must* be exactly "size" bytes long
 *  or you will get an assertion failure.
 */

void NoteMemoryBlock(void *pv, size_t size)
{
	blockinfo *pbi;
	byte *pb = (byte *)pv;

	pbi = pbiGetBlockInfo(pb);
	ASSERT(pb == pbi->pb);           /* must point to start of block */
	ASSERT(size  ==  pbi->size);     /* must be the same size */
	
	/* Make sure that nothing has written off the end of the block. */
	ASSERT(*(pbi->pb + pbi->size) == bDebugByte);         /* EXERCISE */
	
	pbi->fReferenced = TRUE;
}



/*---------------------------------------------------------------------*/
/*                Simulated resource error conditions                  */
/*                                                                     */
/*  These routines are described on pages 228 through 231 of the book. */
/*---------------------------------------------------------------------*/



void SetFailures(
  failureinfo *pfi, 
  unsigned nSucceed, 
  unsigned nFail)
{
	/* if nFail is 0, require that nSucceed be UINT_MAX */
	ASSERT(nFail != 0  ||  nSucceed == UINT_MAX);

	pfi->nSucceed = nSucceed;
	pfi->nFail    = nFail;
	pfi->nTries   = 0;
	pfi->lock     = 0;
}


void EnableFailures(failureinfo *pfi)
{
	ASSERT(pfi->lock > 0);
	pfi->lock--;
}


void DisableFailures(failureinfo *pfi)
{
	ASSERT(pfi->lock >= 0  &&  pfi->lock < INT_MAX);
	pfi->lock++;
}


flag fFakeFailure(failureinfo *pfi)
{	
	ASSERT(pfi != NULL);
	
	if (pfi->lock > 0)
		return (FALSE);
	
	if (pfi->nTries != UINT_MAX)     /* Don't let nTries overflow */
		pfi->nTries++;
	
	if (pfi->nTries <= pfi->nSucceed)
		return (FALSE);
	
	if (pfi->nTries - pfi->nSucceed <= pfi->nFail)
		return (TRUE);

	return (FALSE);
}





#endif /* DEBUG */

