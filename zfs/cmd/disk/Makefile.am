include $(top_srcdir)/config/Rules.am

DEFAULT_INCLUDES += \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/lib/libspl/include \
	-I$(top_srcdir)/lib/libdiskmgt/include \
	-I/usr/include/libxml2

sbin_PROGRAMS = disk 

disk_SOURCES = 	\
	disk.c

EXTRA_DIST = \
	initdisk \
	get_en_slot.sh \
	sg_led_disk.sh 

disk_LDADD = \
	$(top_builddir)/lib/libnvpair/libnvpair.la \
	$(top_builddir)/lib/libuutil/libuutil.la \
	$(top_builddir)/lib/libdiskmgt/libdiskmgt.la \
	$(top_builddir)/lib/libzfs_core/libzfs_core.la \
	$(top_builddir)/lib/libzpool/libzpool.la \
	$(top_builddir)/lib/libzfs/libzfs.la \
	$(top_builddir)/lib/libstmf/libstmf.la

disk_LDADD += $(ZLIB) -lxml2 -lrt
disk_LDFLAGS = -pthread

install-exec-hook:
	cp $(top_srcdir)/cmd/disk/get_en_slot.sh /usr/local/bin
	chmod a+x /usr/local/bin/get_en_slot.sh
	mkdir -p /var/fm
	cp $(top_srcdir)/cmd/disk/sg_led_disk.sh /var/fm/
	chmod a+x /var/fm/sg_led_disk.sh
	cp $(top_srcdir)/cmd/disk/initdisk /usr/local/bin
	chmod a+x /usr/local/bin/initdisk
