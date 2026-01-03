dnl Process this file with autoconf to produce a configure script.

AC_INIT([cmatrix],[3.1xlah],[xylia.allegretta@gmail.com])
AC_CONFIG_SRCDIR([cmatrix.c])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE

AC_CANONICAL_HOST
case $host in
	*mingw*) ;;
	*) native_windows=no ;;
esac

dnl Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET

dnl Checks for header files.
AC_CHECK_HEADERS(fcntl.h sys/ioctl.h unistd.h termios.h termio.h ncurses.h curses.h)

dnl Checks for library functions.
AC_CHECK_FUNCS(putenv)

dnl Checks for libraries.
AC_ARG_ENABLE([utf8], AS_HELP_STRING([--disable-utf8], [Don't use ncursesw for unciode support]), [use_uni=$enableval], [use_uni=true])

if test "$use_uni" = true -o "$use_uni" = yes -o "$use_uni" = on -o "$use_uni" = 1
then
	AC_CHECK_LIB(ncursesw, main)
	CURSES_LIB_NAME=""

	AC_CHECK_HEADERS(ncursesw/ncurses.h)

	AC_CHECK_LIB(ncursesw, tgetent, CURSES_LIB="-lncursesw" CURSES_LIB_NAME=ncursesw)

	if eval "test x$CURSES_LIB_NAME = x"
	then
		AC_CHECK_LIB(termcapw, tgetent, CURSES_LIB="-ltermcapw" CURSES_LIB_NAME=termcapw)
	fi

	if eval "test x$CURSES_LIB_NAME = x"
	then
		AC_CHECK_LIB(tinfow, tgetent, CURSES_LIB="-ltinfow" CURSES_LIB_NAME=tinfow)
	fi
else
	dnl Checks for libraries.
	AC_CHECK_LIB(ncurses, main)

	CURSES_LIB_NAME=""
	AC_CHECK_LIB(ncurses, tgetent, CURSES_LIB="-lncurses" CURSES_LIB_NAME=ncurses)

	if eval "test x$CURSES_LIB_NAME = x"
	then
		AC_CHECK_LIB(curses, tgetent, CURSES_LIB="-lcurses" CURSES_LIB_NAME=curses)
	fi

	if eval "test x$CURSES_LIB_NAME = x"
	then
		AC_CHECK_LIB(termcap, tgetent, CURSES_LIB="-ltermcap" CURSES_LIB_NAME=termcap)
	fi

	if eval "test x$CURSES_LIB_NAME = x"
	then
		AC_CHECK_LIB(tinfo, tgetent, CURSES_LIB="-ltinfo" CURSES_LIB_NAME=tinfo)
	fi
fi

if eval "test x$CURSES_LIB_NAME = x"
then
	AC_MSG_WARN([
*** No termcap lib available, consider getting the official ncurses
*** distribution from ftp://ftp.gnu.org/pub/gnu/ncurses if you get
*** errors compiling cmatrix.])
else
	AC_MSG_RESULT([Using $CURSES_LIB_NAME as the termcap library])
fi

AC_CHECK_LIB($CURSES_LIB_NAME, use_default_colors, AC_DEFINE(HAVE_USE_DEFAULT_COLORS))
AC_CHECK_LIB($CURSES_LIB_NAME, resizeterm, [AC_DEFINE(HAVE_RESIZETERM)])
AC_CHECK_LIB($CURSES_LIB_NAME, wresize, [AC_DEFINE(HAVE_WRESIZE)])

dnl Only change gcc options if we are using gcc.
if test "$CC" = gcc -o "$CC" = g++
then
	CFLAGS="$CLAGS -Wall"
dnl	CFLAGS="$CLAGS -Wall -ansi -pedantic -Wno-overlength-strings"
fi

dnl Build static executable on native Windows
if test $native_windows != no
then
	CFLAGS="$CFLAGS -static"
fi

dnl cmatrix terminal font disable option
AC_ARG_ENABLE([fonts], AS_HELP_STRING([--disable-fonts], [Install cmatrix without cmatrix font]), [enable_fonts=$enableval], [enable_fonts=true])

if test "$enable_fonts" = true -o "$enable_fonts" = yes -o "$enable_fonts" = on -o "$enable_fonts" = 1
then
	enable_fonts="true"
	dnl Check for consolechars and setfonts
	AC_PATH_PROG(CONSOLECHARS, consolechars, "", $PATH)
	AC_PATH_PROG(SETFONT, setfont, "", $PATH)

	if test x$CONSOLECHARS = x
	then
		if test x$SETFONT = x
		then
			AC_MSG_WARN([
*** Neither the consolechars nor the setfont program was not found. You
*** will not be able to see the characters in the matrix font in the
*** console without this program (it may still work in xterms). If you are
*** using Linux, the package containing this program is usually called
*** kbd, kbd-utils, or console-utils])
		else
			AC_DEFINE_UNQUOTED(HAVE_SETFONT, $SETFONT)
		fi
	else
		AC_DEFINE_UNQUOTED(HAVE_CONSOLECHARS, $CONSOLECHARS)
	fi
fi

dnl Parse any configure options

LIBS="$LIBS $CURSES_LIB"
AM_CONDITIONAL([MATRIX_FONTS], [test $enable_fonts = true])

dnl AC_SUBST(CURSES_LIB)

if test $native_windows = no; then
	AC_DEFINE(USE_TIOCSTI)
fi


AH_TEMPLATE([HAVE_USE_DEFAULT_COLORS], [Define this if your curses library has use_default_colors, for cool transparency =-)])
AH_TEMPLATE([HAVE_CONSOLECHARS], [Define this if you have the linux consolechars program])
AH_TEMPLATE([HAVE_SETFONT], [Define this if you have the linux setfont program])
AH_TEMPLATE([HAVE_WRESIZE], [Define this if you have the wresize function in your ncurses-type library])
AH_TEMPLATE([HAVE_RESIZETERM], [Define this if you have the resizeterm function in your ncurses-type library])
AH_TEMPLATE([USE_TIOCSTI], [Define this if you want a character you pressed in the screensaver mode to retain in the terminal])

AC_CONFIG_FILES([Makefile cmatrix.spec])
AC_OUTPUT
