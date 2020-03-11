/******************************************************************************
 *          Copyright (c) 2018 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cnm_duallive.c
 * author     : zjd
 * create date: 2018.08.24
 * description: TODO:
 *
 *****************************************************************************/
#include "cm_cnm_duallive.h"
#include "cm_omi_types.h"

#define CM_CNM_BACKUP 0
#define CM_CNM_DUALLIVE 1

static const sint8 *g_cm_cnm_duallive_script = "/var/cm/script/cm_cnm_duallive_nas.sh";
static const sint8 *g_cm_cnm_backup_script = "/var/cm/script/cm_cnm_backup_nas.sh";

sint32 cm_cnm_duallive_nas_init(void)
{
    return CM_OK;
}

static sint32 cm_cnm_duallive_nas_decode_ext(
    const cm_omi_obj_t ObjParam,void* data,cm_omi_field_flag_t* set)
{
    sint32 iRet = CM_OK;
    cm_cnm_duallive_nas_info_t *info = data;
    cm_cnm_decode_param_t param_str[] = 
    {
        {CM_OMI_FIELD_DUALLIVE_NAS_MNAS,sizeof(info->mnas),info->mnas,NULL},
        {CM_OMI_FIELD_DUALLIVE_NAS_SNAS,sizeof(info->snas),info->snas,NULL},
        {CM_OMI_FIELD_DUALLIVE_NAS_WKLOAD_IF,sizeof(info->work_load_if),info->work_load_if,NULL},          
        {CM_OMI_FIELD_DUALLIVE_NAS_WKLOAD_IP,sizeof(info->work_load_ip),info->work_load_ip,NULL},
        {CM_OMI_FIELD_DUALLIVE_NAS_NETMASK,sizeof(info->netmask),info->netmask,NULL},
    };
    
    cm_cnm_decode_param_t param_num[] =
    {
        {CM_OMI_FIELD_DUALLIVE_NAS_MNID,sizeof(info->mnid),&info->mnid,NULL},
        {CM_OMI_FIELD_DUALLIVE_NAS_SNID,sizeof(info->snid),&info->snid,NULL},
        {CM_OMI_FIELD_DUALLIVE_NAS_SYNC,sizeof(info->synctype),&info->synctype,NULL},
    };

    iRet = cm_cnm_decode_str(ObjParam,param_str,sizeof(param_str)/sizeof(cm_cnm_decode_param_t),set);
     if(iRet != CM_OK)
    {
        CM_LOG_WARNING(CM_MOD_CNM,"cm_cnm_duallive_nas_decode_ext");
        return iRet;
    }

    iRet = cm_cnm_decode_num(ObjParam,param_num,sizeof(param_num)/sizeof(cm_cnm_decode_param_t),set);
    if(iRet != CM_OK)
    {
        return iRet;
    }
    return CM_OK;
}

sint32 cm_cnm_duallive_nas_decode(const cm_omi_obj_t ObjParam,void **ppDecodeParam)
{
    return cm_cnm_decode_comm(ObjParam,sizeof(cm_cnm_duallive_nas_info_t),cm_cnm_duallive_nas_decode_ext,ppDecodeParam);
}

static void cm_cnm_duallive_nas_encode_each(cm_omi_obj_t item,void* eachdata,void* arg)
{
    cm_omi_field_flag_t *field = arg;
    cm_cnm_duallive_nas_info_t *info = eachdata;
    cm_cnm_map_value_str_t cols_str[] = 
    {     
        {CM_OMI_FIELD_DUALLIVE_NAS_MNAS,info->mnas},
        {CM_OMI_FIELD_DUALLIVE_NAS_SNAS,info->snas},
        {CM_OMI_FIELD_DUALLIVE_NAS_STATUAS,info->status},
        {CM_OMI_FIELD_DUALLIVE_NAS_WKLOAD_IF,info->work_load_if},
        {CM_OMI_FIELD_DUALLIVE_NAS_WKLOAD_IP,info->work_load_ip},
    };
    cm_cnm_map_value_num_t cols_num[] = 
    {
        {CM_OMI_FIELD_DUALLIVE_NAS_MNID,info->mnid},
        {CM_OMI_FIELD_DUALLIVE_NAS_SNID,info->snid},
        {CM_OMI_FIELD_DUALLIVE_NAS_SYNC,info->synctype},
    };

    cm_cnm_encode_str(item,field,cols_str,sizeof(cols_str)/sizeof(cm_cnm_map_value_str_t));
    cm_cnm_encode_num(item,field,cols_num,sizeof(cols_num)/sizeof(cm_cnm_map_value_str_t));
    return;
}

