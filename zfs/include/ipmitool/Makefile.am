COMMON_H = 

KERNEL_H =

USER_H = \
	$(top_srcdir)/include/ipmitool/ipmi.h \
	$(top_srcdir)/include/ipmitool/bswap.h \
	$(top_srcdir)/include/ipmitool/ipmi_ekanalyzer.h \
	$(top_srcdir)/include/ipmitool/ipmi_hpmfwupg.h \
	$(top_srcdir)/include/ipmitool/ipmi_oem.h \
	$(top_srcdir)/include/ipmitool/ipmi_session.h \
	$(top_srcdir)/include/ipmitool/helper.h \
	$(top_srcdir)/include/ipmitool/ipmi_entity.h \
	$(top_srcdir)/include/ipmitool/ipmi_ime.h \
	$(top_srcdir)/include/ipmitool/ipmi_pef.h \
	$(top_srcdir)/include/ipmitool/ipmi_sol.h \
	$(top_srcdir)/include/ipmitool/ipmi_cc.h \
	$(top_srcdir)/include/ipmitool/ipmi_event.h \
	$(top_srcdir)/include/ipmitool/ipmi_event.h \
	$(top_srcdir)/include/ipmitool/ipmi_intf.h \
	$(top_srcdir)/include/ipmitool/ipmi_picmg.h \
	$(top_srcdir)/include/ipmitool/ipmi_strings.h \
	$(top_srcdir)/include/ipmitool/ipmi_channel.h \
	$(top_srcdir)/include/ipmitool/ipmi_firewall.h \
	$(top_srcdir)/include/ipmitool/ipmi_isol.h \
	$(top_srcdir)/include/ipmitool/ipmi_raw.h \
	$(top_srcdir)/include/ipmitool/ipmi_sunoem.h \
	$(top_srcdir)/include/ipmitool/ipmi_chassis.h \
	$(top_srcdir)/include/ipmitool/ipmi_fru.h \
	$(top_srcdir)/include/ipmitool/ipmi_kontronoem.h \
	$(top_srcdir)/include/ipmitool/ipmi_sdradd.h \
	$(top_srcdir)/include/ipmitool/ipmi_tsol.h \
	$(top_srcdir)/include/ipmitool/ipmi_constants.h \
	$(top_srcdir)/include/ipmitool/ipmi_fwum.h \
	$(top_srcdir)/include/ipmitool/ipmi_lanp.h \
	$(top_srcdir)/include/ipmitool/ipmi_sdr.h \
	$(top_srcdir)/include/ipmitool/ipmi_user.h \
	$(top_srcdir)/include/ipmitool/ipmi_dcmi.h \
	$(top_srcdir)/include/ipmitool/ipmi_gendev.h \
	$(top_srcdir)/include/ipmitool/ipmi_main.h \
	$(top_srcdir)/include/ipmitool/ipmi_sel.h \
	$(top_srcdir)/include/ipmitool/log.h \
	$(top_srcdir)/include/ipmitool/ipmi_delloem.h \
	$(top_srcdir)/include/ipmitool/ipmi_mc.h \ 
	$(top_srcdir)/include/ipmitool/ipmi_sensor.h

EXTRA_DIST = $(COMMON_H) $(KERNEL_H) $(USER_H)

if CONFIG_KERNEL
kerneldir = @prefix@/src/zfs-$(VERSION)/include/ipmitool/
kernel_HEADERS = $(COMMON_H) $(KERNEL_H)
endif
