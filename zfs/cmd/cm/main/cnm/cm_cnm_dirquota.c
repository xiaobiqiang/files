/******************************************************************************
 * description  : share dirquota
 * author       : zjd
 * create date  : 2018.8.13
 *****************************************************************************/
#include "cm_cnm_dirquota.h"

const uint8* g_cm_cnm_dirquota_script = "/var/cm/script/cm_cnm_dirquota.sh";

static sint32 cm_cnm_dirquota_decode_ext(
        const cm_omi_obj_t ObjParam,void* data,cm_omi_field_flag_t *set)
{
    sint32 iRet = CM_OK;
    cm_cnm_dirquota_info_t* dirquota_info = data;
    
    cm_cnm_decode_param_t param_str[] =
    {
        {CM_OMI_FIRLD_DIRQUOTA_DIR,sizeof(dirquota_info->dir_path),dirquota_info->dir_path,NULL},
        {CM_OMI_FIRLD_DIRQUOTA_QUOTA,sizeof(dirquota_info->quota),dirquota_info->quota,NULL},
        {CM_OMI_FIRLD_DIRQUOTA_NAS,sizeof(dirquota_info->nas),dirquota_info->nas,NULL},
    };

    cm_cnm_decode_param_t param_num[] = 
    {   
        {CM_OMI_NFS_CFG_NID,sizeof(dirquota_info->nid),&dirquota_info->nid,NULL},
    };
    
    iRet = cm_cnm_decode_str(ObjParam,param_str,
        sizeof(param_str)/sizeof(cm_cnm_decode_param_t),set);
    if(iRet != CM_OK)
    {
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

sint32 cm_cnm_dirquota_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam)
{
    return cm_cnm_decode_comm(ObjParam,sizeof(cm_cnm_dirquota_info_t),
        cm_cnm_dirquota_decode_ext,ppDecodeParam);
}

/*divide "/pool/nas/dirpath..." into "/pool/nas" and "/dirpath..."*/
/*
static uint32 cm_cnm_dirquota_anlysis_path(
    const sint8 *src,sint8 *nas,uint32 nas_size,sint8 *dir,uint32 dir_size)
{
    if(NULL == src)
    {
        return CM_FAIL;
    }
    uint32 size = strlen(src) + 1;
    sint8 * tp = NULL;
    sint8 * const tmp_path = CM_MALLOC(size);
    CM_MEM_CPY(tmp_path,size,(src + 1),size);
    tp = strchr(tmp_path,'/');
    if(NULL == tp)
    {
        CM_FREE(tmp_path);
        return CM_FAIL;
    }
    tp = strchr((tp+1),'/');
    if(NULL == tp)
    {
        CM_FREE(tmp_path);
        return CM_FAIL;
    }
    if('\0' == *(tp + 1))
    {
        CM_FREE(tmp_path);
        return CM_FAIL;
    }
    *tp = 0;
    if(NULL != nas && nas_size > 0)
    {
        strncpy(nas,tmp_path,nas_size);
    }
    if(NULL != dir && dir_size > 0)
    {
        strncpy(dir,(tp+1),dir_size);
    }
    CM_FREE(tmp_path);
    return CM_OK;
}
*/
static uint32 cm_cnm_dirquota_anlysis_path(
    const sint8 *src,sint8 *nas,uint32 nas_size,sint8 *dir,uint32 dir_size)
{
    if(NULL == src)
    {
        return CM_FAIL;
    }
    uint32 size = strlen(src) + 1;
    sint8 * tp = NULL;
    sint8 * tmp_path = CM_MALLOC(size);
    CM_MEM_CPY(tmp_path,size,src,size);
    tp = strtok(tmp_path,"/");
    if(tp)
    {
        CM_SNPRINTF_ADD(nas, CM_STRING_256, "%s/", tp);
    }
    tp = strtok(NULL,"/");
    if(tp)
    { 
        CM_SNPRINTF_ADD(nas, CM_STRING_256, "%s", tp);
    }
    tp = strtok(NULL,"/");
    if(tp)
    { 
        CM_SNPRINTF_ADD(dir, CM_STRING_256, "%s", tp);
    }
    CM_FREE(tmp_path);
    return CM_OK;
}

static void cm_cnm_dirquota_encode_each(cm_omi_obj_t item,void *eachdata,void *arg)
{
    cm_omi_field_flag_t *field = arg;
    cm_cnm_dirquota_info_t * dirquota_info = eachdata;
    
    cm_cnm_map_value_str_t cols_str[] = 
    {
        {CM_OMI_FIRLD_DIRQUOTA_DIR,dirquota_info->dir_path},
        {CM_OMI_FIRLD_DIRQUOTA_QUOTA,dirquota_info->quota},
        {CM_OMI_FIRLD_DIRQUOTA_USED,dirquota_info->used},
        {CM_OMI_FIRLD_DIRQUOTA_REST,dirquota_info->rest},
    };

    cm_cnm_map_value_num_t cols_num[] = 
    {
        {CM_OMI_NFS_CFG_NID,dirquota_info->nid},
        
    };
    cm_cnm_map_value_double_t cols_double[] =
    {
        {CM_OMI_FIRLD_DIRQUOTA_PER_USED,dirquota_info->per_used},
    };
    cm_cnm_encode_str(item,field,cols_str,sizeof(cols_str)/sizeof(cm_cnm_map_value_str_t));
    cm_cnm_encode_num(item,field,cols_num,sizeof(cols_num)/sizeof(cm_cnm_map_value_num_t));
    cm_cnm_encode_double(item,field,cols_double,sizeof(cols_double)/sizeof(cm_cnm_map_value_double_t));
    return;
}

cm_omi_obj_t cm_cnm_dirquota_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen)
{
    return cm_cnm_encode_comm_ext(pDecodeParam,pAckData,AckLen,
        sizeof(cm_cnm_dirquota_info_t),cm_cnm_dirquota_encode_each);
}

