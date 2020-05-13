/******************************************************************************
 *          Copyright (c) 2017 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cfg_omi.c
 * author     : wbn
 * create date: 2017年10月26日
 * description: TODO:
 *
 *****************************************************************************/
#include "cm_omi_types.h"
#include "cm_cnm_demo.h"
#include "cm_omi.h"
#include "cm_cnm_node.h"
#include "cm_cnm_cluster.h"
#include "cm_cnm_pmm.h"
#include "cm_cnm_alarm.h"
#include "cm_cnm_systime.h"
#include "cm_cnm_phys.h"
#include "cm_cnm_admin.h"
#include "cm_cnm_disk.h"
#include "cm_cnm_pool.h"
#include "cm_cnm_san.h"
#include "cm_cnm_snapshot.h"
#include "cm_cnm_nas.h"
#include "cm_cnm_user.h"
#include "cm_cnm_snapshot_backup.h"
#include "cm_cnm_task_manager.h"
#include "cm_cnm_quota.h"
#include "cm_cnm_snapshot_clone.h"
#include "cm_cnm_clusternas.h"
#include "cm_cnm_duallive.h"
#include "cm_cnm_migrate.h"
#include "cm_cnm_aggr.h"
#include "cm_cnm_dnsm.h"
#include "cm_pmm_pool.h"
#include "cm_cnm_pmm_lun.h"
#include "cm_cnm_dirquota.h"
#include "cm_cnm_topo_power.h"

const cm_omi_object_cmd_cfg_t g_CmOmiObjectCfgCluster[] =
{
    /* get */
    {
        CM_OMI_CMD_GET, CM_OMI_PERMISSION_ALL,
        NULL, cm_cnm_cluster_get, cm_cnm_cluster_encode,
        NULL
    },
    /* modify */
    {
        CM_OMI_CMD_MODIFY, CM_OMI_PERMISSION_ALL,
        cm_cnm_cluster_decode, cm_cnm_cluster_modify, NULL,
        cm_cnm_cluster_oplog_modify
    },
    /* power off */
    {
        CM_OMI_CMD_OFF, CM_OMI_PERMISSION_ALL,
        cm_cnm_cluster_decode, cm_cnm_cluster_power_off, NULL,
        NULL
    },
    /* reboot */
    {
        CM_OMI_CMD_REBOOT, CM_OMI_PERMISSION_ALL,
        cm_cnm_cluster_decode, cm_cnm_cluster_reboot, NULL,
        NULL
    },
};
const cm_omi_object_cmd_cfg_t g_CmOmiObjectCfgClusterStat[] =
{
    /* get */
    {

        CM_OMI_CMD_GET, CM_OMI_PERMISSION_ALL,
        NULL, cm_cnm_cluster_stat_get, cm_cnm_cluster_stat_encode,
        NULL
    },

};

const cm_omi_object_cmd_cfg_t g_CmOmiObjectCfgNode[] =
{
    /* get */
    {
        CM_OMI_CMD_GET, CM_OMI_PERMISSION_ALL,
        cm_cnm_node_decode, cm_cnm_node_get, cm_cnm_node_encode,
        NULL
    },
    /* get batch */
    {
        CM_OMI_CMD_GET_BATCH, CM_OMI_PERMISSION_ALL,
        cm_cnm_node_decode, cm_cnm_node_get_batch, cm_cnm_node_encode,
        NULL
    },
    /* count */
    {
        CM_OMI_CMD_COUNT, CM_OMI_PERMISSION_ALL,
        NULL, cm_cnm_node_count, cm_omi_encode_count,
        NULL
    },
    /* modify */
    {
        CM_OMI_CMD_MODIFY, CM_OMI_PERMISSION_ALL,
        cm_cnm_node_decode, cm_cnm_node_modify, NULL,
        cm_cnm_node_oplog_modify
    },
    {
        CM_OMI_CMD_ADD, CM_OMI_PERMISSION_ALL,
        cm_cnm_node_decode, cm_cnm_node_add, NULL,
        cm_cnm_node_oplog_add
    },
    {
        CM_OMI_CMD_DELETE, CM_OMI_PERMISSION_ALL,
        cm_cnm_node_decode, cm_cnm_node_delete, NULL,
        cm_cnm_node_oplog_delete
    },
    {
        CM_OMI_CMD_ON, CM_OMI_PERMISSION_ALL,
        cm_cnm_node_decode, cm_cnm_node_power_on, NULL,
        cm_cnm_node_oplog_on
    },
    {
        CM_OMI_CMD_OFF, CM_OMI_PERMISSION_ALL,
        cm_cnm_node_decode, cm_cnm_node_power_off, NULL,
        cm_cnm_node_oplog_off
    },
    {
        CM_OMI_CMD_REBOOT, CM_OMI_PERMISSION_ALL,
        cm_cnm_node_decode, cm_cnm_node_reboot, NULL,
        cm_cnm_node_oplog_reboot
    },
};

const cm_omi_object_cmd_cfg_t g_CmOmiObjectCfgPmmCluster[] =
{
    /* get */
    {
        CM_OMI_CMD_GET, CM_OMI_PERMISSION_ALL,
        cm_cnm_pmm_cluster_decode, cm_cnm_pmm_cluster_current, cm_cnm_pmm_cluster_encode,
        NULL
    },
    /* get batch */
    {
        CM_OMI_CMD_GET_BATCH, CM_OMI_PERMISSION_ALL,
        cm_cnm_pmm_cluster_decode, cm_cnm_pmm_cluster_history, cm_cnm_pmm_cluster_encode,
        NULL
    }
};

const cm_omi_object_cmd_cfg_t g_CmOmiObjectCfgPmmNode[] =
{
    /* get */
    {
        CM_OMI_CMD_GET, CM_OMI_PERMISSION_ALL,
        cm_cnm_pmm_node_decode, cm_cnm_pmm_node_current, cm_cnm_pmm_node_encode,
        NULL
    },
    /* get batch */
    {
        CM_OMI_CMD_GET_BATCH, CM_OMI_PERMISSION_ALL,
        cm_cnm_pmm_node_decode, cm_cnm_pmm_node_history, cm_cnm_pmm_node_encode,
        NULL
    }
};

const cm_omi_object_cmd_cfg_t g_CmOmiObjectCfgAlarmCurrent[] =
{
    /* get */
    {
        CM_OMI_CMD_GET, CM_OMI_PERMISSION_ALL,
        cm_cnm_alarm_decode, cm_cnm_alarm_get, cm_cnm_alarm_encode,
        NULL
    },
    /* get batch */
    {
        CM_OMI_CMD_GET_BATCH, CM_OMI_PERMISSION_ALL,
        cm_cnm_alarm_decode, cm_cnm_alarm_current_getbatch, cm_cnm_alarm_encode,
        NULL
    },
    /* count */
    {
        CM_OMI_CMD_COUNT, CM_OMI_PERMISSION_ALL,
        cm_cnm_alarm_decode, cm_cnm_alarm_current_count, cm_omi_encode_count,
        NULL
    },
};

const cm_omi_object_cmd_cfg_t g_CmOmiObjectCfgAlarmHistory[] =
{
    /* get batch */
    {
        CM_OMI_CMD_GET_BATCH, CM_OMI_PERMISSION_ALL,
        cm_cnm_alarm_decode, cm_cnm_alarm_history_getbatch, cm_cnm_alarm_encode,
        NULL
    },
    /* get */
    {
        CM_OMI_CMD_GET, CM_OMI_PERMISSION_ALL,
        cm_cnm_alarm_decode, cm_cnm_alarm_get, cm_cnm_alarm_encode,
        NULL
    },
    /* count */
    {
        CM_OMI_CMD_COUNT, CM_OMI_PERMISSION_ALL,
        cm_cnm_alarm_decode, cm_cnm_alarm_history_count, cm_omi_encode_count,
        NULL
    },
    /* delete 不记录操作日志，避免删除操作日志时又写操作日志 */
    {
        CM_OMI_CMD_DELETE, CM_OMI_PERMISSION_ALL,
        cm_cnm_alarm_decode, cm_cnm_alarm_history_delete, NULL,
        NULL
    },
};

const cm_omi_object_cmd_cfg_t g_CmOmiObjectCfgAlarmConfig[] =
{
    /* get */
    {
        CM_OMI_CMD_GET, CM_OMI_PERMISSION_ALL,
        cm_cnm_alarm_cfg_decode, cm_cnm_alarm_cfg_get, cm_cnm_alarm_cfg_encode,
        NULL
    },
    /* get batch */
    {
        CM_OMI_CMD_GET_BATCH, CM_OMI_PERMISSION_ALL,
        cm_cnm_alarm_cfg_decode, cm_cnm_alarm_cfg_getbatch, cm_cnm_alarm_cfg_encode,
        NULL
    },
    /* count */
    {
        CM_OMI_CMD_COUNT, CM_OMI_PERMISSION_ALL,
        cm_cnm_alarm_cfg_decode, cm_cnm_alarm_cfg_count, cm_omi_encode_count,
        NULL
    },
    /* update */
    {
        CM_OMI_CMD_MODIFY, CM_OMI_PERMISSION_ALL,
        cm_cnm_alarm_cfg_decode, cm_cnm_alarm_cfg_update, NULL,
        cm_cnm_alarm_cfg_oplog_update
    },
};

const cm_omi_object_cmd_cfg_t g_CmOmiObjectSysTimeConfig[] =
{
    /* get */
    {
        CM_OMI_CMD_GET, CM_OMI_PERMISSION_ALL,
        cm_cnm_systime_decode, cm_cnm_systime_get, cm_cnm_systime_encode,
        NULL
    },
    /* update */
    {
        CM_OMI_CMD_MODIFY, CM_OMI_PERMISSION_ALL,
        cm_cnm_systime_decode, cm_cnm_systime_set, NULL,
        cm_cnm_systime_oplog_set
    },
};

const cm_omi_object_cmd_cfg_t g_CmOmiObjectPhysConfig[] =
{
    /* get */
    {
        CM_OMI_CMD_GET, CM_OMI_PERMISSION_ALL,
        cm_cnm_phys_decode, cm_cnm_phys_get, cm_cnm_phys_encode,
        NULL
    },
    /* getbatch */
    {
        CM_OMI_CMD_GET_BATCH, CM_OMI_PERMISSION_ALL,
        cm_cnm_phys_decode, cm_cnm_phys_get_batch, cm_cnm_phys_encode,
        NULL
    },
    /* count */
    {
        CM_OMI_CMD_COUNT, CM_OMI_PERMISSION_ALL,
        cm_cnm_phys_decode, cm_cnm_phys_count, cm_omi_encode_count,
        NULL
    },
    /* update */
    {
        CM_OMI_CMD_MODIFY, CM_OMI_PERMISSION_ALL,
        cm_cnm_phys_decode, cm_cnm_phys_update, NULL,
        NULL
    },
};

const cm_omi_object_cmd_cfg_t g_CmOmiObjectCfgPmmConfig[] =
{
    /* get */
    {
        CM_OMI_CMD_GET, CM_OMI_PERMISSION_ALL,
        NULL, cm_cnm_pmm_cfg_get, cm_cnm_pmm_cfg_encode,
        NULL
    },
    /* update */
    {
        CM_OMI_CMD_MODIFY, CM_OMI_PERMISSION_ALL,
        cm_cnm_pmm_cfg_decode, cm_cnm_pmm_cfg_update, NULL,
        cm_cnm_pmm_cfg_oplog_update
    },
};

const cm_omi_object_cmd_cfg_t g_CmOmiObjectCfgAdmin[] =
{
    /* get */
    {
        CM_OMI_CMD_GET, CM_OMI_PERMISSION_ALL,
        cm_cnm_admin_decode, cm_cnm_admin_get, cm_cnm_admin_encode,
        NULL
    },
    /* get batch */
    {
        CM_OMI_CMD_GET_BATCH, CM_OMI_PERMISSION_ALL,
        cm_cnm_admin_decode, cm_cnm_admin_getbatch, cm_cnm_admin_encode,
        NULL
    },
    /* count */
    {
        CM_OMI_CMD_COUNT, CM_OMI_PERMISSION_ALL,
        cm_cnm_admin_decode, cm_cnm_admin_count, cm_omi_encode_count,
        NULL
    },
    /* add */
    {
        CM_OMI_CMD_ADD, CM_OMI_PERMISSION_ALL,
        cm_cnm_admin_decode, cm_cnm_admin_create, NULL,
        cm_cnm_admin_oplog_create
    },
    /* modify */
    {
        CM_OMI_CMD_MODIFY, CM_OMI_PERMISSION_ALL,
        cm_cnm_admin_decode, cm_cnm_admin_update, NULL,
        cm_cnm_admin_oplog_update
    },
    /* delete */
    {
        CM_OMI_CMD_DELETE, CM_OMI_PERMISSION_ALL,
        cm_cnm_admin_decode, cm_cnm_admin_delete, NULL,
        cm_cnm_admin_oplog_delete
    }
};