cm_omi_obj_t cm_cnm_duallive_nas_encode(const void *pDecodeParam,void *pAckData,uint32 AckLen)
{
    return cm_cnm_encode_comm_ext(pDecodeParam,pAckData,AckLen,sizeof(cm_cnm_duallive_nas_info_t),cm_cnm_duallive_nas_encode_each);
}

sint32 cm_cnm_duallive_nas_count(const void * pDecodeParam,void **ppAckData,uint32 *pAckLen)
{
    return cm_cnm_nas_duallive_req(CM_OMI_CMD_COUNT,pDecodeParam,ppAckData,pAckLen);
}

sint32 cm_cnm_duallive_nas_getbatch(const void * pDecodeParam,void **ppAckData,uint32 * pAckLen)
{
    return cm_cnm_nas_duallive_req(CM_OMI_CMD_GET_BATCH,pDecodeParam,ppAckData,pAckLen);
}  

sint32 cm_cnm_duallive_nas_create(const void *pDecodeParam,void **ppAckData,uint32 * pAckLen)
{   
    return cm_cnm_nas_duallive_req(CM_OMI_CMD_ADD,pDecodeParam,ppAckData,pAckLen);
}

sint32 cm_cnm_duallive_nas_delete(const void * pDecodeParam,void **ppAckData,uint32 * pAckLen)
{    
    return cm_cnm_nas_duallive_req(CM_OMI_CMD_DELETE,pDecodeParam,ppAckData,pAckLen);
}



static sint32 cm_cnm_duallive_nas_local_get_each(void *arg, sint8 **cols, uint32 col_num)
{
    cm_cnm_duallive_nas_info_t *seki_info = arg;
    const uint32 def_num = 8;    
    CM_LOG_WARNING(CM_MOD_CNM,"cols[0] = %s,cols[1] = %s,cols[2] = %s,cols[3] = %s,cols[4] = %s,cols[5] = %s",
        cols[0],cols[1],cols[2],cols[3],cols[4],cols[5]);
    if(def_num != col_num)
    {
        CM_LOG_WARNING(CM_MOD_CNM,"col_num[%u] def_num[%u]",col_num,def_num);
        return CM_FAIL;
    }
    CM_MEM_ZERO(seki_info,sizeof(cm_cnm_duallive_nas_info_t));

    CM_VSPRINTF(seki_info->mnas,sizeof(seki_info->mnas),"%s",cols[0]);
    seki_info->mnid = atoi(cols[1]);
    CM_VSPRINTF(seki_info->snas,sizeof(seki_info->snas),"%s",cols[2]);
    seki_info->snid = atoi(cols[3]);
    CM_VSPRINTF(seki_info->status,sizeof(seki_info->status),"%s",cols[4]);
    seki_info->synctype= atoi(cols[5]);
    CM_VSPRINTF(seki_info->work_load_if,sizeof(seki_info->work_load_if),"%s",cols[6]);
    CM_VSPRINTF(seki_info->work_load_ip,sizeof(seki_info->work_load_if),"%s",cols[7]);
    
    return CM_OK;
}

sint32 cm_cnm_duallive_nas_local_getbatch(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    sint32 iRet = CM_OK;
    sint8 cmd[CM_STRING_512] = {0};
    CM_LOG_INFO(CM_MOD_CNM,"offset[%llu] total[%u]",offset,total);
    CM_VSPRINTF(cmd,sizeof(cmd),"%s getbatch",g_cm_cnm_duallive_script);
    iRet = cm_cnm_exec_get_list(cmd,cm_cnm_duallive_nas_local_get_each,
        offset,sizeof(cm_cnm_duallive_nas_info_t),ppAck,&total);

    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]",iRet);
        return iRet;
    }
    *pAckLen = total * sizeof(cm_cnm_duallive_nas_info_t);
    return CM_OK;
}

