#
# $Id: Makefile,v 1.1 1999-05-25 15:08:47 gnb Exp $
#
# Makefile for maketool.
#

all:: random

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
	$(CC) -c -o $O/sub1_1.o sub1_1.c
	
