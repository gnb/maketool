
PAGES=		index.html index_c.html index_d.html banner.html toc.html \
		features.html download.html \
		screenshots.html changelog.html
IMAGES=		maketool_n.gif mainw.gif fm.mini.jpg tocbg.gif \
		autoconf.gif autoconf_t.gif kernel.gif kernel_t.gif \
		colors.jpg colors_t.jpg about_make.gif about_make_t.gif \
		arrow_l2.gif arrow_r2.gif
BINARIES:=	$(shell m4 list-binaries.m4)		

# TODO: link to screen2.jpg from somewhere

DELIVERABLES=	$(PAGES) $(IMAGES) $(BINARIES)

############################################################

all:: $(PAGES)

changelog.html: _changelist.html
features.html: _thanks.m4
$(PAGES): _styles.html _common.m4 _copyright.html
index.html index_c.html index_d.html: _index.html
download.html: versions.m4 _download.m4

_changelist.html: ../CHANGES
	./changes2html < $< > $@

%.html: %.html.in
	m4 $< > $@
	
############################################################

INSTALL=	install -c
INSTALLDIR=	$(HOME)/public_html/alphalink/maketool

install:: $(addprefix $(INSTALLDIR)/,$(notdir $(DELIVERABLES)))

$(INSTALLDIR)/%: %
	$(INSTALL) -m 644 $< $@

#$(INSTALLDIR)/%: ../%
#	$(INSTALL) -m 644 $< $@

############################################################

upload:
	weex maketool
