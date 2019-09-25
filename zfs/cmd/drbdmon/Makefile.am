include $(top_srcdir)/config/Rules.am

DEFAULT_INCLUDES += \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/lib/libspl/include 

sbin_PROGRAMS = drbdmonsvc 

drbdmonsvc_SOURCES = 	\
	drbdmon.c

drbdmonsvc_LDADD = \
	$(top_builddir)/lib/libzfs_core/libzfs_core.la
drbdmonsvc_LDADD += $(ZLIB) -lxml2 -lrt
drbdmonsvc_LDFLAGS = -pthread

