include $(top_srcdir)/config/Rules.am

libslow_io_de_ladir=/usr/local/lib/fmd

DEFAULT_INCLUDES += \
	-I$(top_srcdir)/include	\
	-I$(top_srcdir)/lib/libspl/include	\
	-I$(top_srcdir)/cmd/fm/fmd/common	\
	-I$(top_srcdir)/lib/fm/topo/libtopo/common	\
	-I$(top_srcdir)/lib/fm/topo/modules/common/slow-io-de

libslow_io_de_la_LTLIBRARIES = libslow-io-de.la

USER_C = \
	slow-io-de.c

libslow_io_de_la_LIBADD = 					\
	$(top_builddir)/lib/fm/topo/libtopo/common/libtopo.la

dist_libslow_io_de_la_SOURCES = \
	$(USER_C)

EXTRA_DIST = slow-io-de.conf
