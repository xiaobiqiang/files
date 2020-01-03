/******************************************************************************
 *          Copyright (c) 2017 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cfg_global.c
 * author     : wbn
 * create date: 2017年10月26日
 * description: TODO:
 *
 *****************************************************************************/
#include "cm_cmt.h"
#include "cm_htbt.h"
#include "cm_sync.h"
#include "cm_pmm.h"
#include "cm_cmd_server.h"
#include "cm_alarm.h"
#include "cm_task.h"
#include "cm_cnm_demo.h"
#include "cm_cnm_alarm.h"
#include "cm_cnm_systime.h"
#include "cm_cnm_phys.h"
#include "cm_cnm_disk.h"
#include "cm_node.h"
#include "cm_cnm_pool.h"
#include "cm_cnm_common.h"
#include "cm_cnm_san.h"
#include "cm_cnm_snapshot.h"
#include "cm_sche.h"
#include "cm_cnm_node.h"
#include "cm_cnm_nas.h"
#include "cm_cnm_admin.h"
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
#include "cm_cnm_pmm.h"
#include "cm_cnm_dirquota.h"
#include "cm_cnm_topo_power.h"
#include "cm_cnm_expansion_cabinet.h"


extern sint32 cm_cnm_cluster_stat_sync_request(uint64 data_id, void *pdata, uint32 
len);
/* ================= start CFG for cm_cmt===================================*/
/* 根据CM_CmtMsgType定义顺序配置 */
const cm_cmt_config_t g_CmCmtCbks[CM_CMT_MSG_BUTT] =
{
    {CM_CMT_MSG_HTBT, cm_htbt_cbk_recv},
    {CM_CMT_MSG_SYNC_GET_STEP, cm_sync_cbk_cmt_obj_step},
    {CM_CMT_MSG_SYNC_GET_INFO, cm_sync_cbk_cmt_obj_info},
    {CM_CMT_MSG_SYNC_REQUEST, cm_sync_cbk_cmt_request},
    {CM_CMT_MSG_PMM, cm_pmm_cbk_cmt},
    {CM_CMT_MSG_CMD, cm_cmd_cbk_cmt},
    {CM_CMT_MSG_ALARM, cm_alarm_cbk_cmt},
    {CM_CMT_MSG_PHYS_GET, cm_cnm_phys_cbk_cmt},
    {CM_CMT_MSG_NODE, cm_node_cmt_cbk},
    {CM_CMT_MSG_CNM, cm_cnm_cmt_cbk},
    {CM_CMT_MSG_EXEC, cm_cnm_exec_local},
    {CM_CMT_MSG_TASK_EXEC, cm_task_cbk_cmt_exec},
    {CM_CMT_MSG_TASK_GET_STATE, cm_task_cbk_cmt_state},
    {CM_CMT_MSG_TASK_ASK_MASTER, cm_task_cbk_ask_master},
    {CM_CMT_MSG_NODE_ADD, cm_node_cmt_cbk_add},
    {CM_CMT_MSG_NODE_DELETE, cm_node_cmt_cbk_delete},
    {CM_CMT_MSG_SYNC_TOMASTER, cm_sync_cbk_cmt_tomaster},
};
/* ================= end   CFG for cm_cmt===================================*/


/* ================= end   CFG for cm_sync===================================*/

extern sint32 cm_pmm_cbk_sync_request(uint64 data_id, void *pdata, uint32 len);

const cm_sync_config_t g_cm_sync_config[CM_SYNC_OBJ_BUTT] =
{
    {
        CM_SYNC_OBJ_DEMO,
        CM_SYNC_FLAG_MULTI_RECORD,
        NULL,
        NULL,
        NULL
    },
    {
        CM_SYNC_OBJ_PMM,
        CM_SYNC_FLAG_MASTER_DOMAIN | CM_SYNC_FLAG_MULTI_RECORD | CM_SYNC_FLAG_REAL_TIME_ONLY,
        cm_pmm_cbk_sync_request, NULL, NULL
    },
    {
        CM_SYNC_OBJ_ALARM_RECORD,
        CM_SYNC_FLAG_MASTER_DOMAIN | CM_SYNC_FLAG_MULTI_RECORD,
        cm_alarm_cbk_sync_request,
        cm_alarm_cbk_sync_get,
        cm_alarm_cbk_sync_delete
    },
    {
        CM_SYNC_OBJ_ALARM_CONFIG,
        CM_SYNC_FLAG_MULTI_RECORD,
        cm_cnm_alarm_cfg_sync_request,
        cm_cnm_alarm_cfg_sync_get,
        cm_cnm_alarm_cfg_sync_delete
    },
    {
        CM_SYNC_OBJ_SYSTIME,
        CM_SYNC_FLAG_REAL_TIME_ONLY,
        cm_cnm_systime_sync_request,
        NULL,
        NULL
    },
    {
        CM_SYNC_OBJ_PMM_CONFIG,
        0,
        cm_pmm_cfg_sync_request,
        cm_pmm_cfg_sync_get,
        NULL,
    },
    {
        CM_SYNC_OBJ_ADMIN,
        0,
        cm_cnm_admin_sync_request,
        cm_cnm_admin_sync_get,
        cm_cnm_admin_sync_delete
    },
    {
        CM_SYNC_OBJ_SCHE,
        CM_SYNC_FLAG_MASTER_DOMAIN,
        cm_sche_cbk_sync_request,
        cm_sche_cbk_sync_get,
        cm_sche_cbk_sync_delete

    },
    {
        CM_SYNC_OBJ_USER,
        0,
        cm_cnm_user_sync_request,
        cm_cnm_user_sync_get,
        cm_cnm_user_sync_delete
    },
    {
        CM_SYNC_OBJ_GROUP,
        0,
        cm_cnm_group_sync_request,
        cm_cnm_group_sync_get,
        cm_cnm_group_sync_delete
    },
    {
        CM_SYNC_OBJ_TASK,
        CM_SYNC_FLAG_MASTER_DOMAIN | CM_SYNC_FLAG_REAL_TIME_ONLY,
        cm_task_cbk_sync_request,
        cm_task_cbk_sync_get,
        cm_task_cbk_sync_delete
    },
    {
        CM_SYNC_OBJ_PERMISSION,
        0,
        cm_cnm_permission_sync_request,
        cm_cnm_permission_sync_get,
        cm_cnm_permission_sync_delete
    },
    {
        CM_SYNC_OBJ_TOPO_TABLE,
        0,
        cm_cnm_topo_table_sync_request,
        cm_cnm_topo_table_sync_get,
        cm_cnm_topo_table_sync_delete
    },
    {
        CM_SYNC_OBJ_THRESHOLD,
        0,
        cm_cnm_threshold_sync_request,
        cm_cnm_threshold_sync_get,
        NULL
    },
    {
        CM_SYNC_OBJ_NTP,
        CM_SYNC_FLAG_REAL_TIME_ONLY,
        cm_cnm_ntp_sync_request,
        cm_cnm_ntp_sync_get,
        cm_cnm_ntp_sync_delete,
    },

    {
        CM_SYNC_OBJ_CLU_STAT,
        CM_SYNC_FLAG_REAL_TIME_ONLY,
        cm_cnm_cluster_stat_sync_request,
        NULL,
        NULL,
    },
    {
        CM_SYNC_OBJ_DOMAIN_USER,
        CM_SYNC_FLAG_REAL_TIME_ONLY,
        cm_cnm_domain_user_sync_request,
        NULL,
        cm_cnm_domain_user_sync_delete,
    },
	{
        CM_SYNC_OBJ_DNS,
        0,
        cm_cnm_dns_sync_request,
        NULL,
        NULL,
    },
    {
        CM_SYNC_OBJ_LOWDATA_VOLUME,
        0,
        cm_cnm_lowdata_volume_sync_request,
        NULL,
        NULL,
    },
    {
        CM_SYNC_OBJ_MAILSEND,
        0,
        cm_cnm_mailsend_sync_request,
        cm_cnm_mailsend_sync_get,
        NULL,
    },
    {
        CM_SYNC_OBJ_MAILRECV,
        0,
        cm_cnm_mailrecv_sync_request,
        cm_cnm_mailrecv_sync_get,
        cm_cnm_mailrecv_sync_delete,
    },
};

