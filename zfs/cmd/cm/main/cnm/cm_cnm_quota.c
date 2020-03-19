/******************************************************************************
 *          Copyright (c) 2018 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cnm_quota.c
 * author     : xar
 * create date: 2018.08.24
 * description: TODO:
 *
 *****************************************************************************/
#include "cm_cnm_quota.h"
#include "cm_log.h"
#include "cm_xml.h"
#include "cm_node.h"
#include "cm_omi.h"
#include "cm_db.h"

#define cm_cnm_quota_req(cmd,param,ppAck,plen) \
    cm_cnm_request_comm(CM_OMI_OBJECT_QUOTA,cmd,sizeof(cm_cnm_quota_info_t),param,ppAck,plen)

sint32 cm_cnm_quota_init(void)
{
    return CM_OK;
}

static sint32 cm_cnm_quota_decode_ext(const cm_omi_obj_t ObjParam, void* data, cm_omi_field_flag_t *set)
{
    sint32 iRet = CM_OK;
    cm_cnm_quota_info_t* info = data;
    uint32 nid = 0;
    uint32 cut = 0;
    cm_cnm_decode_param_t param_str[] =
    {
        {CM_OMI_FIELD_QUOTA_NAME, sizeof(info->name), info->name, NULL},
        {CM_OMI_FIELD_QUOTA_FILESYSTEM, sizeof(info->filesystem), info->filesystem, NULL},
        {CM_OMI_FIELD_QUOTA_HARDSPACE, sizeof(info->space), info->space, NULL},
        {CM_OMI_FIELD_QUOTA_SOFTSPACE, sizeof(info->softspace), info->softspace, NULL},
        {CM_OMI_FIELD_QUOTA_USED, sizeof(info->used), info->used, NULL},
    };

    cm_cnm_decode_param_t param_num[] =
    {
        {CM_OMI_FIELD_QUOTA_NID, sizeof(nid), &nid, NULL},
        {CM_OMI_FIELD_QUOTA_USERTYPE, sizeof(info->usertype), &info->usertype, NULL},
        {CM_OMI_FIELD_QUOTA_DOMAIN, sizeof(info->domain), &info->domain, NULL},
    };

    info->domain = CM_DOMAIN_LOCAL;
    iRet = cm_cnm_decode_str(ObjParam, param_str,
                             sizeof(param_str) / sizeof(cm_cnm_decode_param_t), set);

    if(CM_OK != iRet)
    {
        return iRet;
    }

    iRet = cm_cnm_decode_num(ObjParam, param_num,
                             sizeof(param_num) / sizeof(cm_cnm_decode_param_t), set);

    if(CM_OK != iRet)
    {
        return iRet;
    }

    if(!CM_OMI_FIELDS_FLAG_ISSET(set, CM_OMI_FIELD_QUOTA_NID) ||
       !CM_OMI_FIELDS_FLAG_ISSET(set, CM_OMI_FIELD_QUOTA_FILESYSTEM))
    {
        return CM_PARAM_ERR;
    }

    cut = cm_cnm_exec_count(nid, "zfs list -H -o name '%s' 2>/dev/null |wc -l", info->filesystem);

    if(0 == cut)
    {
        return CM_PARAM_ERR;
    }

    return CM_OK;
}

sint32 cm_cnm_quota_decode(const cm_omi_obj_t ObjParam, void **ppDecodeParam)
{
    return cm_cnm_decode_comm(ObjParam, sizeof(cm_cnm_quota_info_t), cm_cnm_quota_decode_ext, ppDecodeParam);
}

