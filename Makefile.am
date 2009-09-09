include $(top_srcdir)/Makefile.common

ACLOCAL_AMFLAGS = -I m4

SUBDIRS = lib addrdb src iplink tests test-serial

include_HEADERS = include/ieee802154.h include/nl802154.h
noinst_HEADERS = include/libcommon.h include/addrdb.h include/logging.h

EXTRA_DIST = $(srcdir)/debian/changelog $(srcdir)/debian/compat $(srcdir)/debian/control $(srcdir)/debian/copyright \
	     $(srcdir)/debian/rules $(srcdir)/debian/watch \
	     $(srcdir)/debian/dirs $(srcdir)/debian/docs $(srcdir)/debian/*.examples $(srcdir)/debian/*.install \
	     $(srcdir)/include/linux

libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status --recheck

