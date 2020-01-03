lib_LTLIBRARIES = libcmjni.la
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
	-I$(top)/jni \
	-I$(top)/jni/config
libcmjni_la_SOURCES = \
	./cm_jni.c 
	../opensrc/json/arraylist.c \
	../opensrc/json/debug.c \
	../opensrc/json/json_c_version.c \
	../opensrc/json/json_object.c \
	../opensrc/json/json_object_iterator.c \
	../opensrc/json/json_pointer.c \
	../opensrc/json/json_tokener.c \
	../opensrc/json/json_util.c \
	../opensrc/json/json_visit.c \
	../opensrc/json/libjson.c \
	../opensrc/json/linkhash.c \
	../opensrc/json/printbuf.c \
	../opensrc/json/random_seed.c \
	../opensrc/json/strerror_override.c 
libcmjni_LDADD = \
	$(top)/config/libcfg.la \
	$(top)/base/libcmbase.la

