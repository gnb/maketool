
# relative path to release directory containing tarballs, changelog etc
RELEASEDIR=	..

# Pages which provide backwards compatibility for old URLs
COMPAT_PAGES=	index_c.html index_d.html features.html
PAGES=		index.html download.html \
		screenshots.html changelog.html $(COMPAT_PAGES)
IMAGES=		maketool_n.gif mainw.gif fm.mini.jpg tocbg.gif \
		autoconf.gif autoconf_t.gif kernel.gif kernel_t.gif \
		colors.jpg colors_t.jpg about_make.gif about_make_t.gif \
		arrow_l2.gif arrow_r2.gif gimp.gif \
		acgui.gif acgui_t.gif ja.gif ja_t.gif
BINARIES:=	$(addprefix $(RELEASEDIR)/,$(shell m4 -I$(RELEASEDIR) list-binaries.m4))
SCRIPTS=	backbone.php
DOTFILES=	.htaccess downloads/.htaccess

# TODO: link to screen2.jpg from somewhere

DELIVERABLES=	$(PAGES) $(IMAGES) $(SCRIPTS) $(DOTFILES) \
		$(addprefix downloads/,$(notdir $(BINARIES)))

############################################################

all:: $(PAGES)

changelog.html: _changelist.html
$(PAGES): _styles.html _common.m4 _copyright.html toc.html.in
index.html: _thanks.m4
download.html: $(RELEASEDIR)/version.m4 _download.m4 downloadables.m4

_changelist.html: $(RELEASEDIR)/CHANGES
	./changes2html < $< > $@

HTMLINCDIRS=	-I$(RELEASEDIR)
HTMLDEFINES=	-DHTMLFILE=$@ \
		-DRELEASEDIR=$(RELEASEDIR)
	
%.html: %.html.in
	m4 $(M4FLAGS) $(HTMLINCDIRS) $(HTMLDEFINES) $< > $@
	
ALPHAHOME=	/home/g/gnb
LOGAPO= 	$(ALPHAHOME)/inst/etc/log.apo
backbone.php: backbone.php.in
	m4 $(M4FLAGS) -I$(RELEASEDIR) -DLOGAPO=$(LOGAPO) $< > $@

clean::
	$(RM) $(patsubst %.html.in,%.html,$(wildcard $(patsubst %.html,%.html.in,$(PAGES))))
	$(RM) backbone.php

############################################################
# compat pages

index_c.html: changelog.html
	$(RM) $@
	ln $< $@
	
index_d.html: download.html
	$(RM) $@
	ln $< $@
	
features.html: index.html
	$(RM) $@
	ln $< $@
	
clean::
	$(RM) $(COMPAT_PAGES)
	
############################################################

INSTALL=	install -c
htmldir=	$(HOME)/public_html/alphalink/maketool
uploadhost=	shell.alphalink.com.au
uploaddir=	$(uploadhost):public_html

install:: installdirs $(addprefix $(htmldir)/,$(DELIVERABLES))

installdirs:
	@OLD=OLD`date +%Y%m%d` ;\
	if [ -d $(htmldir).$$OLD ]; then \
	    echo "/bin/rm -rf $(htmldir)" ;\
	    /bin/rm -rf $(htmldir) ;\
	else \
	    echo "/bin/mv $(htmldir) $(htmldir).$$OLD" ;\
	    /bin/mv $(htmldir) $(htmldir).$$OLD ;\
	fi
	$(INSTALL) -m 755 -d $(htmldir)
	$(INSTALL) -m 755 -d $(htmldir)/downloads
	
$(htmldir)/%: %
	$(INSTALL) -m 644 $< $@

$(htmldir)/.%: %
	$(INSTALL) -m 644 $< $@

$(htmldir)/downloads/%: $(RELEASEDIR)/%
	$(INSTALL) -m 644 $< $@
	ln -sf backbone.php $(htmldir)/$*

$(htmldir)/downloads/.%: downloads/%
	$(INSTALL) -m 644 $< $@

############################################################

SSH=			ssh
RSYNC_VERBOSE=		-v
#RSYNC_PATH_FLAGS=	--rsync-path=/home/g/gnb/inst/bin/rsync

upload: upload.$(shell uname -n | cut -d. -f1)

# Upload via SSH-in-SSH tunnel.
upload.ocelot:
	$(MAKE) uploadhost=localhost SSH="ssh -p 1022" upload.generic

# Upload via direct connection
upload.marduk: upload.generic

upload.generic:
	rsync $(RSYNC_VERBOSE) -r --delete --links -e "$(SSH)" $(RSYNC_PATH_FLAGS) $(htmldir) $(uploaddir)

