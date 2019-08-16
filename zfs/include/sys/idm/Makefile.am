COMMON_H =

KERNEL_H = \
	$(top_srcdir)/include/sys/idm/idm_conn_sm.h \
	$(top_srcdir)/include/sys/idm/idm.h \
	$(top_srcdir)/include/sys/idm/idm_impl.h \
	$(top_srcdir)/include/sys/idm/idm_so.h \
	$(top_srcdir)/include/sys/idm/idm_text.h \
	$(top_srcdir)/include/sys/idm/idm_transport.h

USER_H =

EXTRA_DIST = $(COMMON_H) $(KERNEL_H) $(USER_H)

if CONFIG_USER
libzfsdir = $(includedir)/libzfs/sys/idm
libzfs_HEADERS = $(COMMON_H) $(USER_H)
endif

if CONFIG_KERNEL
kerneldir = @prefix@/src/zfs-$(VERSION)/include/sys/idm
kernel_HEADERS = $(COMMON_H) $(KERNEL_H)
endif
