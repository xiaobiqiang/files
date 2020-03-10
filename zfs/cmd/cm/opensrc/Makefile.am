include $(top_srcdir)/config/Rules.am

DEFAULT_INCLUDES += \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/lib/libspl/include

lib_LTLIBRARIES = libcmopensrc.la
libcmopensrc_la_SOURCES = \
	./json/random_seed.c \
	./json/json_pointer.c \
	./json/libjson.c \
	./json/linkhash.c \
	./json/debug.c \
	./json/printbuf.c \
	./json/arraylist.c \
	./json/json_c_version.c \
	./json/json_object_iterator.c \
	./json/json_util.c \
	./json/json_tokener.c \
	./json/json_visit.c \
	./json/strerror_override.c \
	./json/json_object.c \
	./iniparser/iniparser.c \
	./iniparser/dictionary.c \
	./sqlite3/sqlite3.c \
	./mxml/mmd.c \
	./mxml/mxml-attr.c \
	./mxml/mxml-node.c \
	./mxml/mxml-index.c \
	./mxml/mxml-get.c \
	./mxml/mxml-search.c \
	./mxml/mxml-private.c \
	./mxml/mxml-entity.c \
	./mxml/mxml-file.c \
	./mxml/mxml-string.c \
	./mxml/mxml-set.c 
EXTRA_DIST = \
	./json/bits.h \
	./json/json.h \
	./json/json_object.h \
	./json/json_visit.h \
	./json/config.h \
	./json/json_tokener.h \
	./json/math_compat.h \
	./json/json_c_version.h \
	./json/json_pointer.h \
	./json/strerror_override_private.h \
	./json/json_object_private.h \
	./json/json_config.h \
	./json/strerror_override.h \
	./json/json_util.h \
	./json/arraylist.h \
	./json/random_seed.h \
	./json/debug.h \
	./json/json_inttypes.h \
	./json/vasprintf_compat.h \
	./json/linkhash.h \
	./json/strdup_compat.h \
	./json/json_object_iterator.h \
	./json/printbuf.h \
	./iniparser/dictionary.h \
	./iniparser/iniparser.h \
	./sqlite3/sqlite3.h \
	./sqlite3/sqlite3ext.h \
	./mxml/mxml-private.h \
	./mxml/config.h \
	./mxml/mxml.h \
	./mxml/mmd.h 
libcmopensrc_la_CFLAGS = -std=gnu99 -lpthread -ldl