/* ================= end   CFG for cm_sync===================================*/

/* ================= start CFG for cm_cnm===================================*/
const cm_cnm_cfg_cmd_t g_cm_cnm_local_cmd_pool[] =
{
    {CM_OMI_CMD_GET_BATCH, cm_cnm_pool_local_getbatch},
    {CM_OMI_CMD_GET, cm_cnm_pool_local_get},
    {CM_OMI_CMD_COUNT, cm_cnm_pool_local_count},
    {CM_OMI_CMD_ADD, cm_cnm_pool_local_create},
    {CM_OMI_CMD_DELETE, cm_cnm_pool_local_delete},
    {CM_OMI_CMD_MODIFY, cm_cnm_pool_local_update},

    {CM_OMI_CMD_SCAN, cm_cnm_pool_local_scan},
};

const cm_cnm_cfg_cmd_t g_cm_cnm_local_cmd_disk[] =
{
    {CM_OMI_CMD_GET_BATCH, cm_cnm_disk_local_getbatch},
    {CM_OMI_CMD_GET, cm_cnm_disk_local_get},
    {CM_OMI_CMD_COUNT, cm_cnm_disk_local_count},
    {CM_OMI_CMD_MODIFY, cm_cnm_disk_local_clear},
};

const cm_cnm_cfg_cmd_t g_cm_cnm_local_cmd_pooldisk[] =
{
    {CM_OMI_CMD_GET_BATCH, cm_cnm_pooldisk_local_getbatch},
    {CM_OMI_CMD_COUNT, cm_cnm_pooldisk_local_count},
    {CM_OMI_CMD_ADD, cm_cnm_pooldisk_local_add},
    {CM_OMI_CMD_DELETE, cm_cnm_pooldisk_local_delete},
};

const cm_cnm_cfg_cmd_t g_cm_cnm_local_cmd_lun[] =
{
    {CM_OMI_CMD_GET_BATCH, cm_cnm_lun_local_getbatch},
    {CM_OMI_CMD_GET, cm_cnm_lun_local_get},
    {CM_OMI_CMD_COUNT, cm_cnm_lun_local_count},
    {CM_OMI_CMD_ADD, cm_cnm_lun_local_create},
    {CM_OMI_CMD_MODIFY, cm_cnm_lun_local_update},
    {CM_OMI_CMD_DELETE, cm_cnm_lun_local_delete},
};

const cm_cnm_cfg_cmd_t g_cm_cnm_local_cmd_snapshot[] =
{
    {CM_OMI_CMD_GET_BATCH, cm_cnm_snapshot_local_getbatch},
    {CM_OMI_CMD_GET, cm_cnm_snapshot_local_get},
    {CM_OMI_CMD_COUNT, cm_cnm_snapshot_local_count},
    {CM_OMI_CMD_ADD, cm_cnm_snapshot_local_create},
    {CM_OMI_CMD_MODIFY, cm_cnm_snapshot_local_update},
    {CM_OMI_CMD_DELETE, cm_cnm_snapshot_local_delete},
};

const cm_cnm_cfg_cmd_t g_cm_cnm_local_cmd_nas[] =
{
    {CM_OMI_CMD_GET_BATCH, cm_cnm_nas_local_getbatch},
    {CM_OMI_CMD_GET, cm_cnm_nas_local_get},
    {CM_OMI_CMD_COUNT, cm_cnm_nas_local_count},
    {CM_OMI_CMD_ADD, cm_cnm_nas_local_create},
    {CM_OMI_CMD_MODIFY, cm_cnm_nas_local_update},
    {CM_OMI_CMD_DELETE, cm_cnm_nas_local_delete},
};

