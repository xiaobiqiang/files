bin_PROGRAMS = ceres_cmd
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
	-I$(top)/cmd \
	-I$(top)/cmd/config
ceres_cmd_SOURCES = \
	./cm_cmd_client.c \
	./cm_main_cmd.c 
ceres_cmd_LDADD = \
	$(top)/base/libcmbase.la
ceres_cmd_CFLAGS = -std=c99 -lpthread -ldl -lcmopensrc -Wl,-rpath=$(prefix)/lib

