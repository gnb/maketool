
# Pages which provide backwards compatibility for old URLs
COMPAT_PAGES=	index_c.html index_d.html features.html
PAGES=		index.html download.html \
		screenshots.html changelog.html $(COMPAT_PAGES)
IMAGES=		maketool_n.gif mainw.gif fm.mini.jpg tocbg.gif \
		autoconf.gif autoconf_t.gif kernel.gif kernel_t.gif \
		colors.jpg colors_t.jpg about_make.gif about_make_t.gif \
		arrow_l2.gif arrow_r2.gif caubutt1.gif gimp.gif \
		acgui.gif acgui_t.gif ja.gif ja_t.gif
BINARIES:=	$(shell m4 list-binaries.m4)	

# TODO: link to screen2.jpg from somewhere

DELIVERABLES=	$(PAGES) $(IMAGES) $(BINARIES)

############################################################

all:: $(PAGES)

changelog.html: _changelist.html
$(PAGES): _styles.html _common.m4 _copyright.html toc.html.in
index.html: _thanks.m4
download.html: versions.m4 _download.m4

_changelist.html: ../CHANGES
	./changes2html < $< > $@

%.html: %.html.in
	m4 -DHTMLFILE=$@ $< > $@
	
clean::
	$(RM) $(patsubst %.html.in,%.html,$(wildcard $(patsubst %.html,%.html.in,$(PAGES))))
		
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
uploaddir=	shell.alphalink.com.au:public_html

install:: installdirs $(addprefix $(htmldir)/,$(notdir $(DELIVERABLES)))

installdirs:
	test -d $(htmldir) || $(INSTALL) -d $(htmldir)
	
$(htmldir)/%: %
	$(INSTALL) -m 644 $< $@

#$(htmldir)/%: ../%
#	$(INSTALL) -m 644 $< $@

############################################################

upload:
	rsync -v -r -e ssh --rsync-path=/home/g/gnb/inst/bin/rsync $(htmldir) $(uploaddir)
