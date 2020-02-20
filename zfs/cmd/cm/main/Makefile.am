bin_PROGRAMS = ceres_cm
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
	./cmd/cm_cmd_server.c \
	./node/cm_node.c \
	./cm_main.c \
	./task/cm_task.c \
	./htbt/cm_htbt.c \
	./alarm/cm_alarm.c \
	./config/cm_cfg_global.c \
	./config/cm_cfg_omi.c \
	./pmm/cm_pmm_node.c \
	./pmm/cm_pmm_cluster.c \
	./pmm/cm_pmm_common.c \
	./pmm/cm_pmm_pool.c \
	./pmm/cm_pmm.c \
	./sche/cm_sche.c \
	./cnm/cm_cnm_pmm.c \
	./cnm/cm_cnm_phys.c \
	./cnm/cm_cnm_snapshot_clone.c \
	./cnm/cm_cnm_nas.c \
	./cnm/cm_cnm_node.c \
	./cnm/cm_cnm_migrate.c \
	./cnm/cm_cnm_aggr.c \
	./cnm/cm_cnm_common.c \
	./cnm/cm_cnm_dnsm.c \
	./cnm/cm_cnm_quota.c \
	./cnm/cm_cnm_topo_power.c \
	./cnm/cm_cnm_systime.c \
	./cnm/cm_cnm_snapshot.c \
	./cnm/cm_cnm_pool.c \
	./cnm/cm_cnm_clusternas.c \
	./cnm/cm_cnm_duallive.c \
	./cnm/cm_cnm_pmm_lun.c \
	./cnm/cm_cnm_expansion_cabinet.c \
	./cnm/cm_cnm_alarm.c \
	./cnm/cm_cnm_cluster.c \
	./cnm/cm_cnm_admin.c \
	./cnm/cm_cnm_demo.c \
	./cnm/cm_cnm_san.c \
	./cnm/cm_cnm_snapshot_backup.c \
	./cnm/cm_cnm_task_manager.c \
	./cnm/cm_cnm_user.c \
	./cnm/cm_cnm_dirquota.c \
	./cnm/cm_cnm_disk.c \
	./sync/cm_sync.c 
ceres_cm_LDADD = \
	$(top)/config/libcfg.la \
	$(top)/base/libcmbase.la
ceres_cm_CFLAGS = -std=gnu99 -lpthread -ldl -lcmopensrc -Wl,-rpath=$(prefix)/lib

