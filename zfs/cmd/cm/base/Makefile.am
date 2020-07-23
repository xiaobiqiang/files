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
	./rpc/cm_rpc_server.c \
	./rpc/cm_rpc_client.c \
	./rpc/cm_rpc.c \
	./cmt/cm_cmt_server.c \
	./cmt/cm_cmt.c \
	./cmt/cm_cmt_client.c \
	./common/cm_common.c \
	./xml/cm_xml.c \
	./db/cm_db_sqlite3.c \
	./ini/cm_ini.c \
	./log/cm_log.c \
	./config/cm_cfg_public.c \
	./omi/cm_omi_client.c \
	./omi/cm_omi_server.c \
	./omi/cm_omi.c \
	./omi/cm_omi_json.c \
	./queue/cm_queue.c \
	./cm_base.c 
EXTRA_DIST = \
	./rpc/cm_rpc.h \
	./cmt/cm_cmt.h \
	./common/cm_common.h \
	./xml/cm_xml.h \
	./db/cm_db.h \
	./ini/cm_ini.h \
	./include/cm_platform.h \
	./include/cm_sys.h \
	./include/cm_define.h \
	./include/cm_types.h \
	./log/cm_log.h \
	./config/cm_cfg_public.h \
	./cm_base.h \
	./omi/cm_omi.h \
	./omi/cm_omi_types.h \
	./queue/cm_queue.h 
