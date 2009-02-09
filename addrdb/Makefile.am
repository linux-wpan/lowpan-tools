include $(top_srcdir)/Makefile.common

AM_YFLAGS = -d --report=all
BUILT_SOURCE = coord-config-parse.h

noinst_LTLIBRARIES = libaddrdb.la
zbtest_PROGRAMS = parsetest

libaddrdb_la_SOURCES = coord-config-parse.y coord-config-lex.l addrdb.c
parsetest_CFLAGS = $(AM_CFLAGS) -DLEASE_FILE=\"$(lease_file)\"
parsetest_LDADD = libaddrdb.la $(LDADD) $(LEXLIB)
