/******************************************************************************
 *          Copyright (c) 2017 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cnm_node.c
 * author     : wbn
 * create date: 2017年11月13日
 * description: TODO:
 *
 *****************************************************************************/
#include "cm_cnm_node.h"
#include "cm_node.h"
#include "cm_omi.h"
#include "cm_log.h"
#include "cm_cnm_common.h"

typedef struct
{
    uint32 node_id;
    cm_omi_field_flag_t field_flag;
    uint32 offset;
    uint32 total;
    sint8 ipaddr[CM_IP_LEN];
    uint32 sbbid;
    sint8 ipmi_user[CM_NAME_LEN];
    sint8 ipmi_passwd[CM_NAME_LEN];
} cm_cnm_node_decode_t;

typedef struct
{
    uint32 node_id;
    uint32 offset;
    cm_node_info_t *pinfo;
    uint32 tmp_index;
    uint32 get_count;
    uint32 max;
} cm_cnm_node_search_t;

static sint32 cm_cnm_node_subdomain_each(
    cm_subdomain_info_t *pinfo, uint32 *pSum)
{
    *pSum += pinfo->node_cnt;
    return CM_OK;
}

static uint32 cm_cnm_get_node_count(void)
{
    uint32 cnt = 0;

    (void)cm_node_traversal_subdomain(
        (cm_node_trav_func_t)cm_cnm_node_subdomain_each,&cnt, CM_FALSE);
    return cnt;
}

sint32 cm_cnm_node_init(void)
{
    return CM_OK;
}

sint32 cm_cnm_node_decode(const cm_omi_obj_t ObjParam, void
**ppDecodeParam)
{
    cm_cnm_node_decode_t *pDecode = CM_MALLOC(sizeof(cm_cnm_node_decode_t));

    if(NULL == pDecode)
    {
        CM_LOG_ERR(CM_MOD_CNM,"malloc fail");
        return CM_FAIL;
    }
    CM_MEM_ZERO(pDecode, sizeof(cm_cnm_node_decode_t));
    pDecode->node_id = 0;
    pDecode->offset = 0;
    pDecode->total = CM_CNM_MAX_RECORD;
    *ppDecodeParam = pDecode;

    cm_omi_decode_fields_flag(ObjParam, &pDecode->field_flag);

    (void)cm_omi_obj_key_get_u32_ex(ObjParam, CM_OMI_FIELD_NODE_ID, &pDecode->node_id);
    (void)cm_omi_obj_key_get_u32_ex(ObjParam, CM_OMI_FIELD_FROM, &pDecode->offset);
    (void)cm_omi_obj_key_get_u32_ex(ObjParam, CM_OMI_FIELD_TOTAL, &pDecode->total);
    (void)cm_omi_obj_key_get_u32_ex(ObjParam, CM_OMI_FIELD_TOTAL, &pDecode->total);
    (void)cm_omi_obj_key_get_u32_ex(ObjParam, CM_OMI_FIELD_NODE_SBBID, &pDecode->sbbid);
    (void)cm_omi_obj_key_get_str_ex(ObjParam, CM_OMI_FIELD_NODE_IP, &pDecode->ipaddr, sizeof(pDecode->ipaddr));
    (void)cm_omi_obj_key_get_str_ex(ObjParam, CM_OMI_FIELD_NODE_IPMI_USER, &pDecode->ipmi_user, sizeof(pDecode->ipmi_user));
    (void)cm_omi_obj_key_get_str_ex(ObjParam, CM_OMI_FIELD_NODE_IPMI_PASSWD, &pDecode->ipmi_passwd, sizeof(pDecode->ipmi_passwd));
    return CM_OK;
}

