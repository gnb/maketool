#
# $Id: Makefile,v 1.8 1999-05-24 10:06:44 gnb Exp $
#
# Makefile for maketool.
#

CC=		gcc -Wall
CFLAGS=		-g
CPPFLAGS:=	$(shell gtk-config --cflags) -D_USE_BSD -Iicons
LDLIBS:=	$(shell gtk-config --libs)
O=		o.i586-unknown-linux

SOURCES=	main.c filter.c help.c preferences.c log.c ui.c \
		spawn.c util.c glib_extra.c
OBJECTS=	$(SOURCES:%.c=$O/%.o)

###

all:: maketool

maketool: $(OBJECTS)
	$(LINK.c) -o $@ $(OBJECTS) $(LDLIBS)
	
clean::
	$(RM) maketool
		
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
	$(CC) -c -o $O/crud.o crud.c
	
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
