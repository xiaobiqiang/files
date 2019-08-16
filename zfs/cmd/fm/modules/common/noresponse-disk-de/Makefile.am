include $(top_srcdir)/config/Rules.am

libnoresponse_disk_de_ladir=/usr/local/lib/fmd

DEFAULT_INCLUDES += \
	-I$(top_srcdir)/include	\
	-I$(top_srcdir)/lib/libspl/include	\
	-I$(top_srcdir)/cmd/fm/fmd/common	\
	-I$(top_srcdir)/lib/fm/topo/libtopo/common	\
	-I$(top_srcdir)/lib/fm/topo/modules/common/noresponse-disk-de

libnoresponse_disk_de_la_LTLIBRARIES = libnoresponse-disk-de.la

USER_C = \
	noresponse-disk-de.c

libnoresponse_disk_de_la_LIBADD = 					\
	$(top_builddir)/lib/fm/topo/libtopo/common/libtopo.la

dist_libnoresponse_disk_de_la_SOURCES = \
	$(USER_C)