const cm_cnm_cfg_cmd_t g_cm_cnm_local_cmd_cifs[] =
{
    {CM_OMI_CMD_GET_BATCH, cm_cnm_cifs_local_getbatch},
    {CM_OMI_CMD_GET, cm_cnm_cifs_local_get},
    {CM_OMI_CMD_COUNT, cm_cnm_cifs_local_count},
    {CM_OMI_CMD_ADD, cm_cnm_cifs_local_create},
    {CM_OMI_CMD_MODIFY, cm_cnm_cifs_local_update},
    {CM_OMI_CMD_DELETE, cm_cnm_cifs_local_delete},
};

const cm_cnm_cfg_cmd_t g_cm_cnm_local_cmd_nfs[] =
{
    {CM_OMI_CMD_GET_BATCH, cm_cnm_nfs_local_getbatch},
    {CM_OMI_CMD_COUNT, cm_cnm_nfs_local_count},
    {CM_OMI_CMD_ADD, cm_cnm_nfs_local_add},
    {CM_OMI_CMD_DELETE, cm_cnm_nfs_local_delete},
};

/**********************************Snapshot Backup *********************************/
const cm_cnm_cfg_cmd_t g_cm_cnm_local_cmd_snapshot_backup[] =
{
    {CM_OMI_CMD_ADD, cm_cnm_snapshot_backup_local_add},
};

/**********************************Task Manager *********************************/
const cm_cnm_cfg_cmd_t g_cm_cnm_local_cmd_task_manager[] =
{
    /* get cmd */
    {CM_OMI_CMD_GET, cm_cnm_task_manager_local_get},
    /* get batch cmd */
    {CM_OMI_CMD_GET, cm_cnm_task_manager_local_getbatch},
    /* delete cmd */
    {CM_OMI_CMD_GET, cm_cnm_task_manager_local_delete},
    /* count cmd */
    {CM_OMI_CMD_GET, cm_cnm_task_manager_local_count},
};

const cm_cnm_cfg_cmd_t g_cm_cnm_local_cmd_ping[] =
{
    {CM_OMI_CMD_ADD, cm_cnm_ping_local_add},
};

const cm_cnm_cfg_cmd_t g_cm_cnm_local_cmd_quota[] =
{
    {CM_OMI_CMD_GET_BATCH, cm_cnm_quota_local_getbatch},
    {CM_OMI_CMD_GET, cm_cnm_quota_local_get},
    {CM_OMI_CMD_COUNT, cm_cnm_quota_local_count},
    {CM_OMI_CMD_MODIFY, cm_cnm_quota_local_update},
    {CM_OMI_CMD_DELETE, cm_cnm_quota_local_delete},
};

const cm_cnm_cfg_cmd_t g_cm_cnm_local_cmd_ipshift[] =
{
    {CM_OMI_CMD_GET_BATCH, cm_cnm_ipshift_local_getbatch},
    {CM_OMI_CMD_GET, cm_cnm_ipshift_local_get},
    {CM_OMI_CMD_COUNT, cm_cnm_ipshift_local_count},
    {CM_OMI_CMD_ADD, cm_cnm_ipshift_local_create},
    {CM_OMI_CMD_DELETE, cm_cnm_ipshift_local_delete},
};

const cm_cnm_cfg_cmd_t g_cm_cnm_local_cmd_snapshot_clone[] =
{
    {CM_OMI_CMD_ADD, cm_cnm_snapshot_clone_local_add},
};

const cm_cnm_cfg_cmd_t g_cm_cnm_local_cmd_phys[] =
{
    {CM_OMI_CMD_GET_BATCH, cm_cnm_phys_local_getbatch},
    {CM_OMI_CMD_GET, cm_cnm_phys_local_get},
    {CM_OMI_CMD_COUNT, cm_cnm_phys_local_count},
    {CM_OMI_CMD_MODIFY, cm_cnm_phys_local_update},
};
const cm_cnm_cfg_cmd_t g_cm_cnm_local_cmd_route[] =
{
    {CM_OMI_CMD_GET_BATCH, cm_cnm_route_local_getbatch},
    {CM_OMI_CMD_COUNT, cm_cnm_route_local_count},
    {CM_OMI_CMD_ADD, cm_cnm_route_local_create},
    {CM_OMI_CMD_DELETE, cm_cnm_route_local_delete},
};

const cm_cnm_cfg_cmd_t g_cm_cnm_local_cmd_phys_ip[] =
{
    {CM_OMI_CMD_GET_BATCH, cm_cnm_phys_ip_local_getbatch},
    {CM_OMI_CMD_COUNT, cm_cnm_phys_ip_local_count},
    {CM_OMI_CMD_ADD, cm_cnm_phys_ip_local_create},
    {CM_OMI_CMD_DELETE, cm_cnm_phys_ip_local_delete}
};

const cm_cnm_cfg_cmd_t g_cm_cnm_local_cmd_cluster_nas[] =
{
    {CM_OMI_CMD_GET_BATCH, cm_cnm_cluster_nas_local_getbatch},
    {CM_OMI_CMD_COUNT, cm_cnm_cluster_nas_local_count},
    {CM_OMI_CMD_ADD, cm_cnm_cluster_nas_local_create},
    {CM_OMI_CMD_MODIFY, cm_cnm_cluster_nas_local_add},
    {CM_OMI_CMD_DELETE, cm_cnm_cluster_nas_local_delete},
};

const cm_cnm_cfg_cmd_t g_cm_cnm_local_cmd_duallive_nas[] =
{
    {CM_OMI_CMD_GET_BATCH, cm_cnm_duallive_nas_local_getbatch},
    {CM_OMI_CMD_COUNT, cm_cnm_duallive_nas_local_count},
    {CM_OMI_CMD_ADD, cm_cnm_duallive_nas_local_create},
    {CM_OMI_CMD_DELETE, cm_cnm_duallive_nas_local_delete},
};

