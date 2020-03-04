/******************************************************************************
 *          Copyright (c) 2018 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cnm_migrate.c
 * author     : xar
 * create date: 2018.09.13
 * description: TODO:
 *
 *****************************************************************************/
#include "cm_cnm_migrate.h"
#include "cm_log.h"
#include "cm_node.h"
#include "cm_omi.h"
#include "cm_db.h"
#include "cm_cnm_common.h"


const sint8* cm_cnm_migrate_sh="/var/cm/script/cm_cnm_migrate.sh"; 
#define cm_cnm_migrate_req(cmd,param,ppAck,plen) \
    cm_cnm_request_comm(CM_OMI_OBJECT_MIGRATE,cmd,sizeof(cm_cnm_migrate_info_t),param,ppAck,plen)


sint32 cm_cnm_migrate_init(void)
{
    return CM_OK;
}

sint32 cm_cnm_migrate_decode_ext(const cm_omi_obj_t ObjParam,void* data,cm_omi_field_flag_t *set)
{
    sint32 iRet = CM_OK;
    cm_cnm_migrate_info_t *info = data;
    cm_cnm_decode_param_t param_str[] = 
    {
        {CM_OMI_FIELD_MIGRATE_PATH,sizeof(info->path),info->path,NULL},
        {CM_OMI_FIELD_MIGRATE_POOL,sizeof(info->pool),info->pool,NULL},
        {CM_OMI_FIELD_MIGRATE_LUNID,sizeof(info->lunid),info->lunid,NULL},
    };
    cm_cnm_decode_param_t param_num[] =
    {
        {CM_OMI_FIELD_MIGRATE_TYPE,sizeof(info->type),&info->type,NULL},
        {CM_OMI_FIELD_MIGRATE_NID,sizeof(info->nid),&info->nid,NULL},  
        {CM_OMI_FIELD_MIGRATE_OPERATION,sizeof(info->operation),&info->operation,NULL},    
    };
    
    iRet = cm_cnm_decode_str(ObjParam,param_str,
        sizeof(param_str)/sizeof(cm_cnm_decode_param_t),set);
    iRet = cm_cnm_decode_num(ObjParam,param_num,
        sizeof(param_num)/sizeof(cm_cnm_decode_param_t),set);

    return CM_OK;
}

sint32 cm_cnm_migrate_decode(const cm_omi_obj_t ObjParam,void **ppDecodeParam)
{
    return cm_cnm_decode_comm(ObjParam,sizeof(cm_cnm_migrate_info_t),
        cm_cnm_migrate_decode_ext,ppDecodeParam);
}

static void cm_cnm_migrate_encode_each(cm_omi_obj_t item,void *eachdata,void *arg)
{
    cm_omi_field_flag_t *field = arg;
    cm_cnm_migrate_info_t *info = eachdata;
    cm_cnm_map_value_str_t cols_str[] = 
    {
        {CM_OMI_FIELD_MIGRATE_PROGRESS,   info->progress},
    };
    
    cm_cnm_encode_str(item,field,cols_str,sizeof(cols_str)/sizeof(cm_cnm_map_value_str_t));
    return;
}


cm_omi_obj_t cm_cnm_migrate_encode(const void *pDecodeParam, void *pAckData, uint32 AckLen)
{
     return cm_cnm_encode_comm_ext(pDecodeParam,pAckData,AckLen,
        sizeof(cm_cnm_migrate_info_t),cm_cnm_migrate_encode_each);
}

sint32 cm_cnm_migrate_get(const void *pDecodeParam,void **ppAckData,uint32 *pAckLen)
{
    return cm_cnm_migrate_req(CM_OMI_CMD_GET, pDecodeParam, ppAckData, pAckLen);
}

