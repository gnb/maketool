
all clean install uninstall::
	+for d in $(SUBDIRS) ; do \
	  $(MAKE) -C $$d TOPDIR=../$(TOPDIR) CURRDIR=$(CURRDIR)/$$d $@ ;\
	done
