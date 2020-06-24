include $(top_srcdir)/config/Rules.am

DEFAULT_INCLUDES += \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/lib/libspl/include

bin_PROGRAMS = ceres_cm
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
	-I$(top)/main \
	-I$(top)/main/alarm \
	-I$(top)/main/cmd \
	-I$(top)/main/cnm \
	-I$(top)/main/config \
	-I$(top)/main/htbt \
	-I$(top)/main/node \
	-I$(top)/main/pmm \
	-I$(top)/main/sche \
	-I$(top)/main/sync \
	-I$(top)/main/task
ceres_cm_SOURCES = \
	./sche/cm_sche.c \
	./pmm/cm_pmm_cluster.c \
	./pmm/cm_pmm_common.c \
	./pmm/cm_pmm.c \
	./pmm/cm_pmm_node.c \
	./pmm/cm_pmm_pool.c \
	./alarm/cm_alarm.c \
	./task/cm_task.c \
	./sync/cm_sync.c \
	./cm_main.c \
	./node/cm_node.c \
	./cnm/cm_cnm_task_manager.c \
	./cnm/cm_cnm_dirquota.c \
	./cnm/cm_cnm_phys.c \
	./cnm/cm_cnm_duallive.c \
	./cnm/cm_cnm_user.c \
	./cnm/cm_cnm_nas.c \
	./cnm/cm_cnm_expansion_cabinet.c \
	./cnm/cm_cnm_clusternas.c \
	./cnm/cm_cnm_admin.c \
	./cnm/cm_cnm_demo.c \
	./cnm/cm_cnm_migrate.c \
	./cnm/cm_cnm_dnsm.c \
	./cnm/cm_cnm_san.c \
	./cnm/cm_cnm_node.c \
	./cnm/cm_cnm_pmm_lun.c \
	./cnm/cm_cnm_pmm.c \
	./cnm/cm_cnm_quota.c \
	./cnm/cm_cnm_disk.c \
	./cnm/cm_cnm_systime.c \
	./cnm/cm_cnm_common.c \
	./cnm/cm_cnm_snapshot_clone.c \
	./cnm/cm_cnm_topo_power.c \
	./cnm/cm_cnm_aggr.c \
	./cnm/cm_cnm_pool.c \
	./cnm/cm_cnm_snapshot_backup.c \
	./cnm/cm_cnm_cluster.c \
	./cnm/cm_cnm_snapshot.c \
	./cnm/cm_cnm_alarm.c \
	./config/cm_cfg_global.c \
	./config/cm_cfg_omi.c \
	./htbt/cm_htbt.c \
	./cmd/cm_cmd_server.c 
EXTRA_DIST = \
	./sche/cm_sche.h \
	./pmm/cm_pmm_common.h \
	./pmm/cm_pmm_types.h \
	./pmm/cm_pmm_pool.h \
	./pmm/cm_pmm.h \
	./alarm/cm_alarm.h \
	./alarm/cm_alarm_types.h \
	./task/cm_task.h \
	./sync/cm_sync.h \
	./sync/cm_sync_types.h \
	./node/cm_node.h \
	./cnm/cm_cnm_task_manager.h \
	./cnm/cm_cnm_cluster.h \
	./cnm/cm_cnm_san.h \
	./cnm/cm_cnm_quota.h \
	./cnm/cm_cnm_topo_power.h \
	./cnm/cm_cnm_duallive.h \
	./cnm/cm_cnm_pmm.h \
	./cnm/cm_cnm_snapshot_clone.h \
	./cnm/cm_cnm_common.h \
	./cnm/cm_cnm_expansion_cabinet.h \
	./cnm/cm_cnm_phys.h \
	./cnm/cm_cnm_admin.h \
	./cnm/cm_cnm_snapshot.h \
	./cnm/cm_cnm_dirquota.h \
	./cnm/cm_cnm_pmm_lun.h \
	./cnm/cm_cnm_node.h \
	./cnm/cm_cnm_user.h \
	./cnm/cm_cnm_alarm.h \
	./cnm/cm_cnm_clusternas.h \
	./cnm/cm_cnm_aggr.h \
	./cnm/cm_cnm_pool.h \
	./cnm/cm_cnm_nas.h \
	./cnm/cm_cnm_migrate.h \
	./cnm/cm_cnm_dnsm.h \
	./cnm/cm_cnm_demo.h \
	./cnm/cm_cnm_disk.h \
	./cnm/cm_cnm_systime.h \
	./cnm/cm_cnm_snapshot_backup.h \
	./config/cm_cfg_local.h \
	./config/cm_cfg_global.h \
	./htbt/cm_htbt.h \
	./cmd/cm_cmd_server.h 
ceres_cm_LDADD = \
	$(top)/config/libcfg.la \
	$(top)/base/libcmbase.la
ceres_cm_CFLAGS = -std=gnu99 -lpthread -ldl -L../opensrc -lcmopensrc -Wl,-rpath=$(prefix)/lib

