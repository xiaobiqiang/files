COMMON_H = 
KERNEL_H = \
	$(top_srcdir)/include/sys/mltsas/mlt_sas.h 

USER_H =

EXTRA_DIST = $(COMMON_H) $(KERNEL_H) $(USER_H)

if CONFIG_USER
libzfsdir = $(includedir)/libzfs/sys/mltsas
libzfs_HEADERS = $(COMMON_H) $(USER_H)
endif

if CONFIG_KERNEL
kerneldir = @prefix@/src/zfs-$(VERSION)/include/sys/mltsas
kernel_HEADERS = $(COMMON_H) $(KERNEL_H)
endif
