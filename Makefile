#
# $Id: Makefile,v 1.5 1999-05-18 18:32:09 gnb Exp $
#
# Makefile for maketool.
#

CC=		gcc -Wall
CFLAGS=		-g
CPPFLAGS:=	$(shell gtk-config --cflags) -D_USE_BSD -Iicons
LDLIBS:=	$(shell gtk-config --libs)
O=		o.i586-unknown-linux

SOURCES=	main.c filter.c help.c log.c ui.c spawn.c glib_extra.c
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
	
delay:
	sleep 10
