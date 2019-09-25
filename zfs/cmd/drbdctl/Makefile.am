include $(top_srcdir)/config/Rules.am
VPATH = \
        $(top_srcdir)/cmd/common
		
DEFAULT_INCLUDES += \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/lib/libspl/include 

sbin_PROGRAMS = drbdctl 

drbdctl_SOURCES = 	\
	drbdctl.c	\
	cmdparse.c

drbdctl_LDADD = \
	$(top_builddir)/lib/libzfs_core/libzfs_core.la
drbdctl_LDADD += $(ZLIB) -lxml2 -lrt
drbdctl_LDFLAGS = -pthread

