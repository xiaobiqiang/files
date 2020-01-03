/******************************************************************************
 *          Copyright (c) 2018 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cnm_systime.c
 * author     : wbn
 * create date: 2018Äê3ÔÂ2ÈÕ
 * description: TODO:
 *
 *****************************************************************************/
#include "cm_omi.h"
#include "cm_alarm.h"
#include "cm_log.h"
#include "cm_sync.h"
#include "cm_cnm_common.h"
#include "cm_node.h"
#include "cm_cnm_systime.h"
#include "cm_cfg_omi.h"

#define CM_CNM_NTP_ALARM_ID  10000012

static sint8* cm_cnm_ntp_sh = "/var/cm/script/cm_cnm_ntp.sh";
sint8 cm_cnm_ntp_ip[CM_STRING_128] = {'\0'};
#define CM_CNM_CLUSTER_CFG_NTPSERVER "ntpserver"
#define CM_CNM_CLUSTER_CFG_SECTION "NTP"


typedef struct
{
    uint32 tz;
    cm_time_t datatime;
}cm_cnm_systime_info_t;

typedef struct
{
    cm_cnm_systime_info_t info;
    cm_omi_field_flag_t field;
    cm_omi_field_flag_t set;
}cm_cnm_systime_decode_t;

sint32 cm_cnm_systime_init(void)
{
    return CM_OK;
}

sint32 cm_cnm_systime_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam)
{
    cm_cnm_systime_decode_t *query = CM_MALLOC(sizeof(cm_cnm_systime_decode_t));
    sint32 iRet = CM_OK;
    
    if(NULL == query)
    {
        CM_LOG_ERR(CM_MOD_CNM,"malloc fail");
        return CM_FAIL;
    }
    CM_MEM_ZERO(query,sizeof(cm_cnm_systime_decode_t));    
    cm_omi_decode_fields_flag(ObjParam,&query->field);
    
    iRet = cm_omi_obj_key_get_u32_ex(ObjParam,CM_OMI_FIELD_SYSTIME_TZ,(uint32*)&query->info.tz);
    if(CM_OK == iRet)
    {
        CM_OMI_FIELDS_FLAG_SET(&query->set,CM_OMI_FIELD_SYSTIME_TZ);
    }
    
    iRet = cm_omi_obj_key_get_u32_ex(ObjParam,CM_OMI_FIELD_SYSTIME_DT,(uint32*)&query->info.datatime);
    if(CM_OK == iRet)
    {
        CM_OMI_FIELDS_FLAG_SET(&query->set,CM_OMI_FIELD_SYSTIME_DT);
    }

    *ppDecodeParam = query;
    return CM_OK;
}

cm_omi_obj_t cm_cnm_systime_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen)
{
    cm_cnm_systime_info_t *pinfo = pAckData;
    const cm_cnm_systime_decode_t *query = pDecodeParam;

    cm_omi_obj_t items = NULL;
    cm_omi_obj_t item = NULL;
    
    if(NULL == pinfo)
    {
        return NULL;
    }
    
    items = cm_omi_obj_new_array();
    if(NULL == items)
    {
        CM_LOG_ERR(CM_MOD_CNM,"new items fail");
        return NULL;
    }
    item = cm_omi_obj_new();
    if(NULL == item)
    {
        CM_LOG_ERR(CM_MOD_CNM,"new item fail");
        return items;
    }

    if(NULL == query)
    {
        //(void)cm_omi_obj_key_set_u32_ex(item,CM_OMI_FIELD_SYSTIME_TZ,pinfo->tz);
        (void)cm_omi_obj_key_set_u32_ex(item,CM_OMI_FIELD_SYSTIME_DT,(uint32)pinfo->datatime);
    }
    else
    {
        /*if(CM_OMI_FIELDS_FLAG_ISSET(&query->field, CM_OMI_FIELD_SYSTIME_TZ))
        {
            (void)cm_omi_obj_key_set_u32_ex(item,CM_OMI_FIELD_SYSTIME_TZ,pinfo->tz);
        }*/
        if(CM_OMI_FIELDS_FLAG_ISSET(&query->field, CM_OMI_FIELD_SYSTIME_DT))
        {
            (void)cm_omi_obj_key_set_u32_ex(item,CM_OMI_FIELD_SYSTIME_DT,(uint32)pinfo->datatime);
        }
    }

    if(CM_OK != cm_omi_obj_array_add(items,item))
    {
        CM_LOG_ERR(CM_MOD_CNM,"add item fail");
        cm_omi_obj_delete(item);
    }
    return items;
}

