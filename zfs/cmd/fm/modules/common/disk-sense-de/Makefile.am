include $(top_srcdir)/config/Rules.am

libdisk_sense_de_ladir=/usr/local/lib/fmd

DEFAULT_INCLUDES += \
	-I$(top_srcdir)/include	\
	-I$(top_srcdir)/lib/libspl/include	\
	-I$(top_srcdir)/cmd/fm/fmd/common	\
	-I$(top_srcdir)/lib/libdiskmgt/include \
	-I$(top_srcdir)/lib/fm/topo/libtopo/common	\
	-I$(top_srcdir)/lib/fm/topo/modules/common/disk \
	-I/usr/include/libxml2
libdisk_sense_de_la_LTLIBRARIES = libdisk_sense_de.la

USER_C = \
	disk-sense-de.c

libdisk_sense_de_la_LIBADD = 					\
	$(top_builddir)/lib/fm/topo/libtopo/common/libtopo.la

libdisk_sense_de_la_LIBADD +=  -lxml2 

dist_libdisk_sense_de_la_SOURCES = \
	$(USER_C)
