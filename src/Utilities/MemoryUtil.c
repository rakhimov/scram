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

#include <stdio.h>

#include <stdlib.h>
#include <string.h> 

#include "MemoryUtil.h"
#include "MyTypes.h"
#include "Block.h"

#include "AssertUtil.h"

ASSERTFILE(__FILE__)

#define bGarbage 0xA3

/* fNewMemory -- allocate a memory block. */
flag fNewMemory( void **ppv, size_t size )
{
  byte **ppb = (byte **)ppv;

  ASSERT( ppv != NULL && size != 0 );

  *ppb = (byte *)malloc( size + sizeofDebugByte );

  #ifdef DEBUG
  {
    if ( *ppb != NULL )
    {
      *( *ppb + size ) = bDebugByte;

      memset( *ppb, bGarbage, size );

      /* If unable to create the block information,
         fake a total memory failure.
      */
      if ( !fCreateBlockInfo( *ppb, size ) )
      {
        free( *ppb );
        *ppb = NULL;
      }
    }
  }
  #endif /* DEBUG */

  return (*ppb != NULL ); /* Success? */

} /* fNewMemory */

void FreeMemory( void *pv )
{
  ASSERT( pv != NULL );

  #ifdef DEBUG
  {
    memset( pv, bGarbage, sizeofBlock( (byte*)pv ) );
    FreeBlockInfo( (byte*)pv );
  }
  #endif /* DEBUG */

  free( pv );

} /* FreeMemory */

flag fResizeMemory( void **ppv, size_t sizeNew )
{
  byte **ppb = (byte **)ppv;
  byte *pbNew;
  #ifdef DEBUG
    size_t sizeOld;
  #endif /* DEBUG */

  ASSERT( ppb != NULL && sizeNew != 0 );

  #ifdef DEBUG
  {
    sizeOld = sizeofBlock( *ppb );

    /* If the block is shrinking, pre-file the soon-to-be-
       released memory. If the block is expanding, force
       it to move (instead of expanding in place) by faking
       a realloc. If the block is the same size, don't do
       anything.
    */
    if ( sizeNew < sizeOld )
      memset( ( *ppb ) + sizeNew, bGarbage, sizeOld - sizeNew );
    else if ( sizeNew > sizeOld )
    {
      byte *pbForceNew;

      if( fNewMemory( (void **)&pbForceNew, sizeNew ) )
      {
        memcpy( pbForceNew, *ppb, sizeOld );
        FreeMemory( *ppb );
        *ppb = pbForceNew;
      }
    }
  }
  #endif /* DEBUG */

  pbNew = (byte *)realloc( *ppb, sizeNew + sizeofDebugByte );
  if ( pbNew != NULL )
  {
    #ifdef DEBUG
    {
      *( pbNew + sizeNew ) = bDebugByte;

      UpdateBlockInfo( *ppb, pbNew, sizeNew );

      /* If expanding, initialize the new tail. */
      if ( sizeNew > sizeOld )
        memset( pbNew + sizeOld, bGarbage, sizeNew - sizeOld );
    }
    #endif /* DEBUG */

    *ppb = pbNew;
  }

  return ( pbNew != NULL );

} /* fResizeMemory */

