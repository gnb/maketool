#
# Maketool - GTK-based front end for gmake
# Copyright (c) 1999 Greg Banks
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
# $Id: Makefile,v 1.2 1999-08-10 15:45:02 gnb Exp $
#


CC=		gcc -Wall
CFLAGS=		-g

all:: random targets delay recurse longline unendedline

clean::
	@echo Nothing to see here

random:
	gcc -ansi -Wall -c -o crud.o crud.c
	
targets:
	yes ' ' | head -20 | awk '{print NR}'
	: VARIABLE_ONE = $(VARIABLE_ONE)
	: VARIABLE_TWO = $(VARIABLE_TWO)
	: VARIABLE_THREE = $(VARIABLE_THREE)
	: VARIABLE_FOUR = $(VARIABLE_FOUR)
	: VARIABLE_FIVE = $(VARIABLE_FIVE)
	: VARIABLE_SIX = $(VARIABLE_SIX)
	
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