sint32 cm_cnm_systime_set(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    const cm_cnm_systime_decode_t *query = pDecodeParam;

    if(NULL == query)
    {
        CM_LOG_ERR(CM_MOD_CNM,"query NULL");
        return CM_PARAM_ERR;
    }

    return cm_set_time(query->info.datatime);
}

sint32 cm_cnm_systime_get(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    const cm_cnm_systime_decode_t *query = pDecodeParam;
    cm_cnm_systime_info_t *pinfo = CM_MALLOC(sizeof(cm_cnm_systime_info_t));

    if(NULL == pinfo)
    {
        CM_LOG_ERR(CM_MOD_CNM,"malloc fail");
        return CM_FAIL;
    }

    if(NULL == query)
    {
        pinfo->tz = 0; /* TODO: */
        pinfo->datatime = cm_get_time();
    }
    else
    {
        /*if(CM_OMI_FIELDS_FLAG_ISSET(&query->field, CM_OMI_FIELD_SYSTIME_TZ))
        {
            pinfo->tz = 0; 
        }*/
        if(CM_OMI_FIELDS_FLAG_ISSET(&query->field, CM_OMI_FIELD_SYSTIME_DT))
        {
            pinfo->datatime = cm_get_time();
        }
    }

    *ppAckData = pinfo;
    *pAckLen = sizeof(cm_cnm_systime_info_t); 
    
    return CM_OK;
}

sint32 cm_cnm_systime_sync_request(uint64 data_id, void *pdata, uint32 len)
{
    const cm_cnm_systime_decode_t *query = pdata;
    const cm_cnm_systime_info_t *pinfo = &query->info;
    sint32 iRet = CM_OK;
    
    if((NULL == pdata) || (len != sizeof(cm_cnm_systime_decode_t)))
    {
        CM_LOG_ERR(CM_MOD_CNM,"len:%u",len);
        return CM_PARAM_ERR;
    }
    /*if(CM_OMI_FIELDS_FLAG_ISSET(&query->set, CM_OMI_FIELD_SYSTIME_TZ))
    {
        ;
    }*/
    if(CM_OMI_FIELDS_FLAG_ISSET(&query->set, CM_OMI_FIELD_SYSTIME_DT))
    {
        iRet = cm_set_time(pinfo->datatime);
        CM_LOG_WARNING(CM_MOD_CNM,"datatime[%u],iRet[%d]",pinfo->datatime,iRet);
    }

    return iRet;
}

void cm_cnm_systime_oplog_set(const sint8* sessionid,const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK) ? CM_ALARM_LOG_SYSTIME_SET_OK : CM_ALARM_LOG_SYSTIME_SET_FAIL;
    const cm_cnm_systime_decode_t *req = pDecodeParam;
    const uint32 cnt = 2;

    if(NULL == pDecodeParam)
    {   
        cm_cnm_oplog_report(sessionid,alarmid,NULL,cnt,NULL);
    }    
    else    
    {   
        cm_cnm_systime_info_t *info = &req->info;
        cm_cnm_oplog_param_t params[2] = {
            {CM_OMI_DATA_INT,CM_OMI_FIELD_SYSTIME_TZ,sizeof(info->tz), &info->tz},
            {CM_OMI_DATA_TIME,CM_OMI_FIELD_SYSTIME_DT,sizeof(info->datatime), &info->datatime},
        };
        
        cm_cnm_oplog_report(sessionid,alarmid,params,cnt,&req->set);
    }   
    return;
}


static void* cm_cnm_systime_sync_thread(void* arg)
{
    cm_cnm_systime_decode_t req;
    
    CM_MEM_ZERO(&req,sizeof(req));
    CM_OMI_FIELDS_FLAG_SET(&req.set,CM_OMI_FIELD_SYSTIME_DT);
    req.info.datatime = cm_get_time();
    (void)cm_sync_request(CM_SYNC_OBJ_SYSTIME,0,(void*)&req,sizeof(cm_cnm_systime_decode_t));
    return NULL;
}