cm_omi_obj_t cm_cnm_node_encode(const void *pDecodeParam, void *pAckData, uint32 AckLen)
{
    cm_omi_obj_t items = NULL;
    cm_omi_obj_t item = NULL;
    cm_node_info_t *pData = (cm_node_info_t*)pAckData;
    uint32 count = AckLen/sizeof(cm_node_info_t);
    const cm_cnm_node_decode_t *pDecode = pDecodeParam;
    const cm_omi_field_flag_t *pFlag = NULL;
    cm_omi_field_flag_t defa;
    
    items = cm_omi_obj_new_array();
    if(NULL == items)
    {
        CM_LOG_ERR(CM_MOD_CNM,"new items fail");
        return NULL;
    }
    if(NULL == pDecode)
    {
        CM_OMI_FIELDS_FLAG_SET_ALL(&defa);
        pFlag = &defa;
    }
    else
    {
        pFlag = &pDecode->field_flag;
    }
    while(count > 0)
    {
        item = cm_omi_obj_new();
        if(NULL == item)
        {
            CM_LOG_ERR(CM_MOD_CNM,"new item fail");
            return items;
        }
        if(CM_OMI_FIELDS_FLAG_ISSET(pFlag,CM_OMI_FIELD_NODE_ID))
        {
            (void)cm_omi_obj_key_set_u32_ex(item,CM_OMI_FIELD_NODE_ID,pData->id);
        }
        if(CM_OMI_FIELDS_FLAG_ISSET(pFlag,CM_OMI_FIELD_NODE_NAME))
        {
            (void)cm_omi_obj_key_set_str_ex(item,CM_OMI_FIELD_NODE_NAME,pData->name);
        }
        if(CM_OMI_FIELDS_FLAG_ISSET(pFlag,CM_OMI_FIELD_NODE_VERSION))
        {
            (void)cm_omi_obj_key_set_str_ex(item,CM_OMI_FIELD_NODE_VERSION,pData->version);
        }
        if(CM_OMI_FIELDS_FLAG_ISSET(pFlag,CM_OMI_FIELD_NODE_SN))
        {
            (void)cm_omi_obj_key_set_str_ex(item,CM_OMI_FIELD_NODE_SN,pData->sn);
        }
        if(CM_OMI_FIELDS_FLAG_ISSET(pFlag,CM_OMI_FIELD_NODE_IP))
        {
            (void)cm_omi_obj_key_set_str_ex(item,CM_OMI_FIELD_NODE_IP,pData->ip_addr);
        }
        if(CM_OMI_FIELDS_FLAG_ISSET(pFlag,CM_OMI_FIELD_NODE_FRAME))
        {
            (void)cm_omi_obj_key_set_str_ex(item,CM_OMI_FIELD_NODE_FRAME,pData->frame);
        }

        if(CM_OMI_FIELDS_FLAG_ISSET(pFlag,CM_OMI_FIELD_NODE_DEV_TYPE))
        {
            (void)cm_omi_obj_key_set_u32_ex(item,CM_OMI_FIELD_NODE_DEV_TYPE,pData->dev_type);
        }
        if(CM_OMI_FIELDS_FLAG_ISSET(pFlag,CM_OMI_FIELD_NODE_SLOT))
        {
            (void)cm_omi_obj_key_set_u32_ex(item,CM_OMI_FIELD_NODE_SLOT,pData->slot_num);
        }
        if(CM_OMI_FIELDS_FLAG_ISSET(pFlag,CM_OMI_FIELD_NODE_RAM))
        {
            (void)cm_omi_obj_key_set_u32_ex(item,CM_OMI_FIELD_NODE_RAM,pData->ram_size);
        }
        if(CM_OMI_FIELDS_FLAG_ISSET(pFlag,CM_OMI_FIELD_NODE_STATE))
        {
            (void)cm_omi_obj_key_set_u32_ex(item,CM_OMI_FIELD_NODE_STATE,pData->state);
        }
        if(CM_OMI_FIELDS_FLAG_ISSET(pFlag,CM_OMI_FIELD_NODE_SBBID))
        {
            (void)cm_omi_obj_key_set_u32_ex(item,CM_OMI_FIELD_NODE_SBBID,pData->sbbid*1000);
        }
        if(CM_OK != cm_omi_obj_array_add(items,item))
        {
            CM_LOG_ERR(CM_MOD_CNM,"add item[id:%s] fail",pData->id);
            cm_omi_obj_delete(item);
        }
        count--;
        pData++;
    }
    
    return items;
}

static sint32 cm_cnm_node_get_byid(
    cm_node_info_t *pinfo,cm_cnm_node_search_t *pSearch)
{
    if(pinfo->id != pSearch->node_id)
    {
        return CM_OK;
    }
    
    CM_MEM_CPY(pSearch->pinfo,sizeof(cm_node_info_t),pinfo,sizeof(cm_node_info_t));
    pSearch->get_count++;
    
    /*返回失败让跳出循环*/
    return CM_FAIL;
}

sint32 cm_cnm_node_get(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    const cm_cnm_node_decode_t *pDecode = pDecodeParam;
    cm_cnm_node_search_t search;

    if(NULL == pDecodeParam)
    {
        return CM_PARAM_ERR;
    }
    
    search.get_count = 0;
    search.max = 1;
    search.tmp_index = 0;
    search.node_id = pDecode->node_id;
    search.pinfo = CM_MALLOC(sizeof(cm_node_info_t));

    if(NULL == search.pinfo)
    {
        CM_LOG_ERR(CM_MOD_CNM,"malloc fail");
        return CM_FAIL;
    }
    (void)cm_node_traversal_all(
        (cm_node_trav_func_t)cm_cnm_node_get_byid,&search, CM_TRUE);
    if(0 == search.get_count)
    {
        CM_FREE(search.pinfo);
        return CM_FAIL;
    }
    *ppAckData = search.pinfo;
    *pAckLen = sizeof(cm_node_info_t);
    return CM_OK;
}

static sint32 cm_cnm_node_get_search(
    cm_node_info_t *pinfo,cm_cnm_node_search_t *pSearch)
{
    if(pSearch->get_count >= pSearch->max)
    {
        return CM_FAIL;
    }
    pSearch->tmp_index++;
    
    if(pSearch->tmp_index <= pSearch->offset)
    {
        return CM_OK;
    }
    if('\0' == pinfo->version[0])
    {
        /*  重新尝试获取一下节点信息 */
        cm_node_update_info_each(pinfo,NULL);
    }
    CM_MEM_CPY(pSearch->pinfo + pSearch->get_count,
        sizeof(cm_node_info_t),pinfo,sizeof(cm_node_info_t));
    pSearch->get_count++;
    return CM_OK;
}

