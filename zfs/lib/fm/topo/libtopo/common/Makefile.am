include $(top_srcdir)/config/Rules.am

DEFAULT_INCLUDES += \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/lib/libspl/include \
	-I$(top_srcdir)/lib/libdiskmgt/include	\
	-I$(top_srcdir)/lib/fm/topo/libtopo/common \
	-I$(top_srcdir)/cmd/fm/fmd/common/ \
	-I$(top_srcdir)/lib/libipmi \
	-I/usr/include/libxml2

lib_LTLIBRARIES = libtopo.la

USER_C = \
	topo_snap.c	\
	topo_builtin.c	\
	topo_mod.c	\
	topo_string.c	\
	topo_list.c	\
	topo_tree.c	\
	topo_file.c	\
	topo_protocol.c	\
	topo_alloc.c	\
	topo_2xml.c	\
	topo_fmri.c	\
	topo_method.c	\
	topo_module.c	\
	topo_nvl.c	\
	topo_prop.c	\
	topo_rtld.c	\
	topo_node.c	\
	topo_parse.c	\
	topo_subr.c	\
	topo_error.c	\
	topo_tables.c	\
	topo_xml.c	\
	cpu.c		\
	dev.c		\
	zfs.c		\
	fmd.c		\
	mem.c		\
	pkg.c		\
	hc.c		\
	legacy_hc.c	

EXTRA_DIST = \
	cpu.h \
	dev.h \
	fmd.h \
	hc.h \
	legacy_hc.h \
	LHL_DEBUG.h \
	libtopo.h \
	mem.h \
	mod.h \
	pkg.h \
	svc.h \
	topo_alloc.h \
	topo_builtin.h \
	topo_error.h \
	topo_file.h \
	topo_fmri.h \
	topo_fruhash.h \
	topo_hc.h \
	topo_list.h \
	topo_method.h \
	topo_mod.h \
	topo_module.h \
	topo_parse.h \
	topo_prop.h \
	topo_protocol.h \
	topo_string.h \
	topo_subr.h \
	topo_tree.h \
	zfs.h

libtopo_la_LIBADD = $(top_builddir)/lib/libnvpair/libnvpair.la	\
		    $(top_builddir)/lib/libipmi/libipmi.la	\
		    $(top_builddir)/lib/libzfs/libzfs.la	\
		    $(top_builddir)/lib/libstmf/libstmf.la

libtopo_la_LIBADD += -lxml2

dist_libtopo_la_SOURCES = \
	$(USER_C)