const cm_omi_object_cmd_cfg_t g_CmOmiObjectCfgUser[] =
{
    /*get*/
    {
        CM_OMI_CMD_GET, CM_OMI_PERMISSION_ALL,
        cm_cnm_user_decode, cm_cnm_user_get, cm_cnm_user_encode,
        NULL
    },
    /*delete*/
    {
        CM_OMI_CMD_DELETE, CM_OMI_PERMISSION_ALL,
        cm_cnm_user_decode, cm_cnm_user_delete, NULL,
        cm_cnm_user_oplog_delete
    },
    /*getbatch*/
    {
        CM_OMI_CMD_GET_BATCH, CM_OMI_PERMISSION_ALL,
        cm_cnm_user_decode, cm_cnm_user_getbatch, cm_cnm_user_encode,
        NULL
    },
    /*count*/
    {
        CM_OMI_CMD_COUNT, CM_OMI_PERMISSION_ALL,
        cm_cnm_user_decode, cm_cnm_user_count, cm_omi_encode_count,
        NULL
    },
    /*add*/
    {
        CM_OMI_CMD_ADD, CM_OMI_PERMISSION_ALL,
        cm_cnm_user_decode, cm_cnm_user_create, NULL,
        cm_cnm_user_oplog_create
    },
    /* modify */
    {
        CM_OMI_CMD_MODIFY, CM_OMI_PERMISSION_ALL,
        cm_cnm_user_decode, cm_cnm_user_update, NULL,
        cm_cnm_user_oplog_update
    },
};

const cm_omi_object_cmd_cfg_t g_CmOmiObjectCfgSession[] =
{
    /* get */
    {
        CM_OMI_CMD_GET, CM_OMI_PERMISSION_ALL,
        cm_cnm_session_decode, cm_cnm_session_get, cm_cnm_session_encode,
        NULL
    },
    /* delete */
    {
        CM_OMI_CMD_DELETE, CM_OMI_PERMISSION_ALL,
        cm_cnm_session_decode, cm_cnm_session_delete, NULL,
        NULL
    },
};

const cm_omi_object_cmd_cfg_t g_CmOmiObjectCfgDisk[] =
{
    /* get batch */
    {
        CM_OMI_CMD_GET_BATCH, CM_OMI_PERMISSION_ALL,
        cm_cnm_disk_decode, cm_cnm_disk_getbatch, cm_cnm_disk_encode,
        NULL
    },
    /* get */
    {
        CM_OMI_CMD_GET, CM_OMI_PERMISSION_ALL,
        cm_cnm_disk_decode, cm_cnm_disk_get, cm_cnm_disk_encode,
        NULL
    },
    /* count */
    {
        CM_OMI_CMD_COUNT, CM_OMI_PERMISSION_ALL,
        cm_cnm_disk_decode, cm_cnm_disk_count, cm_omi_encode_count,
        NULL
    },
    /* modify */
    {
        CM_OMI_CMD_MODIFY, CM_OMI_PERMISSION_ALL,
        cm_cnm_disk_decode, cm_cnm_disk_clear, cm_omi_encode_taskid,
        cm_cnm_disk_oplog_clear
    },
};

const cm_omi_object_cmd_cfg_t g_CmOmiObjectCfgPoolList[] =
{
    /* get batch */
    {
        CM_OMI_CMD_GET_BATCH, CM_OMI_PERMISSION_ALL,
        cm_cnm_pool_decode, cm_cnm_pool_getbatch, cm_cnm_pool_encode,
        NULL
    },
    /* count */
    {
        CM_OMI_CMD_COUNT, CM_OMI_PERMISSION_ALL,
        cm_cnm_pool_decode, cm_cnm_pool_count, cm_omi_encode_count,
        NULL
    },
    /* add */
    {
        CM_OMI_CMD_ADD, CM_OMI_PERMISSION_ALL,
        cm_cnm_pool_decode, cm_cnm_pool_create, cm_omi_encode_taskid,
        cm_cnm_pool_oplog_create
    },
    /* delete */
    {
        CM_OMI_CMD_DELETE, CM_OMI_PERMISSION_ALL,
        cm_cnm_pool_decode, cm_cnm_pool_delete, cm_omi_encode_taskid,
        cm_cnm_pool_oplog_delete
    },
    /* switch */
    {
        CM_OMI_CMD_MODIFY, CM_OMI_PERMISSION_ALL,
        cm_cnm_pool_decode, cm_cnm_pool_update, NULL,
        cm_cnm_pool_oplog_update
    },
    /*scan*/
    {
        CM_OMI_CMD_SCAN,CM_OMI_PERMISSION_ALL,
        cm_cnm_pool_decode,cm_cnm_pool_scan,cm_cnm_pool_status_encode,
        NULL
    },
};

const cm_omi_object_cmd_cfg_t g_CmOmiObjectCfgPoolDisk[] =
{
    /* get batch */
    {
        CM_OMI_CMD_GET_BATCH, CM_OMI_PERMISSION_ALL,
        cm_cnm_pooldisk_decode, cm_cnm_pooldisk_getbatch, cm_cnm_pooldisk_encode,
        NULL
    },
    /* count */
    {
        CM_OMI_CMD_COUNT, CM_OMI_PERMISSION_ALL,
        cm_cnm_pooldisk_decode, cm_cnm_pooldisk_count, cm_omi_encode_count,
        NULL
    },
    /* add */
    {
        CM_OMI_CMD_ADD, CM_OMI_PERMISSION_ALL,
        cm_cnm_pooldisk_decode, cm_cnm_pooldisk_add, cm_omi_encode_taskid,
        cm_cnm_pooldisk_oplog_add
    },
    /* delete */
    {
        CM_OMI_CMD_DELETE, CM_OMI_PERMISSION_ALL,
        cm_cnm_pooldisk_decode, cm_cnm_pooldisk_delete, cm_omi_encode_taskid,
        cm_cnm_pooldisk_oplog_delete
    },
};

const cm_omi_object_cmd_cfg_t g_CmOmiObjectCfgLun[] =
{
    /* get batch */
    {
        CM_OMI_CMD_GET_BATCH, CM_OMI_PERMISSION_ALL,
        cm_cnm_lun_decode, cm_cnm_lun_getbatch, cm_cnm_lun_encode,
        NULL
    },
    /* get */
    {
        CM_OMI_CMD_GET, CM_OMI_PERMISSION_ALL,
        cm_cnm_lun_decode, cm_cnm_lun_get, cm_cnm_lun_encode,
        NULL
    },
    /* count */
    {
        CM_OMI_CMD_COUNT, CM_OMI_PERMISSION_ALL,
        cm_cnm_lun_decode, cm_cnm_lun_count, cm_omi_encode_count,
        NULL
    },
    /* add */
    {
        CM_OMI_CMD_ADD, CM_OMI_PERMISSION_ALL,
        cm_cnm_lun_decode, cm_cnm_lun_create, cm_omi_encode_taskid,
        cm_cnm_lun_oplog_create
    },
    /* update */
    {
        CM_OMI_CMD_MODIFY, CM_OMI_PERMISSION_ALL,
        cm_cnm_lun_decode, cm_cnm_lun_update, cm_omi_encode_taskid,
        cm_cnm_lun_oplog_update
    },
    /* delete */
    {
        CM_OMI_CMD_DELETE, CM_OMI_PERMISSION_ALL,
        cm_cnm_lun_decode, cm_cnm_lun_delete, cm_omi_encode_taskid,
        cm_cnm_lun_oplog_delete
    },
};

const cm_omi_object_cmd_cfg_t g_CmOmiObjectCfgSnapshot[] =
{
    /* get batch */
    {
        CM_OMI_CMD_GET_BATCH, CM_OMI_PERMISSION_ALL,
        cm_cnm_snapshot_decode, cm_cnm_snapshot_getbatch, cm_cnm_snapshot_encode,
        NULL
    },
    /* get */
    {
        CM_OMI_CMD_GET, CM_OMI_PERMISSION_ALL,
        cm_cnm_snapshot_decode, cm_cnm_snapshot_get, cm_cnm_snapshot_encode,
        NULL
    },
    /* count */
    {
        CM_OMI_CMD_COUNT, CM_OMI_PERMISSION_ALL,
        cm_cnm_snapshot_decode, cm_cnm_snapshot_count, cm_omi_encode_count,
        NULL
    },
    /* add */
    {
        CM_OMI_CMD_ADD, CM_OMI_PERMISSION_ALL,
        cm_cnm_snapshot_decode, cm_cnm_snapshot_create, NULL,
        cm_cnm_snapshot_oplog_create
    },
    /* update */
    {
        CM_OMI_CMD_MODIFY, CM_OMI_PERMISSION_ALL,
        cm_cnm_snapshot_decode, cm_cnm_snapshot_update, NULL,
        cm_cnm_snapshot_oplog_update
    },
    /* delete */
    {
        CM_OMI_CMD_DELETE, CM_OMI_PERMISSION_ALL,
        cm_cnm_snapshot_decode, cm_cnm_snapshot_delete, NULL,
        cm_cnm_snapshot_oplog_delete
    },
};

const cm_omi_object_cmd_cfg_t g_CmOmiObjectCfgSnapshotSche[] =
{
    /* get batch */
    {
        CM_OMI_CMD_GET_BATCH, CM_OMI_PERMISSION_ALL,
        cm_cnm_snapshot_sche_decode, cm_cnm_snapshot_sche_getbatch, cm_cnm_snapshot_sche_encode,
        NULL
    },
    /* get */
    {
        CM_OMI_CMD_GET, CM_OMI_PERMISSION_ALL,
        cm_cnm_snapshot_sche_decode, cm_cnm_snapshot_sche_get, cm_cnm_snapshot_sche_encode,
        NULL
    },
    /* count */
    {
        CM_OMI_CMD_COUNT, CM_OMI_PERMISSION_ALL,
        cm_cnm_snapshot_sche_decode, cm_cnm_snapshot_sche_count, cm_omi_encode_count,
        NULL
    },
    /* add */
    {
        CM_OMI_CMD_ADD, CM_OMI_PERMISSION_ALL,
        cm_cnm_snapshot_sche_decode, cm_cnm_snapshot_sche_create, NULL,
        cm_cnm_snapshot_sche_oplog_create
    },
    /* update */
    {
        CM_OMI_CMD_MODIFY, CM_OMI_PERMISSION_ALL,
        cm_cnm_snapshot_sche_decode, cm_cnm_snapshot_sche_update, NULL,
        cm_cnm_snapshot_sche_oplog_update
    },
    /* delete */
    {
        CM_OMI_CMD_DELETE, CM_OMI_PERMISSION_ALL,
        cm_cnm_snapshot_sche_decode, cm_cnm_snapshot_sche_delete, NULL,
        cm_cnm_snapshot_sche_oplog_delete
    },
};

const cm_omi_object_cmd_cfg_t g_CmOmiObjectCfgHostGroup[] =
{
    /* get batch */
    {
        CM_OMI_CMD_GET_BATCH, CM_OMI_PERMISSION_ALL,
        cm_cnm_hg_decode, cm_cnm_hg_getbatch, cm_cnm_hg_encode,
        NULL
    },
    /* get */
    {
        CM_OMI_CMD_GET, CM_OMI_PERMISSION_ALL,
        cm_cnm_hg_decode, cm_cnm_hg_get, cm_cnm_hg_encode,
        NULL
    },
    /* count */
    {
        CM_OMI_CMD_COUNT, CM_OMI_PERMISSION_ALL,
        cm_cnm_hg_decode, cm_cnm_hg_count, cm_omi_encode_count,
        NULL
    },
    /* add */
    {
        CM_OMI_CMD_ADD, CM_OMI_PERMISSION_ALL,
        cm_cnm_hg_decode, cm_cnm_hg_create, NULL,
        cm_cnm_hg_oplog_create
    },
    /* update */
    {
        CM_OMI_CMD_MODIFY, CM_OMI_PERMISSION_ALL,
        cm_cnm_hg_decode, cm_cnm_hg_update, NULL,
        cm_cnm_hg_oplog_update
    },
    /* delete */
    {
        CM_OMI_CMD_DELETE, CM_OMI_PERMISSION_ALL,
        cm_cnm_hg_decode, cm_cnm_hg_delete, NULL,
        cm_cnm_hg_oplog_delete
    },
};