sint32 cm_cnm_duallive_nas_local_count(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    uint64 cnt = (uint64)cm_exec_int("%s count",
        g_cm_cnm_duallive_script);    
    return cm_cnm_ack_uint64(cnt,ppAck,pAckLen);
}


sint32 cm_cnm_duallive_nas_local_create(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    if( NULL == param)
    {
        CM_LOG_ERR(CM_MOD_CNM,"cm_cnm_backup_nas_local_create pDecodeParam is NULL");
        return CM_PARAM_ERR; 
    }
    const cm_cnm_decode_info_t *req = param;
    const cm_cnm_duallive_nas_info_t *duallive_info =(const cm_cnm_duallive_nas_info_t *)req->data;

    sint32 iRet = CM_OK;
    iRet = cm_system("%s create %d %s %d %s %d %s %s %s",g_cm_cnm_duallive_script,
        duallive_info->mnid,duallive_info->mnas,duallive_info->snid,duallive_info->snas,duallive_info->synctype,
        duallive_info->work_load_ip,duallive_info->work_load_if,duallive_info->netmask);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"%s create %d %s %d %s %d %s %s %s",g_cm_cnm_duallive_script,
        duallive_info->mnid,duallive_info->mnas,duallive_info->snid,duallive_info->snas,duallive_info->synctype,
        duallive_info->work_load_ip,duallive_info->work_load_if,duallive_info->netmask);
        
        return iRet;
    }
    return CM_OK;   
}

sint32 cm_cnm_duallive_nas_local_delete(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    cm_cnm_decode_info_t* req = param;
    const cm_cnm_duallive_nas_info_t *duallive_info = NULL;
    if( NULL == param)
    {
        CM_LOG_ERR(CM_MOD_CNM,"cm_cnm_duallive_nas_create pDecodeParam is NULL");
        return CM_PARAM_ERR; 
    }
    sint32 iRet = CM_OK;

    duallive_info = (const cm_cnm_duallive_nas_info_t *)req->data;

    iRet = cm_system("%s delete %d %s",g_cm_cnm_duallive_script,duallive_info->mnid,duallive_info->mnas);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"%s delete %d %s",g_cm_cnm_duallive_script,duallive_info->mnid,duallive_info->mnas);
        return iRet;
    }
    
    return CM_OK;
}

void cm_cnm_duallive_nas_oplog_create(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK)? CM_ALARM_LOG_NAS_DUALLIVE_CREATR_OK : CM_ALARM_LOG_NAS_DUALLIVE_CREATR_FAIL;
    
    const cm_cnm_decode_info_t *req = pDecodeParam;
    const uint32 cnt = 8;

    /*<master_nid> <master_nas> <slave_nid> <slave_nas> <synctype> <workload_if> <workload_ip> <netmask>*/
    if(NULL == pDecodeParam)
    {   
        cm_cnm_oplog_report(sessionid,alarmid,NULL,cnt,NULL);
    }    
    else    
    {       
        const cm_cnm_duallive_nas_info_t *info = (const cm_cnm_duallive_nas_info_t*)req->data;
        const sint8* mnodename = cm_node_get_name(info->mnid);
        const sint8* snodename = cm_node_get_name(info->snid);
        cm_cnm_oplog_param_t params[] = {
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_DUALLIVE_NAS_MNID,strlen(mnodename),mnodename},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_DUALLIVE_NAS_MNAS,strlen(info->mnas),info->mnas},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_DUALLIVE_NAS_SNID,strlen(snodename),snodename},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_DUALLIVE_NAS_SNAS,strlen(info->snas),info->snas},
            
            {CM_OMI_DATA_INT,CM_OMI_FIELD_DUALLIVE_NAS_SYNC,sizeof(info->synctype),&info->synctype},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_DUALLIVE_NAS_WKLOAD_IF,strlen(info->work_load_if),info->work_load_if},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_DUALLIVE_NAS_WKLOAD_IP,strlen(info->work_load_ip),info->work_load_ip},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_DUALLIVE_NAS_NETMASK,strlen(info->netmask),info->netmask},
        };
        
        cm_cnm_oplog_report(sessionid,alarmid,params,cnt,&req->set);
    }   
    return; 
}

