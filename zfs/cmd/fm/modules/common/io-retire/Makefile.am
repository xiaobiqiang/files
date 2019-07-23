include $(top_srcdir)/config/Rules.am

libio_retire_ladir=/usr/local/lib/fmd

DEFAULT_INCLUDES += \
	-I$(top_srcdir)/include	\
	-I$(top_srcdir)/lib/libspl/include	\
	-I$(top_srcdir)/cmd/fm/fmd/common	\
	-I$(top_srcdir)/lib/fm/topo/libtopo/common	\
	-I$(top_srcdir)/lib/fm/topo/modules/common/io-retire

libio_retire_la_LTLIBRARIES = libio-retire.la


USER_C = \
	rio_main.c

libio_retire_la_LIBADD = 					\
	$(top_builddir)/lib/fm/topo/libtopo/common/libtopo.la

dist_libio_retire_la_SOURCES = \
	$(USER_C)

