#
# $Id: Makefile,v 1.1 1999-05-14 17:23:23 gnb Exp $
#
# Makefile for maketool.
#

CC=		gcc -Wall
CFLAGS=		-g
CPPFLAGS:=	$(shell gtk-config --cflags) -D_USE_BSD
LDLIBS:=	$(shell gtk-config --libs)
O=		o.i586-unknown-linux

SOURCES=	main.c spawn.c
OBJECTS=	$(SOURCES:%.c=$O/%.o)

maketool: $(OBJECTS)
	$(LINK.c) -o $@ $(OBJECTS) $(LDLIBS)
	
$O/%.o: %.c $O/.stamp
	$(COMPILE.c) -o $@ $<
	
$O/.stamp:
	-mkdir -p $(@D)
	touch $@
	
random:
	echo "This is the result of target random"		
	
targets:
	echo "This is the result of target targets"	
	
delay:
	sleep 10
