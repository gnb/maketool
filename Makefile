#
# $Id: Makefile,v 1.3 1999-05-18 07:58:13 gnb Exp $
#
# Makefile for maketool.
#

CC=		gcc -Wall
CFLAGS=		-g
CPPFLAGS:=	$(shell gtk-config --cflags) -D_USE_BSD
LDLIBS:=	$(shell gtk-config --libs)
O=		o.i586-unknown-linux

SOURCES=	main.c filter.c help.c spawn.c glib_extra.c
OBJECTS=	$(SOURCES:%.c=$O/%.o)

all:: maketool

maketool: $(OBJECTS)
	$(LINK.c) -o $@ $(OBJECTS) $(LDLIBS)
	
$O/%.o: %.c $O/.stamp
	$(COMPILE.c) -o $@ $<
	
$O/.stamp:
	-mkdir -p $(@D)
	touch $@
	
random:
	$(CC) -c -o $O/crud.o crud.c
	
targets:
	yes ' ' | head -20 | awk '{print NR}'
	
delay:
	sleep 10