static void cm_cnm_quota_encode_each(cm_omi_obj_t item, void* eachdata, void* arg)
{
    cm_omi_field_flag_t *field = arg;
    cm_cnm_quota_info_t *info = eachdata;
    cm_cnm_map_value_str_t cols_str[] =
    {
        /*{CM_OMI_FIELD_QUOTA_FILESYSTEM,info->filesystem},*/
        {CM_OMI_FIELD_QUOTA_NAME, info->name},
        {CM_OMI_FIELD_QUOTA_HARDSPACE, info->space},
        {CM_OMI_FIELD_QUOTA_SOFTSPACE, info->softspace},
        {CM_OMI_FIELD_QUOTA_USED, info->used},
    };

    cm_cnm_map_value_num_t cols_num[] = 
    {
        {CM_OMI_FIELD_QUOTA_USERTYPE,     (uint32)info->usertype},
        {CM_OMI_FIELD_QUOTA_DOMAIN,        (uint32)info->domain},
    };
    

    cm_cnm_encode_str(item, field, cols_str, sizeof(cols_str) / sizeof(cm_cnm_map_value_str_t));
    cm_cnm_encode_num(item, field, cols_num, sizeof(cols_num) / sizeof(cm_cnm_map_value_num_t));
    return;
}

cm_omi_obj_t cm_cnm_quota_encode(const void *pDecodeParam, void *pAckData, uint32 AckLen)
{
    return cm_cnm_encode_comm_ext(pDecodeParam, pAckData, AckLen,
                                  sizeof(cm_cnm_quota_info_t), cm_cnm_quota_encode_each);
}

sint32 cm_cnm_quota_getbatch(const void *pDecodeParam, void **ppAckData, uint32 *pAckLen)
{
    if(NULL == pDecodeParam)
    {
        return CM_PARAM_ERR;
    }

    return cm_cnm_quota_req(CM_OMI_CMD_GET_BATCH, pDecodeParam, ppAckData, pAckLen);
}

sint32 cm_cnm_quota_get(const void *pDecodeParam, void **ppAckData, uint32 *pAckLen)
{
    if(NULL == pDecodeParam)
    {
        return CM_PARAM_ERR;
    }

    return cm_cnm_quota_req(CM_OMI_CMD_GET, pDecodeParam, ppAckData, pAckLen);
}

sint32 cm_cnm_quota_count(const void *pDecodeParam, void **ppAckData, uint32 *pAckLen)
{
    if(NULL == pDecodeParam)
    {
        return CM_PARAM_ERR;
    }

    return cm_cnm_quota_req(CM_OMI_CMD_COUNT, pDecodeParam, ppAckData, pAckLen);
}

sint32 cm_cnm_quota_update(const void *pDecodeParam, void **ppAckData, uint32 *pAckLen)
{
    if(NULL == pDecodeParam)
    {
        return CM_PARAM_ERR;
    }

    return cm_cnm_quota_req(CM_OMI_CMD_MODIFY, pDecodeParam, ppAckData, pAckLen);
}

sint32 cm_cnm_quota_delete(const void * pDecodeParam, void **ppAckData, uint32 *pAckLen)
{
    if(NULL == pDecodeParam)
    {
        return CM_PARAM_ERR;
    }

    return cm_cnm_quota_req(CM_OMI_CMD_DELETE, pDecodeParam, ppAckData, pAckLen);
}


/*                   local                         */