const cm_cnm_cfg_cmd_t g_cm_cnm_local_cmd_backup_nas[] =
{
    {CM_OMI_CMD_GET_BATCH, cm_cnm_backup_nas_local_getbatch},
    {CM_OMI_CMD_COUNT, cm_cnm_backup_nas_local_count},
    {CM_OMI_CMD_ADD, cm_cnm_backup_nas_local_create},
    {CM_OMI_CMD_DELETE, cm_cnm_backup_nas_local_delete},
};

const cm_cnm_cfg_cmd_t g_cm_cnm_local_cmd_lun_mirror[] =
{
    {CM_OMI_CMD_GET_BATCH, cm_cnm_lunmirror_local_getbatch},
    {CM_OMI_CMD_COUNT, cm_cnm_lunmirror_local_count},
    {CM_OMI_CMD_ADD, cm_cnm_lunmirror_local_create},
    {CM_OMI_CMD_DELETE, cm_cnm_lunmirror_local_delete},
};

const cm_cnm_cfg_cmd_t g_cm_cnm_local_cmd_lun_backup[] =
{
    {CM_OMI_CMD_GET_BATCH, cm_cnm_lunbackup_local_getbatch},
    {CM_OMI_CMD_COUNT, cm_cnm_lunbackup_local_count},
    {CM_OMI_CMD_ADD, cm_cnm_lunbackup_local_create},
    {CM_OMI_CMD_MODIFY, cm_cnm_lunbackup_local_modify},
    {CM_OMI_CMD_DELETE, cm_cnm_lunbackup_local_delete},
};

const cm_cnm_cfg_cmd_t g_cm_cnm_local_cmd_cluster_target[] =
{
    {CM_OMI_CMD_GET_BATCH, cm_cnm_cluster_target_local_getbatch},
};

const cm_cnm_cfg_cmd_t g_cm_cnm_local_cmd_migrate[] =
{
    {CM_OMI_CMD_GET, cm_cnm_migrate_local_get},
    {CM_OMI_CMD_MODIFY, cm_cnm_migrate_local_update},
};

const cm_cnm_cfg_cmd_t g_cm_cnm_local_cmd_pool_reconstruct[] =
{
    {CM_OMI_CMD_GET_BATCH, cm_cnm_pool_reconstruct_local_getbatch},
    {CM_OMI_CMD_COUNT, cm_cnm_pool_reconstruct_local_count},
};

const cm_cnm_cfg_cmd_t g_cm_cnm_local_cmd_ndoe_service[] =
{
    {CM_OMI_CMD_GET_BATCH, cm_cnm_node_service_local_getbatch},
    {CM_OMI_CMD_COUNT, cm_cnm_node_service_local_count},
    {CM_OMI_CMD_MODIFY, cm_cnm_node_service_local_updata},
};

const cm_cnm_cfg_cmd_t g_cm_cnm_local_cmd_aggr[] =
{
    {CM_OMI_CMD_ADD, cm_cnm_aggr_local_create},
    {CM_OMI_CMD_GET_BATCH, cm_cnm_aggr_local_getbatch},
    {CM_OMI_CMD_COUNT, cm_cnm_aggr_local_count},
    {CM_OMI_CMD_MODIFY, cm_cnm_aggr_local_update},
    {CM_OMI_CMD_DELETE, cm_cnm_aggr_local_delete},
};

const cm_cnm_cfg_cmd_t g_cm_cnm_local_cmd_dnsm[] =
{
    {CM_OMI_CMD_GET_BATCH, cm_cnm_dnsm_local_getbatch},
    {CM_OMI_CMD_GET, cm_cnm_dnsm_local_get},
    {CM_OMI_CMD_COUNT, cm_cnm_dnsm_local_count},
    {CM_OMI_CMD_ADD, cm_cnm_dnsm_local_insert},
    {CM_OMI_CMD_MODIFY, cm_cnm_dnsm_local_update},
    {CM_OMI_CMD_DELETE, cm_cnm_dnsm_local_delete},
};

const cm_cnm_cfg_cmd_t g_cm_cnm_local_cmd_sas[] =
{
    {CM_OMI_CMD_GET_BATCH, cm_cnm_sas_local_getbatch},
    {CM_OMI_CMD_COUNT, cm_cnm_sas_local_count},
};

const cm_cnm_cfg_cmd_t g_cm_cnm_local_cmd_protocol[] =
{
    {CM_OMI_CMD_GET_BATCH, cm_cnm_protocol_local_getbatch},
    {CM_OMI_CMD_COUNT, cm_cnm_protocol_local_count},
};

const cm_cnm_cfg_cmd_t g_cm_cnm_local_cmd_nas_shadow[] =
{
    {CM_OMI_CMD_GET, cm_cnm_nas_shadow_local_get},
    {CM_OMI_CMD_GET_BATCH, cm_cnm_nas_shadow_local_get},
    {CM_OMI_CMD_COUNT, cm_cnm_nas_shadow_local_count},
    {CM_OMI_CMD_ADD, cm_cnm_nas_shadow_local_create},
    {CM_OMI_CMD_MODIFY, cm_cnm_nas_shadow_local_update},
    {CM_OMI_CMD_DELETE, cm_cnm_nas_shadow_local_delete},
};

const cm_cnm_cfg_cmd_t g_cm_cnm_local_cmd_fcinfo[] =
{
    {CM_OMI_CMD_GET_BATCH, cm_cnm_fcinfo_local_getbatch},
    {CM_OMI_CMD_COUNT, cm_cnm_fcinfo_local_count},
};

const cm_cnm_cfg_cmd_t g_cm_cnm_local_cmd_explorer[] =
{
    {CM_OMI_CMD_GET_BATCH, cm_cnm_explorer_local_getbatch},
    {CM_OMI_CMD_COUNT, cm_cnm_explorer_local_count},
    {CM_OMI_CMD_ADD, cm_cnm_explorer_local_create},
};