const cm_omi_object_cmd_cfg_t g_CmOmiObjectCfgTarget[] =
{
    /* get batch */
    {
        CM_OMI_CMD_GET_BATCH, CM_OMI_PERMISSION_ALL,
        cm_cnm_target_decode, cm_cnm_target_getbatch, cm_cnm_target_encode,
        NULL
    },
    /* get */
    {
        CM_OMI_CMD_GET, CM_OMI_PERMISSION_ALL,
        cm_cnm_target_decode, cm_cnm_target_get, cm_cnm_target_encode,
        NULL
    },
    /* count */
    {
        CM_OMI_CMD_COUNT, CM_OMI_PERMISSION_ALL,
        cm_cnm_target_decode, cm_cnm_target_count, cm_omi_encode_count,
        NULL
    },
    /* add */
    {
        CM_OMI_CMD_ADD, CM_OMI_PERMISSION_ALL,
        cm_cnm_target_decode, cm_cnm_target_create, NULL,
        cm_cnm_target_oplog_create
    },
    /* update */
    {
        CM_OMI_CMD_MODIFY, CM_OMI_PERMISSION_ALL,
        cm_cnm_target_decode, cm_cnm_target_update, NULL,
        cm_cnm_target_oplog_update
    },
    /* delete */
    {
        CM_OMI_CMD_DELETE, CM_OMI_PERMISSION_ALL,
        cm_cnm_target_decode, cm_cnm_target_delete, NULL,
        cm_cnm_target_oplog_delete
    },
};

const cm_omi_object_cmd_cfg_t g_CmOmiObjectCfgLunmap[] =
{
    /* get batch */
    {
        CM_OMI_CMD_GET_BATCH, CM_OMI_PERMISSION_ALL,
        cm_cnm_lunmap_decode, cm_cnm_lunmap_getbatch, cm_cnm_lunmap_encode,
        NULL
    },
    /* get */
    {
        CM_OMI_CMD_GET, CM_OMI_PERMISSION_ALL,
        cm_cnm_lunmap_decode, cm_cnm_lunmap_get, cm_cnm_lunmap_encode,
        NULL
    },
    /* count */
    {
        CM_OMI_CMD_COUNT, CM_OMI_PERMISSION_ALL,
        cm_cnm_lunmap_decode, cm_cnm_lunmap_count, cm_omi_encode_count,
        NULL
    },
    /* add */
    {
        CM_OMI_CMD_ADD, CM_OMI_PERMISSION_ALL,
        cm_cnm_lunmap_decode, cm_cnm_lunmap_create, NULL,
        cm_cnm_lunmap_oplog_create
    },
    /* update */
    {
        CM_OMI_CMD_MODIFY, CM_OMI_PERMISSION_ALL,
        cm_cnm_lunmap_decode, cm_cnm_lunmap_update, NULL,
        NULL
    },
    /* delete */
    {
        CM_OMI_CMD_DELETE, CM_OMI_PERMISSION_ALL,
        cm_cnm_lunmap_decode, cm_cnm_lunmap_delete, NULL,
        cm_cnm_lunmap_oplog_delete
    },
};

const cm_omi_object_cmd_cfg_t g_CmOmiObjectCfgDiskSpare[] =
{
    /* get batch */
    {
        CM_OMI_CMD_GET_BATCH, CM_OMI_PERMISSION_ALL,
        cm_cnm_diskspare_decode, cm_cnm_diskspare_getbatch, cm_cnm_diskspare_encode,
        NULL
    },
    /* count */
    {
        CM_OMI_CMD_COUNT, CM_OMI_PERMISSION_ALL,
        cm_cnm_diskspare_decode, cm_cnm_diskspare_count, cm_omi_encode_count,
        NULL
    },
    /* modify */
    {
        CM_OMI_CMD_MODIFY, CM_OMI_PERMISSION_ALL,
        NULL, cm_cnm_diskspare_update, NULL,
        NULL,
    },
};

const cm_omi_object_cmd_cfg_t g_CmOmiObjectCfgNas[] =
{
    /* get batch */
    {
        CM_OMI_CMD_GET_BATCH, CM_OMI_PERMISSION_ALL,
        cm_cnm_nas_decode, cm_cnm_nas_getbatch, cm_cnm_nas_encode,
        NULL
    },
    /* get */
    {
        CM_OMI_CMD_GET, CM_OMI_PERMISSION_ALL,
        cm_cnm_nas_decode, cm_cnm_nas_get, cm_cnm_nas_encode,
        NULL
    },
    /* count */
    {
        CM_OMI_CMD_COUNT, CM_OMI_PERMISSION_ALL,
        cm_cnm_nas_decode, cm_cnm_nas_count, cm_omi_encode_count,
        NULL
    },
    /* add */
    {
        CM_OMI_CMD_ADD, CM_OMI_PERMISSION_ALL,
        cm_cnm_nas_decode, cm_cnm_nas_create, cm_omi_encode_taskid,
        cm_cnm_nas_oplog_create
    },
    /* update */
    {
        CM_OMI_CMD_MODIFY, CM_OMI_PERMISSION_ALL,
        cm_cnm_nas_decode, cm_cnm_nas_update, cm_omi_encode_taskid,
        cm_cnm_nas_oplog_update
    },
    /* delete */
    {
        CM_OMI_CMD_DELETE, CM_OMI_PERMISSION_ALL,
        cm_cnm_nas_decode, cm_cnm_nas_delete, cm_omi_encode_taskid,
        cm_cnm_nas_oplog_delete
    },
};

const cm_omi_object_cmd_cfg_t g_CmOmiObjectCfgCifs[] =
{
    /* get batch */
    {
        CM_OMI_CMD_GET_BATCH, CM_OMI_PERMISSION_ALL,
        cm_cnm_cifs_decode, cm_cnm_cifs_getbatch, cm_cnm_cifs_encode,
        NULL
    },
    /* get */
    {
        CM_OMI_CMD_GET, CM_OMI_PERMISSION_ALL,
        cm_cnm_cifs_decode, cm_cnm_cifs_get, cm_cnm_cifs_encode,
        NULL
    },
    /* count */
    {
        CM_OMI_CMD_COUNT, CM_OMI_PERMISSION_ALL,
        cm_cnm_cifs_decode, cm_cnm_cifs_count, cm_omi_encode_count,
        NULL
    },
    /* add */
    {
        CM_OMI_CMD_ADD, CM_OMI_PERMISSION_ALL,
        cm_cnm_cifs_decode, cm_cnm_cifs_create, cm_omi_encode_taskid,
        cm_cnm_cifs_oplog_create
    },
    /* update */
    {
        CM_OMI_CMD_MODIFY, CM_OMI_PERMISSION_ALL,
        cm_cnm_cifs_decode, cm_cnm_cifs_update, cm_omi_encode_taskid,
        cm_cnm_cifs_oplog_update
    },
    /* delete */
    {
        CM_OMI_CMD_DELETE, CM_OMI_PERMISSION_ALL,
        cm_cnm_cifs_decode, cm_cnm_cifs_delete, cm_omi_encode_taskid,
        cm_cnm_cifs_oplog_delete
    },
};

const cm_omi_object_cmd_cfg_t g_CmOmiObjectCfgGroup[] =
{
    /* get batch */
    {
        CM_OMI_CMD_GET_BATCH, CM_OMI_PERMISSION_ALL,
        cm_cnm_group_decode, cm_cnm_group_getbatch, cm_cnm_group_encode,
        NULL
    },
    /* get */
    {
        CM_OMI_CMD_GET, CM_OMI_PERMISSION_ALL,
        cm_cnm_group_decode, cm_cnm_group_get, cm_cnm_group_encode,
        NULL
    },
    /* count */
    {
        CM_OMI_CMD_COUNT, CM_OMI_PERMISSION_ALL,
        cm_cnm_group_decode, cm_cnm_group_count, cm_omi_encode_count,
        NULL
    },
    /* add */
    {
        CM_OMI_CMD_ADD, CM_OMI_PERMISSION_ALL,
        cm_cnm_group_decode, cm_cnm_group_create, cm_omi_encode_taskid,
        cm_cnm_group_oplog_create
    },
    /* delete */
    {
        CM_OMI_CMD_DELETE, CM_OMI_PERMISSION_ALL,
        cm_cnm_group_decode, cm_cnm_group_delete, cm_omi_encode_taskid,
        cm_cnm_group_oplog_delete
    },
};

const cm_omi_object_cmd_cfg_t g_CmOmiObjectCfgNfs[] =
{
    /* get batch */
    {
        CM_OMI_CMD_GET_BATCH, CM_OMI_PERMISSION_ALL,
        cm_cnm_nfs_decode, cm_cnm_nfs_getbatch, cm_cnm_nfs_encode,
        NULL
    },
    /* count */
    {
        CM_OMI_CMD_COUNT, CM_OMI_PERMISSION_ALL,
        cm_cnm_nfs_decode, cm_cnm_nfs_count, cm_omi_encode_count,
        NULL
    },
    /* add */
    {
        CM_OMI_CMD_ADD, CM_OMI_PERMISSION_ALL,
        cm_cnm_nfs_decode, cm_cnm_nfs_add, cm_omi_encode_taskid,
        cm_cnm_nfs_oplog_add
    },
    /* delete */
    {
        CM_OMI_CMD_DELETE, CM_OMI_PERMISSION_ALL,
        cm_cnm_nfs_decode, cm_cnm_nfs_delete, cm_omi_encode_taskid,
        cm_cnm_nfs_oplog_delete
    },
};

/***************************** Snapshot Backup *******************************/
const cm_omi_object_cmd_cfg_t g_CmOmiObjectCfgSnapshotBackup[] =
{
    /* add */
    {
        CM_OMI_CMD_ADD, CM_OMI_PERMISSION_ALL,
        cm_cnm_snapshot_backup_decode, cm_cnm_snapshot_backup_add, NULL,
        cm_cnm_snapshot_backup_oplog_create
    },
};

/***************************** Task Manager ********************************/
const cm_omi_object_cmd_cfg_t g_CmOmiObjectCfgTaskManager[] =
{
    /* get */
    {
        CM_OMI_CMD_GET, CM_OMI_PERMISSION_ALL,
        cm_cnm_task_manager_decode, cm_cnm_task_manager_get, cm_cnm_task_manager_encode,
        NULL
    },

    /* getbatch */
    {
        CM_OMI_CMD_GET_BATCH, CM_OMI_PERMISSION_ALL,
        cm_cnm_task_manager_decode, cm_cnm_task_manager_getbatch, cm_cnm_task_manager_encode,
        NULL
    },

    /* delete */
    {
        CM_OMI_CMD_DELETE, CM_OMI_PERMISSION_ALL,
        cm_cnm_task_manager_decode, cm_cnm_task_manager_delete, NULL,
        cm_cnm_task_manager_oplog_delete
    },

    /* count */
    {
        CM_OMI_CMD_COUNT, CM_OMI_PERMISSION_ALL,
        cm_cnm_task_manager_decode, cm_cnm_task_manager_count, cm_omi_encode_count,
        NULL
    },
};

const cm_omi_object_cmd_cfg_t g_CmOmiObjectCfgRemote[] =
{
    /* get batch */
    {
        CM_OMI_CMD_GET_BATCH, CM_OMI_PERMISSION_ALL,
        cm_cnm_remote_decode, cm_cnm_remote_getbatch, cm_cnm_remote_encode,
        NULL
    },
    /* count */
    {
        CM_OMI_CMD_COUNT, CM_OMI_PERMISSION_ALL,
        cm_cnm_remote_decode, cm_cnm_remote_count, cm_omi_encode_count,
        NULL
    }
};

const cm_omi_object_cmd_cfg_t g_CmOmiObjectCfgPing[] =
{
    /* add */
    {
        CM_OMI_CMD_ADD, CM_OMI_PERMISSION_ALL,
        cm_cnm_ping_decode, cm_cnm_ping_add, NULL,
        cm_cnm_ping_oplog_create
    },
};

const cm_omi_object_cmd_cfg_t g_CmOmiObjectCfgQuota[] =
{
    /* get batch */
    {
        CM_OMI_CMD_GET_BATCH, CM_OMI_PERMISSION_ALL,
        cm_cnm_quota_decode, cm_cnm_quota_getbatch, cm_cnm_quota_encode,
        NULL
    },
    /* count */
    {
        CM_OMI_CMD_COUNT, CM_OMI_PERMISSION_ALL,
        cm_cnm_quota_decode, cm_cnm_quota_count, cm_omi_encode_count,
        NULL
    },
    /* update */
    {
        CM_OMI_CMD_MODIFY, CM_OMI_PERMISSION_ALL,
        cm_cnm_quota_decode, cm_cnm_quota_update, NULL,
        cm_cnm_quota_oplog_update
    },
    /* get */
    {
        CM_OMI_CMD_GET, CM_OMI_PERMISSION_ALL,
        cm_cnm_quota_decode, cm_cnm_quota_get, cm_cnm_quota_encode,
        NULL
    },
    /* delete */
    {
        CM_OMI_CMD_DELETE, CM_OMI_PERMISSION_ALL,
        cm_cnm_quota_decode, cm_cnm_quota_delete, NULL,
        cm_cnm_quota_oplog_delete
    },
};

