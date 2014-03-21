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
##      @(#)Makefile	1.5,    97/07/02, 17:00:02
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
primary_event.o \
PED_shell.o \
primary_events_database.o


LIBNAME = $(BUILDIR)/libprimaryevents.a


# External dependencies
#
include ${TOP}/Appn/Exports
include ${TOP}/Utilities/Exports
include ${TOP}/Widgets/Exports


# Internal dependencies
#
include ${TOP}/PrimaryEvents/Exports
PRIMARY_EVENTS_DATABASE_H = primary_events_database.h\
 $(BOOLEAN_H) $(LIST_SUU_H) $(PRIM_EV_DB_H) $(PRIMARY_EVENT_H)


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
PED_shell.o:       $(PED_SHELL_H) $(LIST_SUU_H) $(LIST_SUU_UTIL_H) \
  $(PRIMARY_EVENTS_DATABASE_H) $(PRIM_EV_DB_H) $(FILE_UTILS_H) $(UTIL_H) \
  $(MEMORY_H) $(ASSERT_H)
primary_event.o:   $(PRIMARY_EVENT_H) $(ITEM_H) $(REC_IO_H) $(MEMORY_H) \
  $(UTIL_H)
primary_events_database.o: $(PRIMARY_EVENTS_DATABASE_H) $(PED_GUI_DEF_H) \
  $(PRIMARY_EVENT_H) $(FTA_H) $(FILE_UTILS_H) $(DIALOGS_H) $(PRIM_EV_DB_H) \
  $(UTIL_H) $(PED_SHELL_H) $(PED_GUI_ACTIONS_H) $(LIST_SUU_UTIL_H) \
  $(FILE_DIALOGS_H) $(PRINTERS_H) $(BTNACT_H) $(TOOLBAR_H) $(PRINT_WID_H) \
  $(MEMORY_H) $(ASSERT_H)


