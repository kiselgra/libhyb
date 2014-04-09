#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
AC_PREREQ(2.61)
AC_INIT([libhyb], [0.0.1])
AM_INIT_AUTOMAKE

# name one of your source files here
AC_CONFIG_SRCDIR([src/main.cpp])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_FILES(Makefile src/Makefile doc/Makefile doc/doxyfile doc/doxygen_mainpage.h)

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC

AC_ARG_ENABLE(gdb,
			[AS_HELP_STRING([--enable-gdb], [Wether to use debugging flags. Default: no.])],
			with_debug_flags="$enableval", with_debug_flags="no")
if test "x$with_debug_flags" == "xyes" ; then
	DEBUG_FLAGS="-ggdb3 -O0"
else
	DEBUG_FLAGS=""
fi
CFLAGS="$CFLAGS $DEBUG_FLAGS"
CXXFLAGS="$CXXFLAGS $DEBUG_FLAGS"

AX_STDCXX11_FLAG
CXXFLAGS="$CXXFLAGS $ac_cv_cxx_cxx11_spec"

# when using flex/bison:
# AC_PROG_LEX
# AC_PROG_YACC

# when building libraries
AC_PROG_LIBTOOL

# Checks for libraries.
AC_CHECK_LIB([cgl], [main])
AC_CHECK_LIB([cgls], [main])

PKG_CHECK_MODULES(RTA, rta)
AC_SUBST(RTA_CFLAGS)
AC_SUBST(RTA_LIBS)

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([inttypes.h stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AC_OUTPUT
