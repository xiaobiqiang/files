/******************************************************************************
 *          Copyright (c) 2017 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cfg_public.h
 * author     : wbn
 * create date: 2017Äê10ÔÂ26ÈÕ
 * description: TODO:
 *
 *****************************************************************************/
#ifndef BASE_CONFIG_CM_CFG_PUBLIC_H_
#define BASE_CONFIG_CM_CFG_PUBLIC_H_

#define CM_RPC_TIMEOUT 1 

#define CM_OS_TYPE_SUNOS 0

#define CM_OS_TYPE CM_OS_TYPE_SUNOS

#define CM_ID_LEN 64
#define CM_NAME_LEN 64
#define CM_VERSION_LEN 64
#define CM_SN_LEN 64
#define CM_PASSWORD_LEN 64

#define CM_NODE_NAME_LEN 64

#define CM_IP_LEN 32

#define CM_MAX_NODE_NUM 256

#define CM_MAX_SUB_DOMAIN_NUM 8

#define CM_MAX_SUB_DOMAIN_NODE_NUM 32

#define CM_MAX_MASTER_DOMAIN_NODE_NUM 16

#define CM_CMT_MAGIC_NUM 0x87654321

#define CM_NODE_CFG_FILE "/etc/clumgt.config"

#define CM_DATA_DIR "/var/cm/data/"
#define CM_LOG_DIR  "/var/cm/log/"

#define CM_CONFIG_DIR "/var/cm/static/"
#define CM_SCRIPT_DIR "/var/cm/script/"

#define CM_LOCAL_INI CM_CONFIG_DIR"comm/cm_local.ini"
#define CM_CLUSTER_INI CM_DATA_DIR"cm_cluster.ini"

/*#define CM_SESSION_TEST_FILE "/var/cm/session_check_off.flag"*/
#define CM_OMI_PERMISSION_CFG_FILE CM_CONFIG_DIR"comm/cm_om_permission.xml"

#define CM_SHELL_EXEC CM_SCRIPT_DIR"cm_shell_exec.sh"

/* ================= start CFG for cm_log ===================================*/
#define CM_LOG_LINE_MAX 1024

#define CM_LOG_QUEUE_LEN 20
/* 64 * 1024 * 1024 BYTE, 64MB */
#define CM_LOG_FILE_MAX_SIZE 67108864
/* ================= end   CFG for cm_log ===================================*/
/* ================= start CFG for cm_rpc ===================================*/
#define CM_RPC_MAGIC_NUM 0x12345678

#define CM_RPC_CLIENT_LIST_MAX 64

#define CM_RPC_MAX_MSG_LEN 8192

#define CM_RPC_SERVER_PORT 5588

#define CM_RPC_SERVER_MAX_CONN 64

#define CM_RPC_SERVER_QUEUE_LEN 128

//#define CM_RPC_USE_NANOMSG 1

/* ================= end   CFG for cm_rpc ===================================*/
/* ns */
#define CM_EXEC_LOG_TIME    2000000000

#define CM_UINT64_MAX      (18446744073709551615ULL)

/* ================= start CFG for cm_cmt ===================================*/
#define CM_CMT_THREAD_NUM 24
/* ================= end CFG for cm_cmt =====================================*/

/* ================= start CFG for cm_omi ===================================*/
#define CM_OMI_THREAD_NUM 24

#define CM_OMI_KEY_CMD "c"
#define CM_OMI_KEY_OBJ "o"
#define CM_OMI_KEY_PARAM "p"
#define CM_OMI_KEY_RESULT "r"
#define CM_OMI_KEY_ITEMS "i"
#define CM_OMI_KEY_SESSION "s"
#define CM_OMI_KEY_IP "ip"

#define CM_OMI_LOCAL 1

/* ================= end   CFG for cm_omi ===================================*/

/* ================= start CFG for cm_cmd ===================================*/
#define CM_CMD_PARAM_MAX_NUM 10
/* ================= end   CFG for cm_cmd ===================================*/

#define CM_CNM_PORT_MAX_NUM 10
#define CM_HOURS_OF_DAY 24

#define CM_CMT_REQ_TMOUT 20

 typedef enum
{
    CM_CMT_MSG_HTBT = 0,
    CM_CMT_MSG_SYNC_GET_STEP,
    CM_CMT_MSG_SYNC_GET_INFO,
    CM_CMT_MSG_SYNC_REQUEST,
    CM_CMT_MSG_PMM,
    CM_CMT_MSG_CMD,
    CM_CMT_MSG_ALARM,
    CM_CMT_MSG_PHYS_GET,
    CM_CMT_MSG_NODE,
    CM_CMT_MSG_CNM,
    CM_CMT_MSG_EXEC,
    CM_CMT_MSG_TASK_EXEC,   
    CM_CMT_MSG_TASK_GET_STATE,  
    CM_CMT_MSG_TASK_ASK_MASTER,
    CM_CMT_MSG_NODE_ADD,
    CM_CMT_MSG_NODE_DELETE,
    CM_CMT_MSG_SYNC_TOMASTER,
    
    CM_CMT_MSG_BUTT
} cm_msg_type_e;

#define CM_NAME_LEN_POOL 64
#define CM_NAME_LEN_DISK 64
#define CM_NAME_LEN_LUN 64
#define CM_NAME_LEN_NAS 64
#define CM_NAS_PATH_LEN (CM_NAME_LEN_POOL + CM_NAME_LEN_NAS + 1)

#define CM_NAME_LEN_SNAPSHOT 64
#define CM_NAME_LEN_SNAPSHOT_BACKUP 64 
#define CM_LEN_CMD_DESC 256
#define CM_NAME_LEN_DIR 256
#define CM_NAME_LEN_HOSTGROUP 64
#define CM_NAME_LEN_TARGET 64

typedef enum
{
    CM_SYS_VER_DEFAULT = 0,
    CM_SYS_VER_SOLARIS_V7R16,
}cm_system_version_e;


extern cm_system_version_e g_cm_sys_ver;


#endif /* BASE_CONFIG_CM_CFG_PUBLIC_H_ */
