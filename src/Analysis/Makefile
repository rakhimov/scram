###############################################################################
## 
## OpenFTA - Fault Tree Analysis
## Copyright (C) 2005 FSC Limited
## 
## This program is free software; you can redistribute it and#or modify it
## under the terms of the GNU General Public License as published by the Free
## Software Foundation; either version 2 of the License, or (at your 
## option) any later version.
## 
## This program is distributed in the hope that it will be useful, but WITHOUT
## ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
## FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for 
## more details.
##
## You should have received a copy of the GNU General Public License along 
## with this program; if not, write to the Free Software Foundation, Inc., 
## 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
##
## To contact FSC, please send an email to support@fsc.co.uk or write to 
## FSC Ltd., Cardiff Business Technology Centre, Senghenydd Road, Cardiff,
## CF24 4AY.
##
###############################################################################

#################################################################################
## Module:
##      Formal-FTA Makefile
##
## SccsId:
##      @(#)Makefile	1.4,    97/07/02, 16:59:05
##
## Origin:
##      FSC FTA product
##
## Author:
##      Formal Software Construction Ltd.
##
################################################################################

# Include general purpose project Makefiles
include ../../master.Makefile


OBJS = \
BaseTimeEstimate.o \
FailureStatistics.o \
CutSetsByAlgebraicMethod.o \
MonteCarloSimulation.o \
NormalisedBooleanExpressions.o \
CutSetsCommon.o \
ProbabilityAnalysis.o \
CutSetsByComboMethod.o

LIBNAME = $(BUILDIR)/libanalysis.a

# External dependencies
#
include ${TOP}/Appn/Exports
include ${TOP}/Utilities/Exports


# Internal dependencies
#
include ${TOP}/Analysis/Exports
BASE_TIME_EST_H = BaseTimeEstimate.h
FAIL_H          = FailureStatistics.h $(BITS_H)

# Define targets
# Define in order to be presented in MakeTool
#
all:    $(OBJS)
	if ( test ! -d $(BUILDIR) ) then \
          mkdir $(BUILDIR); \
        fi
	$(AR) $(ARFLAGS) $(LIBNAME) $(OBJS)

clean:
	-/bin/rm -r $(OBJS) $(LIBNAME) $(BUILDIR) core


veryclean:
	make clean


spotless:
	make veryclean
	-/bin/rm -r *% *~ *.old


# Define dependencies
#
BaseTimeEstimate.o:		$(BASE_TIME_EST_H)
CutSetsByAlgebraicMethod.o: 	$(ALGEBRAIC_H) $(EXPR_H) $(ITEM_H) $(FTA_H) \
  $(EVENT_LIST_H) $(CUT_SETS_H) $(WORKDIAL_H)
CutSetsByComboMethod.o:         $(COMBO_H) $(EVENT_LIST_H) $(PERM_H) $(FTA_H) \
  $(CUT_SETS_H) $(RAND_H) $(WORKDIAL_H) $(BASE_TIME_EST_H) $(MEMORY_H)
CutSetsCommon.o:     		$(CUT_SETS_H) $(UTIL_H) $(FILE_UTILS_H) \
  $(FILE_DIALOGS_H)$(FTA_H) $(TREE_H) $(EVENT_LIST_H) $(MEMORY_H)
FailureStatistics.o:         	$(FAIL_H) $(MEMORY_H)
MonteCarloSimulation.o:  	$(MONTE_CARLO_H) $(COMBO_H) $(BASIC_H) \
  $(FTA_H) $(EVENT_LIST_H) $(UTIL_H) $(TREE_H) $(BITS_H) $(FAIL_H) $(RAND_H) \
  $(WORKDIAL_H) $(BASE_TIME_EST_H) $(MEMORY_H) $(ASSERT_H)
NormalisedBooleanExpressions.o: $(EXPR_H) $(PERM_H) $(BOOLEAN_H) $(MEMORY_H)
ProbabilityAnalysis.o:        	$(PROBS_H) $(PERM_H) $(BASIC_H) \
  $(EVENT_LIST_H) $(UTIL_H) $(BITS_H) $(EXPR_H) $(FTA_H) $(WORKDIAL_H) \
  $(BASE_TIME_EST_H) $(MEMORY_H)


