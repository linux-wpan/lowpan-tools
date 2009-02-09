include $(top_srcdir)/Makefile.common

zbtest_PROGRAMS = listen-packet test test2 test3 test4 test5 test6 gnl
bin_PROGRAMS = izlisten

izlisten_SOURCES = listen.c

gnl_CFLAGS = $(AM_CFLAGS) $(NL_CFLAGS)
gnl_LDADD = $(LDADD) $(NL_LIBS)