const cm_omi_object_cmd_cfg_t g_CmOmiObjectCfgIpshift[] =
{
    /* get batch */
    {
        CM_OMI_CMD_GET_BATCH, CM_OMI_PERMISSION_ALL,
        cm_cnm_ipshift_decode, cm_cnm_ipshift_getbatch, cm_cnm_ipshift_encode,
        NULL
    },
    /* count */
    {
        CM_OMI_CMD_COUNT, CM_OMI_PERMISSION_ALL,
        cm_cnm_ipshift_decode, cm_cnm_ipshift_count, cm_omi_encode_count,
        NULL
    },
    /* create */
    {
        CM_OMI_CMD_ADD, CM_OMI_PERMISSION_ALL,
        cm_cnm_ipshift_decode, cm_cnm_ipshift_create, NULL,
        cm_cnm_ipshift_oplog_create
    },
    /* get */
    {
        CM_OMI_CMD_GET, CM_OMI_PERMISSION_ALL,
        cm_cnm_ipshift_decode, cm_cnm_ipshift_get, cm_cnm_ipshift_encode,
        NULL
    },
    /* delete */
    {
        CM_OMI_CMD_DELETE, CM_OMI_PERMISSION_ALL,
        cm_cnm_ipshift_decode, cm_cnm_ipshift_delete, NULL,
        cm_cnm_ipshift_oplog_delete
    },
};

const cm_omi_object_cmd_cfg_t g_CmOmiObjectCfgSnapshotClone[] =
{
    /* add */
    {
        CM_OMI_CMD_ADD, CM_OMI_PERMISSION_ALL,
        cm_cnm_snapshot_clone_decode, cm_cnm_snapshot_clone_add, NULL,
        cm_cnm_snapshot_clone_oplog_create
    },
};

const cm_omi_object_cmd_cfg_t g_CmOmiObjectCfgRoute[] =
{
    /* get batch */
    {
        CM_OMI_CMD_GET_BATCH, CM_OMI_PERMISSION_ALL,
        cm_cnm_route_decode, cm_cnm_route_getbatch, cm_cnm_route_encode,
        NULL
    },
    /* count */
    {
        CM_OMI_CMD_COUNT, CM_OMI_PERMISSION_ALL,
        cm_cnm_route_decode, cm_cnm_route_count, cm_omi_encode_count,
        NULL
    },
    /* create */
    {
        CM_OMI_CMD_ADD, CM_OMI_PERMISSION_ALL,
        cm_cnm_route_decode, cm_cnm_route_create, NULL,
        cm_cnm_route_oplog_create
    },
    /* delete */
    {
        CM_OMI_CMD_DELETE, CM_OMI_PERMISSION_ALL,
        cm_cnm_route_decode, cm_cnm_route_delete, NULL,
        cm_cnm_route_oplog_delete
    },
};

const cm_omi_object_cmd_cfg_t g_CmOmiObjectCfgClusterNas[] =
{
    /* get batch */
    {
        CM_OMI_CMD_GET_BATCH, CM_OMI_PERMISSION_ALL,
        cm_cnm_cluster_nas_decode, cm_cnm_cluster_nas_getbatch,
        cm_cnm_cluster_nas_encode,
        NULL
    },
    /* add */
    {
        CM_OMI_CMD_ADD, CM_OMI_PERMISSION_ALL,
        cm_cnm_cluster_nas_decode, cm_cnm_cluster_nas_create, cm_omi_encode_taskid,
        cm_cnm_cluster_nas_oplog_create
    },
    /* update */
    {
        CM_OMI_CMD_MODIFY, CM_OMI_PERMISSION_ALL,
        cm_cnm_cluster_nas_decode, cm_cnm_cluster_nas_add, cm_omi_encode_taskid,
        cm_cnm_cluster_nas_oplog_update
    },
    /* count */
    {
        CM_OMI_CMD_COUNT, CM_OMI_PERMISSION_ALL,
        cm_cnm_cluster_nas_decode, cm_cnm_cluster_nas_count, cm_omi_encode_count,
        NULL
    },
    {
        CM_OMI_CMD_DELETE, CM_OMI_PERMISSION_ALL,
        cm_cnm_cluster_nas_decode, cm_cnm_cluster_nas_delete, cm_omi_encode_taskid,
        NULL
    },
};

const cm_omi_object_cmd_cfg_t g_CmOmiObjectCfgPhys_ip[] =
{
    /* get batch */
    {
        CM_OMI_CMD_GET_BATCH, CM_OMI_PERMISSION_ALL,
        cm_cnm_phys_ip_decode, cm_cnm_phys_ip_getbatch, cm_cnm_phys_ip_encode,
        NULL
    },
    /* count */
    {
        CM_OMI_CMD_COUNT, CM_OMI_PERMISSION_ALL,
        cm_cnm_phys_ip_decode, cm_cnm_phys_ip_count, cm_omi_encode_count,
        NULL
    },
    /* create */
    {
        CM_OMI_CMD_ADD, CM_OMI_PERMISSION_ALL,
        cm_cnm_phys_ip_decode, cm_cnm_phys_ip_create,NULL,
        cm_cnm_phys_ip_oplog_create
    },
    /* delete */
    {
        CM_OMI_CMD_DELETE, CM_OMI_PERMISSION_ALL,
        cm_cnm_phys_ip_decode, cm_cnm_phys_ip_delete, NULL,
        cm_cnm_phys_ip_oplog_delete
    },
};

const cm_omi_object_cmd_cfg_t g_CmOmiObjectCfgDualliveNas[] =
{
    /* get batch */
    {
        CM_OMI_CMD_GET_BATCH, CM_OMI_PERMISSION_ALL,
        NULL, cm_cnm_duallive_nas_getbatch, cm_cnm_duallive_nas_encode,
        NULL
    },
    /* count */
    {
        CM_OMI_CMD_COUNT, CM_OMI_PERMISSION_ALL,
        NULL, cm_cnm_duallive_nas_count, cm_omi_encode_count,
        NULL
    },
    /* create */
    {
        CM_OMI_CMD_ADD, CM_OMI_PERMISSION_ALL,
        cm_cnm_duallive_nas_decode, cm_cnm_duallive_nas_create, cm_omi_encode_taskid,
        cm_cnm_duallive_nas_oplog_create
    },

    /* delete */
    {
        CM_OMI_CMD_DELETE, CM_OMI_PERMISSION_ALL,
        cm_cnm_duallive_nas_decode, cm_cnm_duallive_nas_delete, cm_omi_encode_taskid,
        cm_cnm_duallive_nas_oplog_delete
    },
};

const cm_omi_object_cmd_cfg_t g_CmOmiObjectCfgBackUpNas[] =
{
    /* get batch */
    {
        CM_OMI_CMD_GET_BATCH, CM_OMI_PERMISSION_ALL,
        NULL, cm_cnm_backup_nas_getbatch, cm_cnm_backup_nas_encode,
        NULL
    },
    /* count */
    {
        CM_OMI_CMD_COUNT, CM_OMI_PERMISSION_ALL,
        NULL, cm_cnm_backup_nas_count, cm_omi_encode_count,
        NULL
    },
    /* create */
    {
        CM_OMI_CMD_ADD, CM_OMI_PERMISSION_ALL,
        cm_cnm_backup_nas_decode, cm_cnm_backup_nas_create, cm_omi_encode_taskid,
        cm_cnm_backup_nas_oplog_create
    },

    /* delete */
    {
        CM_OMI_CMD_DELETE, CM_OMI_PERMISSION_ALL,
        cm_cnm_backup_nas_decode, cm_cnm_backup_nas_delete, cm_omi_encode_taskid,
        cm_cnm_backup_nas_oplog_delete
    },
};

const cm_omi_object_cmd_cfg_t g_CmOmiObjectCfgLunMirror[] =
{
    /* get batch */
    {
        CM_OMI_CMD_GET_BATCH, CM_OMI_PERMISSION_ALL,
        cm_cnm_lunmirror_decode, cm_cnm_lunmirror_getbatch, cm_cnm_lunmirror_encode,
        NULL
    },
    /* count */
    {
        CM_OMI_CMD_COUNT, CM_OMI_PERMISSION_ALL,
        cm_cnm_lunmirror_decode, cm_cnm_lunmirror_count, cm_omi_encode_count,
        NULL
    },
    /* create */
    {
        CM_OMI_CMD_ADD, CM_OMI_PERMISSION_ALL,
        cm_cnm_lunmirror_decode, cm_cnm_lunmirror_create, cm_omi_encode_taskid,
        cm_cnm_lunmirror_oplog_create
    },

    /* delete */
    {
        CM_OMI_CMD_DELETE, CM_OMI_PERMISSION_ALL,
        cm_cnm_lunmirror_decode, cm_cnm_lunmirror_delete, cm_omi_encode_taskid,
        cm_cnm_lunmirror_oplog_delete
    },
};

const cm_omi_object_cmd_cfg_t g_CmOmiObjectCfgLunBackup[] =
{
    /* get batch */
    {
        CM_OMI_CMD_GET_BATCH, CM_OMI_PERMISSION_ALL,
        cm_cnm_lunbackup_decode, cm_cnm_lunbackup_getbatch, cm_cnm_lunbackup_encode,
        NULL
    },
    /* count */
    {
        CM_OMI_CMD_COUNT, CM_OMI_PERMISSION_ALL,
        cm_cnm_lunbackup_decode, cm_cnm_lunbackup_count, cm_omi_encode_count,
        NULL
    },
    /* create */
    {
        CM_OMI_CMD_ADD, CM_OMI_PERMISSION_ALL,
        cm_cnm_lunbackup_decode, cm_cnm_lunbackup_create, cm_omi_encode_taskid,
        cm_cnm_lunbackup_oplog_create
    },
    /* modify */
    {
        CM_OMI_CMD_MODIFY, CM_OMI_PERMISSION_ALL,
        cm_cnm_lunbackup_decode, cm_cnm_lunbackup_modify, cm_omi_encode_taskid,
        cm_cnm_lunbackup_oplog_modify
    },
    /* delete */
    {
        CM_OMI_CMD_DELETE, CM_OMI_PERMISSION_ALL,
        cm_cnm_lunbackup_decode, cm_cnm_lunbackup_delete, cm_omi_encode_taskid,
        cm_cnm_lunbackup_oplog_delete
    },
};

const cm_omi_object_cmd_cfg_t g_CmOmiObjectCfgClusterTarget[] =
{
    /* get batch */
    {
        CM_OMI_CMD_GET_BATCH, CM_OMI_PERMISSION_ALL,
        cm_cnm_cluster_target_decode, cm_cnm_cluster_target_getbatch, cm_cnm_cluster_target_encode,
        NULL
    },
};
const cm_omi_object_cmd_cfg_t g_CmOmiObjectCfgMigrate[] =
{
    /* get */
    {
        CM_OMI_CMD_GET, CM_OMI_PERMISSION_ALL,
        cm_cnm_migrate_decode, cm_cnm_migrate_get, cm_cnm_migrate_encode,
        NULL
    },
    /* create */
    {
        CM_OMI_CMD_ADD, CM_OMI_PERMISSION_ALL,
        cm_cnm_migrate_decode, cm_cnm_migrate_create, NULL,
        cm_cnm_migrate_oplog_create
    },
    /* update */
    {
        CM_OMI_CMD_MODIFY, CM_OMI_PERMISSION_ALL,
        cm_cnm_migrate_decode, cm_cnm_migrate_update, NULL,
        cm_cnm_migrate_oplog_update
    },
    /* scan */
    {
        CM_OMI_CMD_SCAN, CM_OMI_PERMISSION_ALL,
        cm_cnm_migrate_decode, cm_cnm_migrate_scan, NULL,
        NULL,
    },
};

