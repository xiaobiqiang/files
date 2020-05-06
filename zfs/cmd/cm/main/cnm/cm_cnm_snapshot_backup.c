#include "cm_cnm_snapshot_backup.h"
#include "cm_task.h"
#include "cm_cnm_common.h"
#include "cm_log.h"
#include "cm_cfg_global.h"
#include "cm_sys.h"
#include "cm_node.h"
#include "cm_common.h"
#include "cm_platform.h"
#include "cm_cmt.h"

typedef struct
{
    uint32 nid;
    uint32 acknid;
    sint8 *ipaddr;
} cm_cnm_snapshot_backup_ip_nid_map_t;

/*
 *
 */
sint32 cm_cnm_snapshot_backup_init(void)
{
    return CM_OK;
}

/*
 *
 */
sint32 cm_cnm_ping_init(void)
{
    return CM_OK;
}

static sint32 cm_cnm_snapshot_backup_decode_ext
(const cm_omi_obj_t ObjParam, void* info, cm_omi_field_flag_t *set)
{
    sint32 iRet;
    cm_cnm_snapshot_backup_info_t *pinfo = info;
    cm_cnm_decode_param_t params_num[] =
    {
        {CM_OMI_FIELD_SNAPSHOT_BACKUP_SRC_NID, sizeof(pinfo->src_nid), &pinfo->src_nid, NULL},
    };
    cm_cnm_decode_param_t params_str[] =
    {
        {CM_OMI_FIELD_SNAPSHOT_BACKUP_SRC_PRE_PATH, CM_CNM_SNAPSHOT_BACKUP_PATH_LEN, pinfo->src_path, NULL},
        {CM_OMI_FIELD_SNAPSHOT_BACKUP_DST_IPADDR, CM_IP_LEN, pinfo->dst_ipaddr, NULL},
        {CM_OMI_FIELD_SNAPSHOT_BACKUP_DST_DIR, CM_CNM_SNAPSHOT_BACKUP_PATH_LEN, pinfo->dst_dir, NULL},
        {CM_OMI_FIELD_SNAPSHOT_BACKUP_SRC_AFT_PATH, CM_CNM_SNAPSHOT_BACKUP_PATH_LEN, pinfo->sec_path, NULL},
    };

    iRet = cm_cnm_decode_str(ObjParam, params_str,
                             sizeof(params_str) / sizeof(cm_cnm_decode_param_t), set);

    if(CM_OK != iRet)
    {
        return CM_FAIL;
    }

    return cm_cnm_decode_num(ObjParam, params_num,
                             sizeof(params_num) / sizeof(cm_cnm_decode_param_t), set);
}

sint32 cm_cnm_snapshot_backup_decode
(const cm_omi_obj_t ObjParam, void **ppDecodeParam)
{
    return cm_cnm_decode_comm(ObjParam, sizeof(cm_cnm_snapshot_backup_info_t),
                              cm_cnm_snapshot_backup_decode_ext, ppDecodeParam);
}

sint32 cm_cnm_ping_decode
(const cm_omi_obj_t ObjParam, void **ppDecodeParam)
{
    sint8 *pDecodeParam = CM_MALLOC(CM_IP_LEN);

    if(NULL == pDecodeParam)
    {
        CM_LOG_ERR(CM_MOD_CNM, "malloc fail");
        return CM_FAIL;
    }

    *ppDecodeParam = pDecodeParam;
    return cm_omi_obj_key_get_str_ex(ObjParam, CM_OMI_FIELD_PING_IPADDR,
                                     pDecodeParam, CM_IP_LEN);
}

static sint32 cm_cnm_snapshot_backup_nid_check_is_local(uint32 id, sint8 *ip)
{
    return (sint32)cm_cnm_exec_count(id, "ifconfig -a | grep -w 'inet' | awk '{print $2}' | "
                                     "grep -w '%s' | wc -l", ip);
}


/*
 *
 */
