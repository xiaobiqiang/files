COMMON_H = 

KERNEL_H = \
	$(top_srcdir)/include/sys/scsi/generic/commands.h \
	$(top_srcdir)/include/sys/scsi/generic/inquiry.h \
	$(top_srcdir)/include/sys/scsi/generic/persist.h \
	$(top_srcdir)/include/sys/scsi/generic/sense.h \
	$(top_srcdir)/include/sys/scsi/generic/status.h \
	$(top_srcdir)/include/sys/scsi/generic/dad_mode.h \
	$(top_srcdir)/include/sys/scsi/generic/message.h \
	$(top_srcdir)/include/sys/scsi/generic/mode.h \
	$(top_srcdir)/include/sys/scsi/generic/sas.h \
	$(top_srcdir)/include/sys/scsi/generic/smp_frames.h

USER_H =

EXTRA_DIST = $(COMMON_H) $(KERNEL_H) $(USER_H)

if CONFIG_KERNEL
kerneldir = @prefix@/src/zfs-$(VERSION)/include/sys/scsi/generic/
kernel_HEADERS = $(COMMON_H) $(KERNEL_H)
endif
