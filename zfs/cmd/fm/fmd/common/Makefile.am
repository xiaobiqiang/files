include $(top_srcdir)/config/Rules.am

VPATH = \
	$(top_srcdir)/cmd/common

DEFAULT_INCLUDES += 					\
        -I$(top_srcdir)/include 			\
        -I$(top_srcdir)/lib/libspl/include 		\
		-I$(top_srcdir)/lib/libipmi			\
        -I$(top_srcdir)/cmd/fm/fmd/common 		\
        -I$(top_srcdir)/lib/fm/topo/libtopo/common	\
        -I$(top_srcdir)/lib/fm/libexacct/common		\
        -I$(top_srcdir)/lib/fm/libdiagcode/common	\
        -I$(top_srcdir)/lib/fm/libfmd_msg/common	\
		-I$(top_srcdir)/cmd/common

sbin_PROGRAMS = fmd

fmd_SOURCES =		\
	fmd_main.c	\
	fmd.c		\
	fmd_api.c	\
	fmd_module.c	\
	fmd_subr.c	\
	fmd_conf.c	\
	fmd_log.c	\
	fmd_string.c	\
	fmd_list.c	\
	fmd_nv.c	\
	fmd_topo.c	\
	fmd_protocol.c	\
	fmd_alloc.c	\
	fmd_error.c	\
	fmd_thread.c	\
	fmd_trace.c	\
	fmd_buf.c	\
	fmd_self.c	\
	fmd_builtin.c	\
	fmd_time.c	\
	fmd_timerq.c	\
	fmd_event.c	\
	fmd_eventq.c	\
	fmd_case.c	\
	fmd_scheme.c	\
	fmd_dispq.c	\
	fmd_ctl.c	\
	fmd_fmri.c	\
	fmd_idspace.c	\
	fmd_ustat.c	\
	fmd_xprt.c	\
	fmd_ckpt.c	\
	fmd_asru.c	\
	fmd_serd.c	\
	fmd_rtld.c	\
	fmd_proc.c	\
	fmd_dr.c	\
	fmd_channel.c	\
	fmd_rpc.c	\
	fmd_svc_adm.c	\
	fmd_svc_api.c	\
	fmd_xdr_adm.c	\
	fmd_xdr_api.c 	\
	fmd_rpc_adm.c	\
	fmd_rpc_api.c	\
	../../../common/systemd_util.c

EXTRA_DIST = \
	fmd_alloc.h \
	fmd_api.h \
	fmd_asru.h \
	fmd_buf.h \
	fmd_builtin.h \
	fmd_case.h \
	fmd_channel.h \
	fmd_ckpt.h \
	fmd_conf.h \
	fmd_ctl.h \
	fmd_dispq.h \
	fmd_dr.h \
	fmd_error.h \
	fmd_event.h \
	fmd_eventq.h \
	fmd_fmri.h \
	fmd.h \
	fmd_idspace.h \
	fmd_list.h \
	fmd_log.h \
	fmd_module.h \
	fmd_protocol.h \
	fmd_rpc_adm.h \
	fmd_rpc_api.h \
	fmd_rpc.h \
	fmd_scheme.h \
	fmd_serd.h \
	fmd_string.h \
	fmd_subr.h \
	fmd_thread.h \
	fmd_time.h \
	fmd_timerq.h \
	fmd_topo.h \
	fmd_trace.h \
	fmd_ustat.h \
	fmd_xprt.h \
	fmd_api.map \
	fmd_fmri.map

fmd_LDADD = \
	$(top_builddir)/lib/libnvpair/libnvpair.la			\
        $(top_builddir)/lib/libzpool/libzpool.la 			\
        $(top_builddir)/lib/libzfs/libzfs.la 				\
        $(top_builddir)/lib/libspl/libspl.la 				\
        $(top_builddir)/lib/fm/libdiagcode/common/libdiagcode.la	\
        $(top_builddir)/lib/fm/libexacct/common/libexacct.la		\
        $(top_builddir)/lib/fm/libfmd_msg/common/libfmd_msg.la		\
        $(top_builddir)/lib/fm/libfmd_adm/common/libfmd_adm.la		\
        $(top_builddir)/lib/libfmd/libfmd.la				\
        $(top_builddir)/lib/libipmi/libipmi.la				\
        $(top_builddir)/lib/fm/topo/libtopo/common/libtopo.la 		\
        $(top_builddir)/lib/libstmf/libstmf.la 


fmd_LDFLAGS = -lpthread -ldl -lxml2					\
		-Wl,--dynamic-list=./fmd_api.map			\
		-Wl,--dynamic-list=./fmd_fmri.map