sint32 cm_cnm_snapshot_backup_add
(const void *pDecodeParam, void **ppAckData, uint32 *pAckLen)
{
    sint32 iRet;
    cm_cnm_decode_info_t *pDecode = pDecodeParam;
    cm_cnm_snapshot_backup_info_t *pinfo = pDecode->data;
    sint8 cmd[CM_TASK_CMD_LEN] = {0};
    sint8 desc[CM_TASK_DESC_LEN] = {0};

    iRet = cm_cnm_snapshot_backup_nid_check_is_local(pinfo->src_nid,
            pinfo->dst_ipaddr);

    if(0 == strlen(pinfo->sec_path))    //no incremental.
    {
        if(0 != iRet)   //同节点备份
        {
            CM_SNPRINTF_ADD(cmd, CM_TASK_CMD_LEN, "zfs send %s | zfs recv %s",
                            pinfo->src_path, pinfo->dst_dir);
            CM_SNPRINTF_ADD(desc, CM_TASK_DESC_LEN, "0|%s|%s|%s|%u|", pinfo->dst_ipaddr,
                            pinfo->src_path, pinfo->dst_dir, pinfo->src_nid);
        }
        else    //不是同一个节点
        {
            CM_SNPRINTF_ADD(cmd, CM_TASK_CMD_LEN, "zfs send %s | ssh %s zfs recv %s",
                            pinfo->src_path, pinfo->dst_ipaddr, pinfo->dst_dir);
            CM_SNPRINTF_ADD(desc, CM_TASK_DESC_LEN, "1|%s|%s|%s|%u|", pinfo->dst_ipaddr,
                            pinfo->src_path, pinfo->dst_dir, pinfo->src_nid);
        }
    }
    else
    {
        if(0 != iRet)   //同节点备份
        {
            CM_SNPRINTF_ADD(cmd, CM_TASK_CMD_LEN, "zfs send -i %s %s | zfs recv -F %s",
                            pinfo->src_path, pinfo->sec_path, pinfo->dst_dir);
            CM_SNPRINTF_ADD(desc, CM_TASK_DESC_LEN, "0|%s|%s|%s|%u|%s", pinfo->dst_ipaddr,
                            pinfo->src_path, pinfo->dst_dir, pinfo->src_nid, pinfo->sec_path);
        }
        else    //不是同一个节点
        {
            CM_SNPRINTF_ADD(cmd, CM_TASK_CMD_LEN, "zfs send -i %s %s",
                            pinfo->src_path, pinfo->sec_path);
            CM_SNPRINTF_ADD(cmd, CM_TASK_CMD_LEN, " | ssh %s zfs recv -F %s",
                            pinfo->dst_ipaddr, pinfo->dst_dir);
            CM_SNPRINTF_ADD(desc, CM_TASK_DESC_LEN, "1|%s|%s|%s|%u|%s", pinfo->dst_ipaddr,
                            pinfo->src_path, pinfo->dst_dir, pinfo->src_nid, pinfo->sec_path);
        }
    }

    iRet = cm_task_add(pinfo->src_nid, CM_TASK_TYPE_SNAPSHOT_BACKUP, cmd, desc);

    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM, "add task fail[%d]", iRet);
        return CM_FAIL;
    }

    return CM_OK;
}

/*
 *
 */
sint32 cm_cnm_ping_add
(const void *pDecodeParam, void **ppAckData, uint32 *pAckLen)
{
    sint8 *ipaddr = (sint8 *)pDecodeParam;
    return cm_cnm_exec_ping(ipaddr);
}


/*
 *
 */
void cm_cnm_snapshot_backup_oplog_create
(const sint8 *sessionid, void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK)? CM_ALARM_LOG_SNAPSHOT_BACKUP_CREATE_OK : 
        CM_ALARM_LOG_SNAPSHOT_BACKUP_CREATE_FAIL;
    const cm_cnm_decode_info_t *req = pDecodeParam;
    const uint32 cnt = 5;

    if(NULL == pDecodeParam)
    {   
        cm_cnm_oplog_report(sessionid,alarmid,NULL,cnt,NULL);
    }    
    else    
    {   
        cm_cnm_snapshot_backup_info_t *info = (cm_cnm_snapshot_backup_info_t *)req->data;
        const sint8* nodename = cm_node_get_name(req->nid);
        cm_cnm_oplog_param_t params[5] = {
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_SNAPSHOT_BACKUP_SRC_NID,strlen(nodename),nodename},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_SNAPSHOT_BACKUP_DST_IPADDR,strlen(info->dst_ipaddr),info->dst_ipaddr},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_SNAPSHOT_BACKUP_SRC_PRE_PATH,strlen(info->src_path),info->src_path},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_SNAPSHOT_BACKUP_SRC_AFT_PATH,strlen(info->sec_path),info->sec_path},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_SNAPSHOT_BACKUP_DST_DIR,strlen(info->dst_dir),info->dst_dir},
        };
        
        cm_cnm_oplog_report(sessionid,alarmid,params,cnt,&req->set);
    }   
    return;
}

