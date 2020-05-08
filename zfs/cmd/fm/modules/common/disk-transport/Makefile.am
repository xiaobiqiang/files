include $(top_srcdir)/config/Rules.am

libdisk_transport_ladir=/usr/local/lib/fmd

DEFAULT_INCLUDES += \
	-I$(top_srcdir)/include	\
	-I$(top_srcdir)/lib/libspl/include	\
	-I$(top_srcdir)/cmd/fm/fmd/common	\
	-I$(top_srcdir)/lib/libdiskmgt/include \
	-I$(top_srcdir)/lib/fm/topo/libtopo/common	\
	-I$(top_srcdir)/lib/fm/topo/modules/common/disk \
	-I/usr/include/libxml2

libdisk_transport_la_LTLIBRARIES = libdisk_transport.la
dist_libdisk_transport_la_DATA = libdisk_transport.conf

USER_C = \
	disk-transport.c \
	disk_db.c

libdisk_transport_la_LIBADD = 					\
	$(top_builddir)/lib/fm/topo/libtopo/common/libtopo.la \
	$(top_builddir)/lib/libnvpair/libnvpair.la \
        $(top_builddir)/lib/libuutil/libuutil.la \
        $(top_builddir)/lib/libdiskmgt/libdiskmgt.la \
        $(top_builddir)/lib/libzfs_core/libzfs_core.la \
        $(top_builddir)/lib/libzpool/libzpool.la \
        $(top_builddir)/lib/libzfs/libzfs.la 

libdisk_transport_la_LIBADD +=  -lxml2 -L$(top_srcdir)/cmd/cm/opensrc -lcmopensrc 

dist_libdisk_transport_la_SOURCES = \
	$(USER_C) \
	disk-transport.h \
	disk_db.h
