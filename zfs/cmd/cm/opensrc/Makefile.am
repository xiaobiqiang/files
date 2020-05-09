include $(top_srcdir)/config/Rules.am

DEFAULT_INCLUDES += \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/lib/libspl/include

lib_LTLIBRARIES = libcmopensrc.la
libcmopensrc_la_SOURCES = \
	./iniparser/iniparser.c \
	./iniparser/dictionary.c \
	./json/libjson.c \
	./json/json_visit.c \
	./json/strerror_override.c \
	./json/json_pointer.c \
	./json/json_tokener.c \
	./json/json_object.c \
	./json/json_object_iterator.c \
	./json/json_c_version.c \
	./json/random_seed.c \
	./json/printbuf.c \
	./json/arraylist.c \
	./json/debug.c \
	./json/linkhash.c \
	./json/json_util.c \
	./mxml/mmd.c \
	./mxml/mxml-string.c \
	./mxml/mxml-search.c \
	./mxml/mxml-entity.c \
	./mxml/mxml-attr.c \
	./mxml/mxml-set.c \
	./mxml/mxml-private.c \
	./mxml/mxml-index.c \
	./mxml/mxml-node.c \
	./mxml/mxml-file.c \
	./mxml/mxml-get.c \
	./sqlite3/sqlite3.c 
EXTRA_DIST = \
	./iniparser/iniparser.h \
	./iniparser/dictionary.h \
	./json/strerror_override_private.h \
	./json/json_visit.h \
	./json/strdup_compat.h \
	./json/math_compat.h \
	./json/printbuf.h \
	./json/json_config.h \
	./json/random_seed.h \
	./json/json.h \
	./json/json_c_version.h \
	./json/json_pointer.h \
	./json/arraylist.h \
	./json/bits.h \
	./json/debug.h \
	./json/json_util.h \
	./json/json_tokener.h \
	./json/json_inttypes.h \
	./json/config.h \
	./json/strerror_override.h \
	./json/vasprintf_compat.h \
	./json/linkhash.h \
	./json/json_object_private.h \
	./json/json_object_iterator.h \
	./json/json_object.h \
	./mxml/config.h \
	./mxml/mxml.h \
	./mxml/mmd.h \
	./mxml/mxml-private.h \
	./sqlite3/sqlite3.h \
	./sqlite3/sqlite3ext.h 
libcmopensrc_la_CFLAGS = -std=gnu99 -lpthread -ldl