const cm_omi_object_cmd_cfg_t g_CmOmiObjectCfgPoolReconstruct[] =
{
    /* get batch */
    {
        CM_OMI_CMD_GET_BATCH, CM_OMI_PERMISSION_ALL,
        cm_cnm_pool_reconstruct_decode, cm_cnm_pool_reconstruct_getbatch,
        cm_cnm_pool_reconstruct_encode,
        NULL
    },
    /* count */
    {
        CM_OMI_CMD_COUNT, CM_OMI_PERMISSION_ALL,
        cm_cnm_pool_reconstruct_decode, cm_cnm_pool_reconstruct_count, cm_omi_encode_count,
        NULL
    },
};
const cm_omi_object_cmd_cfg_t g_CmOmiObjectCfgNodeService[] =
{
    /* get batch */
    {
        CM_OMI_CMD_GET_BATCH, CM_OMI_PERMISSION_ALL,
        cm_cnm_node_service_decode, cm_cnm_node_service_getbatch, cm_cnm_node_service_encode,
        NULL
    },
    /* count */
    {
        CM_OMI_CMD_COUNT, CM_OMI_PERMISSION_ALL,
        NULL, cm_cnm_node_service_count, cm_omi_encode_count,
        NULL
    },
    /* update */
    {
        CM_OMI_CMD_MODIFY, CM_OMI_PERMISSION_ALL,
        cm_cnm_node_service_decode, cm_cnm_node_service_updata, NULL,
        cm_cnm_node_service_oplog_update
    },
};

const cm_omi_object_cmd_cfg_t g_CmOmiObjectCfgAggr[] =
{
    /* get batch*/
    {
        CM_OMI_CMD_GET_BATCH, CM_OMI_PERMISSION_ALL,
        cm_cnm_aggr_decode, cm_cnm_aggr_getbatch, cm_cnm_aggr_encode,
        NULL
    },
    /* count */
    {
        CM_OMI_CMD_COUNT, CM_OMI_PERMISSION_ALL,
        cm_cnm_aggr_decode, cm_cnm_aggr_count, cm_omi_encode_count,
        NULL
    },
    /* create */
    {
        CM_OMI_CMD_ADD, CM_OMI_PERMISSION_ALL,
        cm_cnm_aggr_decode, cm_cnm_aggr_create, NULL,
        cm_cnm_aggr_oplog_create
    },
    /* update */
    {
        CM_OMI_CMD_MODIFY, CM_OMI_PERMISSION_ALL,
        cm_cnm_aggr_decode, cm_cnm_aggr_update, NULL,
        cm_cnm_aggr_oplog_update
    },
    /* delete */
    {
        CM_OMI_CMD_DELETE, CM_OMI_PERMISSION_ALL,
        cm_cnm_aggr_decode, cm_cnm_aggr_delete, NULL,
        cm_cnm_aggr_oplog_delete
    },
};

const cm_omi_object_cmd_cfg_t g_CmOmiObjectCfgDnsm[] =
{
    /* get batch */
    {
        CM_OMI_CMD_GET_BATCH, CM_OMI_PERMISSION_ALL,
        cm_cnm_dnsm_decode, cm_cnm_dnsm_getbatch, cm_cnm_dnsm_encode,
        NULL
    },
    /* get */
    {
        CM_OMI_CMD_GET, CM_OMI_PERMISSION_ALL,
        cm_cnm_dnsm_decode, cm_cnm_dnsm_get, cm_cnm_dnsm_encode,
        NULL
    },
    /* count */
    {
        CM_OMI_CMD_COUNT, CM_OMI_PERMISSION_ALL,
        cm_cnm_dnsm_decode, cm_cnm_dnsm_count, cm_omi_encode_count,
        NULL
    },
    /* add */
    {
        CM_OMI_CMD_ADD, CM_OMI_PERMISSION_ALL,
        cm_cnm_dnsm_decode, cm_cnm_dnsm_insert, NULL,
        cm_cnm_dnsm_oplog_insert
    },
    /* update */
    {
        CM_OMI_CMD_MODIFY, CM_OMI_PERMISSION_ALL,
        cm_cnm_dnsm_decode, cm_cnm_dnsm_update, NULL,
        cm_cnm_dnsm_oplog_update
    },
    /* delete */
    {
        CM_OMI_CMD_DELETE, CM_OMI_PERMISSION_ALL,
        cm_cnm_dnsm_decode, cm_cnm_dnsm_delete, NULL,
        cm_cnm_dnsm_oplog_delete
    },
};

const cm_omi_object_cmd_cfg_t g_CmOmiObjectCfgPermission[] =
{
    /* get batch*/
    {
        CM_OMI_CMD_GET_BATCH, CM_OMI_PERMISSION_ALL,
        cm_cnm_permission_decode, cm_cnm_permission_getbatch, cm_cnm_permission_encode,
        NULL
    },
    /* count */
    {
        CM_OMI_CMD_COUNT, CM_OMI_PERMISSION_ALL,
        cm_cnm_permission_decode, cm_cnm_permission_count, cm_omi_encode_count,
        NULL
    },
    /* create */
    {
        CM_OMI_CMD_ADD, CM_OMI_PERMISSION_ALL,
        cm_cnm_permission_decode, cm_cnm_permission_add, NULL,
        NULL
    },
    /* update */
    {
        CM_OMI_CMD_MODIFY, CM_OMI_PERMISSION_ALL,
        cm_cnm_permission_decode, cm_cnm_permission_update, NULL,
        NULL
    },
    /* delete */
    {
        CM_OMI_CMD_DELETE, CM_OMI_PERMISSION_ALL,
        cm_cnm_permission_decode, cm_cnm_permission_delete, NULL,
        NULL
    },
};
const cm_omi_object_cmd_cfg_t g_CmOmiObjectCfgPmmPool[] =
{
    /* get */
    {
        CM_OMI_CMD_GET, CM_OMI_PERMISSION_ALL,
        cm_cnm_pmm_pool_decode, cm_cnm_pmm_pool_current, cm_cnm_pmm_pool_encode,
        NULL
    },
};

const cm_omi_object_cmd_cfg_t g_CmOmiObjectCfgPmmNas[] =
{
    /* get */
    {
        CM_OMI_CMD_GET, CM_OMI_PERMISSION_ALL,
        cm_cnm_pmm_nas_decode, cm_cnm_pmm_nas_current, cm_cnm_pmm_nas_encode,
        NULL
    },
};
const cm_omi_object_cmd_cfg_t g_CmOmiObjectCfgPmmSas[] =
{
    /* get*/
    {
        CM_OMI_CMD_GET, CM_OMI_PERMISSION_ALL,
        cm_cnm_pmm_sas_decode, cm_cnm_pmm_sas_current, cm_cnm_pmm_sas_encode,
        NULL
    },
};

const cm_omi_object_cmd_cfg_t g_CmOmiObjectCfgPmmProto[] =
{
    /* get*/
    {
        CM_OMI_CMD_GET, CM_OMI_PERMISSION_ALL,
        cm_cnm_pmm_protocol_decode, cm_cnm_pmm_protocol_current, cm_cnm_pmm_protocol_encode,
        NULL
    },
};

const cm_omi_object_cmd_cfg_t g_CmOmiObjectCfgPmmCache[] =
{
    /* get */
    {
        CM_OMI_CMD_GET, CM_OMI_PERMISSION_ALL,
        cm_cnm_pmm_cache_decode, cm_cnm_pmm_cache_current, cm_cnm_pmm_cache_encode,
        NULL
    },
};

const cm_omi_object_cmd_cfg_t g_CmOmiObjectCfgCnmSas[] =
{
    /* getbatch */
    {
        CM_OMI_CMD_GET_BATCH, CM_OMI_PERMISSION_ALL,
        NULL, cm_cnm_sas_getbatch, cm_cnm_name_encode,
        NULL
    },
    /*count*/
    {
        CM_OMI_CMD_COUNT,CM_OMI_PERMISSION_ALL,
        NULL,cm_cnm_sas_count,cm_omi_encode_count,
        NULL
    },
};

const cm_omi_object_cmd_cfg_t g_CmOmiObjectCfgCnmProto[] =
{
    /* getbatch */
    {
        CM_OMI_CMD_GET_BATCH, CM_OMI_PERMISSION_ALL,
        NULL, cm_cnm_protocol_getbatch, cm_cnm_name_encode,
        NULL
    },
    /*count*/
    {
        CM_OMI_CMD_COUNT,CM_OMI_PERMISSION_ALL,
        NULL,cm_cnm_protocol_count,cm_omi_encode_count,
        NULL
    },
};

const cm_omi_object_cmd_cfg_t g_CmOmiObjectCfgPmmLun[] =
{
    /* get */
    {
        CM_OMI_CMD_GET, CM_OMI_PERMISSION_ALL,
        cm_cnm_pmm_lun_decode, cm_cnm_pmm_lun_current, cm_cnm_pmm_lun_encode,
        NULL
    },
};

const cm_omi_object_cmd_cfg_t g_CmOmiObjectCfgPmmNic[] =
{
    /* get */
    {
        CM_OMI_CMD_GET, CM_OMI_PERMISSION_ALL,
        cm_cnm_pmm_nic_decode, cm_cnm_pmm_nic_current, cm_cnm_pmm_nic_encode,
        NULL
    },
};

const cm_omi_object_cmd_cfg_t g_CmOmiObjectCfgPmmDisk[] =
{
    /* get */
    {
        CM_OMI_CMD_GET, CM_OMI_PERMISSION_ALL,
        cm_cnm_pmm_disk_decode, cm_cnm_pmm_disk_current, cm_cnm_pmm_disk_encode,
        NULL
    },
};
const cm_omi_object_cmd_cfg_t g_CmOmiObjectCfgCnmNas_shadow[] =
{
    /* get */
    {
        CM_OMI_CMD_GET, CM_OMI_PERMISSION_ALL,
        cm_cnm_nas_shadow_decode, cm_cnm_nas_shadow_get, cm_cnm_nas_shadow_encode,
        NULL
    },
    /* getbatch */
    {
        CM_OMI_CMD_GET_BATCH, CM_OMI_PERMISSION_ALL,
        cm_cnm_nas_shadow_decode, cm_cnm_nas_shadow_getbatch, cm_cnm_nas_shadow_encode,
        NULL
    },
    /* count */
    {
        CM_OMI_CMD_COUNT, CM_OMI_PERMISSION_ALL,
        cm_cnm_nas_shadow_decode, cm_cnm_nas_shadow_count, cm_omi_encode_count,
        NULL
    },
    /* insert */
    {
        CM_OMI_CMD_ADD, CM_OMI_PERMISSION_ALL,
        cm_cnm_nas_shadow_decode, cm_cnm_nas_shadow_create, NULL,
        cm_cnm_nas_shadow_oplog_create
    },
    /* update */
    {
        CM_OMI_CMD_MODIFY, CM_OMI_PERMISSION_ALL,
        cm_cnm_nas_shadow_decode, cm_cnm_nas_shadow_update, NULL,
        cm_cnm_nas_shadow_oplog_update
    },
    /* delete */
    {
        CM_OMI_CMD_DELETE, CM_OMI_PERMISSION_ALL,
        cm_cnm_nas_shadow_decode, cm_cnm_nas_shadow_delete, NULL,
        cm_cnm_nas_shadow_oplog_delete
    },
};

const cm_omi_object_cmd_cfg_t g_CmOmiObjectCfgFcinfo[] =
{
    /* getbatch*/
    {
        CM_OMI_CMD_GET_BATCH, CM_OMI_PERMISSION_ALL,
        cm_cnm_fcinfo_decode, cm_cnm_fcinfo_getbatch, cm_cnm_fcinfo_encode,
        NULL
    },
    /* count */
    {
        CM_OMI_CMD_COUNT, CM_OMI_PERMISSION_ALL,
        cm_cnm_fcinfo_decode, cm_cnm_fcinfo_count, cm_omi_encode_count,
        NULL
    },
};

const cm_omi_object_cmd_cfg_t g_CmOmiObjectCfgCnmExplorer[] =
{
    /*getbatch*/
    {
        CM_OMI_CMD_GET_BATCH, CM_OMI_PERMISSION_ALL,
        cm_cnm_explorer_decode, cm_cnm_explorer_getbatch, cm_cnm_explorer_encode,
        NULL
    },
    /*get*/
    {
        CM_OMI_CMD_GET, CM_OMI_PERMISSION_ALL,
        cm_cnm_explorer_decode, cm_cnm_explorer_get, cm_cnm_explorer_encode,
        NULL
    },
    /*count*/
    {
        CM_OMI_CMD_COUNT, CM_OMI_PERMISSION_ALL,
        cm_cnm_explorer_decode, cm_cnm_explorer_count, cm_omi_encode_count,
        NULL
    },
    /*add*/
    {
        CM_OMI_CMD_ADD, CM_OMI_PERMISSION_ALL,
        cm_cnm_explorer_decode, cm_cnm_explorer_create, NULL,
        NULL
    },
    {
        CM_OMI_CMD_DELETE, CM_OMI_PERMISSION_ALL,
        cm_cnm_explorer_decode, cm_cnm_explorer_delete, NULL,
        NULL
    },
    {
        CM_OMI_CMD_MODIFY, CM_OMI_PERMISSION_ALL,
        cm_cnm_explorer_decode, cm_cnm_explorer_modify, NULL,
        NULL
    },
};