#define cm_cnm_dirquota_req(cmd,param,ppAck,plen) \
    cm_cnm_request_comm(CM_OMI_OBJECT_DIRQUOTA,cmd,sizeof(cm_cnm_dirquota_info_t),param,ppAck,plen)


sint32 cm_cnm_dirquota_getbatch(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{   
    return cm_cnm_dirquota_req(CM_OMI_CMD_GET_BATCH,pDecodeParam,ppAckData,pAckLen);
}

sint32 cm_cnm_dirquota_get(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{   
    if( NULL == pDecodeParam)
    {
        return CM_FAIL; 
    }
    return cm_cnm_dirquota_req(CM_OMI_CMD_GET,pDecodeParam,ppAckData,pAckLen);
}   
sint32 cm_cnm_dirquota_add(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    if( NULL == pDecodeParam)
    {
        return CM_FAIL; 
    }
    return cm_cnm_dirquota_req(CM_OMI_CMD_ADD,pDecodeParam,ppAckData,pAckLen);
}

sint32 cm_cnm_dirquota_count(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_cnm_dirquota_req(CM_OMI_CMD_COUNT,pDecodeParam,ppAckData,pAckLen);
}

sint32 cm_cnm_dirquota_delete(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    if( NULL == pDecodeParam )
    {
        return CM_FAIL; 
    }
    return cm_cnm_dirquota_req(CM_OMI_CMD_DELETE,pDecodeParam,ppAckData,pAckLen);
}

static sint32 cm_cnm_dirquota_local_get_each(void *arg, sint8 **cols, uint32 col_num)
{
    cm_cnm_dirquota_info_t *dirquota_info = arg;
    const uint32 def_num = 3;    
    float used, rest, quota;
    if(def_num != col_num)
    {
        CM_LOG_WARNING(CM_MOD_CNM,"col_num[%u] def_num[%u]",col_num,def_num);
        return CM_FAIL;
    }
    /*get ret*/
    CM_MEM_CPY(dirquota_info->dir_path,sizeof(dirquota_info->dir_path),cols[0],strlen(cols[0])+1);
    CM_MEM_CPY(dirquota_info->quota,sizeof(dirquota_info->quota),cols[1],strlen(cols[1])+1);
    CM_MEM_CPY(dirquota_info->used,sizeof(dirquota_info->used),cols[2],strlen(cols[2])+1);
    if(atoi(cols[2]) != 0)
    {
        used = (float)cm_cnm_trans_result(cols[2]);
    }
    else
    {
        used = 0.0;
    }
    quota = (float)cm_cnm_trans_result(cols[1]);
    rest = quota - used;
    CM_SNPRINTF_ADD(dirquota_info->rest, sizeof(dirquota_info->rest),"%fM",rest);
    dirquota_info->per_used = used / quota;   
    dirquota_info->nid = cm_node_get_id();

    return CM_OK;
}

sint32 cm_cnm_dirquota_local_getbatch(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    sint32 iRet = CM_OK;
    sint8 cmd[CM_STRING_1K] = {0};
    cm_cnm_decode_info_t * req = param;
    const cm_cnm_dirquota_info_t * dirquota_info = NULL;
    
    if(NULL == req)
    {
        CM_VSPRINTF(cmd,sizeof(cmd),"%s getbatch",g_cm_cnm_dirquota_script);
    }
    else
    {
        if(CM_OMI_FIELDS_FLAG_ISSET(&req->set, CM_OMI_FIRLD_DIRQUOTA_NAS))
        {
            dirquota_info = req->data;
            CM_VSPRINTF(cmd,sizeof(cmd),"%s get %s all 2>/dev/null",
                g_cm_cnm_dirquota_script,dirquota_info->nas);
        }
        else
        {
            CM_VSPRINTF(cmd,sizeof(cmd),"%s getbatch",g_cm_cnm_dirquota_script);
        }
    }
    iRet = cm_cnm_exec_get_list(cmd,cm_cnm_dirquota_local_get_each,
        (uint32)offset,sizeof(cm_cnm_dirquota_info_t),ppAck,&total);

    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]",iRet);
        return iRet;
    }
    *pAckLen = total * sizeof(cm_cnm_dirquota_info_t);
    
    return CM_OK;

}