sint32 cm_cnm_quota_local_get(
    void *param, uint32 len,
    uint64 offset, uint32 total,
    void **ppAck, uint32 *pAckLen)
{
    const cm_cnm_decode_info_t *req = param;
    const cm_cnm_quota_info_t *info = NULL;
    cm_cnm_quota_info_t *data = NULL;

    if(NULL == req)
    {
        return CM_PARAM_ERR;
    }

    info = (const cm_cnm_quota_info_t *)req->data;

    if(info->usertype != CM_NAME_USER && info->usertype != CM_NAME_GROUP)
    {
        return CM_PARAM_ERR;
    }

    data = CM_MALLOC(sizeof(cm_cnm_quota_info_t));

    if(NULL == data)
    {
        CM_LOG_ERR(CM_MOD_CNM, "malloc fail");
        return CM_FAIL;
    }

    CM_MEM_CPY(data, sizeof(cm_cnm_quota_info_t), info, sizeof(cm_cnm_quota_info_t));

    if(info->usertype == CM_NAME_USER)
    {
        (void)cm_exec_tmout(data->space, CM_STRING_32, CM_CMT_REQ_TMOUT,
                            "zfs get userquota@%s %s | awk 'NR>1{printf $3}'", info->name, info->filesystem);
        (void)cm_exec_tmout(data->softspace, CM_STRING_32, CM_CMT_REQ_TMOUT,
                            "zfs get softuserquota@%s %s | awk 'NR>1{printf $3}'", info->name, info->filesystem);
        (void)cm_exec_tmout(data->used, CM_STRING_32, CM_CMT_REQ_TMOUT,
                            "zfs get userused@%s %s | awk 'NR>1{printf $3}'", info->name, info->filesystem);
    }

    if(info->usertype == CM_NAME_GROUP)
    {
        (void)cm_exec_tmout(data->space, CM_STRING_32, CM_CMT_REQ_TMOUT,
                            "zfs get groupquota@%s %s | awk 'NR>1{printf $3}'", info->name, info->filesystem);
        (void)cm_exec_tmout(data->softspace, CM_STRING_32, CM_CMT_REQ_TMOUT,
                            "zfs get softgroupquota@%s %s | awk 'NR>1{printf $3}'", info->name, info->filesystem);
        (void)cm_exec_tmout(data->used, CM_STRING_32, CM_CMT_REQ_TMOUT,
                            "zfs get groupused@%s %s | awk 'NR>1{printf $3}'", info->name, info->filesystem);
    }

    *ppAck = data;
    *pAckLen = sizeof(cm_cnm_quota_info_t);
    return CM_OK;
}

static sint32 cm_cnm_quota_get_each(void *arg, sint8 **cols, uint32 col_num)
{
    cm_cnm_quota_info_t* info = arg;

    if(col_num != 5)
    {
        CM_LOG_ERR(CM_MOD_CNM, "col_num %u", col_num);
        return CM_FAIL;
    }

    CM_VSPRINTF(info->filesystem, sizeof(info->filesystem), "%s", cols[0]);
    CM_VSPRINTF(info->name, sizeof(info->name), "%s", cols[1]);
    CM_VSPRINTF(info->space, sizeof(info->space), "%s", cols[2]);
    CM_VSPRINTF(info->softspace, sizeof(info->softspace), "%s", cols[3]);
    CM_VSPRINTF(info->used, sizeof(info->used), "%s", cols[4]);
    info->domain = CM_DOMAIN_LOCAL;
    return CM_OK;
}

sint32 cm_cnm_quota_local_getbatch(
    void *param, uint32 len,
    uint64 offset, uint32 total,
    void **ppAck, uint32 *pAckLen)
{
    const cm_cnm_decode_info_t *decode = param;
    const cm_cnm_quota_info_t *info = (const cm_cnm_quota_info_t *)decode->data;
    sint32 iRet = CM_OK;
    sint8 cmd[CM_STRING_256] = {0};

    if(info->usertype != CM_NAME_USER && info->usertype != CM_NAME_GROUP)
    {
        return CM_PARAM_ERR;
    }

    CM_VSPRINTF(cmd, CM_STRING_256, CM_SCRIPT_DIR
        "cm_cnm_quota.sh getbatch '%u' '%s' '%u' '%llu' '%u'", 
        info->usertype, info->filesystem,info->domain,offset,total);
    iRet = cm_cnm_exec_get_list(cmd, cm_cnm_quota_get_each, 0, sizeof(cm_cnm_quota_info_t), ppAck, &total);

    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM, "iRet[%d]", iRet);
        return iRet;
    }

    *pAckLen = total * sizeof(cm_cnm_quota_info_t);
    return CM_OK;
}

