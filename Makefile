#
# $Id: Makefile,v 1.2 1999-05-15 14:44:30 gnb Exp $
#
# Makefile for maketool.
#

CC=		gcc -Wall
CFLAGS=		-g
CPPFLAGS:=	$(shell gtk-config --cflags) -D_USE_BSD
LDLIBS:=	$(shell gtk-config --libs)
O=		o.i586-unknown-linux

SOURCES=	main.c spawn.c filter.c
OBJECTS=	$(SOURCES:%.c=$O/%.o)

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