sint32 cm_cnm_dirquota_local_count(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    uint64 cnt = 0;
    sint8 cmd[CM_STRING_1K] = {0};
    const cm_cnm_decode_info_t * req = param;
    const cm_cnm_dirquota_info_t * dirquota_info = NULL;
    if(NULL == req)
    {
        CM_VSPRINTF(cmd,sizeof(cmd),"%s getbatch|wc -l",g_cm_cnm_dirquota_script);
    }
    else
    {
        if(CM_OMI_FIELDS_FLAG_ISSET(&req->set, CM_OMI_FIRLD_DIRQUOTA_NAS))
        {
            dirquota_info = req->data;
            CM_VSPRINTF(cmd,sizeof(cmd),"%s get %s all 2>/dev/null|wc -l",
                g_cm_cnm_dirquota_script,dirquota_info->nas);
        }
        else
        {
            CM_VSPRINTF(cmd,sizeof(cmd),"%s getbatch|wc -l",g_cm_cnm_dirquota_script);
        }
    }
    cnt = (uint64)cm_exec_int("%s",cmd);

    return cm_cnm_ack_uint64(cnt,ppAck,pAckLen);
}

sint32 cm_cnm_dirquota_local_add(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    cm_cnm_decode_info_t* req = param;
    const cm_cnm_dirquota_info_t *dirquota_info = NULL;	
    sint32 iRet = CM_OK;
    sint8 cmd[CM_STRING_1K] = {0};
    sint8 nas[CM_STRING_256] = {0};
    sint8 dir[CM_STRING_256] = {0};
    if(NULL == req)
    {
        return CM_OK;
    }
    dirquota_info = (const cm_cnm_dirquota_info_t *)req->data;

    cm_cnm_dirquota_anlysis_path(dirquota_info->dir_path,nas,sizeof(nas),dir,sizeof(dir));
    CM_VSPRINTF(cmd,sizeof(cmd),"%s add %s %s %s",g_cm_cnm_dirquota_script,
                                                    nas,dir,dirquota_info->quota);
    iRet = cm_exec_cmd(cmd,CM_CMT_REQ_TMOUT);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]",iRet);
        return iRet;
    }

    return CM_OK;
}

