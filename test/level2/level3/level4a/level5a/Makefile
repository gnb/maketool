
include $(TOPDIR)/crud.mk

#
# Test recursive make invocation *without* changing directory
#
all::
	$(MAKE) hello_world
	
hello_world:
	@echo "This line should appear at the same indent as the previous compilation"