const cm_omi_object_cmd_cfg_t g_CmOmiObjectCfgIpf[] =
{
    /* get batch */
    {
        CM_OMI_CMD_GET_BATCH, CM_OMI_PERMISSION_ALL,
        cm_cnm_ipf_decode, cm_cnm_ipf_getbatch, cm_cnm_ipf_encode,
        NULL
    },
    /* count */
    {
        CM_OMI_CMD_COUNT, CM_OMI_PERMISSION_ALL,
        cm_cnm_ipf_decode, cm_cnm_ipf_count, cm_omi_encode_count,
        NULL
    },
    /* add */
    {
        CM_OMI_CMD_ADD, CM_OMI_PERMISSION_ALL,
        cm_cnm_ipf_decode, cm_cnm_ipf_insert, NULL,
        cm_cnm_ipf_oplog_insert
    },
    /* update */
    {
        CM_OMI_CMD_MODIFY, CM_OMI_PERMISSION_ALL,
        cm_cnm_ipf_decode, cm_cnm_ipf_update, NULL,
        cm_cnm_ipf_oplog_update
    },
    /* delete */
    {
        CM_OMI_CMD_DELETE, CM_OMI_PERMISSION_ALL,
        cm_cnm_ipf_decode, cm_cnm_ipf_delete, NULL,
        cm_cnm_ipf_oplog_delete
    },
};
const cm_omi_object_cmd_cfg_t g_CmOmiObjectCfgCnmDirQuota[] =
{
    /*getbatch*/
    {
        CM_OMI_CMD_GET_BATCH, CM_OMI_PERMISSION_ALL,
        cm_cnm_dirquota_decode, cm_cnm_dirquota_getbatch, cm_cnm_dirquota_encode,
        NULL
    },
    {
        CM_OMI_CMD_GET, CM_OMI_PERMISSION_ALL,
        cm_cnm_dirquota_decode, cm_cnm_dirquota_get, cm_cnm_dirquota_encode,
        NULL
    },
    /*count*/
    {
        CM_OMI_CMD_COUNT, CM_OMI_PERMISSION_ALL,
        cm_cnm_dirquota_decode, cm_cnm_dirquota_count, cm_omi_encode_count,
        NULL
    },
    {
        CM_OMI_CMD_ADD, CM_OMI_PERMISSION_ALL,
        cm_cnm_dirquota_decode, cm_cnm_dirquota_add,NULL,
        cm_cnm_dirquota_oplog_add
    },
    {
        CM_OMI_CMD_DELETE, CM_OMI_PERMISSION_ALL,
        cm_cnm_dirquota_decode, cm_cnm_dirquota_delete,NULL,
        cm_cnm_dirquota_oplog_delete
    },
};

const cm_omi_object_cmd_cfg_t g_CmOmiObjectCfgCnmTopoPower[] =
{
    /*getbatch*/
    {
        CM_OMI_CMD_GET_BATCH, CM_OMI_PERMISSION_ALL,
        cm_cnm_topo_power_decode, cm_cnm_topo_power_getbatch, cm_cnm_topo_power_encode,
        NULL
    },
    /*count*/
    {
        CM_OMI_CMD_COUNT, CM_OMI_PERMISSION_ALL,
        cm_cnm_topo_power_decode, cm_cnm_topo_power_count, cm_omi_encode_count,
        NULL
    },
};

const cm_omi_object_cmd_cfg_t g_CmOmiObjectCfgCnmTopoFan[] =
{
    /*getbatch*/
    {
        CM_OMI_CMD_GET_BATCH, CM_OMI_PERMISSION_ALL,
        cm_cnm_topo_fan_decode, cm_cnm_topo_fan_getbatch, cm_cnm_topo_fan_encode,
        NULL
    },
    /*count*/
    {
        CM_OMI_CMD_COUNT, CM_OMI_PERMISSION_ALL,
        cm_cnm_topo_fan_decode, cm_cnm_topo_fan_count, cm_omi_encode_count,
        NULL
    },
};

const cm_omi_object_cmd_cfg_t g_CmOmiObjectCfgCnmTopoTable[] =
{
    /* get batch */
    {
        CM_OMI_CMD_GET, CM_OMI_PERMISSION_ALL,
        cm_cnm_topo_table_decode, cm_cnm_topo_table_get, cm_cnm_topo_table_encode,
        NULL
    },
    /* count */
    {
        CM_OMI_CMD_DELETE, CM_OMI_PERMISSION_ALL,
        cm_cnm_topo_table_decode, cm_cnm_topo_table_delete, NULL,
        NULL
    },
    /* get batch */
    {
        CM_OMI_CMD_GET_BATCH, CM_OMI_PERMISSION_ALL,
        cm_cnm_topo_table_decode, cm_cnm_topo_table_getbatch, cm_cnm_topo_table_encode,
        NULL
    },
    /* count */
    {
        CM_OMI_CMD_COUNT, CM_OMI_PERMISSION_ALL,
        cm_cnm_topo_table_decode, cm_cnm_topo_table_count, cm_omi_encode_count,
        NULL
    },
    {
        CM_OMI_CMD_MODIFY, CM_OMI_PERMISSION_ALL,
        cm_cnm_topo_table_decode, cm_cnm_topo_table_update, NULL,
        NULL
    },
    {
        CM_OMI_CMD_ADD, CM_OMI_PERMISSION_ALL,
        cm_cnm_topo_table_decode, cm_cnm_topo_table_insert, NULL,
        NULL
    },
    {
        CM_OMI_CMD_OFF, CM_OMI_PERMISSION_ALL,
        cm_cnm_topo_table_decode, cm_cnm_topo_table_off, NULL,
        NULL
    },
};

const cm_omi_object_cmd_cfg_t g_CmOmiObjectCfgCnmNtp[] =
{
     /* insert */
    {
        CM_OMI_CMD_ADD, CM_OMI_PERMISSION_ALL,
        cm_cnm_ntp_decode, cm_cnm_ntp_insert, NULL,
        NULL
    },
    /* delete */
    {
        CM_OMI_CMD_DELETE, CM_OMI_PERMISSION_ALL,
        cm_cnm_ntp_decode, cm_cnm_ntp_delete, NULL,
        NULL
    },
    /* get */
    {
        CM_OMI_CMD_GET, CM_OMI_PERMISSION_ALL,
        cm_cnm_ntp_decode, cm_cnm_ntp_get, cm_cnm_ntp_encode,
        NULL
    },
};

const cm_omi_object_cmd_cfg_t g_CmOmiObjectCfgCnmThreshold[] =
{
    /* get */
    {
        CM_OMI_CMD_GET, CM_OMI_PERMISSION_ALL,
        cm_cnm_threshold_decode, cm_cnm_threshold_get, cm_cnm_threshold_encode,
        NULL
    },
    /* get batch */
    {
        CM_OMI_CMD_GET_BATCH, CM_OMI_PERMISSION_ALL,
        cm_cnm_threshold_decode, cm_cnm_threshold_getbatch, cm_cnm_threshold_encode,
        NULL
    },
    /* count */
    {
        CM_OMI_CMD_COUNT, CM_OMI_PERMISSION_ALL,
        cm_cnm_threshold_decode, cm_cnm_threshold_count, cm_omi_encode_count,
        NULL
    },
    {
        CM_OMI_CMD_MODIFY, CM_OMI_PERMISSION_ALL,
        cm_cnm_threshold_decode, cm_cnm_threshold_update, NULL,
        NULL
    },
};

const cm_omi_object_cmd_cfg_t g_CmOmiObjectCfgCnmLowdata[] =
{
    /* count */
    {
        CM_OMI_CMD_COUNT, CM_OMI_PERMISSION_ALL,
        cm_cnm_lowdata_decode, cm_cnm_lowdata_count, cm_omi_encode_count,
        NULL
    },
    /* get batch */
    {
        CM_OMI_CMD_GET_BATCH, CM_OMI_PERMISSION_ALL,
        cm_cnm_lowdata_decode, cm_cnm_lowdata_getbatch, cm_cnm_lowdata_encode,
        NULL
    },
    /* get  */
    {
        CM_OMI_CMD_GET, CM_OMI_PERMISSION_ALL,
        cm_cnm_lowdata_decode, cm_cnm_lowdata_get, cm_cnm_lowdata_encode,
        NULL
    },
    {
        CM_OMI_CMD_MODIFY, CM_OMI_PERMISSION_ALL,
        cm_cnm_lowdata_decode, cm_cnm_lowdata_update, NULL,
        cm_cnm_lowdata_oplog_update
    },
};

const cm_omi_object_cmd_cfg_t g_CmOmiObjectCfgCnmDirLowdata[] =
{
    /* count */
    {
        CM_OMI_CMD_COUNT, CM_OMI_PERMISSION_ALL,
        cm_cnm_dirlowdata_decode, cm_cnm_dirlowdata_count, cm_omi_encode_count,
        NULL
    },
    /* get batch */
    {
        CM_OMI_CMD_GET_BATCH, CM_OMI_PERMISSION_ALL,
        cm_cnm_dirlowdata_decode, cm_cnm_dirlowdata_getbatch, cm_cnm_dirlowdata_encode,
        NULL
    },
    {
        CM_OMI_CMD_GET, CM_OMI_PERMISSION_ALL,
        cm_cnm_dirlowdata_decode, cm_cnm_dirlowdata_get, cm_cnm_dirlowdata_encode,
        NULL
    },
    {
        CM_OMI_CMD_MODIFY, CM_OMI_PERMISSION_ALL,
        cm_cnm_dirlowdata_decode, cm_cnm_dirlowdata_update, NULL,
        cm_cnm_dirlowdata_oplog_update
    },
};

const cm_omi_object_cmd_cfg_t g_CmOmiObjectCfgCnmDomainUser[] =
{
    /* count */
    {
        CM_OMI_CMD_COUNT, CM_OMI_PERMISSION_ALL,
        cm_cnm_domain_user_decode, cm_cnm_domain_user_count, cm_omi_encode_count,
        NULL
    },
    /* get batch */
    {
        CM_OMI_CMD_GET_BATCH, CM_OMI_PERMISSION_ALL,
        cm_cnm_domain_user_decode, cm_cnm_domain_user_getbatch, cm_cnm_domain_user_encode,
        NULL
    },
    {
        CM_OMI_CMD_ADD, CM_OMI_PERMISSION_ALL,
        cm_cnm_domain_user_decode, cm_cnm_domain_user_create, NULL,
        cm_cnm_domain_user_oplog_create
    },
    {
        CM_OMI_CMD_DELETE,CM_OMI_PERMISSION_ALL,
        cm_cnm_domain_user_decode, cm_cnm_domain_user_delete, NULL,
        cm_cnm_domain_user_oplog_delete
    },
};

const cm_omi_object_cmd_cfg_t g_CmOmiObjectCfgCnmDns[] =
{
    /* get batch */
    {
        CM_OMI_CMD_GET, CM_OMI_PERMISSION_ALL,
        cm_cnm_dns_decode, cm_cnm_dns_get, cm_cnm_dns_encode,
        NULL
    },
    /* count */
    {
        CM_OMI_CMD_DELETE, CM_OMI_PERMISSION_ALL,
        cm_cnm_dns_decode, cm_cnm_dns_delete, NULL,
        NULL
    },
    {
        CM_OMI_CMD_ADD, CM_OMI_PERMISSION_ALL,
        cm_cnm_dns_decode, cm_cnm_dns_insert, NULL,
        NULL
    },
};

const cm_omi_object_cmd_cfg_t g_CmOmiObjectCfgCnmDomianAd[] =
{
    /* get batch */
    {
        CM_OMI_CMD_GET, CM_OMI_PERMISSION_ALL,
        cm_cnm_domain_ad_decode, cm_cnm_domain_ad_get, cm_cnm_domain_ad_encode,
        NULL
    },
    {
        CM_OMI_CMD_ADD, CM_OMI_PERMISSION_ALL,
        cm_cnm_domain_ad_decode, cm_cnm_domain_ad_insert, NULL,
        NULL
    },
    {
        CM_OMI_CMD_DELETE, CM_OMI_PERMISSION_ALL,
        cm_cnm_domain_ad_decode, cm_cnm_domain_ad_delete, NULL,
        NULL
    },
};

