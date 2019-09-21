include $(top_srcdir)/config/Rules.am

DEFAULT_INCLUDES += \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/lib/libspl/include 

sbin_PROGRAMS = ipmisvc 

ipmisvc_SOURCES = 	\
	ini_parse.c \
	ipmi_checker.c \
	power_checker.c

ipmisvc_LDADD = $(ZLIB) -lxml2 -lrt
ipmisvc_LDFLAGS = -pthread

install-exec-hook:
	rm -rf /etc/ipmi_checker.ini
	cp $(top_srcdir)/cmd/ipmi_checker/ipmi_checker.ini /etc/