sint32 cm_cnm_node_get_batch(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    const cm_cnm_node_decode_t *pDecode = pDecodeParam;
    cm_cnm_node_search_t search;
    uint32 cnt = cm_cnm_get_node_count();
    
    if(0 == cnt)
    {
        return CM_OK;
    }
    CM_MEM_ZERO(&search,sizeof(search));
    if(NULL == pDecode)
    {
        search.max = cnt;
    }
    else if(pDecode->offset >= cnt)
    {
        return CM_OK;
    }
    else
    {
        search.offset = pDecode->offset;
        search.max = cnt - search.offset;
        if(pDecode->total > 0)
        {
            search.max = CM_MIN(search.max,pDecode->total);
        }
    }

    search.pinfo = CM_MALLOC(sizeof(cm_node_info_t) * search.max);
    
    if(NULL == search.pinfo)
    {
        CM_LOG_ERR(CM_MOD_CNM,"malloc fail");
        return CM_FAIL;
    }
    (void)cm_node_traversal_all(
        (cm_node_trav_func_t)cm_cnm_node_get_search,&search, CM_TRUE);
    if(0 == search.get_count)
    {
        CM_FREE(search.pinfo);
        return CM_FAIL;
    }
    *ppAckData = search.pinfo;
    *pAckLen = sizeof(cm_node_info_t) * search.get_count;
    return CM_OK;
}

sint32 cm_cnm_node_count(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    uint64 *pCount = CM_MALLOC(sizeof(uint64));

    if(NULL == pCount)
    {
        CM_LOG_ERR(CM_MOD_CNM,"malloc fail");
        return CM_FAIL;
    }
    *pCount = (uint64)cm_cnm_get_node_count();
    *ppAckData = pCount;
    *pAckLen = sizeof(uint64);
    return CM_OK;
}

sint32 cm_cnm_node_modify(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return CM_OK;
}

sint32 cm_cnm_node_add(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    const cm_cnm_node_decode_t *pDecode = pDecodeParam;
    sint32 iRet = CM_OK;
    uint64 cnt = 0;
    if(NULL == pDecode)
    {
        return CM_PARAM_ERR;
    }
    if(strlen(pDecode->ipaddr) == 0)
    {
        return CM_PARAM_ERR;
    }
    cnt = cm_cnm_get_node_count();
    iRet = cm_node_add(pDecode->ipaddr,pDecode->sbbid);
    /* add for 00006213 */
    if((CM_OK == iRet) && (cnt > 0))
    {
        /* 添加其他节点的时候睡眠一下，防止其他节点没有添加完成又添加新节点 */
        CM_LOG_WARNING(CM_MOD_CNM,"ip:%s sleep 8s",pDecode->ipaddr);
        CM_SLEEP_S(8);
    }
    return iRet;
}

sint32 cm_cnm_node_delete(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    const cm_cnm_node_decode_t *pDecode = pDecodeParam;

    if(NULL == pDecode)
    {
        return CM_PARAM_ERR;
    }
    if(pDecode->node_id == CM_NODE_ID_NONE)
    {
        return CM_PARAM_ERR;
    }
    return cm_node_delete(pDecode->node_id);
}

sint32 cm_cnm_node_power_on(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    const cm_cnm_node_decode_t *pDecode = pDecodeParam;

    if(NULL == pDecode)
    {
        return CM_PARAM_ERR;
    }

    return cm_node_power_on
           (pDecode->node_id, pDecode->ipmi_user, pDecode->ipmi_passwd);
}


sint32 cm_cnm_node_request(uint32 cmd, uint32 nid)
{
    cm_cnm_req_param_t param;
    CM_MEM_ZERO(&param,sizeof(cm_cnm_req_param_t));

    param.nid = nid;
    param.obj = CM_OMI_OBJECT_NODE;
    param.cmd = cmd;
    return cm_cnm_request(&param);
}

sint32 cm_cnm_node_power_off(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    const cm_cnm_node_decode_t *pDecode = pDecodeParam;

    if(NULL == pDecode)
    {
        return CM_PARAM_ERR;
    }

    return cm_cnm_node_request(CM_OMI_CMD_OFF,pDecode->node_id);
}

sint32 cm_cnm_node_reboot(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    const cm_cnm_node_decode_t *pDecode = pDecodeParam;

    if(NULL == pDecode)
    {
        return CM_PARAM_ERR;
    }

    return cm_cnm_node_request(CM_OMI_CMD_REBOOT,pDecode->node_id);
}

/*
 *  暂不支持修改节点
 */
void cm_cnm_node_oplog_modify(const sint8* sessionid,const void *pDecodeParam, sint32 Result)
{
    return;
}


void cm_cnm_node_oplog_add(const sint8* sessionid,const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK)? CM_ALARM_LOG_NODE_ADD_OK : CM_ALARM_LOG_NODE_ADD_FAIL;
    const cm_cnm_node_decode_t *req = pDecodeParam;
    const uint32 cnt = 1;
    cm_omi_field_flag_t set;

    CM_OMI_FIELDS_FLAG_CLR_ALL(&set);
    if(NULL == pDecodeParam)
    {   
        cm_cnm_oplog_report(sessionid,alarmid,NULL,cnt,NULL);
    }    
    else    
    {       
        cm_cnm_oplog_param_t params[1] = {
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_NODE_IP,strlen(req->ipaddr),req->ipaddr},
        };
        CM_OMI_FIELDS_FLAG_SET(&set, CM_OMI_FIELD_NODE_IP);
        cm_cnm_oplog_report(sessionid,alarmid,params,cnt,&set);
    }   
    return; 
}