/*
 *
 */
void cm_cnm_ping_oplog_create
(const sint8 *sessionid, void *pDecodeParam, sint32 Result)
{
    return;
}


/*
 *
 */
sint32 cm_cnm_snapshot_backup_local_add
(void *param, uint32 len,
 uint64 offset, uint32 total,
 void **ppAck, uint32 *pAckLen
)
{
    return CM_OK;
}

/*
 *
 */
sint32 cm_cnm_ping_local_add
(void *param, uint32 len,
 uint64 offset, uint32 total,
 void **ppAck, uint32 *pAckLen
)
{
    return CM_OK;
}

sint32 cm_cnm_snapshot_backup_cbk_task_aft
(sint8 *pParam, uint32 len, uint32 tmout)
{
    sint32 iRet;
    uint32 myId = cm_node_get_id();
    uint32 subdomainId = cm_node_get_subdomain_id();
    cm_node_info_t info;

    iRet = cm_node_get_info(subdomainId, myId, &info);

    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM, "get node info fail");
        return CM_FAIL;
    }

    iRet = cm_exec_tmout(NULL, 0, tmout,
                         "/bin/bash "CM_SCRIPT_DIR"cm_cnm_snapshot_backup.sh term %s "
                         "'%s' 'local'", info.ip_addr, pParam);

    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM, "clean fail[%d]", iRet);
        return CM_FAIL;
    }

    return CM_OK;
}

sint32 cm_cnm_snapshot_backup_cbk_task_pre
(sint8 *pParam, uint32 len, uint32 tmout)
{
    sint32 iRet;
    uint32 myId = cm_node_get_id();
    uint32 subdomainId = cm_node_get_subdomain_id();
    cm_node_info_t info;

    iRet = cm_node_get_info(subdomainId, myId, &info);

    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM, "get node info fail");
        return CM_FAIL;
    }

    iRet = cm_exec_tmout(NULL, 0, tmout,
                         "/bin/bash "CM_SCRIPT_DIR"cm_cnm_snapshot_backup.sh init %s "
                         "'%s' 'local'", info.ip_addr, pParam);

    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM, "init fail[%d]", iRet);
        (void)cm_cnm_snapshot_backup_cbk_task_aft(pParam, len, tmout);
        return CM_FAIL;
    }

    return CM_OK;
}

sint32 cm_cnm_snapshot_backup_cbk_task_report
(cm_task_send_state_t *pSnd, cm_task_cmt_echo_t **pproc)
{
    cm_task_cmt_echo_t *pAck = NULL;
    sint32 prog = cm_exec_int("sqlite3 %s 'select prog from record_t "
                              "WHERE id=%u'", CM_TASK_DB_FILE, pSnd->task_id);
    prog = (prog >= 90 ? 90 : prog + 10);

    pAck = CM_MALLOC(sizeof(cm_task_cmt_echo_t));

    if(NULL == pAck)
    {
        CM_LOG_ERR(CM_MOD_CNM, "malloc fail");
        return CM_FAIL;
    }
    
    pAck->task_pid = pSnd->task_pid;
    pAck->task_prog = prog;
    /*start  for 00006382 by wbn */
    if(0 == cm_exec_int("ps -ef|awk '{print $2}'|grep -w %u|wc -l",pSnd->task_pid))
    {
        CM_LOG_ERR(CM_MOD_CNM, "pid[%u] not found",pSnd->task_pid);
        pAck->task_state = CM_TASK_STATE_WRONG;
        pAck->task_prog = 100;
        pAck->task_end = cm_get_time();
    }
    else
    /*end  for 00006382 by wbn */
    {
        pAck->task_state = CM_TASK_STATE_RUNNING;
        pAck->task_end = 0;
    }    
    
    *pproc = pAck;
    return CM_OK;
}

