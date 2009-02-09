include $(top_srcdir)/Makefile.common

sbin_PROGRAMS = izattach izconfig izcoordinator iz
bin_PROGRAMS = izchat

izattach_SOURCES = serial.c

izcoordinator_SOURCES = coordinator.c
izcoordinator_CFLAGS = $(AM_CFLAGS) $(NL_CFLAGS) -DLEASE_FILE=\"$(leasefile)\"
izcoordinator_LDADD = ../addrdb/libaddrdb.la $(LDADD) $(NL_LIBS) $(LEXLIB)

iz_CFLAGS = $(AM_CFLAGS) $(NL_CFLAGS)
iz_LDADD = $(LDADD) $(NL_LIBS)

