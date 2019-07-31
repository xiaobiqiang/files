include $(top_srcdir)/config/Rules.am

VPATH = \
	$(top_srcdir)/cmd/common	\
	$(top_srcdir)/module/idm

DEFAULT_INCLUDES += \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/lib/libspl/include \
	-I$(top_srcdir)/cmd/common

sbin_PROGRAMS = itadm

itadm_SOURCES = \
	base64.c	\
	bufsplit.c	\
	itadm.c
	
itadm_LDADD = \
	$(top_builddir)/lib/libnvpair/libnvpair.la \
	$(top_builddir)/lib/libiscsit/libiscsit.la \
	$(top_builddir)/lib/libstmf/libstmf.la 

itadm_LDADD += $(ZLIB)
