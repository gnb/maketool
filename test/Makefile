#
# Maketool - GTK-based front end for gmake
# Copyright (c) 1999-2003 Greg Banks
# 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
# 
# $Id: Makefile,v 1.9 2003-05-24 05:49:52 gnb Exp $
#


CC=		gcc -Wall
CFLAGS=		-g

all:: errors targets nested delay recurse longline unendedline emptyline

errors: error1 error2 error3 error4

clean::
	@echo Nothing to see here

error1:
	gcc -ansi -Wall -c crud.c

error2:
	gcc -ansi -Wall -c fred.c
	
error3:
	gcc -ansi -Wall -c jane.c

error4:
	+$(MAKE) -C abs_sub $@
	
targets:
	yes ' ' | head -20 | awk '{print NR}'
	: VARIABLE_ONE = $(VARIABLE_ONE)
	: VARIABLE_TWO = $(VARIABLE_TWO)
	: VARIABLE_THREE = $(VARIABLE_THREE)
	: VARIABLE_FOUR = $(VARIABLE_FOUR)
	: VARIABLE_FIVE = $(VARIABLE_FIVE)
	: VARIABLE_SIX = $(VARIABLE_SIX)
	
nested:
	+$(MAKE) -C level2 TOPDIR=.. CURRDIR=level2 all

clean::
	+$(MAKE) -C level2 TOPDIR=.. CURRDIR=level2 clean

delay:
	sleep 10

SUBDIRS= subdir1 subdir2
recurse:
	for d in $(SUBDIRS) ; do \
	    $(MAKE) -C $$d all ;\
	done
	
really/deeply/located/target:
	touch $@

.PHONY: longline
longline:
	./longline

.PHONY: unendedline
unendedline:
	./unendedline

.PHONY: emptyline
emptyline:
	@echo "The next line is empty"
	@echo ""
	@echo "The previous line was empty"
		
.PHONY: emptyfirstline
emptyfirstline:
	@echo ""
	@echo "The previous line was empty"
		
LONGN=2048
.PHONY: longlisting
longlisting:
	@echo "Generating $(LONGN) lines of output"
	@i=0; while [ $$i -lt $(LONGN) ]; do \
	  echo "gcc -ansi -Wall -o foo$$i.o -c foo$$i.c" ;\
	  i=`expr $$i + 1` ;\
	done

.PHONY: this_directory_is_test
this_directory_is_test:

