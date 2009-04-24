include $(top_srcdir)/Makefile.common

SUBDIRS = lib addrdb src tests test-serial

include_HEADERS = include/ieee802154.h include/ieee80215-nl.h
noinst_HEADERS = include/libcommon.h include/addrdb.h include/logging.h

EXTRA_DIST = $(srcdir)/debian/changelog $(srcdir)/debian/compat $(srcdir)/debian/control $(srcdir)/debian/copyright \
	     $(srcdir)/debian/rules $(srcdir)/debian/watch \
	     $(srcdir)/debian/dirs $(srcdir)/debian/docs $(srcdir)/debian/*.examples $(srcdir)/debian/*.install

LIBTOOL_DEPS = @LIBTOOL_DEPS@
libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status --recheck