const cm_cnm_cfg_cmd_t g_cm_cnm_local_cmd_ipf[] =
{
    {CM_OMI_CMD_GET_BATCH, cm_cnm_ipf_local_getbatch},
    {CM_OMI_CMD_COUNT, cm_cnm_ipf_local_count},
    {CM_OMI_CMD_ADD, cm_cnm_ipf_local_insert},
    {CM_OMI_CMD_MODIFY, cm_cnm_ipf_local_update},
    {CM_OMI_CMD_DELETE, cm_cnm_ipf_local_delete},
};
const cm_cnm_cfg_cmd_t g_cm_cnm_local_cmd_dirquota[] =
{
    {CM_OMI_CMD_GET_BATCH, cm_cnm_dirquota_local_getbatch},
    {CM_OMI_CMD_GET, cm_cnm_dirquota_local_get},
    {CM_OMI_CMD_COUNT,cm_cnm_dirquota_local_count},
    {CM_OMI_CMD_ADD, cm_cnm_dirquota_local_add},
    {CM_OMI_CMD_DELETE, cm_cnm_dirquota_local_delete},
};

const cm_cnm_cfg_cmd_t g_cm_cnm_local_cmd_topo_power[] =
{
    {CM_OMI_CMD_GET_BATCH, cm_cnm_topo_power_local_getbatch},
    {CM_OMI_CMD_COUNT,cm_cnm_topo_power_local_count},
};

const cm_cnm_cfg_cmd_t g_cm_cnm_local_cmd_topo_fan[] =
{
    {CM_OMI_CMD_GET_BATCH, cm_cnm_topo_fan_local_getbatch},
    {CM_OMI_CMD_COUNT,cm_cnm_topo_fan_local_count},
};

const cm_cnm_cfg_cmd_t cm_cnm_cabinet_local_cabinet[] =
{
    {CM_OMI_CMD_GET_BATCH, cm_cnm_cabinet_local_getbatch},
    {CM_OMI_CMD_COUNT,cm_cnm_cabinet_local_count},
};


const cm_cnm_cfg_cmd_t g_cm_cnm_local_node[] =
{
    {CM_OMI_CMD_OFF, cm_cnm_node_local_off},
    {CM_OMI_CMD_REBOOT,cm_cnm_node_local_reboot},
};

const cm_cnm_cfg_cmd_t g_cm_cnm_local_lowdata[] =
{
    {CM_OMI_CMD_GET_BATCH, cm_cnm_lowdata_local_getbatch},
    {CM_OMI_CMD_GET, cm_cnm_lowdata_local_get},
    {CM_OMI_CMD_COUNT,cm_cnm_lowdata_local_count},
    {CM_OMI_CMD_MODIFY,cm_cnm_lowdata_local_update},
};

const cm_cnm_cfg_cmd_t g_cm_cnm_local_dirlowdata[] =
{
    {CM_OMI_CMD_GET_BATCH, cm_cnm_dirlowdata_local_getbatch},
    {CM_OMI_CMD_GET, cm_cnm_dirlowdata_local_get},
    {CM_OMI_CMD_COUNT,cm_cnm_dirlowdata_local_count},
    {CM_OMI_CMD_MODIFY,cm_cnm_dirlowdata_local_update},
};

const cm_cnm_cfg_cmd_t g_cm_cnm_local_poolext[] =
{
    {CM_OMI_CMD_ADD, cm_cnm_poolext_local_create},
    {CM_OMI_CMD_MODIFY, cm_cnm_poolext_local_add},
};

const cm_cnm_cfg_cmd_t g_cm_cnm_local_cmd_upgrade[] =
{
    {CM_OMI_CMD_GET_BATCH, cm_cnm_upgrade_local_getbatch},
    {CM_OMI_CMD_COUNT, cm_cnm_upgrade_local_count},
    {CM_OMI_CMD_ADD, cm_cnm_upgrade_local_insert},
};

