include $(top_srcdir)/config/Rules.am

VPATH = $(top_srcdir)/module/iscsit/

AM_CFLAGS += $(DEBUG_STACKFLAGS) $(FRAME_LARGER_THAN)

DEFAULT_INCLUDES += \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/lib/libspl/include

noinst_LTLIBRARIES = libiscsit.la

USER_C =

KERNEL_C = \
	iscsit_common.c

nodist_libavl_la_SOURCES = \
	$(USER_C) \
	$(KERNEL_C)

EXTRA_DIST = $(USER_C)
