include $(top_srcdir)/config/Rules.am

DEFAULT_INCLUDES += 					\
        -I$(top_srcdir)/include 			\
        -I$(top_srcdir)/lib/libspl/include

sbin_PROGRAMS = fm_simulate

fm_simulate_SOURCES =		\
	fm_simulate.c	

fm_simulate_LDADD = \
	$(top_builddir)/lib/libzfs/libzfs.la	\
	$(top_builddir)/lib/libstmf/libstmf.la