sint32 cm_cnm_migrate_create(const void *pDecodeParam,void **ppAckData,uint32 *pAckLen)
{
    const cm_cnm_decode_info_t *decode = pDecodeParam;
    const cm_cnm_migrate_info_t *info =NULL;
    sint8 cmd[CM_STRING_256] = {0};
    sint8 desc[CM_STRING_256] = {0};
    sint8 ip[CM_STRING_256] = {0};
    uint32 cut = 0;
    sint32 iRet = CM_OK;
    if(NULL == decode)
    {
        return CM_PARAM_ERR;
    }
    info = (const cm_cnm_migrate_info_t *)decode->data;
    if(CM_MIGRATE_DISK == info->type&&(!CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_MIGRATE_LUNID)))
    {
        CM_LOG_ERR(CM_MOD_CNM,"lunid not exist");
        return CM_PARAM_ERR;
    }

    if(CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_MIGRATE_LUNID))
    {
        cut = strlen(info->lunid);
        if(CM_STRING_32 != cut)
        {
            CM_LOG_ERR(CM_MOD_CNM,"lunid length error");
            return CM_PARAM_ERR; 
        }
    }
    if(CM_MIGRATE_DISK == info->type)
    {
        CM_VSPRINTF(cmd,sizeof(cmd),"%s insert %u %s %s %s %u",cm_cnm_migrate_sh,info->type,info->path,info->pool,info->lunid,info->nid);
        CM_VSPRINTF(desc,sizeof(desc),"%u %s %s %u",info->nid,info->path,info->pool,info->type);
    }else
    {
        CM_VSPRINTF(cmd,sizeof(cmd),"%s insert %u %s %s null %u",cm_cnm_migrate_sh,info->type,info->path,info->pool,info->nid);
        CM_VSPRINTF(desc,sizeof(desc),"%u %s %s %u",info->nid,info->path,info->pool,info->type);
    }
    cm_system(cmd);
    iRet = cm_task_add(info->nid, CM_TASK_TYPE_LUN_MIGRATE, cmd, desc);

    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM, "add task fail[%d]", iRet);
        return CM_FAIL;
    }

    return CM_OK;
}

sint32 cm_cnm_migrate_update(const void *pDecodeParam,void **ppAckData,uint32 *pAckLen)
{
    return cm_cnm_migrate_req(CM_OMI_CMD_MODIFY, pDecodeParam, ppAckData, pAckLen);
}

sint32 cm_cnm_migrate_scan(const void *pDecodeParam,void **ppAckData,uint32 *pAckLen)
{
    cm_system("%s scan",cm_cnm_migrate_sh);
    return CM_OK;
}


sint32 cm_cnm_migrate_local_get(
    void * param,uint32 len,
    uint64 offset,uint32 total,
    void * * ppAck,uint32 * pAckLen)
{
    const cm_cnm_decode_info_t *decode = param;
    const cm_cnm_migrate_info_t *info = NULL;
    cm_cnm_migrate_info_t *data = NULL;
    if(NULL == decode)
    {
        return CM_PARAM_ERR;
    }
    info = (const cm_cnm_migrate_info_t *)decode->data;
    data = CM_MALLOC(sizeof(cm_cnm_migrate_info_t));
    if(NULL == data)
    {
        CM_LOG_ERR(CM_MOD_CNM,"malloc fail");
        return CM_FAIL;
    }
    
    (void)cm_exec_tmout(data->progress,sizeof(info->progress),CM_CMT_REQ_TMOUT,
         "%s get %u %s",cm_cnm_migrate_sh,info->type,info->path);
    
    *ppAck= data;
    *pAckLen = sizeof(cm_cnm_migrate_info_t);
    return CM_OK;
}

sint32 cm_cnm_migrate_local_insert(
    void * param,uint32 len,
    uint64 offset,uint32 total,
    void * * ppAck,uint32 * pAckLen)
{
    /*const cm_cnm_decode_info_t *decode = param;
    const cm_cnm_migrate_info_t *info =NULL;
    uint32 cut = 0;
    if(NULL == decode)
    {
        return CM_PARAM_ERR;
    }
    info = (const cm_cnm_migrate_info_t *)decode->data;
    if(CM_MIGRATE_DISK == info->type&&(!CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_MIGRATE_LUNID)))
    {
        CM_LOG_ERR(CM_MOD_CNM,"lunid not exist");
        return CM_PARAM_ERR;
    }

    if(CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_MIGRATE_LUNID))
    {
        cut = strlen(info->lunid);
        if(CM_STRING_32 != cut)
        {
            CM_LOG_ERR(CM_MOD_CNM,"lunid length error");
            return CM_PARAM_ERR; 
        }
    }

    if(CM_MIGRATE_DISK == info->type)
    {
        return cm_system("%s insert %u %s %s %s",cm_cnm_migrate_sh,info->type,info->path,info->pool,info->lunid);
    }else
    {
        return cm_system("%s insert %u %s %s null",cm_cnm_migrate_sh,info->type,info->path,info->pool);
    }*/
    CM_LOG_ERR(CM_MOD_CNM,"xxx");
    return CM_OK;
}