const cm_omi_object_cmd_cfg_t g_CmOmiObjectCfgCnmLowdataSche[] =
{
    /* count */
    {
        CM_OMI_CMD_COUNT, CM_OMI_PERMISSION_ALL,
        cm_cnm_lowdata_sche_decode, cm_cnm_lowdata_sche_count, cm_omi_encode_count,
        NULL
    },
    /* get batch */
    {
        CM_OMI_CMD_GET_BATCH, CM_OMI_PERMISSION_ALL,
        cm_cnm_lowdata_sche_decode, cm_cnm_lowdata_sche_getbatch, cm_cnm_lowdata_sche_encode,
        NULL
    },
    {
        CM_OMI_CMD_DELETE, CM_OMI_PERMISSION_ALL,
        cm_cnm_lowdata_sche_decode, cm_cnm_lowdata_sche_delete, NULL,
        NULL
    },
    {
        CM_OMI_CMD_MODIFY, CM_OMI_PERMISSION_ALL,
        cm_cnm_lowdata_sche_decode, cm_cnm_lowdata_sche_update, NULL,
        NULL,
    },
    {
        CM_OMI_CMD_ADD, CM_OMI_PERMISSION_ALL,
        cm_cnm_lowdata_sche_decode, cm_cnm_lowdata_sche_create, NULL,
        NULL,
    },
};

const cm_omi_object_cmd_cfg_t g_CmOmiObjectCfgCnmNasClient[] =
{
    /* count */
    {
        CM_OMI_CMD_COUNT, CM_OMI_PERMISSION_ALL,
        cm_cnm_nas_client_decode, cm_cnm_nas_client_count, cm_omi_encode_count,
        NULL
    },
    /* get batch */
    {
        CM_OMI_CMD_GET_BATCH, CM_OMI_PERMISSION_ALL,
        cm_cnm_nas_client_decode, cm_cnm_nas_client_getbatch, cm_cnm_nas_client_encode,
        NULL
    },
};

const cm_omi_object_cmd_cfg_t g_CmOmiObjectCfgCnmPoolext[] =
{
    /* create */
    {
        CM_OMI_CMD_ADD, CM_OMI_PERMISSION_ALL,
        cm_cnm_poolext_decode, cm_cnm_poolext_create, NULL,
        cm_cnm_poolext_oplog_create
    },
    /* add */
    {
        CM_OMI_CMD_MODIFY, CM_OMI_PERMISSION_ALL,
        cm_cnm_poolext_decode, cm_cnm_poolext_add, NULL,
        cm_cnm_poolext_oplog_add
    },
};

const cm_omi_object_cmd_cfg_t g_CmOmiObjectCfgCnmUpgrade[] =
{
    /* count */
    {
        CM_OMI_CMD_COUNT, CM_OMI_PERMISSION_ALL,
        cm_cnm_upgrade_decode, cm_cnm_upgrade_count, cm_omi_encode_count,
        NULL
    },
    /* get batch */
    {
        CM_OMI_CMD_GET_BATCH, CM_OMI_PERMISSION_ALL,
        cm_cnm_upgrade_decode, cm_cnm_upgrade_getbatch, cm_cnm_upgrade_encode,
        NULL
    },
    /* add */
    {
        CM_OMI_CMD_ADD, CM_OMI_PERMISSION_ALL,
        cm_cnm_upgrade_decode, cm_cnm_upgrade_insert, NULL,
        NULL
    },
};

const cm_omi_object_cmd_cfg_t g_CmOmiObjectCfgCnmLowdataVolume[] =
{
    /* add */
    {
        CM_OMI_CMD_ADD, CM_OMI_PERMISSION_ALL,
        cm_cnm_lowdata_volume_decode, cm_cnm_lowdata_volume_create, cm_omi_encode_taskid,
        NULL
    },
    /* get */
    {
        CM_OMI_CMD_GET, CM_OMI_PERMISSION_ALL,
        cm_cnm_lowdata_volume_decode, cm_cnm_lowdata_volume_get, cm_cnm_lowdata_volume_encode,
        NULL
    },
};
const cm_omi_object_cmd_cfg_t g_CmOmiObjectCfgCnmMailsend[] =
{
    /* get */
    {
        CM_OMI_CMD_GET, CM_OMI_PERMISSION_ALL,
        cm_cnm_mailsend_decode, cm_cnm_mailsend_get, cm_cnm_mailsend_encode,
        NULL
    },
    {
        CM_OMI_CMD_MODIFY, CM_OMI_PERMISSION_ALL,
        cm_cnm_mailsend_decode, cm_cnm_mailsend_update, NULL,
        NULL
    },
    {
        CM_OMI_CMD_TEST, CM_OMI_PERMISSION_ALL,
        cm_cnm_mailsend_decode, cm_cnm_mailsend_test, NULL,
        NULL
    },
};

const cm_omi_object_cmd_cfg_t g_CmOmiObjectCfgCnmMailrecv[] =
{
    /* getbatch */
    {
        CM_OMI_CMD_GET_BATCH, CM_OMI_PERMISSION_ALL,
        cm_cnm_mailrecv_decode, cm_cnm_mailrecv_getbatch, cm_cnm_mailrecv_encode,
        NULL
    },
    /* get */
    {
        CM_OMI_CMD_GET, CM_OMI_PERMISSION_ALL,
        cm_cnm_mailrecv_decode, cm_cnm_mailrecv_get, cm_cnm_mailrecv_encode,
        NULL
    },
    /* count */
    {
        CM_OMI_CMD_DELETE, CM_OMI_PERMISSION_ALL,
        cm_cnm_mailrecv_decode, cm_cnm_mailrecv_delete, NULL,
        NULL
    },
    /* count */
    {
        CM_OMI_CMD_COUNT, CM_OMI_PERMISSION_ALL,
        cm_cnm_mailrecv_decode, cm_cnm_mailrecv_count, cm_omi_encode_count,
        NULL
    },
    {
        CM_OMI_CMD_MODIFY, CM_OMI_PERMISSION_ALL,
        cm_cnm_mailrecv_decode, cm_cnm_mailrecv_update, NULL,
        NULL
    },
    {
        CM_OMI_CMD_ADD, CM_OMI_PERMISSION_ALL,
        cm_cnm_mailrecv_decode, cm_cnm_mailrecv_insert, NULL,
        NULL
    },
    {
        CM_OMI_CMD_TEST, CM_OMI_PERMISSION_ALL,
        cm_cnm_mailrecv_decode, cm_cnm_mailrecv_test, NULL,
        NULL
    },
};

const cm_omi_object_cmd_cfg_t g_CmOmiObjectCfgCnmNascopy[] =
{
    /* count */
    {
        CM_OMI_CMD_COUNT, CM_OMI_PERMISSION_ALL,
        cm_cnm_nas_copy_decode, cm_cnm_nas_copy_count, cm_omi_encode_count,
        NULL
    },
   
    /* update */
    {
        CM_OMI_CMD_MODIFY, CM_OMI_PERMISSION_ALL,
        cm_cnm_nas_copy_decode, cm_cnm_nas_copy_update, NULL,
        NULL
    },
};

const cm_omi_object_cmd_cfg_t g_CmOmiObjectCfgCnmUcache[] =
{
    /* getbatch */
    {
        CM_OMI_CMD_GET_BATCH, CM_OMI_PERMISSION_ALL,
        cm_cnm_ucache_decode, cm_cnm_ucache_getbatch, cm_cnm_ucache_encode
    },
   
    /* count */
    {
        CM_OMI_CMD_COUNT, CM_OMI_PERMISSION_ALL,
        cm_cnm_ucache_decode, cm_cnm_ucache_count, cm_omi_encode_count,
        NULL
    },

    /* test */
    {
        CM_OMI_CMD_TEST, CM_OMI_PERMISSION_ALL,
        cm_cnm_ucache_decode, cm_cnm_ucache_test,NULL,
        NULL
    },
};

const cm_omi_object_cmd_cfg_t g_CmOmiObjectCfgCnmRemoteCluster[] =
{
    /* getbatch */
    {
        CM_OMI_CMD_GET_BATCH, CM_OMI_PERMISSION_ALL,
        cm_cnm_cluster_remote_decode, cm_cnm_cluster_remote_getbatch, cm_cnm_cluster_remote_encode
    },
   
    /* count */
    {
        CM_OMI_CMD_COUNT, CM_OMI_PERMISSION_ALL,
        cm_cnm_cluster_remote_decode, cm_cnm_cluster_remote_count, cm_omi_encode_count,
        NULL
    },

    {
        CM_OMI_CMD_ADD, CM_OMI_PERMISSION_ALL,
        cm_cnm_cluster_remote_decode, cm_cnm_cluster_remote_insert, NULL,
        NULL
    },

    {
        CM_OMI_CMD_DELETE, CM_OMI_PERMISSION_ALL,
        cm_cnm_cluster_remote_decode, cm_cnm_cluster_remote_delete, NULL,
        NULL
    },
    {
        CM_OMI_CMD_MODIFY, CM_OMI_PERMISSION_ALL,
        cm_cnm_cluster_remote_decode, cm_cnm_cluster_remote_update, NULL,
        NULL
    },
};

const cm_omi_object_cmd_cfg_t g_CmOmiObjectCfgCnmLocalTask[] =
{
    /* getbatch */
    {
        CM_OMI_CMD_GET_BATCH, CM_OMI_PERMISSION_ALL,
        cm_cnm_localtask_decode, cm_cnm_localtask_getbatch, cm_cnm_localtask_encode
    },
   
    /* count */
    {
        CM_OMI_CMD_COUNT, CM_OMI_PERMISSION_ALL,
        cm_cnm_localtask_decode, cm_cnm_localtask_count, cm_omi_encode_count,
        NULL
    },

    {
        CM_OMI_CMD_GET, CM_OMI_PERMISSION_ALL,
        cm_cnm_localtask_decode, cm_cnm_localtask_get, cm_cnm_localtask_encode,
        NULL
    },

    {
        CM_OMI_CMD_DELETE, CM_OMI_PERMISSION_ALL,
        cm_cnm_localtask_decode, cm_cnm_localtask_delete, NULL,
        NULL
    },
};


#define CM_OMI_OBJ_CFG(id,cfg,init) \
    {(id),sizeof(cfg)/sizeof(cm_omi_object_cmd_cfg_t),(cfg),(init)}

