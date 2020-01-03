noinst_LTLIBRARIES = libcmbase.la
AM_CPPFLAGS = \
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
	./cmt/cm_cmt.c \
	./cmt/cm_cmt_client.c \
	./cmt/cm_cmt_server.c \
	./cm_base.c \
	./common/cm_common.c \
	./config/cm_cfg_public.c \
	./db/cm_db_sqlite3.c \
	./ini/cm_ini.c \
	./log/cm_log.c \
	./omi/cm_omi.c \
	./omi/cm_omi_client.c \
	./omi/cm_omi_json.c \
	./omi/cm_omi_server.c \
	./queue/cm_queue.c \
	./rpc/cm_rpc.c \
	./rpc/cm_rpc_client.c \
	./rpc/cm_rpc_server.c \
	./xml/cm_xml.c 
