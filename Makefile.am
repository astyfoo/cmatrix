bin_PROGRAMS = cmatrix
cmatrix_SOURCES = cmatrix.c

man_MANS = cmatrix.1

EXTRA_DIST =	COPYING INSTALL install-sh \
		missing mkinstalldirs matrix.fnt \
		matrix.psf.gz mtx.pcf cmatrix.1

install-data-local:
if MATRIX_FONTS
	@if test -d $(DESTDIR)${datarootdir}/kbd/consolefonts; \
	then \
		echo " Installing matrix fonts in $(DESTDIR)${datarootdir}/kbd/consolefonts..."; \
		$(INSTALL_DATA) $(srcdir)/matrix.fnt $(DESTDIR)${datarootdir}/kbd/consolefonts; \
		$(INSTALL_DATA) $(srcdir)/matrix.psf.gz $(DESTDIR)${datarootdir}/kbd/consolefonts; \
	else \
		echo " Installing matrix fonts in $(DESTDIR)${datarootdir}/consolefonts..."; \
		if ! test -d $(DESTDIR)${datarootdir}/consolefonts; \
		then \
			install -vdm755 $(DESTDIR)${datarootdir}/consolefonts; \
		fi; \
		$(INSTALL_DATA) $(srcdir)/matrix.fnt $(DESTDIR)${datarootdir}/consolefonts; \
		$(INSTALL_DATA) $(srcdir)/matrix.psf.gz $(DESTDIR)${datarootdir}/consolefonts; \
	fi; \
	echo " Installing X window matrix fonts in $(DESTDIR)${datarootdir}/fonts/misc..."; \
	if test ! -d $(DESTDIR)${datarootdir}/fonts/misc; \
	then \
		install -vdm755 $(DESTDIR)${datarootdir}/fonts/misc; \
	fi; \
	$(INSTALL_DATA) $(srcdir)/mtx.pcf $(DESTDIR)${datarootdir}/fonts/misc; \
	$(INSTALL_DATA) $(srcdir)/mtx.pcf $(DESTDIR)${datarootdir}/fonts/misc; \
	echo " Running mkfontdir $(DESTDIR)${datarootdir}/fonts/misc..."; \
	$(MKFONTDIR) $(DESTDIR)${datarootdir}/fonts/misc; \
	echo " Done.  If this is the first time you have installed CMatrix you will"; \
	echo " probably have to restart X window in order to use the mtx.pcf font.";
else
	echo "Not installing Matrix fonts"
endif