sint32 cm_cnm_quota_local_update(
    void *param, uint32 len,
    uint64 offset, uint32 total,
    void **ppAck, uint32 *pAckLen)
{
    cm_cnm_decode_info_t* decode = param;
    const cm_cnm_quota_info_t *info = (const cm_cnm_quota_info_t*)decode->data;
    uint32 cut = 0;
    const sint8* pquota="null";
    const sint8* psoftquota="null";

    if(info->usertype != CM_NAME_USER && info->usertype != CM_NAME_GROUP)
    {
        return CM_PARAM_ERR;
    }

    if(!CM_OMI_FIELDS_FLAG_ISSET(&decode->set, CM_OMI_FIELD_QUOTA_HARDSPACE) && !CM_OMI_FIELDS_FLAG_ISSET(&decode->set, CM_OMI_FIELD_QUOTA_SOFTSPACE))
    {
        return CM_PARAM_ERR;
    }

    if(info->space[0] != '\0')
    {
        pquota=info->space;
    }

    if(info->softspace[0] != '\0')
    {
        psoftquota=info->softspace;
    }

    return cm_system(CM_SCRIPT_DIR"cm_cnm_quota.sh update '%u' '%s' '%s' '%s' '%s' '%u'", 
        info->usertype, info->name, info->filesystem, pquota, psoftquota,info->domain);
}

sint32 cm_cnm_quota_local_delete(
    void *param, uint32 len,
    uint64 offset, uint32 total,
    void **ppAck, uint32 *pAckLen)
{
    cm_cnm_decode_info_t* decode = param;
    const cm_cnm_quota_info_t *info = (const cm_cnm_quota_info_t*)decode->data;

    if(!CM_OMI_FIELDS_FLAG_ISSET(&decode->set, CM_OMI_FIELD_QUOTA_USERTYPE))
    {
        return CM_PARAM_ERR;
    }

    if(!CM_OMI_FIELDS_FLAG_ISSET(&decode->set, CM_OMI_FIELD_QUOTA_NAME))
    {
        return CM_PARAM_ERR;
    }

    if(!CM_OMI_FIELDS_FLAG_ISSET(&decode->set, CM_OMI_FIELD_QUOTA_FILESYSTEM))
    {
        return CM_PARAM_ERR;
    }

    return cm_system(CM_SCRIPT_DIR"cm_cnm_quota.sh delete '%u' '%s' '%s' '%u'", 
        info->usertype, info->name, info->filesystem, info->domain);
}

sint32 cm_cnm_quota_local_count(
    void *param, uint32 len,
    uint64 offset, uint32 total,
    void **ppAck, uint32 *pAckLen)
{
    cm_cnm_decode_info_t* decode = param;
    const cm_cnm_quota_info_t *info = (const cm_cnm_quota_info_t*)decode->data;
    uint64 cut = 0;
    cut = cm_exec_int(CM_SCRIPT_DIR"cm_cnm_quota.sh count '%u' '%s' '%u'", 
        info->usertype, info->filesystem,info->domain);

    return  cm_cnm_ack_uint64(cut, ppAck, pAckLen);
}

static void cm_cnm_quota_oplog_report(
    const sint8* sessionid, const void *pDecodeParam, uint32 alarmid)
{
    const cm_cnm_decode_info_t *req = pDecodeParam;
    const uint32 cnt = 6;

    if(NULL == pDecodeParam)
    {   
        cm_cnm_oplog_report(sessionid,alarmid,NULL,cnt,NULL);
    }    
    else    
    {       
        const cm_cnm_quota_info_t *info = (const cm_cnm_quota_info_t*)req->data;
        const sint8* nodename = cm_node_get_name(info->nid);
        cm_cnm_oplog_param_t params[6] = {
            {CM_OMI_DATA_INT,CM_OMI_FIELD_QUOTA_USERTYPE,sizeof(info->usertype),&info->usertype},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_QUOTA_NID,strlen(nodename),nodename},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_QUOTA_NAME,strlen(info->name),info->name},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_QUOTA_FILESYSTEM,strlen(info->filesystem),info->filesystem},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_QUOTA_HARDSPACE,strlen(info->space),info->space},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_QUOTA_SOFTSPACE,strlen(info->softspace),info->softspace},
        };
        
        cm_cnm_oplog_report(sessionid,alarmid,params,cnt,&req->set);
    }    
    
    return;
}    

