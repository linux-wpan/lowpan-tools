zbtestdir = @zbtestdir@
zbtest_PROGRAMS = serial listen listen-packet tests/test tests/test2 tests/test3 tests/test4

AM_CPPFLAGS = -I$(top_srcdir)/include
LDADD = -L. -lcommon

noinst_HEADERS = include/ieee802154.h
noinst_HEADERS += include/libcommon.h
noinst_LIBRARIES = libcommon.a

libcommon_a_SOURCES = printbuf.c