void cm_cnm_node_oplog_delete(const sint8* sessionid,const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK)? CM_ALARM_LOG_NODE_DELETE_OK : CM_ALARM_LOG_NODE_DELETE_FAIL;
    const cm_cnm_node_decode_t *req = pDecodeParam;
    const uint32 cnt = 1;
    cm_omi_field_flag_t set;

    CM_OMI_FIELDS_FLAG_CLR_ALL(&set);
    if(NULL == pDecodeParam)
    {   
        cm_cnm_oplog_report(sessionid,alarmid,NULL,cnt,NULL);
    }    
    else    
    {   
/*   
 *      const sint8* nodename = cm_node_get_name(req->node_id);
 *      节点树的该节点信息已经被删除了
 */
        cm_cnm_oplog_param_t params[1] = {
            {CM_OMI_DATA_INT,CM_OMI_FIELD_NODE_ID,sizeof(req->node_id), &req->node_id},
        };
        CM_OMI_FIELDS_FLAG_SET(&set, CM_OMI_FIELD_NODE_ID);
        cm_cnm_oplog_report(sessionid,alarmid,params,cnt,&set);
    }   
    return;
}

void cm_cnm_node_oplog_on(const sint8* sessionid,const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK)? CM_ALARM_LOG_NODE_ON_OK : CM_ALARM_LOG_NODE_ON_FAIL;
    const cm_cnm_node_decode_t *req = pDecodeParam;
    const uint32 cnt = 2;
    cm_omi_field_flag_t set;

    CM_OMI_FIELDS_FLAG_CLR_ALL(&set);
    if(NULL == pDecodeParam)
    {   
        cm_cnm_oplog_report(sessionid,alarmid,NULL,cnt,NULL);
    }    
    else    
    {       
        const sint8* nodename = cm_node_get_name(req->node_id);
        cm_cnm_oplog_param_t params[2] = {
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_NODE_ID,strlen(nodename),nodename},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_NODE_IPMI_USER,strlen(req->ipmi_user),req->ipmi_user},
        };
        CM_OMI_FIELDS_FLAG_SET(&set, CM_OMI_FIELD_NODE_ID);
        CM_OMI_FIELDS_FLAG_SET(&set, CM_OMI_FIELD_NODE_IPMI_USER);
        cm_cnm_oplog_report(sessionid,alarmid,params,cnt,&set);
    }   
    return;
}

void cm_cnm_node_oplog_off(const sint8* sessionid,const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK)? CM_ALARM_LOG_NODE_OFF_OK : CM_ALARM_LOG_NODE_OFF_FAIL;
    const cm_cnm_node_decode_t *req = pDecodeParam;
    const uint32 cnt = 1;
    cm_omi_field_flag_t set;

    CM_OMI_FIELDS_FLAG_CLR_ALL(&set);
    if(NULL == pDecodeParam)
    {   
        cm_cnm_oplog_report(sessionid,alarmid,NULL,cnt,NULL);
    }    
    else    
    {       
        const sint8* nodename = cm_node_get_name(req->node_id);
        cm_cnm_oplog_param_t params[1] = {
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_NODE_ID,strlen(nodename),nodename},
        };
        CM_OMI_FIELDS_FLAG_SET(&set, CM_OMI_FIELD_NODE_ID);
        cm_cnm_oplog_report(sessionid,alarmid,params,cnt,&set);
    }   
    return;
}

void cm_cnm_node_oplog_reboot(const sint8* sessionid,const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK)? CM_ALARM_LOG_NODE_REBOOT_OK : CM_ALARM_LOG_NODE_REBOOT_FAIL;
    const cm_cnm_node_decode_t *req = pDecodeParam;
    const uint32 cnt = 2;
    cm_omi_field_flag_t set;

    CM_OMI_FIELDS_FLAG_CLR_ALL(&set);
    if(NULL == pDecodeParam)
    {   
        cm_cnm_oplog_report(sessionid,alarmid,NULL,cnt,NULL);
    }    
    else    
    {       
        const sint8* nodename = cm_node_get_name(req->node_id);
        cm_cnm_oplog_param_t params[1] = {
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_NODE_ID,strlen(nodename),nodename},
        };
        CM_OMI_FIELDS_FLAG_SET(&set, CM_OMI_FIELD_NODE_ID);
        cm_cnm_oplog_report(sessionid,alarmid,params,cnt,&set);
    }   
    return;
}

static void* cm_cnm_node_local_thread(void* arg)
{
    uint32 iloop = 5;
    uint32 cmd = (uint32)arg;

    while(iloop > 0)
    {
        CM_LOG_WARNING(CM_MOD_CNM,"cmd[%u] wait %u seconds",cmd,iloop);
        iloop--;
    }

    switch(cmd)
    {
        case CM_OMI_CMD_OFF:
            (void)cm_node_power_off(cm_node_get_id());
            break;
        case CM_OMI_CMD_REBOOT:
            (void)cm_node_reboot(cm_node_get_id());
            break;
        default:
            break;
    }

    return NULL;
}

sint32 cm_cnm_node_local_off(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    CM_LOG_WARNING(CM_MOD_CNM,"shutdown");
    
    return cm_thread_start(cm_cnm_node_local_thread,(void*)CM_OMI_CMD_OFF);
}

sint32 cm_cnm_node_local_reboot(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    CM_LOG_WARNING(CM_MOD_CNM,"reboot");
    return cm_thread_start(cm_cnm_node_local_thread,(void*)CM_OMI_CMD_REBOOT);
}
    

/*xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx*/
 static const sint8 *g_cm_cnm_node_service_script = "/var/cm/script/cm_cnm_node_servce.sh";

static sint32 cm_cnm_service_decode_check_set_status(void *val)
{
    uint32 status = *((uint32*)val);
    if((0 != status) && (1 != status))
    {
        CM_LOG_ERR(CM_MOD_CNM,"status[%u]",status);
        return CM_PARAM_ERR;
    }
    return CM_OK;
}