void cm_cnm_duallive_nas_oplog_delete(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK)? CM_ALARM_LOG_NAS_DUALLIVE_DELETE_OK : CM_ALARM_LOG_NAS_DUALLIVE_DELETE_FAIL;
    const cm_cnm_decode_info_t *req = pDecodeParam;
    const uint32 cnt = 2;

    /*<master_nid> <master_nas>*/
    if(NULL == pDecodeParam)
    {   
        cm_cnm_oplog_report(sessionid,alarmid,NULL,cnt,NULL);
    }    
    else    
    {       
        const cm_cnm_duallive_nas_info_t *info = (const cm_cnm_duallive_nas_info_t*)req->data;
        const sint8* mnodename = cm_node_get_name(info->mnid);
        //const sint8* snodename = cm_node_get_name(info->snid);
        cm_cnm_oplog_param_t params[] = {
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_DUALLIVE_NAS_MNID,strlen(mnodename),mnodename},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_DUALLIVE_NAS_MNAS,strlen(info->mnas),info->mnas},
        };
        
        cm_cnm_oplog_report(sessionid,alarmid,params,cnt,&req->set);
    }   
    return; 
}    


/******************************************************************************
 *          Copyright (c) 2018 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cnm_duallive.c
 * author     : zjd
 * create date: 2018.08.24
 * description: TODO:
 *
 *****************************************************************************/

sint32 cm_cnm_backup_nas_init(void)
{
    return CM_OK;
}

static sint32 cm_cnm_backup_nas_decode_ext(const cm_omi_obj_t ObjParam,void* data,cm_omi_field_flag_t* set)
{
    sint32 iRet = CM_OK;
    cm_cnm_backup_nas_info_t *info = data;
    cm_cnm_decode_param_t param_str[] = 
    {
        {CM_OMI_FIELD_BACKUP_NAS_MNAS,sizeof(info->mnas),info->mnas,NULL},
        {CM_OMI_FIELD_BACKUP_NAS_SNAS,sizeof(info->snas),info->snas,NULL},
    };
    cm_cnm_decode_param_t param_num[] =
    {
        {CM_OMI_FIELD_BACKUP_NAS_MNID,sizeof(info->mnid),&info->mnid,NULL},
        {CM_OMI_FIELD_BACKUP_NAS_SNID,sizeof(info->snid),&info->snid,NULL},
        {CM_OMI_FIELD_BACKUP_NAS_SYNC,sizeof(info->synctype),&info->synctype,NULL},
    };

    iRet = cm_cnm_decode_str(ObjParam,param_str,
        sizeof(param_str)/sizeof(cm_cnm_decode_param_t),set);
     if(iRet != CM_OK)
    {
        CM_LOG_WARNING(CM_MOD_CNM,"cm_cnm_backup_nas_decode_ext");
        return iRet;
    }

    iRet = cm_cnm_decode_num(ObjParam,param_num,
        sizeof(param_num)/sizeof(cm_cnm_decode_param_t),set);
    if(iRet != CM_OK)
    {
        return iRet;
    }
    return CM_OK;
}

sint32 cm_cnm_backup_nas_decode(const cm_omi_obj_t ObjParam,void **ppDecodeParam)
{
    return cm_cnm_decode_comm(ObjParam,sizeof(cm_cnm_backup_nas_info_t),cm_cnm_backup_nas_decode_ext,ppDecodeParam);
}

