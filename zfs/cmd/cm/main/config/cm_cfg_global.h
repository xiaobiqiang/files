/******************************************************************************
 *          Copyright (c) 2017 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cfg_global.h
 * author     : wbn
 * create date: 2017Äê10ÔÂ26ÈÕ
 * description: TODO:
 *
 *****************************************************************************/
#ifndef MAIN_CONFIG_CM_CFG_GLOBAL_H_
#define MAIN_CONFIG_CM_CFG_GLOBAL_H_
#include "cm_define.h"
#include "cm_cfg_errcode.h"
/* ================= start CFG for cm_log ===================================*/
#define CM_LOG_MODE CM_LOG_MODE_NORMAL
#define CM_LOG_LVL CM_LOG_LVL_WARNING
#define CM_LOG_FILE "ceres_cm.log"

/* ================= end CFG for cm_log =====================================*/

/* ================= start CFG for cm_rpc ===================================*/
#define CM_RPC_SUPPORT_SERVER CM_ON
/* ================= end   CFG for cm_rpc ===================================*/

typedef enum
{
    CM_SYNC_OBJ_DEMO = 0,
    CM_SYNC_OBJ_PMM,
    CM_SYNC_OBJ_ALARM_RECORD,
    CM_SYNC_OBJ_ALARM_CONFIG,
    CM_SYNC_OBJ_SYSTIME,
    CM_SYNC_OBJ_PMM_CONFIG,
    CM_SYNC_OBJ_ADMIN,
    CM_SYNC_OBJ_SCHE,
    CM_SYNC_OBJ_USER,
    CM_SYNC_OBJ_GROUP,
    CM_SYNC_OBJ_TASK,
    CM_SYNC_OBJ_PERMISSION,
    CM_SYNC_OBJ_TOPO_TABLE,
    CM_SYNC_OBJ_THRESHOLD,
    CM_SYNC_OBJ_NTP,

    CM_SYNC_OBJ_CLU_STAT,
    CM_SYNC_OBJ_DOMAIN_USER,
    CM_SYNC_OBJ_DNS,
    CM_SYNC_OBJ_LOWDATA_VOLUME,
    CM_SYNC_OBJ_MAILSEND,
    CM_SYNC_OBJ_MAILRECV,

    CM_SYNC_OBJ_REMOTE_CLUSTER,
    
    CM_SYNC_OBJ_BUTT
} cm_sync_obj_e;

/*
 * task type structure.
 *
 */
typedef enum
{
    CM_TASK_TYPE_SNAPSHOT_BACKUP = 0,   /* snapshot backup task */
    CM_TASK_TYPE_SNAPSHOT_SAN_BACKUP,
    CM_TASK_TYPE_ZPOOL_EXPROT,
    CM_TASK_TYPE_ZPOOL_IMPROT,
    CM_TASK_TYPE_LUN_MIGRATE,
    CM_TASK_TYPE_BUTT                   /* a flag marked end of struction */
} cm_task_type_e;

#define CM_CNM_DISK_FILE CM_DATA_DIR"cm_db_disk.db"
#define CM_CNM_DISK_FILE_FMD CM_DATA_DIR"db_disk.db"

#endif /* MAIN_CONFIG_CM_CFG_GLOBAL_H_ */
