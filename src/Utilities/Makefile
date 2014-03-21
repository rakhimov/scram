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
##      @(#)Makefile	1.3,    97/07/02, 17:01:47
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
AssertUtil.o \
StringUtilities.o \
Block.o \
RandomNumbers.o \
list_suu.o \
FileUtilities.o \
Record_Io.o \
list_suu_utilities.o \
MemoryUtil.o \
StatisticalMaths.o


LIBNAME = $(BUILDIR)/libutilities.a


# External dependencies
#
include $(TOP)/Appn/Exports


# Internal dependencies
#
include $(TOP)/Utilities/Exports
BLOCK_H = ${TOP}/Utilities/Block.h
MY_TYPES_H = ${TOP}/Utilities/MyTypes.h


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
AssertUtil.o:             $(ASSERT_H)
Block.o:              $(BLOCK_H) $(MY_TYPES_H) $(ASSERT_H) $(BOOLEAN_H)
FileUtilities.o:      $(FILE_UTILS_H) $(UTIL_H) $(ASSERT_H) $(STR_UTILS_H) \
  $(MEMORY_H)
MemoryUtil.o:             $(MEMORY_H) $(BLOCK_H) $(MY_TYPES_H) $(ASSERT_H)
list_suu.o:           $(LIST_SUU_H) $(MEMORY_H) $(ASSERT_H)
list_suu_utilities.o: $(LIST_SUU_UTIL_H) $(ASSERT_H)
Printers.o:           $(PRINTERS_H) $(UTIL_H)
RandomNumbers.o:      $(RAND_H) $(MATHMACROS_H)
RecordIo.o:           $(REC_IO_H) $(UTIL_H)
StatisticalMaths.o:   $(PERM_H)
StringUtilities.o:    $(STR_UTILS_H) $(UTIL_H) $(MEMORY_H) $(ASSERT_H)