static sint32 cm_cnm_node_service_decode_ext(
    const cm_omi_obj_t ObjParam,void* data,cm_omi_field_flag_t* set)
{
    sint32 iRet = CM_OK;
    cm_cnm_node_service_info_t *info = data;
    cm_cnm_decode_param_t param_num[] =
    {
        {CM_OMI_FIELD_NODE_SERVCE_NID,sizeof(info->nid),&info->nid,NULL},
        {CM_OMI_FIELD_NODE_SERVCE_NFS,sizeof(info->nfs),&info->nfs,cm_cnm_service_decode_check_set_status},
        {CM_OMI_FIELD_NODE_SERVCE_STMF,sizeof(info->stmf),&info->stmf,cm_cnm_service_decode_check_set_status},   
        {CM_OMI_FIELD_NODE_SERVCE_SSH,sizeof(info->ssh),&info->ssh,cm_cnm_service_decode_check_set_status},      
        {CM_OMI_FIELD_NODE_SERVCE_FTP,sizeof(info->ftp),&info->ftp,cm_cnm_service_decode_check_set_status},
        {CM_OMI_FIELD_NODE_SERVCE_SMB,sizeof(info->smb),&info->smb,cm_cnm_service_decode_check_set_status},
        {CM_OMI_FIELD_NODE_SERVCE_GUIVIEW,sizeof(info->guiview),&info->guiview,cm_cnm_service_decode_check_set_status},
        {CM_OMI_FIELD_NODE_SERVCE_FMD,sizeof(info->fmd),&info->fmd,cm_cnm_service_decode_check_set_status},
        {CM_OMI_FIELD_NODE_SERVCE_ISCSI,sizeof(info->iscsi),&info->iscsi,cm_cnm_service_decode_check_set_status},  
    };

    
    iRet = cm_cnm_decode_num(ObjParam,param_num,sizeof(param_num)/sizeof(cm_cnm_decode_param_t),set);
    if(iRet != CM_OK)
    {
        return iRet;
    }
    return CM_OK;
}


sint32 cm_cnm_node_service_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam)
{
    return cm_cnm_decode_comm(ObjParam,sizeof(cm_cnm_node_service_info_t),cm_cnm_node_service_decode_ext,ppDecodeParam);
}

static void cm_cnm_node_service_encode_each(cm_omi_obj_t item,void* eachdata,void* arg)
{
    cm_omi_field_flag_t *field = arg;
    cm_cnm_node_service_info_t *info = eachdata;
    cm_cnm_map_value_num_t cols_num[] = 
    {  
        {CM_OMI_FIELD_NODE_SERVCE_NID,info->nid},
        {CM_OMI_FIELD_NODE_SERVCE_NFS,info->nfs},
        {CM_OMI_FIELD_NODE_SERVCE_STMF,info->stmf},   
        {CM_OMI_FIELD_NODE_SERVCE_SSH,info->ssh},      
        {CM_OMI_FIELD_NODE_SERVCE_FTP,info->ftp},
        {CM_OMI_FIELD_NODE_SERVCE_SMB,info->smb},
        {CM_OMI_FIELD_NODE_SERVCE_GUIVIEW,info->guiview},
        {CM_OMI_FIELD_NODE_SERVCE_FMD,info->fmd},
        {CM_OMI_FIELD_NODE_SERVCE_ISCSI,info->iscsi},  
    };
    cm_cnm_encode_num(item,field,cols_num,sizeof(cols_num)/sizeof(cm_cnm_map_value_str_t));
    return;
}

cm_omi_obj_t cm_cnm_node_service_encode(const void *pDecodeParam,void *pAckData,uint32 AckLen)
{
    return cm_cnm_encode_comm_ext(pDecodeParam,pAckData,AckLen,sizeof(cm_cnm_node_service_info_t),cm_cnm_node_service_encode_each);
}


sint32 cm_cnm_node_service_count(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_cnm_node_service_req(CM_OMI_CMD_COUNT,pDecodeParam,ppAckData,pAckLen);
}


sint32 cm_cnm_node_service_getbatch(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_cnm_node_service_req(CM_OMI_CMD_GET_BATCH,pDecodeParam,ppAckData,pAckLen);
}
  
sint32 cm_cnm_node_service_updata(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_cnm_node_service_req(CM_OMI_CMD_MODIFY,pDecodeParam,ppAckData,pAckLen);
}



static sint32 cm_cnm_node_service_local_get_each(void *arg, sint8 **cols, uint32 col_num)
{
    cm_cnm_node_service_info_t *info = arg;
    const uint32 def_num = 8;   
    if(def_num != col_num)
    {
        CM_LOG_WARNING(CM_MOD_CNM,"col_num[%u] def_num[%u]",col_num,def_num);
        return CM_FAIL;
    }
    CM_MEM_ZERO(info,sizeof(cm_cnm_node_service_info_t));
    info->nid=cm_node_get_id();
    info->nfs=atoi(cols[0]);
    info->stmf=atoi(cols[1]);
    info->ssh=atoi(cols[2]);
    info->ftp=atoi(cols[3]);
    info->smb=atoi(cols[4]);
    info->guiview=atoi(cols[5]);
    info->fmd=atoi(cols[6]);
    info->iscsi=atoi(cols[7]);
    
    return CM_OK;
}

