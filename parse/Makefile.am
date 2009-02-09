AM_CPPFLAGS = -I$(top_srcdir)/include
noinst_LTLIBRARIES = libconfigparse.la
AM_YFLAGS = -d --report=all
PARSER_HEADER = coord-config-parse.h
PARSER_CODE = coord-config-parse.c coord-config-lex.c
BUILT_SOURCE = $(PARSER_HEADER) $(PARSER_CODE)
libconfigparse_la_SOURCES = $(PARSER_HEADER) coord-config-parse.y coord-config-lex.l
libconfigparse_la_DEPENDENCIES = $(PARSER_HEADER)

$(PARSER_HEADER): coord-config-parse.c

coord-config-lex.l: $(PARSER_HEADER)

CLEANFILES = $(BUILT_SOURCE) coord-config-parse.output

