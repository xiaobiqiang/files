COMMON_H = 

KERNEL_H = \
	$(top_srcdir)/include/sys/scsi/impl/commands.h \
	$(top_srcdir)/include/sys/scsi/impl/inquiry.h \
	$(top_srcdir)/include/sys/scsi/impl/sense.h \
	$(top_srcdir)/include/sys/scsi/impl/status.h \
	$(top_srcdir)/include/sys/scsi/impl/mode.h \
	$(top_srcdir)/include/sys/scsi/impl/scsi_reset_notify.h \
	$(top_srcdir)/include/sys/scsi/impl/scsi_sas.h \
	$(top_srcdir)/include/sys/scsi/impl/services.h \
	$(top_srcdir)/include/sys/scsi/impl/smp_transport.h \
	$(top_srcdir)/include/sys/scsi/impl/spc3_types.h \
	$(top_srcdir)/include/sys/scsi/impl/transport.h \
	$(top_srcdir)/include/sys/scsi/impl/types.h \
	$(top_srcdir)/include/sys/scsi/impl/uscsi.h \
	$(top_srcdir)/include/sys/scsi/impl/usmp.h

USER_H =

EXTRA_DIST = $(COMMON_H) $(KERNEL_H) $(USER_H)

if CONFIG_KERNEL
kerneldir = @prefix@/src/zfs-$(VERSION)/include/sys/scsi/impl/
kernel_HEADERS = $(COMMON_H) $(KERNEL_H)
endif