static void cm_cnm_backup_nas_encode_each(cm_omi_obj_t item,void* eachdata,void* arg)
{
    cm_omi_field_flag_t *field = arg;
    cm_cnm_backup_nas_info_t *info = eachdata;
    cm_cnm_map_value_str_t cols_str[] = 
    {     
        {CM_OMI_FIELD_BACKUP_NAS_MNAS,info->mnas},
        {CM_OMI_FIELD_BACKUP_NAS_SNAS,info->snas},
        {CM_OMI_FIELD_BACKUP_NAS_STATUAS,info->status},
    };
    cm_cnm_map_value_num_t cols_num[] = 
    {
        {CM_OMI_FIELD_BACKUP_NAS_MNID,info->mnid},
        {CM_OMI_FIELD_BACKUP_NAS_SNID,info->snid},
        {CM_OMI_FIELD_BACKUP_NAS_SYNC,info->synctype},
    };

    cm_cnm_encode_str(item,field,cols_str,sizeof(cols_str)/sizeof(cm_cnm_map_value_str_t));
    cm_cnm_encode_num(item,field,cols_num,sizeof(cols_num)/sizeof(cm_cnm_map_value_str_t));
    return;
}

cm_omi_obj_t cm_cnm_backup_nas_encode(const void *pDecodeParam,void *pAckData,
uint32 AckLen)
{
    return cm_cnm_encode_comm_ext(pDecodeParam,pAckData,AckLen,
        sizeof(cm_cnm_backup_nas_info_t),cm_cnm_backup_nas_encode_each);
}

sint32 cm_cnm_backup_nas_count(const void * pDecodeParam,void **ppAckData,
uint32 *pAckLen)
{
    return cm_cnm_nas_backup_req(CM_OMI_CMD_COUNT,pDecodeParam,ppAckData,pAckLen);
}

sint32 cm_cnm_backup_nas_getbatch(
    const void * pDecodeParam,void **ppAckData,uint32 * pAckLen)
{
    return cm_cnm_nas_backup_req(CM_OMI_CMD_GET_BATCH,pDecodeParam,ppAckData,pAckLen);
}  

sint32 cm_cnm_backup_nas_create(const void *pDecodeParam,void **ppAckData,
uint32 * pAckLen)
{ 
    return cm_cnm_nas_backup_req(CM_OMI_CMD_ADD,pDecodeParam,ppAckData,pAckLen);
}


sint32 cm_cnm_backup_nas_delete(const void * pDecodeParam,void **ppAckData,uint32 * pAckLen)
{   
    return cm_cnm_nas_backup_req(CM_OMI_CMD_DELETE,pDecodeParam,ppAckData,pAckLen);
}



static sint32 cm_cnm_backup_nas_local_get_each(void *arg, sint8 **cols, uint32 col_num)
{
    cm_cnm_backup_nas_info_t *seki_info = arg;
    const uint32 def_num = 6;    
    
    if(def_num != col_num)
    {
        CM_LOG_WARNING(CM_MOD_CNM,"col_num[%u] def_num[%u]",col_num,def_num);
        return CM_FAIL;
    }
    CM_MEM_ZERO(seki_info,sizeof(cm_cnm_backup_nas_info_t));

    CM_VSPRINTF(seki_info->mnas,sizeof(seki_info->mnas),"%s",cols[0]);
    seki_info->mnid = atoi(cols[1]);
    CM_VSPRINTF(seki_info->snas,sizeof(seki_info->snas),"%s",cols[2]);
    seki_info->snid = atoi(cols[3]);
    CM_VSPRINTF(seki_info->status,sizeof(seki_info->status),"%s",cols[4]);
    seki_info->synctype= atoi(cols[5]);
    
    return CM_OK;
}

sint32 cm_cnm_backup_nas_local_getbatch(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    sint32 iRet = CM_OK;
    sint8 cmd[CM_STRING_512] = {0};

    CM_LOG_INFO(CM_MOD_CNM,"offset[%llu] total[%u]",offset,total);
    CM_VSPRINTF(cmd,sizeof(cmd),"%s getbatch",g_cm_cnm_backup_script);
    iRet = cm_cnm_exec_get_list(cmd,cm_cnm_backup_nas_local_get_each,offset,sizeof(cm_cnm_backup_nas_info_t),ppAck,&total);

    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]",iRet);
        return iRet;
    }
    *pAckLen = total * sizeof(cm_cnm_backup_nas_info_t);
    return CM_OK;

}

sint32 cm_cnm_backup_nas_local_count(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    uint64 cnt = (uint64)cm_exec_int("%s count",g_cm_cnm_backup_script);    
    return cm_cnm_ack_uint64(cnt,ppAck,pAckLen);
}

