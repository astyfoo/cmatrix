dnl Process this file with autoconf to produce a configure script.

AC_INIT([cmatrix],[3.0ca1-git],[chrisa@asty.org])
AC_CONFIG_SRCDIR([cmatrix.c])
AC_CONFIG_HEADERS(config.h)
AM_INIT_AUTOMAKE
PKG_PROG_PKG_CONFIG

if eval "test x$PKG_CONFIG = x"
then
  AC_MSG_ERROR([
  *** The pkg-config is required to properly set flags.
  *** Please install it and re-run ./configure.])
  exit 1
fi

AC_CANONICAL_HOST
case $host in
  *mingw*)
    ;;
  *)
    native_windows=no
    ;;
esac

dnl Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET

dnl Checks for libraries.
dnl Replace `main' with a function in -lncurses:
AC_CHECK_LIB(ncursesw, main)

dnl Checks for header files.
m4_warn([obsolete],
[The preprocessor macro 'STDC_HEADERS' is obsolete.
  Except in unusual embedded environments, you can safely include all
  C89 headers unconditionally.])dnl
# Autoupdate added the next two lines to ensure that your configure
# script's behavior did not change.  They are probably safe to remove.
AC_CHECK_INCLUDES_DEFAULT
AC_PROG_EGREP

AC_CHECK_HEADERS(fcntl.h sys/ioctl.h unistd.h termios.h termio.h getopt.h)

dnl Checks for library functions.
AC_CHECK_FUNCS(putenv)

dnl Checks for libraries.
AC_CHECK_HEADERS(curses.h ncurses.h)

CURSES_LIB_NAME=""

AC_ARG_WITH([ncursesw], AS_HELP_STRING([--with-ncursesw], [Build with ncursesw (required, use for alternate install dir)]))
PKG_CHECK_MODULES([NCURSESW], [ncursesw])

AC_CHECK_LIB(ncursesw, tgetent, CURSES_LIB="-lncursesw" CURSES_LIB_NAME=ncursesw)

AC_CHECK_LIB($CURSES_LIB_NAME, use_default_colors,
	 AC_DEFINE(HAVE_USE_DEFAULT_COLORS))
AC_CHECK_LIB($CURSES_LIB_NAME, resizeterm, [AC_DEFINE(HAVE_RESIZETERM)])
AC_CHECK_LIB([$CURSES_LIB_NAME], wresize, [AC_DEFINE(HAVE_WRESIZE)])


dnl Only change gcc options if we are using gcc.
if test "$ac_cv_prog_CC" = gcc -o "$ac_cv_prog_CC" = g++; then
  CFLAGS="$CFLAGS -Wall -Wno-comment"
fi

dnl Build static executable on native Windows
if test x$native_windows != xno; then
  CFLAGS="$CFLAGS -static"
fi

dnl Parse any configure options

LIBS="$LIBS $CURSES_LIB"
AC_ARG_ENABLE(debug, [  --enable-debug          Enable debugging (def disabled)],)
AM_CONDITIONAL([MATRIX_FONTS], [test x$enable_fonts = xtrue])

AC_SUBST(CURSES_LIB)

if test x$native_windows = xno; then
  AC_DEFINE(USE_TIOCSTI)
fi

AH_TEMPLATE([HAVE_USE_DEFAULT_COLORS], [Define this if your curses library has use_default_colors, for cool transparency =-)])
AH_TEMPLATE([HAVE_WRESIZE], [Define this if you have the wresize function in your ncurses-type library])
AH_TEMPLATE([HAVE_RESIZETERM], [Define this if you have the resizeterm function in your ncurses-type library])
AH_TEMPLATE([USE_TIOCSTI], [Define this if you want a character you pressed in the screensaver mode to retain in the terminal])

AC_CONFIG_FILES([Makefile cmatrix.spec])
AC_OUTPUT
