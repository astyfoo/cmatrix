bin_PROGRAMS = cmatrix
cmatrix_SOURCES = cmatrix.c

cmatrix_CFLAGS = @NCURSESW_CFLAGS@
cmatrix_LDFLAGS = @NCURSESW_LIBS@

man_MANS = cmatrix.1

if MATRIX_FONTS
BUILD_FONTS = 1
else
BUILD_FONTS = 0
endif

EXTRA_DIST =	COPYING INSTALL install-sh \
		missing mkinstalldirs cmatrix.1 cmatrix.spec