sint32 cm_cnm_backup_nas_local_create(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    if( NULL == param)
    {
        CM_LOG_ERR(CM_MOD_CNM,"cm_cnm_backup_nas_local_create pDecodeParam is NULL");
        return CM_PARAM_ERR; 
    }
    sint32 iRet = CM_OK;
    
    const cm_cnm_decode_info_t *req = param;
    const cm_cnm_backup_nas_info_t *duallive_info =(const cm_cnm_backup_nas_info_t *)req->data;

    iRet = cm_system("%s create %d %s %d %s %d",g_cm_cnm_backup_script,
        duallive_info->mnid,duallive_info->mnas,duallive_info->snid,
        duallive_info->snas,duallive_info->synctype);
    
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM," %d %s %d %s fail[%d]",
        duallive_info->mnid,duallive_info->mnas,duallive_info->snid,duallive_info->snas,iRet);
        return iRet;
    }
    
    return CM_OK;
}

sint32 cm_cnm_backup_nas_local_delete(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    const cm_cnm_backup_nas_info_t *duallive_info = NULL;
    sint32 iRet = CM_OK;
    cm_cnm_decode_info_t* req = param;
    if( NULL == req)
    {
        CM_LOG_ERR(CM_MOD_CNM,"cm_cnm_backup_nas_create pDecodeParam is NULL");
        return CM_PARAM_ERR; 
    }
    
    duallive_info = (const cm_cnm_backup_nas_info_t *)req->data;

    CM_LOG_ERR(CM_MOD_CNM,"delete start");
    iRet = cm_system("%s delete %d %s",g_cm_cnm_backup_script,duallive_info->mnid,duallive_info->mnas);
    CM_LOG_ERR(CM_MOD_CNM,"delete end");
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"%s delete %d %s",g_cm_cnm_backup_script,duallive_info->mnid,duallive_info->mnas);
        return iRet;
    }
    
    return CM_OK;
}

void cm_cnm_backup_nas_oplog_create(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK)? CM_ALARM_LOG_NAS_BACKUP_CREATR_OK : CM_ALARM_LOG_NAS_BACKUP_CREATR_FAIL;
    
    const cm_cnm_decode_info_t *req = pDecodeParam;
    const uint32 cnt = 5;

    /*<master_nid> <master_nas> <slave_nid> <slave_nas> <synctype> <workload_if> <workload_ip> <netmask>*/
    if(NULL == pDecodeParam)
    {   
        cm_cnm_oplog_report(sessionid,alarmid,NULL,cnt,NULL);
    }    
    else    
    {       
        const cm_cnm_backup_nas_info_t *info = (const cm_cnm_backup_nas_info_t*)req->data;
        const sint8* mnodename = cm_node_get_name(info->mnid);
        const sint8* snodename = cm_node_get_name(info->snid);
        cm_cnm_oplog_param_t params[] = {
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_BACKUP_NAS_MNID,strlen(mnodename),mnodename},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_BACKUP_NAS_MNAS,strlen(info->mnas),info->mnas},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_BACKUP_NAS_SNID,strlen(snodename),snodename},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_BACKUP_NAS_SNAS,strlen(info->snas),info->snas},           
            {CM_OMI_DATA_INT,CM_OMI_FIELD_BACKUP_NAS_SYNC,sizeof(info->synctype),&info->synctype},
        };
        
        cm_cnm_oplog_report(sessionid,alarmid,params,cnt,&req->set);
    }   
    return; 
}

void cm_cnm_backup_nas_oplog_delete(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK)? CM_ALARM_LOG_NAS_BACKUP_DELETE_OK : CM_ALARM_LOG_NAS_BACKUP_DELETE_FAIL;
    const cm_cnm_decode_info_t *req = pDecodeParam;
    const uint32 cnt = 2;

    /*<master_nid> <master_nas>*/
    if(NULL == pDecodeParam)
    {   
        cm_cnm_oplog_report(sessionid,alarmid,NULL,cnt,NULL);
    }    
    else    
    {       
        const cm_cnm_backup_nas_info_t *info = (const cm_cnm_backup_nas_info_t*)req->data;
        const sint8* mnodename = cm_node_get_name(info->mnid);
        const sint8* snodename = cm_node_get_name(info->snid);
        cm_cnm_oplog_param_t params[] = {
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_BACKUP_NAS_MNID,strlen(mnodename),mnodename},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_BACKUP_NAS_MNAS,strlen(info->mnas),info->mnas},
        };
        
        cm_cnm_oplog_report(sessionid,alarmid,params,cnt,&req->set);
    }   
    return; 
}   

