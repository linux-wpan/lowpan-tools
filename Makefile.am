zbtestdir = @zbtestdir@
zbtest_PROGRAMS = serial listen listen-packet izconfig
zbtest_PROGRAMS += tests/test tests/test2 tests/test3 tests/test4

AM_CPPFLAGS = -I$(top_srcdir)/include
AM_CFLAGS = -g3 -Wall -Werror
LDADD = libcommon.la

noinst_HEADERS = include/ieee802154.h
noinst_HEADERS += include/libcommon.h
noinst_LTLIBRARIES = libcommon.la

libcommon_la_SOURCES = printbuf.c

#LIBTOOL_DEPS = @LIBTOOL_DEPS@
#libtool: $(LIBTOOL_DEPS)
#	$(SHELL) ./config.status --recheck