void cm_cnm_quota_oplog_update(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK)? CM_ALARM_LOG_QUOTA_UPDATE_OK : CM_ALARM_LOG_QUOTA_UPDATE_FAIL;
    
    cm_cnm_quota_oplog_report(sessionid,pDecodeParam,alarmid);
    return;
}

void cm_cnm_quota_oplog_delete(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK)? CM_ALARM_LOG_QUOTA_DELETE_OK : CM_ALARM_LOG_QUOTA_DELETE_FAIL;
    const cm_cnm_decode_info_t *req = pDecodeParam;
    const uint32 cnt = 4;

    if(NULL == pDecodeParam)
    {   
        cm_cnm_oplog_report(sessionid,alarmid,NULL,cnt,NULL);
    }    
    else    
    {       
        const cm_cnm_quota_info_t *info = (const cm_cnm_quota_info_t*)req->data;
        const sint8* nodename = cm_node_get_name(info->nid);
        cm_cnm_oplog_param_t params[4] = {
            {CM_OMI_DATA_INT,CM_OMI_FIELD_QUOTA_USERTYPE,sizeof(info->usertype),&info->usertype},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_QUOTA_NID,strlen(nodename),nodename},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_QUOTA_NAME,strlen(info->name),info->name},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_QUOTA_FILESYSTEM,strlen(info->filesystem),info->filesystem},
        };
        
        cm_cnm_oplog_report(sessionid,alarmid,params,cnt,&req->set);
    }    
    return; 
}    


/************************************************************************
                        ipshift(ip?¡¥¨°?)
************************************************************************/

#define cm_cnm_ipshift_req(cmd,param,ppAck,plen)\
    cm_cnm_request_comm(CM_OMI_OBJECT_IPSHFIT,cmd,sizeof(cm_cnm_ipshift_info_t),param,ppAck,plen)
const sint8* cm_cnm_ipshift_sh = "/var/cm/script/cm_cnm_ipshift.sh";


cm_cnm_ipshift_init(void)
{
    return CM_OK;
}

sint32 cm_cnm_ipshift_decode_ext(const cm_omi_obj_t ObjParam,void* data,cm_omi_field_flag_t *set)
{
    sint32 iRet = CM_OK;
    cm_cnm_ipshift_info_t* info = data;
    uint32 cut = 0;
    cm_cnm_decode_param_t param_str[] = 
    {
        {CM_OMI_FIELD_IPSHIFT_FILESYSTEM,sizeof(info->filesystem),info->filesystem,NULL},
        {CM_OMI_FIELD_IPSHIFT_ADAPTER,sizeof(info->adapter),info->adapter,NULL},
        {CM_OMI_FIELD_IPSHIFT_IP,sizeof(info->ip),info->ip,NULL},
        {CM_OMI_FIELD_IPSHIFT_NETMASK,sizeof(info->netmask),info->netmask,NULL}
    };

    cm_cnm_decode_param_t param_num[] =
    {
        {CM_OMI_FIELD_IPSHIFT_NID,sizeof(info->nid),&info->nid,NULL},
    };

    iRet = cm_cnm_decode_str(ObjParam,param_str,
        sizeof(param_str)/sizeof(cm_cnm_decode_param_t),set);
    if(CM_OK != iRet)
    {
        return iRet;
    }
    iRet = cm_cnm_decode_num(ObjParam,param_num,
        sizeof(param_num)/sizeof(cm_cnm_decode_param_t),set);

    cut = cm_cnm_exec_count(info->nid,"zfs list -H -t filesystem -o name '%s' 2>/dev/null |wc -l",info->filesystem);
    if(0 == cut)
    {
        return CM_PARAM_ERR;
    }

    if(CM_OK != iRet)
    {
        return iRet;
    }
    return CM_OK;
}

