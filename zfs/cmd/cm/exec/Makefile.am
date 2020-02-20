bin_PROGRAMS = ceres_exec
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
	-I$(top)/exec \
	-I$(top)/exec/config
ceres_exec_SOURCES = \
	./cm_main_exec.c 
ceres_exec_LDADD = \
	$(top)/base/libcmbase.la
ceres_exec_CFLAGS = -std=gnu99 -lpthread -ldl -lcmopensrc -Wl,-rpath=$(prefix)/lib

