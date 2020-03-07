include $(top_srcdir)/config/Rules.am
VPATH = \
        $(top_srcdir)/cmd/common

DEFAULT_INCLUDES += \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/lib/libspl/include\
	-I$(top_srcdir)/cmd/common

sbin_PROGRAMS = iscsitsvc

iscsitsvc_SOURCES = \
	iscsitsvc.c \
	../common/cmdparse.c

iscsitsvc_LDADD = \
	$(top_builddir)/lib/libnvpair/libnvpair.la
iscsitsvc_LDFLAGS = -pthread
