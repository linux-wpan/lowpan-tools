SUBDIRS = parse

zbtestdir = @zbtestdir@
zbtest_PROGRAMS = listen-packet tests/test tests/test2 tests/test3 tests/test4 tests/test5 tests/test6 tests/gnl
python_PYTHON = test-serial/test_DQ.py
zbtest_SCRIPTS = $(srcdir)/test-serial/test.py $(srcdir)/test-serial/test_edscan.py

sbin_PROGRAMS = izattach izconfig izcoordinator iz
bin_PROGRAMS = izchat izlisten

AM_CPPFLAGS = -I$(top_srcdir)/include
AM_CFLAGS = -g3 -Wall -Werror
LDADD = lib/libcommon.la

include_HEADERS = include/ieee802154.h include/ieee80215-nl.h
noinst_HEADERS = include/libcommon.h include/addrdb.h
noinst_LTLIBRARIES = lib/libcommon.la 
noinst_PROGRAMS = parsetest

izattach_SOURCES = serial.c
izcoordinator_SOURCES = coordinator.c addrdb.c
izlisten_SOURCES = listen.c
parsetest_CFLAGS = $(AM_CFLAGS) $(NL_CFLAGS) -DLEASE_FILE=\"$(leasefile)\"
parsetest_SOURCES = parse/parsetest.c addrdb.c
parsetest_DEPENDENCIES = parse/libconfigparse.la lib/libcommon.la
parsetest_LDADD = parse/libconfigparse.la lib/libcommon.la $(LEXLIB)

lib_libcommon_la_SOURCES = lib/printbuf.c lib/genl.c lib/parse.c lib/shash.c
lib_libcommon_la_DEPENDENCIES = libtool

tests_gnl_CFLAGS = $(AM_CFLAGS) $(NL_CFLAGS)
tests_gnl_LDADD = $(LDADD) $(NL_LIBS)

izcoordinator_CFLAGS = $(AM_CFLAGS) $(NL_CFLAGS) -DLEASE_FILE=\"$(leasefile)\"
izcoordinator_LDADD = $(LDADD) $(NL_LIBS) parse/libconfigparse.la $(LEXLIB)

iz_CFLAGS = $(AM_CFLAGS) $(NL_CFLAGS)
iz_LDADD = $(LDADD) $(NL_LIBS)

EXTRA_DIST = debian/

LIBTOOL_DEPS = @LIBTOOL_DEPS@
libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status --recheck

