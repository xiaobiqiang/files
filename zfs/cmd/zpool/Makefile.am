include $(top_srcdir)/config/Rules.am

DEFAULT_INCLUDES += \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/lib/libspl/include \
	-I$(top_srcdir)/lib/libdiskmgt/include \
	-I/usr/include/libxml2

sbin_PROGRAMS = zpool

zpool_SOURCES = \
	zpool_iter.c \
	zpool_main.c \
	zpool_util.c \
	zpool_util.h \
	zpool_vdev.c

zpool_LDADD = \
	$(top_builddir)/lib/libnvpair/libnvpair.la \
	$(top_builddir)/lib/libuutil/libuutil.la \
	$(top_builddir)/lib/libzpool/libzpool.la \
	$(top_builddir)/lib/libzfs/libzfs.la \
	$(top_builddir)/lib/libzfs_core/libzfs_core.la \
	$(top_builddir)/lib/libstmf/libstmf.la \
	$(top_builddir)/lib/libdiskmgt/libdiskmgt.la \
	$(LIBBLKID)

zpool_LDADD += -lxml2 -lrt
