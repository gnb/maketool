#
# Makefile for running maketool with various numbers of targets
# to test the multiple-submenus feature of the Build menu.
#

# This should be the same as the define in main.c
BUILD_MENU_THRESHOLD=20
LOTS=6
SOME=8
TEST_NUMBERS=		0, \
			1, \
			SOME, \
			BUILD_MENU_THRESHOLD-1, \
			BUILD_MENU_THRESHOLD, \
			BUILD_MENU_THRESHOLD+1, \
			(LOTS*BUILD_MENU_THRESHOLD)-1, \
			(LOTS*BUILD_MENU_THRESHOLD), \
			(LOTS*BUILD_MENU_THRESHOLD)+1, \
			(LOTS*BUILD_MENU_THRESHOLD)+SOME

AWKFLAGS=	-v BUILD_MENU_THRESHOLD=$(BUILD_MENU_THRESHOLD) \
		-v LOTS=$(LOTS) \
		-v SOME=$(SOME)

all:
	echo "$(TEST_NUMBERS)" | tr , '\012' |\
	while read e ; do \
	  n=`awk $(AWKFLAGS) "BEGIN {print $$e ; exit}"` ;\
	  echo "============================================================" ;\
	  echo "$$n = $$e" ;\
	  gmake -f Makefile clean ;\
	  gmake -f Makefile NUM_TARGETS=$$n .lotsa-targets.mk;\
	  ../../maketool ;\
	done