sint32 cm_cnm_dirquota_local_get(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    cm_cnm_decode_info_t* req = param;
    const cm_cnm_dirquota_info_t *dirquota_info = NULL;
    sint32 iRet = CM_OK;
    sint8 cmd[CM_STRING_1K] = {0};
    sint8 nas[CM_STRING_256] = {0};
    if(NULL == req)
    {
        return CM_OK;
    }
    dirquota_info = (const cm_cnm_dirquota_info_t *)req->data;
    cm_cnm_dirquota_anlysis_path(dirquota_info->dir_path,nas,sizeof(nas),NULL,0);
    
    CM_LOG_INFO(CM_MOD_CNM,"nas:%s  dir_path:%s",nas,dirquota_info->dir_path);
    CM_VSPRINTF(cmd,sizeof(cmd),"%s get %s %s",g_cm_cnm_dirquota_script,nas,dirquota_info->dir_path);
    iRet = cm_cnm_exec_get_list(cmd,cm_cnm_dirquota_local_get_each,
            0,sizeof(cm_cnm_dirquota_info_t),ppAck,&total);
    
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]",iRet);
        return iRet;
    }
    
    *pAckLen = total * sizeof(cm_cnm_dirquota_info_t);
    
    return CM_OK;
}

sint32 cm_cnm_dirquota_local_delete(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    cm_cnm_decode_info_t* req = param;
    const cm_cnm_dirquota_info_t *dirquota_info = NULL;	
    sint32 iRet = CM_OK;
    sint8 cmd[CM_STRING_1K] = {0};
    sint8 nas[CM_STRING_256] = {0};
    sint8 dir[CM_STRING_256] = {0};
    if(NULL == req)
    {
        return CM_OK;
    }
    dirquota_info = (const cm_cnm_dirquota_info_t *)req->data;

    cm_cnm_dirquota_anlysis_path(dirquota_info->dir_path,nas,sizeof(nas),dir,sizeof(dir));
    CM_VSPRINTF(cmd,sizeof(cmd),"%s add %s %s 0M",g_cm_cnm_dirquota_script,nas,dir);
    iRet = cm_exec_cmd(cmd,CM_CMT_REQ_TMOUT);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]",iRet);
        return iRet;
    }

    return CM_OK;
}

void cm_cnm_dirquota_oplog_add(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK)? CM_ALARM_LOG_DIRQUOTA_CREATR_OK : CM_ALARM_LOG_DIRQUOTA_CREATR_FAIL;
    
    const cm_cnm_decode_info_t *req = pDecodeParam;
    const uint32 cnt = 3;

    if(NULL == pDecodeParam)
    {   
        cm_cnm_oplog_report(sessionid,alarmid,NULL,cnt,NULL);
    }    
    else    
    {       
        const cm_cnm_dirquota_info_t *info = (const cm_cnm_dirquota_info_t*)req->data;
        const sint8* nodename = cm_node_get_name(info->nid);
        cm_cnm_oplog_param_t params[] = {
            {CM_OMI_DATA_STRING,CM_OMI_FIRLD_DIRQUOTA_NID,strlen(nodename),nodename},
            {CM_OMI_DATA_STRING,CM_OMI_FIRLD_DIRQUOTA_DIR,strlen(info->dir_path),info->dir_path},
            {CM_OMI_DATA_STRING,CM_OMI_FIRLD_DIRQUOTA_QUOTA,strlen(info->quota),info->quota},
        };
        
        cm_cnm_oplog_report(sessionid,alarmid,params,cnt,&req->set);
    }   
    return;
}

void cm_cnm_dirquota_oplog_delete(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK)? CM_ALARM_LOG_NFS_DELETE_OK : CM_ALARM_LOG_NFS_DELETE_FAIL;
    const cm_cnm_decode_info_t *req = pDecodeParam;
    const uint32 cnt = 2;

    if(NULL == pDecodeParam)
    {   
        cm_cnm_oplog_report(sessionid,alarmid,NULL,cnt,NULL);
    }    
    else    
    {       
        const cm_cnm_dirquota_info_t *info = (const cm_cnm_dirquota_info_t*)req->data;
        const sint8* nodename = cm_node_get_name(info->nid);
        cm_cnm_oplog_param_t params[] = {
            {CM_OMI_DATA_STRING,CM_OMI_FIRLD_DIRQUOTA_NID,strlen(nodename),nodename},
            {CM_OMI_DATA_STRING,CM_OMI_FIRLD_DIRQUOTA_DIR,strlen(info->dir_path),info->dir_path},
        };
        
        cm_cnm_oplog_report(sessionid,alarmid,params,cnt,&req->set);
    }   
    return; 
}    

