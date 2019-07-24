COMMON_H = iscsit_common.h

KERNEL_H = \
	$(top_srcdir)/include/sys/iscsit/chap.h \
	$(top_srcdir)/include/sys/iscsit/iscsi_if.h \
	$(top_srcdir)/include/sys/iscsit/isns_protocol.h \
	$(top_srcdir)/include/sys/iscsit/idm_so.h \
	$(top_srcdir)/include/sys/iscsit/radius_packet.h \
	$(top_srcdir)/include/sys/iscsit/radius_protocol.h

USER_H =

EXTRA_DIST = $(COMMON_H) $(KERNEL_H) $(USER_H)

if CONFIG_USER
libzfsdir = $(includedir)/libzfs/sys/iscsit
libzfs_HEADERS = $(COMMON_H) $(USER_H)
endif

if CONFIG_KERNEL
kerneldir = @prefix@/src/zfs-$(VERSION)/include/sys/iscsit
kernel_HEADERS = $(COMMON_H) $(KERNEL_H)
endif
