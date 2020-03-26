include $(top_srcdir)/config/Rules.am

DEFAULT_INCLUDES += \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/lib/libspl/include

sbin_PROGRAMS = mpt3ctl

mpt3ctl_SOURCES = 	\
	mpt3ctl.c 



mpt3ctl_LDADD = $(ZLIB) -lrt
mpt3ctl_LDFLAGS = -pthread