sint32 cm_cnm_migrate_local_update(
    void * param,uint32 len,
    uint64 offset,uint32 total,
    void * * ppAck,uint32 * pAckLen)
{
    const cm_cnm_decode_info_t *decode = param;
    const cm_cnm_migrate_info_t *info =NULL;
    if(NULL == decode)
    {
        return CM_PARAM_ERR;
    }
    info = (const cm_cnm_migrate_info_t *)decode->data;

    return cm_system("%s update %u %s %u",cm_cnm_migrate_sh,info->type,info->path,info->operation);
}

static void cm_cnm_migrate_oplog_report(
    const sint8* sessionid, const void *pDecodeParam, uint32 alarmid)
{
    const cm_cnm_decode_info_t *req = pDecodeParam;
    const uint32 cnt = 7;

    if(NULL == pDecodeParam)
    {   
        cm_cnm_oplog_report(sessionid,alarmid,NULL,cnt,NULL);
    }    
    else    
    {       
        const cm_cnm_migrate_info_t *info = (const cm_cnm_migrate_info_t*)req->data;
        const sint8* nodename = cm_node_get_name(info->nid); 
        cm_cnm_oplog_param_t params[7] = {
            {CM_OMI_DATA_INT,CM_OMI_FIELD_MIGRATE_TYPE,sizeof(info->type),&info->type},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_MIGRATE_NID,strlen(nodename),nodename},
            {CM_OMI_DATA_INT,CM_OMI_FIELD_MIGRATE_OPERATION,sizeof(info->operation),&info->operation},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_MIGRATE_PATH,strlen(info->path),info->path},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_MIGRATE_POOL,strlen(info->pool),info->pool},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_MIGRATE_LUNID,strlen(info->lunid),info->lunid},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_MIGRATE_PROGRESS,strlen(info->progress),info->progress},
        };
        
        cm_cnm_oplog_report(sessionid,alarmid,params,cnt,&req->set);
    }    
    
    return;
}    

void cm_cnm_migrate_oplog_update(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK)? CM_ALARM_LOG_MIGRATE_UPDATE_OK : CM_ALARM_LOG_MIGRATE_UPDATE_FAIL;
    
    cm_cnm_migrate_oplog_report(sessionid,pDecodeParam,alarmid);
    return;
}
void cm_cnm_migrate_oplog_create(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK)? CM_ALARM_LOG_MIGRATE_CREATE_OK : CM_ALARM_LOG_MIGRATE_CREATE_FAIL;
    
    cm_cnm_migrate_oplog_report(sessionid,pDecodeParam,alarmid);
    return;
}

sint32 cm_cnm_lun_migrate_task_report
(cm_task_send_state_t *pSnd, cm_task_cmt_echo_t **pproc)
{
    cm_task_cmt_echo_t *pCmt = CM_MALLOC(sizeof(cm_task_cmt_echo_t));
    if(NULL == pCmt)
    {
        CM_LOG_ERR(CM_MOD_CNM, "malloc fail");
        return CM_FAIL;
    }

    pCmt->task_pid = pSnd->task_pid;
    pCmt->task_prog = cm_exec_int("/var/cm/script/cm_cnm_migrate.sh task %s", pSnd->task_param);
    CM_LOG_ERR(CM_MOD_CNM, pSnd->task_param);
    if(pCmt->task_prog >= 100)
    {
        pCmt->task_prog = 100;
        pCmt->task_state = CM_TASK_STATE_FINISHED;
        pCmt->task_end = cm_get_time();
    }
    else
    {
        pCmt->task_state = CM_TASK_STATE_RUNNING;
        pCmt->task_end = 0;
    }
    *pproc = pCmt;
    return CM_OK;
}