void cm_cnm_systime_master_change(uint32 old_id,uint32 new_id)
{
    if((new_id == CM_NODE_ID_NONE) || (new_id != cm_node_get_id()))
    {
        return;
    }
    if (cm_cnm_ntp_ip[0] == '\0')
    {
        (void)cm_thread_start(cm_cnm_systime_sync_thread,NULL);
    }
    return;
}


/*********************  ntp  **************************/


sint32 cm_cnm_ntp_init(void)
{   
    sint8 ntpserver[128] = {0};
    sint32 iRet = CM_OK;
    

    iRet = cm_ini_get_ext(CM_CLUSTER_INI, CM_CNM_CLUSTER_CFG_SECTION,
                         CM_CNM_CLUSTER_CFG_NTPSERVER, ntpserver, sizeof(ntpserver));
    if(iRet != CM_OK)
    {
        return CM_OK;
    }else
    {
        CM_VSPRINTF(cm_cnm_ntp_ip,sizeof(cm_cnm_ntp_ip),ntpserver);
    }
    
    
    return CM_OK;
}

static cm_cnm_ntp_decode_ext(const cm_omi_obj_t ObjParam,void* data,cm_omi_field_flag_t* set)
{
    sint32 iRet = CM_OK;
    cm_cnm_ntp_info_t *info = data;
    cm_cnm_decode_param_t param_str[] = 
    {
        {CM_OMI_FIELD_NTP_IP,sizeof(info->ip),info->ip,NULL},
    };

    iRet = cm_cnm_decode_str(ObjParam,param_str,
        sizeof(param_str)/sizeof(cm_cnm_decode_param_t),set);
    if(iRet != CM_OK)
    {
        return iRet;
    }

    return CM_OK;
}

sint32 cm_cnm_ntp_decode(const cm_omi_obj_t ObjParam, void**ppDecodeParam)
{
    return cm_cnm_decode_comm(ObjParam,sizeof(cm_cnm_ntp_info_t),cm_cnm_ntp_decode_ext,ppDecodeParam);
}

static void cm_cnm_ntp_encode_each(cm_omi_obj_t item,void* eachdata,void* arg)
{
    cm_omi_field_flag_t *field = arg;
    cm_cnm_ntp_info_t *info = eachdata;

    cm_cnm_map_value_str_t cols_str[] =
    {
        {CM_OMI_FIELD_NTP_IP,info->ip},
    };

    cm_cnm_encode_str(item,field,cols_str,sizeof(cols_str)/sizeof(cm_cnm_map_value_str_t));
    return;
}

cm_omi_obj_t cm_cnm_ntp_encode(const void *pDecodeParam,void *pAckData,uint32 AckLen)
{
    return cm_cnm_encode_comm_ext(pDecodeParam,pAckData,AckLen,
        sizeof(cm_cnm_ntp_info_t),cm_cnm_ntp_encode_each);
}


sint32 cm_cnm_ntp_insert(const void * pDecodeParam,void * * ppAckData,uint32 * pAckLen)
{
    sint32 iRet = CM_OK;
    const cm_cnm_decode_info_t *decode = pDecodeParam;
    cm_cnm_ntp_info_t *info =  (const cm_cnm_ntp_info_t *)decode->data;

    iRet = cm_system("ntpdate -u %s",info->ip);
    if (iRet != CM_OK)
    {
        CM_ALARM_REPORT(CM_CNM_NTP_ALARM_ID,cm_cnm_ntp_ip);
        CM_LOG_ERR(CM_MOD_CNM,"ntpdate sync error ip:%s",info->ip);
        return iRet;
    }

    CM_VSPRINTF(cm_cnm_ntp_ip,sizeof(cm_cnm_ntp_ip),"%s",info->ip);
    cm_system("%s server",cm_cnm_ntp_sh);

    return cm_sync_request(CM_SYNC_OBJ_NTP,CM_CNM_NTP_ALARM_ID,(void*)info,sizeof(cm_cnm_decode_info_t));
}

