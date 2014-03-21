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

#include <stdlib.h>
#include <time.h>

#include "BaseTimeEstimate.h"

float StartTime;

float
  ReferenceEstimate( void )
{
  unsigned int i;
  unsigned int j;
  clock_t StartTime;
  clock_t EndTime;
  float TimeTaken = 0.0;
#define StructureLimit 100
  int Structure[ StructureLimit ][ StructureLimit ];
 
  StartTime = clock();
  for ( i = 0; i < StructureLimit; i++ )
    for ( j = 0; j < StructureLimit; j++ )
    {
      Structure[ i ][ j ] = rand();
    }
 
  EndTime = ( clock() - StartTime );
  if(EndTime == 0) {
	    /* don't allow time estimate to fall below minimum positive value */
		EndTime = 1;
  }	
  TimeTaken = (float)EndTime / (float)CLOCKS_PER_SEC;
 
  return TimeTaken;

} /* BaseTimeEstimate */

void
  StartTimingPeriod( void )
{
  StartTime = (float)( clock() / CLOCKS_PER_SEC );

} /* StartTimingPeriod */

float
  TimeElapsed( void )
{

  return (float)( clock() / CLOCKS_PER_SEC ) - StartTime;

} /* TimeElapsed */
