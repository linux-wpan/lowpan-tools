include $(top_srcdir)/Makefile.common

SUBDIRS = lib addrdb src tests test-serial

include_HEADERS = include/ieee802154.h include/ieee80215-nl.h
noinst_HEADERS = include/libcommon.h include/addrdb.h

EXTRA_DIST = debian/

LIBTOOL_DEPS = @LIBTOOL_DEPS@
libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status --recheck

