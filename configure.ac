#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT([lowpan-utils],[0.1],[dbaryshkov@gmail.com])
AC_CONFIG_AUX_DIR([misc])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([-Wall -Werror gnu])
AC_CONFIG_SRCDIR([src/serial.c])
AC_CONFIG_HEADER([config.h])

AC_ARG_WITH(zbtestdir, [  --with-zbtestdir	directory where all test programs will be installed],
	    [zbtestdir=$withval],
	    [zbtestdir='${libexecdir}/zigbee'])
AC_SUBST(zbtestdir)

AC_ARG_WITH(leasefile, [  --with-leasefile	file for persistent lease storage],
	    [leasefile=$withval],
	    [leasefile='${localstatedir}/zigbee/izcoordinator.leases'])
AC_SUBST(leasefile)

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_RANLIB
AC_PROG_LIBTOOL
AM_PROG_LEX
AC_PROG_YACC
AC_SUBST(LIBTOOL_DEPS)
AM_PATH_PYTHON(,, [:])

# Checks for libraries.
PKG_CHECK_MODULES([NL], [libnl-1])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h stdint.h stdlib.h string.h sys/ioctl.h sys/socket.h termios.h unistd.h net/ieee80215.h net/af_ieee80215.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_UINT64_T
AC_CHECK_TYPE(struct sockaddr_ieee80215,
	      [AC_DEFINE([HAVE_STRUCT_SOCKADDR_IEEE80215],[],[Define to 1 if you have struct sockaddr_ieee80215])],[],
	      [#include <sys/types.h>
	       #include <sys/socket.h>
	       #ifdef HAVE_NET_IEEE80215_H
	       #include <net/ieee80215.h>
	       #endif
	       #ifdef HAVE_NET_AF_IEEE80215_H
	       #include <net/af_ieee80215.h>
	       #endif
	       ])

# Checks for library functions.
AC_PROG_GCC_TRADITIONAL
AC_CHECK_FUNCS([memset socket])

AC_CONFIG_FILES([
	Makefile
	lib/Makefile
	addrdb/Makefile
	src/Makefile
	tests/Makefile
	test-serial/Makefile
])
AC_OUTPUT
