#
# $Id: Makefile,v 1.1 1999-05-25 15:08:45 gnb Exp $
#
# Makefile for maketool.
#

all:: random recurse

###

$O/%.o: %.c $O/.stamp
	$(COMPILE.c) -o $@ $<
	
$O/.stamp:
	-mkdir -p $(@D)
	touch $@

clean::
	$(RM) -r $O
		
###

random:
	$(CC) -c -o $O/sub1.o sub1.c
	
SUBDIRS= subdir1_1
recurse:
	for d in $(SUBDIRS) ; do \
	    $(MAKE) -C $$d all ;\
	done
