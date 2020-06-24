include $(top_srcdir)/config/Rules.am

VPATH = \
	$(top_srcdir)/cmd/common

DEFAULT_INCLUDES += \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/lib/libspl/include \
	-I$(top_srcdir)/cmd/common

sbin_PROGRAMS = mltsas_svc

mltsas_svc_SOURCES = \
	mltsas_svc.c 	\
	../common/cmdparse.c

mltsas_svc_LDADD = \
	$(top_builddir)/lib/libnvpair/libnvpair.la \
	$(top_builddir)/lib/libuutil/libuutil.la \
	$(top_builddir)/lib/libstmf/libstmf.la \
	$(top_builddir)/lib/libzpool/libzpool.la \
	$(top_builddir)/lib/libzfs/libzfs.la 