sint32 cm_cnm_node_service_local_getbatch(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    sint32 iRet = CM_OK;
    sint8 cmd[128]={0};
    CM_LOG_INFO(CM_MOD_CNM,"cm_cnm_node_service_local_getbatch");
    CM_VSPRINTF(cmd,sizeof(cmd),"%s getbatch",g_cm_cnm_node_service_script);
    iRet = cm_cnm_exec_get_list(cmd,cm_cnm_node_service_local_get_each,offset,sizeof(cm_cnm_node_service_info_t),ppAck,&total);

    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"zfs clustersan list-target fail iRet[%d]",iRet);
        return iRet;
    }
    *pAckLen = total * sizeof(cm_cnm_node_service_info_t);
    return CM_OK;
}

sint32 cm_cnm_node_service_local_count(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    return cm_cnm_ack_uint64(1,ppAck,pAckLen);

}

#define cm_cnm_node_service_get_index(obj) ((obj)-2)
    


static sint32 cm_cnm_node_service_local_set(
    const cm_omi_field_flag_t *set,const cm_cnm_node_service_info_t *info)
{
    uint32 failnum = 0;    
    uint32 iRet=CM_OK;
    if(CM_OMI_FIELDS_FLAG_ISSET(set,CM_OMI_FIELD_NODE_SERVCE_NFS))    
    {   
        iRet = cm_system("%s update %d %d",g_cm_cnm_node_service_script,
            cm_cnm_node_service_get_index(CM_OMI_FIELD_NODE_SERVCE_NFS),info->nfs);        
        if(CM_OK != iRet)        
        {            
                CM_LOG_ERR(CM_MOD_CNM,"%s update nfs fail",g_cm_cnm_node_service_script);               
                failnum++;        

        }    

    }
    if(CM_OMI_FIELDS_FLAG_ISSET(set,CM_OMI_FIELD_NODE_SERVCE_STMF))    
    {   
        iRet = cm_system("%s update %d %d",g_cm_cnm_node_service_script,
            cm_cnm_node_service_get_index(CM_OMI_FIELD_NODE_SERVCE_STMF),info->stmf);        
        if(CM_OK != iRet)        
        {            
                CM_LOG_ERR(CM_MOD_CNM,"%s update stmf fail",g_cm_cnm_node_service_script);               
                failnum++;        

        }    

    }
    if(CM_OMI_FIELDS_FLAG_ISSET(set,CM_OMI_FIELD_NODE_SERVCE_SSH))    
    {   
        iRet = cm_system("%s update %d %d",g_cm_cnm_node_service_script,
            cm_cnm_node_service_get_index(CM_OMI_FIELD_NODE_SERVCE_SSH),info->ssh);        
        if(CM_OK != iRet)        
        {            
                CM_LOG_ERR(CM_MOD_CNM,"%s update ssh fail",g_cm_cnm_node_service_script);               
                failnum++;        

        }    

    }
    if(CM_OMI_FIELDS_FLAG_ISSET(set,CM_OMI_FIELD_NODE_SERVCE_FTP))    
    {   
        iRet = cm_system("%s update %d %d",g_cm_cnm_node_service_script,
            cm_cnm_node_service_get_index(CM_OMI_FIELD_NODE_SERVCE_FTP),info->ftp);        
        if(CM_OK != iRet)        
        {            
                CM_LOG_ERR(CM_MOD_CNM,"%s update ftp fail",g_cm_cnm_node_service_script);               
                failnum++;        

        }    

    }
    
    if(CM_OMI_FIELDS_FLAG_ISSET(set,CM_OMI_FIELD_NODE_SERVCE_SMB))    
    {   
        iRet = cm_system("%s update %d %d",g_cm_cnm_node_service_script,
            cm_cnm_node_service_get_index(CM_OMI_FIELD_NODE_SERVCE_SMB),info->smb);        
        if(CM_OK != iRet)        
        {            
                CM_LOG_ERR(CM_MOD_CNM,"%s update smb fail",g_cm_cnm_node_service_script);               
                failnum++;        

        }    

    }
    if(CM_OMI_FIELDS_FLAG_ISSET(set,CM_OMI_FIELD_NODE_SERVCE_GUIVIEW))    
    {  
        iRet = cm_system("%s update %d %d",g_cm_cnm_node_service_script,
            cm_cnm_node_service_get_index(CM_OMI_FIELD_NODE_SERVCE_GUIVIEW),info->guiview);        
        if(CM_OK != iRet)        
        {            
                CM_LOG_ERR(CM_MOD_CNM,"%s update guiview fail",g_cm_cnm_node_service_script);               
                failnum++;        

        }    

    }

    if(CM_OMI_FIELDS_FLAG_ISSET(set, CM_OMI_FIELD_NODE_SERVCE_FMD))
    {
        iRet = cm_system("%s update %d %d", g_cm_cnm_node_service_script,
                         cm_cnm_node_service_get_index(CM_OMI_FIELD_NODE_SERVCE_FMD), info->fmd);

        if(CM_OK != iRet)
        {
            CM_LOG_ERR(CM_MOD_CNM, "%s update fmd fail", g_cm_cnm_node_service_script);
            failnum++;

        }

    }

    if(CM_OMI_FIELDS_FLAG_ISSET(set, CM_OMI_FIELD_NODE_SERVCE_ISCSI))
    {
        iRet = cm_system("%s update %d %d", g_cm_cnm_node_service_script,
                         cm_cnm_node_service_get_index(CM_OMI_FIELD_NODE_SERVCE_ISCSI), info->iscsi);

        if(CM_OK != iRet)
        {
            CM_LOG_ERR(CM_MOD_CNM, "%s update iscsi fail", g_cm_cnm_node_service_script);
            failnum++;

        }    

    }
    return (0 == failnum)? CM_OK: CM_FAIL;
}

