
PAGES=		index.html banner.html toc.html features.html download.html \
		screenshots.html changelog.html
IMAGES=		maketool_n.gif mainw.gif fm.mini.jpg tocbg.gif

# TODO: link to screen2.jpg from somewhere

DELIVERABLES=	$(PAGES) $(IMAGES)

############################################################

all:: $(PAGES)

changelog.html: _changelist.html
features.html: _thanks.m4
$(PAGES): _styles.html _common.m4

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

$(INSTALLDIR)/%: ../%
	$(INSTALL) -m 644 $< $@
