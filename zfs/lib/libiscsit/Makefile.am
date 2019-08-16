include $(top_srcdir)/config/Rules.am

VPATH = $(top_srcdir)/module/iscsit/

AM_CFLAGS += $(DEBUG_STACKFLAGS) $(FRAME_LARGER_THAN)

DEFAULT_INCLUDES += \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/lib/libspl/include

noinst_LTLIBRARIES = libiscsit.la

USER_C = \
	libiscsit.c

KERNEL_C = \
	iscsit_common.c

nodist_libiscsit_la_SOURCES = \
	$(USER_C) \
	$(KERNEL_C)
	
libiscsit_la_LIBADD = \
	$(top_builddir)/lib/libshare/libshare.la \
	$(top_builddir)/lib/libnvpair/libnvpair.la \
	$(top_builddir)/lib/libcomm/libcomm.la 

EXTRA_DIST = $(USER_C)