sint32 cm_cnm_node_service_local_updata(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    const cm_cnm_decode_info_t *req = param;
    if( NULL == req)
   {
       return CM_PARAM_ERR; 
   }
    const cm_cnm_node_service_info_t *info =(const cm_cnm_node_service_info_t *)req->data;
    return cm_cnm_node_service_local_set(&req->set,info);
}

void cm_cnm_node_service_oplog_update(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK)? CM_ALARM_LOG_NODE_SERVICE_UPDATE_OK : CM_ALARM_LOG_NODE_SERVICE_UPDATE_FAIL;
    
    const cm_cnm_decode_info_t *req = pDecodeParam;
    const uint32 cnt = 9;

    /*<nid> [-nfs nfs] [-stmf stmf] [-ssh ssh] [-ftp ftp] [-smb smb] [-gui guiview] [-fmd fmd] [-iscsi iscsi]*/
    if(NULL == pDecodeParam)
    {   
        cm_cnm_oplog_report(sessionid,alarmid,NULL,cnt,NULL);
    }    
    else    
    {       
        const cm_cnm_node_service_info_t *info = (const cm_cnm_node_service_info_t*)req->data;
        const sint8* nodename = cm_node_get_name(info->nid);
        cm_cnm_oplog_param_t params[] = {
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_NODE_SERVCE_NID,strlen(nodename),nodename},
            {CM_OMI_DATA_INT,CM_OMI_FIELD_NODE_SERVCE_NFS,sizeof(info->nfs),&info->nfs},
            {CM_OMI_DATA_INT,CM_OMI_FIELD_NODE_SERVCE_STMF,sizeof(info->stmf),&info->stmf},           
            {CM_OMI_DATA_INT,CM_OMI_FIELD_NODE_SERVCE_SSH,sizeof(info->ssh),&info->ssh},
            {CM_OMI_DATA_INT,CM_OMI_FIELD_NODE_SERVCE_FTP,sizeof(info->ftp),&info->ftp},
            {CM_OMI_DATA_INT,CM_OMI_FIELD_NODE_SERVCE_SMB,sizeof(info->smb),&info->smb},           
            {CM_OMI_DATA_INT,CM_OMI_FIELD_NODE_SERVCE_GUIVIEW,sizeof(info->guiview),&info->guiview},
            {CM_OMI_DATA_INT,CM_OMI_FIELD_NODE_SERVCE_FMD,sizeof(info->fmd),&info->fmd},
            {CM_OMI_DATA_INT,CM_OMI_FIELD_NODE_SERVCE_ISCSI,sizeof(info->iscsi),&info->iscsi},           
        };
        
        cm_cnm_oplog_report(sessionid,alarmid,params,cnt,&req->set);
    }   
    return; 
}


/*      upgrade      */

#define cm_cnm_upgrade_req(cmd,param,ppAck,plen) \
    cm_cnm_request_comm(CM_OMI_OBJECT_UPGRADE,cmd,sizeof(cm_cnm_upgrade_info_t),param,ppAck,plen)
static sint8* cm_cnm_upgrade_sh = "/var/cm/script/cm_cnm_upgrade.sh";

extern const cm_omi_map_enum_t CmOmiEnumUpgradeModeType;
extern const sint8* cm_node_getversion(void);

sint32 cm_cnm_upgrade_init()
{
    return cm_system("%s init",cm_cnm_upgrade_sh);
}

static sint32 cm_cnm_upgrade_decode_ext(
    const cm_omi_obj_t ObjParam,void* data,cm_omi_field_flag_t* set)
{
    sint32 iRet = CM_OK;
    cm_cnm_upgrade_info_t *info = data;

    cm_cnm_decode_param_t param_str[] = 
    {
        {CM_OMI_FIELD_UPGRADE_NAME,sizeof(info->name),info->name,NULL},
        {CM_OMI_FIELD_UPGRADE_RDDIR,sizeof(info->rddir),info->rddir,NULL},    
    };
    
    cm_cnm_decode_param_t param_num[] =
    {
        {CM_OMI_FIELD_UPGRADE_NID,sizeof(info->nid),&info->nid,NULL},
        {CM_OMI_FIELD_UPGRADE_STATE,sizeof(info->state),&info->state,NULL},
        {CM_OMI_FIELD_UPGRADE_PROCESS,sizeof(info->process),&info->process,NULL},    
        {CM_OMI_FIELD_UPGRADE_MODE,sizeof(info->mode),&info->mode,NULL}, 
    };    
    iRet = cm_cnm_decode_num(ObjParam,param_num,sizeof(param_num)/sizeof(cm_cnm_decode_param_t),set);
    if(iRet != CM_OK)
    {
        return iRet;
    }
    iRet = cm_cnm_decode_str(ObjParam,param_str,sizeof(param_str)/sizeof(cm_cnm_decode_param_t),set);
    if(iRet != CM_OK)
    {
        return iRet;
    }
    
    return CM_OK;
}


sint32 cm_cnm_upgrade_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam)
{
    return cm_cnm_decode_comm(ObjParam,sizeof(cm_cnm_upgrade_info_t),cm_cnm_upgrade_decode_ext,ppDecodeParam);
}

