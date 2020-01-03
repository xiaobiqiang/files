bin_PROGRAMS = ceres_cli
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
	-I$(top)/config \
	-I$(top)/cli \
	-I$(top)/cli/config
ceres_cli_SOURCES = \
	./cm_cli.c \
	./cm_main_cli.c 
ceres_cli_LDADD = \
	$(top)/config/libcfg.la \
	$(top)/base/libcmbase.la
ceres_cli_CFLAGS = -std=c99 -lpthread -ldl -lcmopensrc -Wl,-rpath=$(prefix)/lib