const cm_cnm_cfg_obj_t g_cm_cnm_cfg_array[] =
{
    {
        CM_OMI_OBJECT_DISK,
        sizeof(cm_cnm_disk_info_t), sizeof(cm_cnm_disk_req_param_t),
        sizeof(g_cm_cnm_local_cmd_disk) / sizeof(cm_cnm_cfg_cmd_t),
        g_cm_cnm_local_cmd_disk
    },
    {
        CM_OMI_OBJECT_POOL,
        sizeof(cm_cnm_pool_list_t), sizeof(cm_cnm_pool_list_param_t),
        sizeof(g_cm_cnm_local_cmd_pool) / sizeof(cm_cnm_cfg_cmd_t),
        g_cm_cnm_local_cmd_pool
    },
    {
        CM_OMI_OBJECT_POOLDISK,
        sizeof(cm_cnm_pooldisk_info_t), sizeof(cm_cnm_pool_list_param_t),
        sizeof(g_cm_cnm_local_cmd_pooldisk) / sizeof(cm_cnm_cfg_cmd_t),
        g_cm_cnm_local_cmd_pooldisk
    },
    {
        CM_OMI_OBJECT_LUN,
        sizeof(cm_cnm_lun_info_t), sizeof(cm_cnm_lun_param_t),
        sizeof(g_cm_cnm_local_cmd_lun) / sizeof(cm_cnm_cfg_cmd_t),
        g_cm_cnm_local_cmd_lun
    },
    {
        CM_OMI_OBJECT_SNAPSHOT,
        sizeof(cm_cnm_snapshot_info_t), sizeof(cm_cnm_decode_info_t) + sizeof(cm_cnm_snapshot_info_t),
        sizeof(g_cm_cnm_local_cmd_snapshot) / sizeof(cm_cnm_cfg_cmd_t),
        g_cm_cnm_local_cmd_snapshot
    },
    {
        CM_OMI_OBJECT_NAS,
        sizeof(cm_cnm_nas_info_t), sizeof(cm_cnm_decode_info_t) + sizeof(cm_cnm_nas_info_t),
        sizeof(g_cm_cnm_local_cmd_nas) / sizeof(cm_cnm_cfg_cmd_t),
        g_cm_cnm_local_cmd_nas
    },
    {
        CM_OMI_OBJECT_CIFS,
        sizeof(cm_cnm_cifs_info_t), sizeof(cm_cnm_decode_info_t) + sizeof(cm_cnm_cifs_param_t),
        sizeof(g_cm_cnm_local_cmd_cifs) / sizeof(cm_cnm_cfg_cmd_t),
        g_cm_cnm_local_cmd_cifs
    },

    {
        CM_OMI_OBJECT_SHARE_NFS,
        sizeof(cm_cnm_nfs_info_t), sizeof(cm_cnm_decode_info_t) + sizeof(cm_cnm_nfs_info_t),
        sizeof(g_cm_cnm_local_cmd_nfs) / sizeof(cm_cnm_cfg_cmd_t),
        g_cm_cnm_local_cmd_nfs
    },

    {
        CM_OMI_OBJECT_SNAPSHOT_BACKUP,
        sizeof(cm_cnm_snapshot_backup_info_t), sizeof(cm_cnm_snapshot_backup_info_t),
        sizeof(g_cm_cnm_local_cmd_snapshot_backup) / sizeof(cm_cnm_cfg_cmd_t),
        g_cm_cnm_local_cmd_snapshot_backup
    },
    {
        CM_OMI_OBJECT_TASK_MANAGER,
        sizeof(cm_cnm_task_manager_info_t), sizeof(cm_cnm_task_manager_info_t) + sizeof(cm_cnm_decode_info_t),
        sizeof(g_cm_cnm_local_cmd_task_manager) / sizeof(cm_cnm_cfg_cmd_t),
        g_cm_cnm_local_cmd_task_manager
    },
    {
        CM_OMI_OBJECT_PING,
        sizeof(cm_cnm_ping_info_t), sizeof(cm_cnm_ping_info_t),
        sizeof(g_cm_cnm_local_cmd_ping) / sizeof(cm_cnm_cfg_cmd_t),
        g_cm_cnm_local_cmd_ping
    },
    {
        CM_OMI_OBJECT_QUOTA,
        sizeof(cm_cnm_quota_info_t), sizeof(cm_cnm_decode_info_t) + sizeof(cm_cnm_quota_info_t),
        sizeof(g_cm_cnm_local_cmd_quota) / sizeof(cm_cnm_cfg_cmd_t),
        g_cm_cnm_local_cmd_quota
    },
    {
        CM_OMI_OBJECT_IPSHFIT,
        sizeof(cm_cnm_ipshift_info_t), sizeof(cm_cnm_decode_info_t) + sizeof(cm_cnm_ipshift_info_t),
        sizeof(g_cm_cnm_local_cmd_ipshift) / sizeof(cm_cnm_cfg_cmd_t),
        g_cm_cnm_local_cmd_ipshift
    },
    {
        CM_OMI_OBJECT_SNAPSHOT_CLONE,
        sizeof(cm_cnm_snapshot_clone_info_t), sizeof(cm_cnm_decode_info_t) + sizeof(cm_cnm_snapshot_clone_info_t),
        sizeof(g_cm_cnm_local_cmd_snapshot_clone) / sizeof(cm_cnm_cfg_cmd_t),
        g_cm_cnm_local_cmd_snapshot_clone
    },

    {
        CM_OMI_OBJECT_PHYS,
        sizeof(cm_cnm_phys_info_t), sizeof(cm_cnm_decode_info_t) + sizeof(cm_cnm_phys_info_t),
        sizeof(g_cm_cnm_local_cmd_phys) / sizeof(cm_cnm_cfg_cmd_t),
        g_cm_cnm_local_cmd_phys
    },
    {
        CM_OMI_OBJECT_ROUTE,
        sizeof(cm_cnm_route_info_t), sizeof(cm_cnm_decode_info_t) + sizeof(cm_cnm_route_info_t),
        sizeof(g_cm_cnm_local_cmd_route) / sizeof(cm_cnm_cfg_cmd_t),
        g_cm_cnm_local_cmd_route
    },
    {
        CM_OMI_OBJECT_PHYS_IP,
        sizeof(cm_cnm_phys_ip_info_t), sizeof(cm_cnm_decode_info_t) + sizeof(cm_cnm_phys_ip_info_t),
        sizeof(g_cm_cnm_local_cmd_phys_ip) / sizeof(cm_cnm_cfg_cmd_t),
        g_cm_cnm_local_cmd_phys_ip
    },
    {
        CM_OMI_OBJECT_CLUSTER_NAS,
        sizeof(cm_cnm_cluster_nas_info_t), sizeof(cm_cnm_decode_info_t) + sizeof(cm_cnm_cluster_nas_info_t),
        sizeof(g_cm_cnm_local_cmd_cluster_nas) / sizeof(cm_cnm_cfg_cmd_t),
        g_cm_cnm_local_cmd_cluster_nas
    },
    {
        CM_OMI_OBJECT_DUALLIVE_NAS,
        sizeof(cm_cnm_duallive_nas_info_t), sizeof(cm_cnm_decode_info_t) + sizeof(cm_cnm_duallive_nas_info_t),
        sizeof(g_cm_cnm_local_cmd_duallive_nas) / sizeof(cm_cnm_cfg_cmd_t),
        g_cm_cnm_local_cmd_duallive_nas

    },
    {
        CM_OMI_OBJECT_BACKUP_NAS,
        sizeof(cm_cnm_backup_nas_info_t), sizeof(cm_cnm_decode_info_t) + sizeof(cm_cnm_backup_nas_info_t),
        sizeof(g_cm_cnm_local_cmd_backup_nas) / sizeof(cm_cnm_cfg_cmd_t),
        g_cm_cnm_local_cmd_backup_nas
    },

    {
        CM_OMI_OBJECT_LUN_MIRROR,
        sizeof(cm_cnm_lunmirror_info_t), sizeof(cm_cnm_decode_info_t) + sizeof(cm_cnm_lunmirror_info_t),
        sizeof(g_cm_cnm_local_cmd_lun_mirror) / sizeof(cm_cnm_cfg_cmd_t),
        g_cm_cnm_local_cmd_lun_mirror
    },

    {
        CM_OMI_OBJECT_LUN_BACKUP,
        sizeof(cm_cnm_lunbackup_info_t), sizeof(cm_cnm_decode_info_t) + sizeof(cm_cnm_lunbackup_info_t),
        sizeof(g_cm_cnm_local_cmd_lun_backup) / sizeof(cm_cnm_cfg_cmd_t),
        g_cm_cnm_local_cmd_lun_backup
    },

    {
        CM_OMI_OBJECT_CLUSTERSAN_TARGET,
        sizeof(cm_cnm_cluster_target_info_t), sizeof(cm_cnm_decode_info_t) + sizeof(cm_cnm_cluster_target_info_t),
        sizeof(g_cm_cnm_local_cmd_cluster_target) / sizeof(cm_cnm_cfg_cmd_t),
        g_cm_cnm_local_cmd_cluster_target
    },
    {
        CM_OMI_OBJECT_MIGRATE,
        sizeof(cm_cnm_migrate_info_t), sizeof(cm_cnm_decode_info_t) + sizeof(cm_cnm_migrate_info_t),
        sizeof(g_cm_cnm_local_cmd_migrate) / sizeof(cm_cnm_cfg_cmd_t),
        g_cm_cnm_local_cmd_migrate
    },
    {
        CM_OMI_OBJECT_POOL_RECONSTRUCT,
        sizeof(cm_cnm_pool_reconstruct_info_t), sizeof(cm_cnm_decode_info_t) + sizeof(cm_cnm_pool_reconstruct_info_t),
        sizeof(g_cm_cnm_local_cmd_pool_reconstruct) / sizeof(cm_cnm_cfg_cmd_t),
        g_cm_cnm_local_cmd_pool_reconstruct
    },
    {
        CM_OMI_OBJECT_NODE_SERVCE,
        sizeof(cm_cnm_node_service_info_t), sizeof(cm_cnm_decode_info_t) + sizeof(cm_cnm_node_service_info_t),
        sizeof(g_cm_cnm_local_cmd_ndoe_service) / sizeof(cm_cnm_cfg_cmd_t),
        g_cm_cnm_local_cmd_ndoe_service
    },
    {
        CM_OMI_OBJECT_AGGR,
        sizeof(cm_cnm_aggr_info_t), sizeof(cm_cnm_decode_info_t) + sizeof(cm_cnm_aggr_info_t),
        sizeof(g_cm_cnm_local_cmd_aggr) / sizeof(cm_cnm_cfg_cmd_t),
        g_cm_cnm_local_cmd_aggr
    },
	{
        CM_OMI_OBJECT_DNSM,
        sizeof(cm_cnm_dnsm_info_t),sizeof(cm_cnm_decode_info_t)+sizeof(cm_cnm_dnsm_info_t),
        sizeof(g_cm_cnm_local_cmd_dnsm)/sizeof(cm_cnm_cfg_cmd_t),
        g_cm_cnm_local_cmd_dnsm
    },
    {
        CM_OMI_OBJECT_CNM_SAS,
        sizeof(cm_pmm_name_t),sizeof(cm_cnm_decode_info_t)+sizeof(cm_pmm_name_t),
        sizeof(g_cm_cnm_local_cmd_sas)/sizeof(cm_cnm_cfg_cmd_t),
        g_cm_cnm_local_cmd_sas
    },
    {
        CM_OMI_OBJECT_CNM_PROTO,
        sizeof(cm_pmm_name_t),sizeof(cm_cnm_decode_info_t)+sizeof(cm_pmm_name_t),
        sizeof(g_cm_cnm_local_cmd_protocol)/sizeof(cm_cnm_cfg_cmd_t),
        g_cm_cnm_local_cmd_protocol
    },
    {
        CM_OMI_OBJECT_CNM_NAS_SHADOW,
        sizeof(cm_cnm_nas_shadow_info_t),sizeof(cm_cnm_decode_info_t)+sizeof(cm_cnm_nas_shadow_info_t),
        sizeof(g_cm_cnm_local_cmd_nas_shadow)/sizeof(cm_cnm_cfg_cmd_t),
        g_cm_cnm_local_cmd_nas_shadow
    },
	{
        CM_OMI_OBJECT_FCINFO,
        sizeof(cm_cnm_fcinfo_info_t),sizeof(cm_cnm_decode_info_t)+sizeof(cm_cnm_fcinfo_info_t),
        sizeof(g_cm_cnm_local_cmd_fcinfo)/sizeof(cm_cnm_cfg_cmd_t),
        g_cm_cnm_local_cmd_fcinfo
    },
	{
        CM_OMI_OBJECT_EXPLORER,
        sizeof(cm_cnm_explorer_info_t),sizeof(cm_cnm_decode_info_t)+sizeof(cm_cnm_explorer_info_t),
        sizeof(g_cm_cnm_local_cmd_explorer)/sizeof(cm_cnm_cfg_cmd_t),
        g_cm_cnm_local_cmd_explorer
    },
	{
        CM_OMI_OBJECT_IPF,
        sizeof(cm_cnm_ipf_info_t),sizeof(cm_cnm_decode_info_t)+sizeof(cm_cnm_ipf_info_t),
        sizeof(g_cm_cnm_local_cmd_ipf)/sizeof(cm_cnm_cfg_cmd_t),
        g_cm_cnm_local_cmd_ipf
    },
	{
        CM_OMI_OBJECT_DIRQUOTA,
        sizeof(cm_cnm_dirquota_info_t),sizeof(cm_cnm_decode_info_t)+sizeof(cm_cnm_dirquota_info_t),
        sizeof(g_cm_cnm_local_cmd_dirquota)/sizeof(cm_cnm_cfg_cmd_t),
        g_cm_cnm_local_cmd_dirquota
    },
    {
        CM_OMI_OBJECT_TOPO_POWER,
        sizeof(cm_cnm_topo_power_info_t),sizeof(cm_cnm_decode_info_t)+sizeof(cm_cnm_topo_power_info_t),
        sizeof(g_cm_cnm_local_cmd_topo_power)/sizeof(cm_cnm_cfg_cmd_t),
        g_cm_cnm_local_cmd_topo_power
    },
    {
        CM_OMI_OBJECT_TOPO_FAN,
        sizeof(cm_cnm_topo_fan_info_t),sizeof(cm_cnm_decode_info_t)+sizeof(cm_cnm_topo_fan_info_t),
        sizeof(g_cm_cnm_local_cmd_topo_fan)/sizeof(cm_cnm_cfg_cmd_t),
        g_cm_cnm_local_cmd_topo_fan
    },
    {
        CM_OMI_OBJECT_TOPO_TABLE,
        sizeof(cm_cnm_cabinet_info_t),sizeof(cm_cnm_decode_info_t)+sizeof(cm_cnm_cabinet_info_t),
        sizeof(cm_cnm_cabinet_local_cabinet)/sizeof(cm_cnm_cfg_cmd_t),
        cm_cnm_cabinet_local_cabinet
    },
    {
        CM_OMI_OBJECT_NODE,
        0,sizeof(cm_cnm_decode_info_t),
        sizeof(g_cm_cnm_local_node)/sizeof(cm_cnm_cfg_cmd_t),
        g_cm_cnm_local_node
    },
    {
        CM_OMI_OBJECT_LOWDATA,
        sizeof(cm_cnm_lowdata_info_t),sizeof(cm_cnm_decode_info_t)+sizeof(cm_cnm_lowdata_info_t),
        sizeof(g_cm_cnm_local_lowdata)/sizeof(cm_cnm_cfg_cmd_t),
        g_cm_cnm_local_lowdata
    },
    {
        CM_OMI_OBJECT_DIRLOWDATA,
        sizeof(cm_cnm_dirlowdata_info_t),sizeof(cm_cnm_decode_info_t)+sizeof(cm_cnm_dirlowdata_info_t),
        sizeof(g_cm_cnm_local_dirlowdata)/sizeof(cm_cnm_cfg_cmd_t),
        g_cm_cnm_local_dirlowdata
    },

    {
        CM_OMI_OBJECT_POOLEXT,
        sizeof(cm_cnm_poolext_info_t),sizeof(cm_cnm_decode_info_t)+sizeof(cm_cnm_poolext_info_t),
        sizeof(g_cm_cnm_local_poolext)/sizeof(cm_cnm_cfg_cmd_t),
        g_cm_cnm_local_poolext
    },
    {
        CM_OMI_OBJECT_UPGRADE,
        sizeof(cm_cnm_upgrade_info_t),sizeof(cm_cnm_decode_info_t)+sizeof(cm_cnm_upgrade_info_t),
        sizeof(g_cm_cnm_local_cmd_upgrade)/sizeof(cm_cnm_cfg_cmd_t),
        g_cm_cnm_local_cmd_upgrade
    },
    /* 下面是最后一个，一直保留，新配置放前面 */
    {CM_OMI_OBJECT_DEMO, 0, 0, 0, NULL}
};