sint32 cm_cnm_ipshift_decode(const cm_omi_obj_t ObjParam,void **ppDecodeParam)
{
    return cm_cnm_decode_comm(ObjParam,sizeof(cm_cnm_ipshift_info_t),cm_cnm_ipshift_decode_ext,ppDecodeParam);
}

static void cm_cnm_ipshift_encode_each(cm_omi_obj_t item,void* eachdata,void* arg)
{
    cm_omi_field_flag_t *field = arg;
    cm_cnm_ipshift_info_t *info = eachdata;
    cm_cnm_map_value_str_t cols_str[] =
    {
        /*{CM_OMI_FIELD_IPSHIFT_FILESYSTEM,info->filesystem},*/
        {CM_OMI_FIELD_IPSHIFT_ADAPTER,info->adapter},
        {CM_OMI_FIELD_IPSHIFT_IP,info->ip},
        {CM_OMI_FIELD_IPSHIFT_NETMASK,info->netmask},
    };


    cm_cnm_encode_str(item,field,cols_str,sizeof(cols_str)/sizeof(cm_cnm_map_value_str_t));
    return;
}

cm_omi_obj_t cm_cnm_ipshift_encode(const void *pDecodeParam,void *pAckData,uint32 AckLen)
{
    return cm_cnm_encode_comm_ext(pDecodeParam,pAckData,AckLen,
        sizeof(cm_cnm_ipshift_info_t),cm_cnm_ipshift_encode_each);
}

sint32 cm_cnm_ipshift_getbatch(const void *pDecodeParam,void **ppAckData,uint32 *pAckLen)
{
    if(NULL == pDecodeParam)
    {
        return CM_PARAM_ERR; 
    }
    return cm_cnm_ipshift_req(CM_OMI_CMD_GET_BATCH,pDecodeParam,ppAckData,pAckLen);
}

sint32 cm_cnm_ipshift_get(const void *pDecodeParam,void **ppAckData,uint32 *pAckLen)
{
    if(NULL == pDecodeParam)
    {
        return CM_PARAM_ERR; 
    }
    return cm_cnm_ipshift_req(CM_OMI_CMD_GET,pDecodeParam,ppAckData,pAckLen);
}

sint32 cm_cnm_ipshift_count(const void *pDecodeParam,void **ppAckData,uint32 *pAckLen)
{
    if(NULL == pDecodeParam)
    {
        return CM_PARAM_ERR; 
    }
    return cm_cnm_ipshift_req(CM_OMI_CMD_COUNT,pDecodeParam,ppAckData,pAckLen);
}

sint32 cm_cnm_ipshift_create(const void *pDecodeParam,void **ppAckData,uint32 *pAckLen)
{
    if(NULL == pDecodeParam)
    {
        return CM_PARAM_ERR; 
    }
    return cm_cnm_ipshift_req(CM_OMI_CMD_ADD,pDecodeParam,ppAckData,pAckLen);
}

sint32 cm_cnm_ipshift_delete(const void *pDecodeParam,void **ppAckData,uint32 *pAckLen)
{
    if(NULL == pDecodeParam)
    {
        return CM_PARAM_ERR; 
    }
    return cm_cnm_ipshift_req(CM_OMI_CMD_DELETE,pDecodeParam,ppAckData,pAckLen);
}

static sint32 cm_cnm_ipshift_get_each(void *arg, sint8 **cols, uint32 col_num)
{
    cm_cnm_ipshift_info_t* info = arg;
    if(col_num != 4)
    {
        CM_LOG_ERR(CM_MOD_CNM,"col_num %u",col_num);
        return CM_FAIL;
    }

    CM_VSPRINTF(info->filesystem,sizeof(info->filesystem),"%s",cols[0]);
    CM_VSPRINTF(info->adapter,sizeof(info->adapter),"%s",cols[1]);
    CM_VSPRINTF(info->ip,sizeof(info->ip),"%s",cols[2]);
    CM_VSPRINTF(info->netmask,sizeof(info->netmask),"%s",cols[3]);

    return CM_OK;
}

