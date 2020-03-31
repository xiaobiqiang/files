include $(top_srcdir)/config/Rules.am

DEFAULT_INCLUDES += \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/lib/libspl/include 

sbin_PROGRAMS = ipmisvc 

ipmisvc_SOURCES = 	\
	ini_parse.c \
	ipmi_checker.c \
	power_checker.c \
	ini_parse.h \
	ipmi_checker.h \
	ipmi_defines.h \
	power_checker.h

EXTRA_DIST = \
	ipmi_checker.ini

ipmisvc_LDADD =	\
	$(top_builddir)/lib/libzfs_core/libzfs_core.la \
	$(top_builddir)/lib/libspl/libspl.la
ipmisvc_LDADD += $(ZLIB) -lxml2 -lrt
ipmisvc_LDFLAGS = -pthread

install-exec-hook:
	$(INSTALL) ./ipmi_checker.ini /etc/

