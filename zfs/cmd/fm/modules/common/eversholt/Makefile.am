include $(top_srcdir)/config/Rules.am

SUBDIRS=files

libeversholt_ladir=/usr/local/lib/fmd

DEFAULT_INCLUDES += \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/lib/libspl/include	\
	-I$(top_srcdir)/cmd/fm/modules/common/eversholt	\
	-I$(top_srcdir)/cmd/fm/fmd/common/		\
	-I$(top_srcdir)/lib/fm/topo/libtopo/common/

libeversholt_la_LTLIBRARIES = libeversholt.la

USER_C = \
	eft.c		\
	alloc.c		\
	config.c	\
	eval.c		\
	evnv.c		\
	fme.c		\
	iexpr.c		\
	io.c		\
	ipath.c		\
	itree.c		\
	platform.c	\
	stats.c		\
	check.c		\
	esclex.c	\
	literals.c	\
	lut.c		\
	out.c		\
	ptree.c		\
	stable.c	\
	tree.c		\
	eftread.c	\
	y_tab.c

libeversholt_la_LIBADD = 				\
	$(top_builddir)/lib/libuutil/libuutil.la	
	$(top_builddir)/lib/libzfs/libzfs.la 

dist_libeversholt_la_SOURCES = \
	$(USER_C) \
	alloc.h \
	check.h \
	config.h \
	config_impl.h \
	eft.h \
	eftread.h \
	esclex.h \
	eval.h \
	evnv.h \
	fme.h \
	iexpr.h \
	io.h \
	ipath.h \
	ipath_impl.h \
	itree.h \
	literals.h \
	lut.h \
	lut_impl.h \
	out.h \
	platform.h \
	ptree.h \
	stable.h \
	stats.h \
	stats_impl.h \
	tree.h \
	version.h \
	y_tab.h
