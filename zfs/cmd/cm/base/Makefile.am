include $(top_srcdir)/config/Rules.am

DEFAULT_INCLUDES += \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/lib/libspl/include

noinst_LTLIBRARIES = libcmbase.la
AM_CPPFLAGS += \
	-I$(top)/base/config \
	-I$(top)/base/rpc \
	-I$(top)/base/common \
	-I$(top)/base/cmt \
	-I$(top)/base/xml \
	-I$(top)/base/db \
	-I$(top)/base/log \
	-I$(top)/base/include \
	-I$(top)/base/ini \
	-I$(top)/base/queue \
	-I$(top)/base/omi \
	-I$(top)/base/config \
	-I$(top)/base \
	-I$(top)/opensrc/sqlite3 \
	-I$(top)/opensrc/json \
	-I$(top)/opensrc/mxml \
	-I$(top)/opensrc/iniparser \
	-I$(top)/main/node
libcmbase_la_SOURCES = \
	./xml/cm_xml.c \
	./cmt/cm_cmt_server.c \
	./cmt/cm_cmt_client.c \
	./cmt/cm_cmt.c \
	./rpc/cm_rpc_server.c \
	./rpc/cm_rpc_client.c \
	./rpc/cm_rpc.c \
	./omi/cm_omi_client.c \
	./omi/cm_omi.c \
	./omi/cm_omi_server.c \
	./omi/cm_omi_json.c \
	./queue/cm_queue.c \
	./config/cm_cfg_public.c \
	./db/cm_db_sqlite3.c \
	./cm_base.c \
	./ini/cm_ini.c \
	./log/cm_log.c \
	./common/cm_common.c 
EXTRA_DIST = \
	./cm_base.h \
	./xml/cm_xml.h \
	./cmt/cm_cmt.h \
	./rpc/cm_rpc.h \
	./omi/cm_omi.h \
	./omi/cm_omi_types.h \
	./include/cm_sys.h \
	./include/cm_platform.h \
	./include/cm_types.h \
	./include/cm_define.h \
	./queue/cm_queue.h \
	./config/cm_cfg_public.h \
	./db/cm_db.h \
	./ini/cm_ini.h \
	./log/cm_log.h \
	./common/cm_common.h 