sint32 cm_cnm_ipshift_local_getbatch(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    const cm_cnm_decode_info_t *decode = param;
    const cm_cnm_ipshift_info_t *info = (const cm_cnm_ipshift_info_t *)decode->data;
    sint32 iRet = CM_OK;
    sint8 cmd[CM_STRING_256] = {0};

    CM_VSPRINTF(cmd,CM_STRING_256,"%s getbatch %s",cm_cnm_ipshift_sh,info->filesystem);
    iRet = cm_cnm_exec_get_list(cmd,cm_cnm_ipshift_get_each,offset,sizeof(cm_cnm_ipshift_info_t),ppAck,&total);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]",iRet);
        return iRet;
    }

    *pAckLen = total * sizeof(cm_cnm_ipshift_info_t);
    return CM_OK;
}

sint32 cm_cnm_ipshift_local_get(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    const cm_cnm_decode_info_t *decode = param;
    const cm_cnm_ipshift_info_t *info = (const cm_cnm_ipshift_info_t *)decode->data;
    cm_cnm_ipshift_info_t *data = CM_MALLOC(sizeof(cm_cnm_ipshift_info_t));

    if(NULL == data)
    {
        CM_LOG_ERR(CM_MOD_CNM,"malloc fail");
        return CM_FAIL;
    }

    (void)cm_exec_tmout(data->ip,CM_NAME_LEN,CM_CMT_REQ_TMOUT,
        "zfs get all %s | grep 'failover:%s'|awk '{print $3}'|awk -F',' '{printf $1}'",
        info->filesystem,info->adapter);
    (void)cm_exec_tmout(data->netmask,CM_NAME_LEN,CM_CMT_REQ_TMOUT,
        "zfs get all %s | grep 'failover:%s'|awk '{print $3}'|awk -F',' '{printf $2}'",
        info->filesystem,info->adapter);

    *ppAck = data;
    *pAckLen = sizeof(cm_cnm_ipshift_info_t);
    return CM_OK;
}

sint32 cm_cnm_ipshift_local_create(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    const cm_cnm_decode_info_t *decode = param;
    const cm_cnm_ipshift_info_t *info = (const cm_cnm_ipshift_info_t *)decode->data;
    uint32 num = 0;
    uint32 cut = 0;

    cut = cm_cnm_exec_count(info->nid,"zfs get all %s | grep failover | grep '%s,'| wc -l",info->filesystem,info->ip);
    if(0 != cut)
    {
        return CM_ERR_ALREADY_EXISTS;
    }

    cut = cm_cnm_exec_count(info->nid,"dladm show-link| grep '^%s'|wc -l",info->adapter);
    if(0 == cut)
    {
        return CM_PARAM_ERR;
    }
    cut = cm_system("ping %s 1 2>/dev/null",info->ip);
    if(0 == cut)
    {
        return CM_ERR_IPSHIFT_EXISTS;
    }
        
    //cut = cm_cnm_exec_count(info->nid,"zfs get all %s|grep 'failover:%s:'|wc -l",info->filesystem,info->adapter);
  
    num = cm_cnm_exec_count(info->nid,
    "zfs get all %s|grep 'failover:%s:'|awk '{print $2}'|awk -F':' '{print $3}'|sort -n| awk  'BEGIN {MAX=1}{if($1-MAX==1) MAX=$1} END{print MAX}'",
    info->filesystem,info->adapter);

    return cm_system("%s create %s %s %u %s %s",cm_cnm_ipshift_sh,info->filesystem,info->adapter,(num+1),info->ip,info->netmask);
    
}

