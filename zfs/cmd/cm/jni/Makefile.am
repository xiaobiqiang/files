include $(top_srcdir)/config/Rules.am

DEFAULT_INCLUDES += \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/lib/libspl/include

lib_LTLIBRARIES = libcmjni.la
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
	-I$(top)/config \
	-I/usr/lib/jvm/java-1.8.0-swjdk-H318P.sw_64/include/linux/ \
	-I/usr/lib/jvm/java-1.8.0-swjdk-H318P.sw_64/include/ \
	-I/usr/lib/jvm/java-1.8.0-openjdk-sw_64/include/ \
	-I/usr/lib/jvm/java-1.8.0-openjdk-sw_64/include/linux/ \
	-I$(top)/jni \
	-I$(top)/jni/config
libcmjni_la_SOURCES = \
	./cm_jni.c \
	../base/xml/cm_xml.c \
	../base/cmt/cm_cmt_server.c \
	../base/cmt/cm_cmt_client.c \
	../base/cmt/cm_cmt.c \
	../base/rpc/cm_rpc_server.c \
	../base/rpc/cm_rpc_client.c \
	../base/rpc/cm_rpc.c \
	../base/omi/cm_omi_client.c \
	../base/omi/cm_omi.c \
	../base/omi/cm_omi_server.c \
	../base/omi/cm_omi_json.c \
	../base/queue/cm_queue.c \
	../base/config/cm_cfg_public.c \
	../base/db/cm_db_sqlite3.c \
	../base/cm_base.c \
	../base/ini/cm_ini.c \
	../base/log/cm_log.c \
	../base/common/cm_common.c \
	../config/cm_cfg_cnm.c \
	../config/cm_cfg_cnm_phys.c \
	../config/cm_cfg_cnm_nfs.c \
	../config/cm_cfg_cnm_group.c \
	../config/cm_cfg_cnm_duallive_nas.c \
	../config/cm_cfg_cnm_clusternas.c \
	../config/cm_cfg_cnm_snapshot.c \
	../config/cm_cfg_cnm_node.c \
	../config/cm_cfg_cnm_nas.c \
	../config/cm_cfg_cnm_pmm_lun.c \
	../config/cm_cfg_cnm_pmm.c \
	../config/cm_cfg_cnm_alarm.c \
	../config/cm_cfg_cnm_threshold.c \
	../config/cm_cfg_cnm_demo.c \
	../config/cm_cfg_cnm_dnsm.c \
	../config/cm_cfg_cnm_admin.c \
	../config/cm_cfg_cnm_quota.c \
	../config/cm_cfg_cnm_dirquota.c \
	../config/cm_cfg_cnm_pmm_pool.c \
	../config/cm_cfg_cnm_aggr.c \
	../config/cm_cfg_cnm_pool.c \
	../config/cm_cfg_cnm_san.c \
	../config/cm_cfg_cnm_migrate.c \
	../config/cm_cfg_cnm_user.c \
	../config/cm_cfg_cnm_clustat.c \
	../config/cm_cfg_cnm_snapshot_backup.c \
	../config/cm_cfg_cnm_task_manager.c \
	../config/cm_cfg_cnm_topo_power.c \
	../config/cm_cfg_cnm_cluster.c \
	../config/cm_cfg_cnm_disk.c \
	../config/cm_cfg_cnm_snapshot_clone.c \
	../config/cm_cfg_cnm_systime.c \
	../opensrc/json/random_seed.c \
	../opensrc/json/json_pointer.c \
	../opensrc/json/libjson.c \
	../opensrc/json/linkhash.c \
	../opensrc/json/debug.c \
	../opensrc/json/printbuf.c \
	../opensrc/json/arraylist.c \
	../opensrc/json/json_c_version.c \
	../opensrc/json/json_object_iterator.c \
	../opensrc/json/json_util.c \
	../opensrc/json/json_tokener.c \
	../opensrc/json/json_visit.c \
	../opensrc/json/strerror_override.c \
	../opensrc/json/json_object.c 
EXTRA_DIST = \
	./config/cm_cfg_global.h \
	./cm_jni.h 
libcmjni_la_CFLAGS = -std=gnu99 -lpthread -ldl
