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
 Module : MonteCarloSimulation
 Author : FSC Limited
 Date   : 24 July 1996
 Origin : FTA project,
          FSC Limited Private Venture.
 
 SccsId : @(#)MonteCarloSimulation.h	1.1 08/22/96
 
 Implement the Monte-Carlo analysis.
*****************************************************************/

#ifndef MONTE_CARLO_H
#define MONTE_CARLO_H

#include "treeP.h"
#include "boolean.h"
/*---------------------------------------------------------------
 Routine : generate_monte_carlo_simulation
 Purpose : Do a monte-carlo simulation to obtain an estimate of
 1) top event probability
 2) individual failure mode probabilities
---------------------------------------------------------------*/
extern BOOL 
  generate_monte_carlo_simulation(
    BOOL testing,
    time_t *test_time,
    char *filename,
    TREE *tree,
    int   monte_carlo_n,
    float unit_time );

/*---------------------------------------------------------------
 Routine : monte_estimate
 Purpose : Estimate time for a monte-carlo simulation
---------------------------------------------------------------*/
extern float                        /* RET - time (seconds)       */
  monte_estimate(
    TREE *tree,   /* IN  - tree                 */
    long int n);  /* IN  - number of runs       */

#endif