const cm_cnm_cfg_obj_t* g_cm_cnm_config = g_cm_cnm_cfg_array;

/* ================= start CFG for cm_sche ===================================*/
const cm_sche_config_t g_cm_sche_cfgs[] =
{
    {CM_OMI_OBJECT_SNAPSHOT_SCHE, cm_cnm_snapshot_sche_cbk},
    {CM_OMI_OBJECT_LOWDATA_SCHE,cm_cnm_lowdata_sche_cbk},
    /* 下面是最后一个，一直保留，新配置放前面 */
    {CM_OMI_OBJECT_DEMO, NULL}
};
const cm_sche_config_t* g_cm_sche_config = g_cm_sche_cfgs;

const cm_task_type_cfg_t g_cm_task_cfg[CM_TASK_TYPE_BUTT] =
{
    {
        CM_TASK_TYPE_SNAPSHOT_BACKUP, 
        CM_FALSE, 
        CM_TASK_REPORT_BY_SYS,
        cm_cnm_snapshot_backup_cbk_task_pre,
        cm_cnm_snapshot_backup_cbk_task_report,
        cm_cnm_snapshot_backup_cbk_task_aft
    },
    {
        CM_TASK_TYPE_SNAPSHOT_SAN_BACKUP, 
        CM_TRUE,
        CM_TASK_REPORT_BY_CALL,
        NULL,
        cm_cnm_lunbackup_cbk_task_report,
        NULL
    },
    {
        CM_TASK_TYPE_ZPOOL_EXPROT,
        CM_FALSE,
        CM_TASK_REPORT_BY_SYS,
        NULL,
        cm_cnm_zpool_eximport_task_report,
        NULL,
    },
    {
        CM_TASK_TYPE_ZPOOL_IMPROT,
        CM_FALSE,
        CM_TASK_REPORT_BY_SYS,
        NULL,
        cm_cnm_zpool_eximport_task_report,
        NULL,
    },
    {
        CM_TASK_TYPE_LUN_MIGRATE,
        CM_FALSE,
        CM_TASK_REPORT_BY_CALL,
        NULL,
        cm_cnm_lun_migrate_task_report,
        NULL,
    },
};

