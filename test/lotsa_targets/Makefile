
all::

ifeq ($(filter clean,$(MAKECMDGOALS)),)	
-include .lotsa-targets.mk
endif

NUM_TARGETS=128
NUM_DIGITS=3

.lotsa-targets.mk:
	@echo "Makefile: making $@" 1>&2
	$(RM) $@
	n=0 ;\
	while [ $$n -lt $(NUM_TARGETS) ]; do \
	  nf=`echo $$n | awk '{printf "%0$(NUM_DIGITS)d", \$$1}'` ;\
	  echo -n " $$nf" 1>&2 ;\
	  echo "" ;\
	  echo "all:: target_$$nf" ;\
	  echo "target_$$nf:" ;\
	  echo "	@echo Making \$$@" ;\
	  n=`expr $$n + 1` ;\
	done > $@
	@echo "" 1>&2

clean::
	$(RM) .lotsa-targets.mk
