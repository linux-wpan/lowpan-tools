include $(top_srcdir)/Makefile.common

sbin_PROGRAMS = izattach izconfig izcoordinator iz
bin_PROGRAMS = izchat

if MANPAGES
manpages = izconfig.8 izcoordinator.8 iz.8 izattach.8
else
manpages =
endif
dist_man_MANS = $(manpages)
izattach_DESC = "attach a serial device to IEEE 802.15.4 stack"
izcoordinator_DESC = "simple coordinator for IEEE 802.15.4 network"
iz_DESC = "configure an IEEE 802.15.4 interface"
izconfig_DESC = "configure an IEEE 802.15.4 interface"

izattach_SOURCES = serial.c

izcoordinator_SOURCES = coordinator.c
izcoordinator_CFLAGS = $(AM_CFLAGS) $(NL_CFLAGS) -DLEASE_FILE=\"$(leasefile)\"
izcoordinator_LDADD = ../addrdb/libaddrdb.la $(LDADD) $(NL_LIBS) $(LEXLIB)

iz_CFLAGS = $(AM_CFLAGS) $(NL_CFLAGS)
iz_LDADD = $(LDADD) $(NL_LIBS)

izattach.8: $(izattach_SOURCES)
	-$(HELP2MAN) -o $@ -s 8 -N -n $(izattach_DESC)  ./`basename $@ .8`

izconfig.8: $(izconfig_SOURCES)
	-$(HELP2MAN) -o $@ -s 8 -N -n $(izconfig_DESC)  ./`basename $@ .8`

izcoordinator.8: $(izcoordinator_SOURCES)
	-$(HELP2MAN) -o $@ -s 8 -N -n $(izcoordinator_DESC)  ./`basename $@ .8`

iz.8: $(iz_SOURCES)
	-$(HELP2MAN) -o $@ -s 8 -N -n $(iz_DESC)  ./`basename $@ .8`