const cm_omi_object_cfg_t g_CmOmiObjectCfg[CM_OMI_OBJECT_BUTT] =
{
    /* Demo */
    {CM_OMI_OBJECT_DEMO, 0, NULL, NULL},
    /* Cluster */
    CM_OMI_OBJ_CFG(CM_OMI_OBJECT_CLUSTER, g_CmOmiObjectCfgCluster, cm_cnm_cluster_init),
    /* Node */
    CM_OMI_OBJ_CFG(CM_OMI_OBJECT_NODE, g_CmOmiObjectCfgNode, cm_cnm_node_init),
    /* PmmCluster */
    CM_OMI_OBJ_CFG(CM_OMI_OBJECT_PMM_CLUSTER, g_CmOmiObjectCfgPmmCluster, cm_cnm_pmm_init),
    /* PmmNode */
    CM_OMI_OBJ_CFG(CM_OMI_OBJECT_PMM_NODE, g_CmOmiObjectCfgPmmNode, NULL),
    /* AlarmCurrent*/
    CM_OMI_OBJ_CFG(CM_OMI_OBJECT_ALARM_CURRENT, g_CmOmiObjectCfgAlarmCurrent, NULL),
    /* AlarmHistory*/
    CM_OMI_OBJ_CFG(CM_OMI_OBJECT_ALARM_HISTORY, g_CmOmiObjectCfgAlarmHistory, cm_cnm_storage_alarm_init),
    /* AlarmConfig*/
    CM_OMI_OBJ_CFG(CM_OMI_OBJECT_ALARM_CONFIG, g_CmOmiObjectCfgAlarmConfig, NULL),
    /* SysTime*/
    CM_OMI_OBJ_CFG(CM_OMI_OBJECT_SYS_TIME, g_CmOmiObjectSysTimeConfig, NULL),
    /* Phys */
    CM_OMI_OBJ_CFG(CM_OMI_OBJECT_PHYS, g_CmOmiObjectPhysConfig, cm_cnm_phys_init),
    /* PmmConfig */
    CM_OMI_OBJ_CFG(CM_OMI_OBJECT_PMM_CONFIG, g_CmOmiObjectCfgPmmConfig, NULL),
    /* Admin */
    CM_OMI_OBJ_CFG(CM_OMI_OBJECT_ADMIN, g_CmOmiObjectCfgAdmin, cm_cnm_admin_init),
    /* Session */
    CM_OMI_OBJ_CFG(CM_OMI_OBJECT_SESSION, g_CmOmiObjectCfgSession, cm_cnm_session_init),
    /* Disk */
    CM_OMI_OBJ_CFG(CM_OMI_OBJECT_DISK, g_CmOmiObjectCfgDisk, cm_cnm_disk_init),
    /* Pool */
    CM_OMI_OBJ_CFG(CM_OMI_OBJECT_POOL, g_CmOmiObjectCfgPoolList, cm_cnm_pool_init),
    /* PoolDisk */
    CM_OMI_OBJ_CFG(CM_OMI_OBJECT_POOLDISK, g_CmOmiObjectCfgPoolDisk, NULL),
    /* Lun */
    CM_OMI_OBJ_CFG(CM_OMI_OBJECT_LUN, g_CmOmiObjectCfgLun, cm_cnm_lun_init),
    /* Snapshot */
    CM_OMI_OBJ_CFG(CM_OMI_OBJECT_SNAPSHOT, g_CmOmiObjectCfgSnapshot, cm_cnm_snapshot_init),
    /* SnapshotSche */
    CM_OMI_OBJ_CFG(CM_OMI_OBJECT_SNAPSHOT_SCHE, g_CmOmiObjectCfgSnapshotSche, NULL),
    /* host group */
    CM_OMI_OBJ_CFG(CM_OMI_OBJECT_HOSTGROUP, g_CmOmiObjectCfgHostGroup, cm_cnm_hg_init),
    /* target */
    CM_OMI_OBJ_CFG(CM_OMI_OBJECT_TARGET, g_CmOmiObjectCfgTarget, cm_cnm_target_init),
    /* Lunmap */
    CM_OMI_OBJ_CFG(CM_OMI_OBJECT_LUNMAP, g_CmOmiObjectCfgLunmap, cm_cnm_lunmap_init),
    /* DiskSpare */
    CM_OMI_OBJ_CFG(CM_OMI_OBJECT_DISK_SPARE, g_CmOmiObjectCfgDiskSpare, cm_cnm_diskspare_init),
    /* Nas */
    CM_OMI_OBJ_CFG(CM_OMI_OBJECT_NAS, g_CmOmiObjectCfgNas, cm_cnm_nas_init),
    /* Cifs */
    CM_OMI_OBJ_CFG(CM_OMI_OBJECT_CIFS, g_CmOmiObjectCfgCifs, cm_cnm_cifs_init),
    /*capacity*/
    CM_OMI_OBJ_CFG(CM_OMI_OBJECT_CAPACITY, g_CmOmiObjectCfgClusterStat, cm_cnm_cluster_stat_init),
    /*User*/
    CM_OMI_OBJ_CFG(CM_OMI_OBJECT_USER, g_CmOmiObjectCfgUser, cm_cnm_user_init),
    /*Group*/
    CM_OMI_OBJ_CFG(CM_OMI_OBJECT_GROUP, g_CmOmiObjectCfgGroup, cm_cnm_group_init),
    /* Share nfs */
    CM_OMI_OBJ_CFG(CM_OMI_OBJECT_SHARE_NFS, g_CmOmiObjectCfgNfs, cm_cnm_nfs_init),
    /* Snapshot Backup */
    CM_OMI_OBJ_CFG(CM_OMI_OBJECT_SNAPSHOT_BACKUP, g_CmOmiObjectCfgSnapshotBackup, cm_cnm_snapshot_backup_init),
    /* Task Manager */
    CM_OMI_OBJ_CFG(CM_OMI_OBJECT_TASK_MANAGER, g_CmOmiObjectCfgTaskManager, cm_cnm_task_manager_init),
    /* Remote */
    CM_OMI_OBJ_CFG(CM_OMI_OBJECT_REMOTE, g_CmOmiObjectCfgRemote, cm_cnm_remote_init),
    /* Ping */
    CM_OMI_OBJ_CFG(CM_OMI_OBJECT_PING, g_CmOmiObjectCfgPing, cm_cnm_ping_init),
    CM_OMI_OBJ_CFG(CM_OMI_OBJECT_QUOTA, g_CmOmiObjectCfgQuota, cm_cnm_quota_init),
    CM_OMI_OBJ_CFG(CM_OMI_OBJECT_IPSHFIT, g_CmOmiObjectCfgIpshift, cm_cnm_ipshift_init),
    /* Snapshot Clone */
    CM_OMI_OBJ_CFG(CM_OMI_OBJECT_SNAPSHOT_CLONE, g_CmOmiObjectCfgSnapshotClone, cm_cnm_snapshot_clone_init),
    CM_OMI_OBJ_CFG(CM_OMI_OBJECT_ROUTE, g_CmOmiObjectCfgRoute, cm_cnm_route_init),

    CM_OMI_OBJ_CFG(CM_OMI_OBJECT_PHYS_IP, g_CmOmiObjectCfgPhys_ip, cm_cnm_phys_ip_init),
    /*Cluster nas*/
    CM_OMI_OBJ_CFG(CM_OMI_OBJECT_CLUSTER_NAS, g_CmOmiObjectCfgClusterNas, cm_cnm_cluster_nas_init),
    CM_OMI_OBJ_CFG(CM_OMI_OBJECT_DUALLIVE_NAS, g_CmOmiObjectCfgDualliveNas, cm_cnm_duallive_nas_init),
    CM_OMI_OBJ_CFG(CM_OMI_OBJECT_BACKUP_NAS, g_CmOmiObjectCfgBackUpNas, cm_cnm_backup_nas_init),

    CM_OMI_OBJ_CFG(CM_OMI_OBJECT_LUN_MIRROR, g_CmOmiObjectCfgLunMirror, NULL),
    CM_OMI_OBJ_CFG(CM_OMI_OBJECT_LUN_BACKUP, g_CmOmiObjectCfgLunBackup, NULL),
    CM_OMI_OBJ_CFG(CM_OMI_OBJECT_CLUSTERSAN_TARGET, g_CmOmiObjectCfgClusterTarget, cm_cnm_cluster_target_init),
    CM_OMI_OBJ_CFG(CM_OMI_OBJECT_MIGRATE, g_CmOmiObjectCfgMigrate, cm_cnm_migrate_init),
    /*Pool_Reconstruct*/
    CM_OMI_OBJ_CFG(CM_OMI_OBJECT_POOL_RECONSTRUCT, g_CmOmiObjectCfgPoolReconstruct, cm_cnm_pool_reconstruct_init),
    CM_OMI_OBJ_CFG(CM_OMI_OBJECT_NODE_SERVCE, g_CmOmiObjectCfgNodeService, NULL),
    CM_OMI_OBJ_CFG(CM_OMI_OBJECT_AGGR, g_CmOmiObjectCfgAggr, cm_cnm_aggr_init),
	/*Dnsm*/
    CM_OMI_OBJ_CFG(CM_OMI_OBJECT_DNSM, g_CmOmiObjectCfgDnsm,cm_cnm_dnsm_init),

    CM_OMI_OBJ_CFG(CM_OMI_OBJECT_PERMISSION, g_CmOmiObjectCfgPermission,NULL),
    CM_OMI_OBJ_CFG(CM_OMI_OBJECT_PMM_POOL, g_CmOmiObjectCfgPmmPool,NULL),
	CM_OMI_OBJ_CFG(CM_OMI_OBJECT_PMM_NAS,g_CmOmiObjectCfgPmmNas,NULL),
	CM_OMI_OBJ_CFG(CM_OMI_OBJECT_PMM_SAS,g_CmOmiObjectCfgPmmSas,NULL),
	CM_OMI_OBJ_CFG(CM_OMI_OBJECT_PMM_PROTO,g_CmOmiObjectCfgPmmProto,NULL),
	CM_OMI_OBJ_CFG(CM_OMI_OBJECT_PMM_CACHE,g_CmOmiObjectCfgPmmCache,NULL),
	
	CM_OMI_OBJ_CFG(CM_OMI_OBJECT_CNM_SAS,g_CmOmiObjectCfgCnmSas,NULL),
	CM_OMI_OBJ_CFG(CM_OMI_OBJECT_CNM_PROTO,g_CmOmiObjectCfgCnmProto,NULL),
	/*pmm_lun*/
    CM_OMI_OBJ_CFG(CM_OMI_OBJECT_PMM_LUN, g_CmOmiObjectCfgPmmLun,cm_cnm_pmm_lun_init),
    /*pmm_nic*/
    CM_OMI_OBJ_CFG(CM_OMI_OBJECT_PMM_NIC, g_CmOmiObjectCfgPmmNic,cm_cnm_pmm_nic_init),
    /*pmm_disk*/
    CM_OMI_OBJ_CFG(CM_OMI_OBJECT_PMM_DISK, g_CmOmiObjectCfgPmmDisk,cm_cnm_pmm_disk_init),
	CM_OMI_OBJ_CFG(CM_OMI_OBJECT_CNM_NAS_SHADOW,g_CmOmiObjectCfgCnmNas_shadow,cm_cnm_nas_shadow_init),
	/*fcinfo*/
    CM_OMI_OBJ_CFG(CM_OMI_OBJECT_FCINFO, g_CmOmiObjectCfgFcinfo,cm_cnm_fcinfo_init),
	CM_OMI_OBJ_CFG(CM_OMI_OBJECT_EXPLORER,g_CmOmiObjectCfgCnmExplorer,cm_cnm_explorer_init),
	 /*ipf*/
    CM_OMI_OBJ_CFG(CM_OMI_OBJECT_IPF, g_CmOmiObjectCfgIpf,cm_cnm_ipf_init),
	CM_OMI_OBJ_CFG(CM_OMI_OBJECT_DIRQUOTA,g_CmOmiObjectCfgCnmDirQuota,NULL),
	CM_OMI_OBJ_CFG(CM_OMI_OBJECT_TOPO_POWER,g_CmOmiObjectCfgCnmTopoPower,NULL),
	CM_OMI_OBJ_CFG(CM_OMI_OBJECT_TOPO_FAN,g_CmOmiObjectCfgCnmTopoFan,NULL),
	CM_OMI_OBJ_CFG(CM_OMI_OBJECT_TOPO_TABLE,g_CmOmiObjectCfgCnmTopoTable,cm_cnm_topo_table_init),
	CM_OMI_OBJ_CFG(CM_OMI_OBJECT_NTP,g_CmOmiObjectCfgCnmNtp,cm_cnm_ntp_init),
	CM_OMI_OBJ_CFG(CM_OMI_OBJECT_THRESHOLD,g_CmOmiObjectCfgCnmThreshold,NULL),
	CM_OMI_OBJ_CFG(CM_OMI_OBJECT_LOWDATA,g_CmOmiObjectCfgCnmLowdata,NULL),
	CM_OMI_OBJ_CFG(CM_OMI_OBJECT_DIRLOWDATA,g_CmOmiObjectCfgCnmDirLowdata,NULL),
	CM_OMI_OBJ_CFG(CM_OMI_OBJECT_DOMAIN_USER,g_CmOmiObjectCfgCnmDomainUser,NULL),
	CM_OMI_OBJ_CFG(CM_OMI_OBJECT_DNS,g_CmOmiObjectCfgCnmDns,NULL),
	CM_OMI_OBJ_CFG(CM_OMI_OBJECT_DOMAIN_AD,g_CmOmiObjectCfgCnmDomianAd,NULL),
	CM_OMI_OBJ_CFG(CM_OMI_OBJECT_LOWDATA_SCHE,g_CmOmiObjectCfgCnmLowdataSche,NULL),
    CM_OMI_OBJ_CFG(CM_OMI_OBJECT_NAS_CLIENT,g_CmOmiObjectCfgCnmNasClient,NULL),

	CM_OMI_OBJ_CFG(CM_OMI_OBJECT_POOLEXT,g_CmOmiObjectCfgCnmPoolext,NULL),
	CM_OMI_OBJ_CFG(CM_OMI_OBJECT_UPGRADE,g_CmOmiObjectCfgCnmUpgrade,cm_cnm_upgrade_init),
	CM_OMI_OBJ_CFG(CM_OMI_OBJECT_LOWDATA_VOLUME,g_CmOmiObjectCfgCnmLowdataVolume,cm_cnm_lowdata_volume_init),
	CM_OMI_OBJ_CFG(CM_OMI_OBJECT_MAILSEND,g_CmOmiObjectCfgCnmMailsend,cm_cnm_mailsend_init),
	CM_OMI_OBJ_CFG(CM_OMI_OBJECT_MAILRECV,g_CmOmiObjectCfgCnmMailrecv,cm_cnm_mailrecv_init),
	CM_OMI_OBJ_CFG(CM_OMI_OBJECT_NASCOPY,g_CmOmiObjectCfgCnmNascopy,cm_cnm_nas_copy_init),
	
	CM_OMI_OBJ_CFG(CM_OMI_OBJECT_UCACHE,g_CmOmiObjectCfgCnmUcache,NULL),

	CM_OMI_OBJ_CFG(CM_OMI_OBJECT_REMOTE_CLUSTER, g_CmOmiObjectCfgCnmRemoteCluster, cm_cnm_cluster_remote_init),
	CM_OMI_OBJ_CFG(CM_OMI_OBJECT_LOCAL_TASK, g_CmOmiObjectCfgCnmLocalTask, cm_cnm_localtask_init),
  /*mem*/

};