sint32 cm_cnm_ipshift_local_delete(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    const cm_cnm_decode_info_t *decode = param;
    const cm_cnm_ipshift_info_t *info = (const cm_cnm_ipshift_info_t *)decode->data;
    uint32 cut = 0;
    uint32 num = 0;

    
    cut = cm_cnm_exec_count(info->nid,"zfs get all %s | grep '%s,'|wc -l",info->filesystem,info->ip);
    if(0 == cut)
    {
        return CM_ERR_NOT_EXISTS;
    }
    
    num = cm_cnm_exec_count(info->nid,
    "zfs get all %s | grep 'failover:%s:' | awk '{print $2}' | awk -F':' 'BEGIN {MAX=0}{if($3>MAX) MAX=$3} END{print MAX}'",
    info->filesystem,info->adapter);

    return cm_system("%s delete %s %s ",cm_cnm_ipshift_sh,info->filesystem,info->ip);
}

sint32 cm_cnm_ipshift_local_count(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    const cm_cnm_decode_info_t *decode = param;
    const cm_cnm_ipshift_info_t *info = (const cm_cnm_ipshift_info_t *)decode->data;
    uint32 cut = 0;

    cut = cm_exec_int("%s count %s",cm_cnm_ipshift_sh,info->filesystem);
    return cm_cnm_ack_uint64(cut,ppAck,pAckLen);
}

static void cm_cnm_ipshift_oplog_report(
    const sint8* sessionid, const void *pDecodeParam, uint32 alarmid)
{
    const cm_cnm_decode_info_t *req = pDecodeParam;
    const uint32 cnt = 5;

    if(NULL == pDecodeParam)
    {   
        cm_cnm_oplog_report(sessionid,alarmid,NULL,cnt,NULL);
    }    
    else    
    {       
        const cm_cnm_ipshift_info_t *info = (const cm_cnm_ipshift_info_t*)req->data;
        const sint8* nodename = cm_node_get_name(info->nid);
        cm_cnm_oplog_param_t params[5] = {
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_IPSHIFT_FILESYSTEM,strlen(info->filesystem),info->filesystem},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_IPSHIFT_NID,strlen(nodename),nodename},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_IPSHIFT_ADAPTER,strlen(info->adapter),info->adapter},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_IPSHIFT_IP,strlen(info->ip),info->ip},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_IPSHIFT_NETMASK,strlen(info->netmask),info->netmask},
        };
        
        cm_cnm_oplog_report(sessionid,alarmid,params,cnt,&req->set);
    }    
    
    return;
}    

void cm_cnm_ipshift_oplog_create(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK)? CM_ALARM_LOG_IPSHIFT_CREATE_OK : CM_ALARM_LOG_IPSHIFT_CREATE_FAIL;
    
    cm_cnm_ipshift_oplog_report(sessionid,pDecodeParam,alarmid);
    return;
}

void cm_cnm_ipshift_oplog_delete(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK)? CM_ALARM_LOG_IPSHIFT_DELETE_OK : CM_ALARM_LOG_IPSHIFT_DELETE_FAIL;
    const cm_cnm_decode_info_t *req = pDecodeParam;
    const uint32 cnt = 3;

    if(NULL == pDecodeParam)
    {   
        cm_cnm_oplog_report(sessionid,alarmid,NULL,cnt,NULL);
    }    
    else    
    {       
        const cm_cnm_ipshift_info_t *info = (const cm_cnm_ipshift_info_t*)req->data;
        const sint8* nodename = cm_node_get_name(info->nid);
        cm_cnm_oplog_param_t params[3] = {
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_IPSHIFT_FILESYSTEM,strlen(info->filesystem),info->filesystem},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_IPSHIFT_NID,strlen(nodename),nodename},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_IPSHIFT_IP,strlen(info->ip),info->ip},
        };
        
        cm_cnm_oplog_report(sessionid,alarmid,params,cnt,&req->set);
    }    
    return; 
}   