static void cm_cnm_upgrade_encode_each(cm_omi_obj_t item,void* eachdata,void* arg)
{
    cm_omi_field_flag_t *field = arg;
    cm_cnm_upgrade_info_t *info = eachdata;
    cm_cnm_map_value_str_t cols_str[] = 
    {
        {CM_OMI_FIELD_UPGRADE_NAME,     info->name},
        {CM_OMI_FIELD_UPGRADE_VERSION,     info->version},
    };

    cm_cnm_map_value_str_t cols_num[] =
    {
        {CM_OMI_FIELD_UPGRADE_PROCESS,     info->process},
        {CM_OMI_FIELD_UPGRADE_STATE,     info->state},
    };

    cm_cnm_encode_str(item,field,cols_str,sizeof(cols_str)/sizeof(cm_cnm_map_value_str_t));
    cm_cnm_encode_num(item,field,cols_num,sizeof(cols_num)/sizeof(cm_cnm_map_value_num_t)); 
    return;
}

cm_omi_obj_t cm_cnm_upgrade_encode(const void *pDecodeParam,void *pAckData,uint32 AckLen)
{
    return cm_cnm_encode_comm_ext(pDecodeParam,pAckData,AckLen,sizeof(cm_cnm_upgrade_info_t),cm_cnm_upgrade_encode_each);
}

static sint32 cm_cnm_upgrade_local_get_each(void *arg, sint8 **cols, uint32 col_num)
{
    cm_cnm_upgrade_info_t *info = arg;
    const uint32 def_num = 3;  
    
    /* index domain nametype name permission */
    if(def_num > col_num)
    {
        CM_LOG_WARNING(CM_MOD_CNM,"col_num[%u] def_num[%u]",col_num,def_num);
        return CM_FAIL;
    }

    CM_VSPRINTF(info->name,sizeof(info->name),"%s",cols[0]);
    info->process = atoi(cols[1]);
    info->state = atoi(cols[2]);
    CM_VSPRINTF(info->version,sizeof(info->version),"%s",cm_node_getversion);
    
    return CM_OK;
}


sint32 cm_cnm_upgrade_getbatch(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    sint32 iRet = CM_OK;
    const cm_cnm_decode_info_t *decode = pDecodeParam;
    sint8 cmd[CM_STRING_128] = {0};
    uint32 total = 0;
    uint32 offset = 0;

    if(decode == NULL)
    {
        total = 100;
    }else
    {
        total = decode->total;
        offset = decode->offset;
    }
    
    CM_VSPRINTF(cmd,CM_STRING_128,"%s getbatch",cm_cnm_upgrade_sh);

    iRet = cm_cnm_exec_get_list(cmd,cm_cnm_upgrade_local_get_each,
        offset,sizeof(cm_cnm_upgrade_info_t),ppAckData,&total);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]",iRet);
        return iRet;
    }
    *pAckLen = total * sizeof(cm_cnm_upgrade_info_t);
    return CM_OK;
}

sint32 cm_cnm_upgrade_count(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    uint32 cut = 0;

    cut = cm_exec_int("%s count",cm_cnm_upgrade_sh);
    
    return cm_cnm_ack_uint64(cut,ppAckData,pAckLen);
}

sint32 cm_cnm_upgrade_insert(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_cnm_upgrade_req(CM_OMI_CMD_ADD,pDecodeParam,ppAckData,pAckLen);
}    

sint32 cm_cnm_upgrade_local_getbatch(
    void *param, uint32 len,
    uint64 offset, uint32 total,  
    void **ppAck, uint32 *pAckLen)
{
    sint32 iRet = CM_OK;
    sint8 cmd[CM_STRING_128] = {0};
    
    CM_VSPRINTF(cmd,CM_STRING_128,"%s getbatch",cm_cnm_upgrade_sh);

    iRet = cm_cnm_exec_get_list(cmd,cm_cnm_upgrade_local_get_each,
        offset,sizeof(cm_cnm_upgrade_info_t),ppAck,&total);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]",iRet);
        return iRet;
    }
    *pAckLen = total * sizeof(cm_cnm_upgrade_info_t);
    return CM_OK;
}

sint32 cm_cnm_upgrade_local_insert(
    void *param, uint32 len,
    uint64 offset, uint32 total,  
    void **ppAck, uint32 *pAckLen)
{
    sint32 iRet = CM_OK;
    cm_cnm_decode_info_t* decode = param;
    cm_cnm_upgrade_info_t* info = (cm_cnm_upgrade_info_t* )decode->data;
    const sint8 *mode = cm_cnm_get_enum_str(&CmOmiEnumUpgradeModeType,info->mode);

    if(!CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_UPGRADE_RDDIR))
    {
        CM_LOG_ERR(CM_MOD_CNM,"not rd dir");
        return CM_FAIL;
    }
    iRet = cm_system("%s insert %s %s",cm_cnm_upgrade_sh,info->rddir,mode);
    if(iRet != CM_OK)
    {
        CM_LOG_ERR(CM_MOD_CNM,"upgrade server fail");
        return CM_FAIL;
    }
    
    return CM_OK;
}


sint32 cm_cnm_upgrade_local_count(
    void *param, uint32 len,
    uint64 offset, uint32 total,  
    void **ppAck, uint32 *pAckLen)
{
    uint32 cut = 0;

    cut = cm_exec_int("%s count",cm_cnm_upgrade_sh);
    
    return cm_cnm_ack_uint64(cut,ppAck,pAckLen);
}