sint32 cm_cnm_ntp_delete(const void * pDecodeParam,void * * ppAckData,uint32 * pAckLen)
{
    cm_cnm_ntp_ip[0] = '\0';
    if( 0 != strlen(cm_cnm_ntp_ip))
    {
        CM_LOG_ERR(CM_MOD_CNM,"ntpdate delete fail");
        return CM_FAIL;
    }
    
    CM_ALARM_RECOVERY(CM_CNM_NTP_ALARM_ID,"recovery");
    return cm_sync_delete(CM_SYNC_OBJ_NTP,CM_CNM_NTP_ALARM_ID);
}

sint32 cm_cnm_ntp_get(const void * pDecodeParam,void ** ppAckData,uint32 * pAckLen)
{
    cm_cnm_ntp_info_t *info = NULL;

    info = CM_MALLOC(sizeof(cm_cnm_ntp_info_t));
    if(NULL == info)
    {
        CM_LOG_ERR(CM_MOD_CNM,"malloc fail");
        return CM_FAIL;
    }

    if(cm_cnm_ntp_ip[0] != '\0')
    {
        CM_VSPRINTF(info->ip,sizeof(info->ip),cm_cnm_ntp_ip);
    }else
    {
        *ppAckData = NULL;
        *pAckLen = 0;
         CM_FREE(info);
         return CM_OK;
    }

    *ppAckData = info;
    *pAckLen = sizeof(cm_cnm_ntp_info_t);
    return CM_OK;
}


void cm_cnm_ntp_master_change(uint32 old_id, uint32 new_id)
{
    uint32 local_id;
    
    local_id = cm_node_get_id();
  
    if(new_id != local_id)
    {
        return;
    }

    cm_system("%s server",cm_cnm_ntp_sh);
    return;
}

void cm_cnm_ntp_sync_thread(void)
{
    sint32 iRet = CM_OK;
    static int flag = 0; 

    if(cm_node_get_id() != cm_node_get_master())
    {
        return;
    }

    if(cm_cnm_ntp_ip[0] == '\0')
    {
        return;
    }
    
    iRet = cm_system("ntpdate -u %s",cm_cnm_ntp_ip);
    if (iRet != CM_OK && flag == 0)
    {
        iRet = CM_ALARM_REPORT(CM_CNM_NTP_ALARM_ID,cm_cnm_ntp_ip);
        if(iRet == CM_OK)
        {
            flag = 1;
            CM_LOG_ERR(CM_MOD_CNM,"ntpdate sync error ip:%s",cm_cnm_ntp_ip);
        }
        return;
    }

    if( iRet == CM_OK && flag == 1)
    {
        CM_ALARM_RECOVERY(CM_CNM_NTP_ALARM_ID,"recovery");
        flag = 0;
    }

    return;
}


sint32 cm_cnm_ntp_sync_request(uint64 data_id, void *pdata, uint32 len)
{
    cm_cnm_ntp_info_t* info = (cm_cnm_ntp_info_t*)pdata;

    (void)cm_ini_set_ext(CM_CLUSTER_INI, CM_CNM_CLUSTER_CFG_SECTION,
                         CM_CNM_CLUSTER_CFG_NTPSERVER, info->ip);
    CM_VSPRINTF(cm_cnm_ntp_ip,sizeof(cm_cnm_ntp_ip),info->ip);
    return CM_OK;
}

sint32 cm_cnm_ntp_sync_delete(uint64 enid)
{
    (void)cm_ini_set_ext(CM_CLUSTER_INI, CM_CNM_CLUSTER_CFG_SECTION,
                         CM_CNM_CLUSTER_CFG_NTPSERVER, "\0");
    cm_cnm_ntp_ip[0] = '\0';
    return CM_OK;
}


sint32 cm_cnm_ntp_sync_get(uint64 data_id, void **pdata, uint32 *plen)
{
    cm_cnm_ntp_info_t* info = NULL;

    info = CM_MALLOC(sizeof(cm_cnm_ntp_info_t));
    if(info == NULL)
    {
        CM_LOG_ERR(CM_MOD_CNM,"malloc fail");
        return CM_FAIL;
    }

    CM_VSPRINTF(info->ip,sizeof(info->ip),cm_cnm_ntp_ip);

    *pdata = info;
    *plen = sizeof(cm_cnm_ntp_info_t);

    return CM_OK;
}