sint32 cm_cnm_cluster_target_init(void)
{
    return CM_OK;
}

static sint32 cm_cnm_cluster_target_decode_ext(const cm_omi_obj_t ObjParam,void* data,cm_omi_field_flag_t* set)
{
    sint32 iRet = CM_OK;
    cm_cnm_cluster_target_info_t *info = data;
    cm_cnm_decode_param_t param_num[] =
    {
        {CM_OMI_FIELD_DUALLIVE_NETIF_NID,sizeof(info->nid),&info->nid,NULL},       
    };
    iRet = cm_cnm_decode_num(ObjParam,param_num,
        sizeof(param_num)/sizeof(cm_cnm_decode_param_t),set);
    if(iRet != CM_OK)
    {   CM_LOG_ERR(CM_MOD_CNM,"cm_cnm_duallive_netif_decode_ext");
        return iRet;
    }
    return CM_OK;
}

sint32 cm_cnm_cluster_target_decode(
    const cm_omi_obj_t ObjParam,void **ppDecodeParam)
{
    return cm_cnm_decode_comm(ObjParam,sizeof(cm_cnm_cluster_target_info_t),cm_cnm_cluster_target_decode_ext,ppDecodeParam);
}

static void cm_cnm_cluster_target_encode_each(cm_omi_obj_t item,void* eachdata,void* arg)
{
    cm_omi_field_flag_t *field = arg;
    cm_cnm_cluster_target_info_t *info = eachdata;
    cm_cnm_map_value_str_t cols_str[] = 
    {     
        {CM_OMI_FIELD_DUALLIVE_NETIF_TARGET,info->list_target},
    };

    cm_cnm_encode_str(item,field,cols_str,sizeof(cols_str)/sizeof(cm_cnm_map_value_str_t));
    return;
}

cm_omi_obj_t cm_cnm_cluster_target_encode(
    const void *pDecodeParam,void *pAckData,uint32 AckLen)
{
    return cm_cnm_encode_comm_ext(pDecodeParam,pAckData,AckLen,
        sizeof(cm_cnm_cluster_target_info_t),cm_cnm_cluster_target_encode_each);
}

sint32 cm_cnm_cluster_target_getbatch(
    const void * pDecodeParam,void **ppAckData,uint32 * pAckLen)
{
    if(NULL == pDecodeParam)
    {
        return CM_PARAM_ERR;
    }
    return cm_cnm_cluster_target_req(CM_OMI_CMD_GET_BATCH,pDecodeParam,ppAckData,pAckLen);
}

static sint32 cm_cnm_cluster_target_local_get_each(void *arg, sint8 **cols, uint32 col_num)
{
    cm_cnm_cluster_target_info_t *seki_info = arg;
    const uint32 def_num = 1;   
    if(def_num != col_num)
    {
        CM_LOG_WARNING(CM_MOD_CNM,"col_num[%u] def_num[%u]",col_num,def_num);
        return CM_FAIL;
    }
    CM_MEM_ZERO(seki_info,sizeof(cm_cnm_cluster_target_info_t));
    CM_VSPRINTF(seki_info->list_target,sizeof(seki_info->list_target),"%s",cols[0]);
    
    return CM_OK;
}

sint32 cm_cnm_cluster_target_local_getbatch(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    sint32 iRet = CM_OK;
    iRet = cm_cnm_exec_get_list("zfs clustersan list-target | awk '{print $2}'",cm_cnm_cluster_target_local_get_each,offset,sizeof(cm_cnm_cluster_target_info_t),ppAck,&total);

    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"zfs clustersan list-target fail iRet[%d]",iRet);
        return iRet;
    }
    *pAckLen = total * sizeof(cm_cnm_cluster_target_info_t);
    return CM_OK;
}




