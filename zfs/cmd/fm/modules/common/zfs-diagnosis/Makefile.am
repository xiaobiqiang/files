include $(top_srcdir)/config/Rules.am

libzfs_diagnose_ladir=/usr/local/lib/fmd

DEFAULT_INCLUDES += \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/lib/fm/topo/libtopo/common \
	-I$(top_srcdir)/lib/libspl/include

libzfs_diagnose_la_LTLIBRARIES = libzfs_diagnose.la
libzfs_diagnose_la_DATA = libzfs_diagnose.conf

USER_C = \
	zfs_de.c
libzfs_diagnose_la_LIBADD = 					\
	$(top_builddir)/lib/libuutil/libuutil.la	\
	$(top_builddir)/lib/libzfs/libzfs.la

dist_libzfs_diagnose_la_SOURCES = \
	$(USER_C)
