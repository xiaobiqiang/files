/******************************************************************************
 *          Copyright (c) 2018 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cnm_dnsm.c
 * author     : wbn
 * create date: 2018年3月16日
 * description: TODO:
 *
 *****************************************************************************/

#include "cm_cnm_dnsm.h"
#include "cm_node.h"
#include "cm_omi.h"
#include "cm_log.h"
#include "cm_cmt.h"
#include "cm_sync.h"

const sint8* g_cm_cnm_dnsm_script = "/var/cm/script/cm_cnm_dnsm.sh";

#define cm_cnm_dnsm_request(cmd,param,ppAck,plen) \
    cm_cnm_request_comm(CM_OMI_OBJECT_DNSM,cmd,sizeof(cm_cnm_dnsm_info_t),param,ppAck,plen)

sint32 cm_cnm_dnsm_init(void)
{
    return CM_OK;
}
  
/************LOCAL*************************************************************/

static sint32 cm_cnm_dnsm_local_get_each(void *arg, sint8 **cols, uint32 col_num)
{
    cm_cnm_dnsm_info_t *info = arg;
    const uint32 def_num = 2;       
    if(def_num > col_num)
    {
        CM_LOG_WARNING(CM_MOD_CNM,"col_num[%u] def_num[%u]",col_num,def_num);
        return CM_FAIL;
    }
    CM_MEM_ZERO(info,sizeof(cm_cnm_dnsm_info_t));     
    CM_VSPRINTF(info->ip,sizeof(info->ip),"%s",cols[0]);    
    CM_VSPRINTF(info->domain,sizeof(info->domain),"%s",cols[1]);
    info->nid = cm_node_get_id();
    return CM_OK;
}

sint32  cm_cnm_dnsm_local_update(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    const cm_cnm_decode_info_t *decode = param;
    const cm_cnm_dnsm_info_t *info = (const cm_cnm_dnsm_info_t *)decode->data;
    uint32 cnt = 0;
    cnt = cm_exec_int("grep -w %s /etc/hosts | wc -l",info->domain);
    if(cnt == 1)
    {
        return cm_system("%s modify %s %s",g_cm_cnm_dnsm_script,info->domain,info->ip);
    }
    return CM_ERR_NOT_EXISTS;   
}    

sint32  cm_cnm_dnsm_local_insert(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    const cm_cnm_decode_info_t *decode = param;
    const cm_cnm_dnsm_info_t *info = (const cm_cnm_dnsm_info_t *)decode->data;

    return cm_system("%s add %s %s",g_cm_cnm_dnsm_script,info->ip,info->domain);
}    

sint32  cm_cnm_dnsm_local_get(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    sint32 iRet = CM_OK;
    uint32 domain_check = 0;
    const cm_cnm_decode_info_t *dec = param;
    cm_cnm_dnsm_info_t *info = NULL;
    cm_cnm_dnsm_info_t *pdata = NULL;
    if(dec == NULL)
    {
        return CM_PARAM_ERR; 
    }
    if(!CM_OMI_FIELDS_FLAG_ISSET(&dec->set,CM_OMI_FIELD_DNSM_DOMAIN))
    {
        return CM_PARAM_ERR;
    }
    
    info = (const cm_cnm_dnsm_info_t*)dec->data;
    domain_check = cm_exec_int("grep -w %s /etc/hosts| wc -l",info->domain);
    if(domain_check == 0)
    {
        return CM_ERR_NOT_EXISTS;
    }
    pdata = CM_MALLOC(sizeof(cm_cnm_dnsm_info_t));
    iRet = cm_cnm_exec_get_col(cm_cnm_dnsm_local_get_each,pdata,
        "%s get %s",g_cm_cnm_dnsm_script,info->domain);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"iRet[%d] get_col_fail",iRet);
        CM_FREE(pdata);
        return iRet;
    }
    *ppAck = pdata;
    *pAckLen = sizeof(cm_cnm_dnsm_info_t);
    return CM_OK; 
}    

sint32  cm_cnm_dnsm_local_delete(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    const cm_cnm_decode_info_t *decode = param;
    const cm_cnm_dnsm_info_t *info = (const cm_cnm_dnsm_info_t *)decode->data;
    uint32 cnt = 0;
    cnt = cm_exec_int("grep -w %s /etc/hosts | wc -l",info->domain);
    if(cnt == 1)
    {
        return cm_system("%s delete %s",g_cm_cnm_dnsm_script,info->domain);
    }
    return CM_ERR_NOT_EXISTS;
}    

sint32 cm_cnm_dnsm_local_count(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    uint64 cnt = 0;
    cnt = cm_exec_int("%s count",g_cm_cnm_dnsm_script);

    return cm_cnm_ack_uint64(cnt,ppAck,pAckLen);
}

sint32  cm_cnm_dnsm_local_getbatch(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    sint32 iRet = CM_OK;
    sint8 cmd[CM_STRING_512] = {0};
    CM_VSPRINTF(cmd,CM_STRING_128,"%s getbatch",g_cm_cnm_dnsm_script);
    iRet = cm_cnm_exec_get_list(cmd,cm_cnm_dnsm_local_get_each,
        (uint32)offset,sizeof(cm_cnm_dnsm_info_t),ppAck,&total); 
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]",iRet);
        return iRet;
    }
    *pAckLen = total * sizeof(cm_cnm_dnsm_info_t);
    return CM_OK;
}    


sint32 cm_cnm_dnsm_getbatch(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_cnm_dnsm_request(CM_OMI_CMD_GET_BATCH,pDecodeParam,ppAckData,pAckLen);
}


sint32 cm_cnm_dnsm_update(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_cnm_dnsm_request(CM_OMI_CMD_MODIFY,pDecodeParam,ppAckData,pAckLen);
}  

sint32 cm_cnm_dnsm_insert(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_cnm_dnsm_request(CM_OMI_CMD_ADD,pDecodeParam,ppAckData,pAckLen);
}    

sint32 cm_cnm_dnsm_count(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_cnm_dnsm_request(CM_OMI_CMD_COUNT,pDecodeParam,ppAckData,pAckLen);
}    

sint32 cm_cnm_dnsm_get(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_cnm_dnsm_request(CM_OMI_CMD_GET,pDecodeParam,ppAckData,pAckLen);
}    

sint32 cm_cnm_dnsm_delete(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_cnm_dnsm_request(CM_OMI_CMD_DELETE,pDecodeParam,ppAckData,pAckLen);
}    

static sint32 cm_cnm_dnsm_decode_ext(
    const cm_omi_obj_t ObjParam,void* data,cm_omi_field_flag_t *set)
{
    sint32 iRet = CM_OK;
    cm_cnm_dnsm_info_t *info = data;
    cm_cnm_decode_param_t param_str[] = 
    {
        {CM_OMI_FIELD_DNSM_IP,sizeof(info->ip),info->ip,NULL},
        {CM_OMI_FIELD_DNSM_DOMAIN,sizeof(info->domain),info->domain,NULL},
    };

    cm_cnm_decode_param_t param_num[] = 
    {
        {CM_OMI_FIELD_DNSM_NID,sizeof(info->nid),&info->nid,NULL},
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

sint32 cm_cnm_dnsm_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam)
{
    return cm_cnm_decode_comm(ObjParam,sizeof(cm_cnm_dnsm_info_t),
        cm_cnm_dnsm_decode_ext,ppDecodeParam);
}

static void cm_cnm_dnsm_encode_each(cm_omi_obj_t item,void *eachdata,void *arg)
{
    cm_omi_field_flag_t *field = arg;
    cm_cnm_dnsm_info_t *info = eachdata;
    cm_cnm_map_value_str_t cols_str[] = 
    {
        {CM_OMI_FIELD_DNSM_IP,     info->ip},
        {CM_OMI_FIELD_DNSM_DOMAIN,     info->domain},
    };
    cm_cnm_map_value_num_t cols_num[] = 
    {
        {CM_OMI_FIELD_DNSM_NID,          info->nid},
    };
    cm_cnm_encode_str(item,field,cols_str,sizeof(cols_str)/sizeof(cm_cnm_map_value_str_t));
    cm_cnm_encode_num(item,field,cols_num,sizeof(cols_num)/sizeof(cm_cnm_map_value_num_t)); 
    return;
}

cm_omi_obj_t cm_cnm_dnsm_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen)
{
    return cm_cnm_encode_comm_ext(pDecodeParam,pAckData,AckLen,
        sizeof(cm_cnm_dnsm_info_t),cm_cnm_dnsm_encode_each);
}

static void cm_cnm_dnsm_oplog_report(
    const sint8* sessionid, const void *pDecodeParam, uint32 alarmid)
{
    const cm_cnm_decode_info_t *req = pDecodeParam;
    const uint32 cnt = 3;

    if(NULL == pDecodeParam)
    {   
        cm_cnm_oplog_report(sessionid,alarmid,NULL,cnt,NULL);
    }    
    else    
    {       
        const cm_cnm_dnsm_info_t *info = (const cm_cnm_dnsm_info_t*)req->data;
        const sint8* nodename = cm_node_get_name(info->nid);
        cm_cnm_oplog_param_t params[3] = {
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_DNSM_DOMAIN,strlen(info->domain),info->domain},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_DNSM_IP,strlen(info->ip),info->ip},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_DNSM_NID,strlen(nodename),nodename},
        };
        cm_cnm_oplog_report(sessionid,alarmid,params,cnt,&req->set);
    }    
    return;
}    

void cm_cnm_dnsm_oplog_update(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK)? CM_ALARM_LOG_DNSM_UPDATE_OK : CM_ALARM_LOG_DNSM_UPDATE_FAIL;
    cm_cnm_dnsm_oplog_report(sessionid,pDecodeParam,alarmid);
    return;
}
void cm_cnm_dnsm_oplog_insert(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK)? CM_ALARM_LOG_DNSM_INSERT_OK : CM_ALARM_LOG_DNSM_INSERT_FAIL;
    const cm_cnm_decode_info_t *req = pDecodeParam;
    const uint32 cnt = 3;
    if(NULL == pDecodeParam)
    {   
        cm_cnm_oplog_report(sessionid,alarmid,NULL,cnt,NULL);
    }    
    else    
    {       
        const cm_cnm_dnsm_info_t *info = (const cm_cnm_dnsm_info_t*)req->data;
        const sint8* nodename = cm_node_get_name(info->nid);
        cm_cnm_oplog_param_t params[3] = {
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_DNSM_IP,strlen(info->ip),info->ip},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_DNSM_DOMAIN,strlen(info->domain),info->domain},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_DNSM_NID,strlen(nodename),nodename},      
        };
        cm_cnm_oplog_report(sessionid,alarmid,params,cnt,&req->set);
    }   
    return;
}

void cm_cnm_dnsm_oplog_delete(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK)? CM_ALARM_LOG_DNSM_DELETE_OK : CM_ALARM_LOG_DNSM_DELETE_FAIL;
    const cm_cnm_decode_info_t *req = pDecodeParam;
    const uint32 cnt = 2;

    if(NULL == pDecodeParam)
    {   
        cm_cnm_oplog_report(sessionid,alarmid,NULL,cnt,NULL);
    }    
    else    
    {       
        const cm_cnm_dnsm_info_t *info = (const cm_cnm_dnsm_info_t*)req->data;
        const sint8* nodename = cm_node_get_name(info->nid);
        cm_cnm_oplog_param_t params[2] = {
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_DNSM_DOMAIN,strlen(info->domain),info->domain},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_DNSM_NID,strlen(nodename),nodename},      
        };
        cm_cnm_oplog_report(sessionid,alarmid,params,cnt,&req->set);
    }   
    return; 
}    

const sint8* g_cm_domain_ad_script = "/var/cm/script/cm_cnm.sh";

static sint32 cm_cnm_dns_local_get_each(void *arg, sint8 **cols, uint32 col_num)
{
    cm_cnm_dns_info_t *info = arg;

    CM_MEM_ZERO(info,sizeof(cm_cnm_dns_info_t));
    if(col_num > 1)
    {
        CM_VSPRINTF(info->ip_slave,sizeof(info->ip_slave),"%s",cols[1]); 
    }
    if(col_num > 0)
    {
        CM_VSPRINTF(info->ip_master,sizeof(info->ip_master),"%s",cols[0]); 
    }
    return CM_OK;
}

sint32  cm_cnm_dns_insert(void *pDecodeParam,void **ppAckData,uint32 * pAckLen)
{
    sint32 iRet = CM_OK;
    const cm_cnm_decode_info_t *decode = pDecodeParam;
    cm_cnm_dns_info_t info;
    const cm_cnm_dns_info_t *pinfo = NULL;
    if(decode == NULL)
    {
        CM_LOG_ERR(CM_MOD_CNM,"decode param null");
        return CM_PARAM_ERR;
    }
    CM_MEM_ZERO(&info,sizeof(info));
    iRet = cm_cnm_exec_get_col(cm_cnm_dns_local_get_each,&info,
        "%s dnsserver_get",g_cm_domain_ad_script);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"iRet[%d] get_info_fail",iRet);
    } 
    pinfo = (const cm_cnm_dns_info_t *)decode->data;
    if(CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_DNS_IPMASTER))
    {
        CM_VSPRINTF(info.ip_master,sizeof(info.ip_master),"%s",pinfo->ip_master);
    }
    if(CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_DNS_IPSLAVE))
    {
        CM_VSPRINTF(info.ip_slave,sizeof(info.ip_slave),"%s",pinfo->ip_slave);
    }
    /******若修改后ip_master和ip_slave相同，则不进行同步*********/
    if(strcmp(info.ip_master,info.ip_slave) == 0)
    {
        CM_LOG_ERR(CM_MOD_CNM,"ip_master[%s],ip_slave[%s] is same",
            info.ip_master,info.ip_slave);
        return CM_ERR_ALREADY_EXISTS;
    }
    return cm_sync_request(CM_SYNC_OBJ_DNS,0,&info,sizeof(cm_cnm_dns_info_t));
}    

sint32 cm_cnm_dns_delete(const void * pDecodeParam,void **ppAckData,uint32 * pAckLen)
{
    sint32 iRet = CM_OK;
    const cm_cnm_decode_info_t *decode = pDecodeParam;
    cm_cnm_dns_info_t info;
    const cm_cnm_dns_info_t *pinfo = NULL;
    if(decode==NULL)
    {
        CM_LOG_ERR(CM_MOD_CNM,"decode param null");
        return CM_PARAM_ERR;
    }
    CM_MEM_ZERO(&info,sizeof(info));
    iRet = cm_cnm_exec_get_col(cm_cnm_dns_local_get_each,&info,
        "%s dnsserver_get",g_cm_domain_ad_script);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"iRet[%d] get_info_fail",iRet);
        return CM_OK;
    }
    pinfo = (const cm_cnm_dns_info_t *)decode->data;
    if(CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_DNS_IPMASTER))
    {
        if(strcmp(pinfo->ip_master,info.ip_master) != 0)
        {
            CM_LOG_ERR(CM_MOD_CNM,"ip_master[%s] not exist",info.ip_master);
            return CM_ERR_NOT_EXISTS;
        }
        if(strlen(pinfo->ip_slave) != 0)
        {
            CM_VSPRINTF(info.ip_master,sizeof(info.ip_master),"%s",pinfo->ip_slave);
        }
        else
        {
            CM_VSPRINTF(info.ip_master,sizeof(info.ip_master),"");
        }     
    }
    if(CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_DNS_IPSLAVE))
    {
        if(strcmp(pinfo->ip_slave,info.ip_slave) != 0)
        {
            CM_LOG_ERR(CM_MOD_CNM,"ip_slave[%s] not exist",info.ip_slave);
            return CM_ERR_NOT_EXISTS;
        }
        CM_VSPRINTF(info.ip_slave,sizeof(info.ip_slave),"");
    } 
    return cm_sync_request(CM_SYNC_OBJ_DNS,0,&info,sizeof(cm_cnm_dns_info_t));
}

sint32 cm_cnm_dns_sync_request(uint64 enid, void *pdata, uint32 len)
{
    sint32 iRet = CM_OK;
    cm_cnm_dns_info_t *info = (cm_cnm_dns_info_t*)pdata;
    iRet = cm_system("%s dnsserver_set %s %s",g_cm_domain_ad_script,info->ip_master,info->ip_slave);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"insert dns fail");
        return CM_FAIL;
    }
    return CM_OK;
}

sint32 cm_cnm_dns_sync_get(uint64 enid, void **pdata, uint32 *plen)
{
    return cm_cnm_dns_get(NULL,pdata,plen);
}

sint32 cm_cnm_dns_sync_delete(uint64 enid)
{
    return cm_system("%s dnsserver_set '' ''",g_cm_domain_ad_script);
}

sint32 cm_cnm_dns_get(const void * pDecodeParam,void **ppAckData,uint32 * pAckLen)
{
    sint32 iRet = CM_OK;
    cm_cnm_dns_info_t *pdata = CM_MALLOC(sizeof(cm_cnm_dns_info_t));
    if(pdata == NULL)
    {
        CM_LOG_ERR(CM_MOD_CNM,"malloc fail");
        return CM_FAIL;
    }
    CM_MEM_ZERO(pdata, sizeof(cm_cnm_dns_info_t));
    iRet = cm_cnm_exec_get_col(cm_cnm_dns_local_get_each,pdata,
            "%s dnsserver_get",g_cm_domain_ad_script);    
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"iRet[%d] get_col_fail",iRet);
    }
    *ppAckData = pdata;
    *pAckLen = sizeof(cm_cnm_dns_info_t);
    return CM_OK;
}

static sint32 cm_cnm_dns_decode_ext(
    const cm_omi_obj_t ObjParam,void* data,cm_omi_field_flag_t *set)
{
    sint32 iRet = CM_OK;
    cm_cnm_dns_info_t *info = data;
    cm_cnm_decode_param_t param_str[] = 
    {
        {CM_OMI_FIELD_DNS_IPMASTER,sizeof(info->ip_master),info->ip_master,NULL},
        {CM_OMI_FIELD_DNS_IPSLAVE,sizeof(info->ip_slave),info->ip_slave,NULL},
    }; 
    iRet = cm_cnm_decode_str(ObjParam,param_str,
        sizeof(param_str)/sizeof(cm_cnm_decode_param_t),set);
    if(iRet != CM_OK)
    {
        return iRet;
    }
    return CM_OK;
}

sint32 cm_cnm_dns_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam)
{
    return cm_cnm_decode_comm(ObjParam,sizeof(cm_cnm_dns_info_t),
        cm_cnm_dns_decode_ext,ppDecodeParam);
}

static void cm_cnm_dns_encode_each(cm_omi_obj_t item,void *eachdata,
void *arg)
{
    cm_omi_field_flag_t *field = arg;
    cm_cnm_dns_info_t *info = eachdata;
    cm_cnm_map_value_str_t cols_str[] = 
    {
        {CM_OMI_FIELD_DNS_IPMASTER,    info->ip_master},
        {CM_OMI_FIELD_DNS_IPSLAVE,    info->ip_slave},
    };
    cm_cnm_encode_str(item,field,cols_str,sizeof(cols_str)/sizeof(cm_cnm_map_value_str_t));
    return;
}

cm_omi_obj_t cm_cnm_dns_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen)
{
    return cm_cnm_encode_comm_ext(pDecodeParam,pAckData,AckLen,
        sizeof(cm_cnm_dns_info_t),cm_cnm_dns_encode_each);
}

static void cm_cnm_dns_oplog_report(
    const sint8* sessionid, const void *pDecodeParam, uint32 alarmid)
{
    const cm_cnm_decode_info_t *req = pDecodeParam;
    const uint32 cnt = 2;
    if(NULL == pDecodeParam)
    {   
        cm_cnm_oplog_report(sessionid,alarmid,NULL,cnt,NULL);
    }    
    else    
    {       
        const cm_cnm_dns_info_t *info = (const cm_cnm_dns_info_t*)req->data;
        const sint8* nodename = cm_node_get_name(cm_node_get_id());
        cm_cnm_oplog_param_t params[2] = {
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_DNS_IPMASTER,strlen(info->ip_master),info->ip_master},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_DNS_IPSLAVE,strlen(info->ip_slave),info->ip_slave},
        };
        cm_cnm_oplog_report(sessionid,alarmid,params,cnt,&req->set);
    }    
    return;
}    

void cm_cnm_dns_oplog_insert(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK)? CM_ALARM_LOG_DNS_INSERT_OK : CM_ALARM_LOG_DNS_INSERT_FAIL;
    cm_cnm_dnsm_oplog_report(sessionid,pDecodeParam,alarmid);
    return;
}

void cm_cnm_dns_oplog_delete(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK)? CM_ALARM_LOG_DNS_DELETE_OK : CM_ALARM_LOG_DNS_DELETE_FAIL;
    cm_cnm_dnsm_oplog_report(sessionid,pDecodeParam,alarmid);
    return;
}

static sint32 cm_cnm_domain_ad_decode_ext(
    const cm_omi_obj_t ObjParam,void* data,cm_omi_field_flag_t *set)
{
    sint32 iRet = CM_OK;
    cm_cnm_domain_ad_info_t *info = data;
    cm_cnm_decode_param_t param_str[] = 
    {
        {CM_OMI_FIELD_DOMAINAD_DOMAINCTL,sizeof(info->dc_name),info->dc_name,NULL},
        {CM_OMI_FIELD_DOMAINAD_DOMIAN,sizeof(info->domain),info->domain,NULL},
        {CM_OMI_FIELD_DOMAINAD_USERNAME,sizeof(info->username),info->username,NULL},
        {CM_OMI_FIELD_DOMAINAD_USERPWD,sizeof(info->userpwd),info->userpwd,NULL},
    }; 
    iRet = cm_cnm_decode_str(ObjParam,param_str,
        sizeof(param_str)/sizeof(cm_cnm_decode_param_t),set);
    if(iRet != CM_OK)
    {
        return iRet;
    }
    return CM_OK;
}
sint32 cm_cnm_domain_ad_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam)
{
    return cm_cnm_decode_comm(ObjParam,sizeof(cm_cnm_domain_ad_info_t),
        cm_cnm_domain_ad_decode_ext,ppDecodeParam);
}
static void cm_cnm_domain_ad_encode_each(cm_omi_obj_t item,void *eachdata,
void *arg)
{
    cm_omi_field_flag_t *field = arg;
    cm_cnm_domain_ad_info_t *info = eachdata;
    cm_cnm_map_value_str_t cols_str[] = 
    {
        {CM_OMI_FIELD_DOMAINAD_DOMAINCTL,    info->dc_name},
        {CM_OMI_FIELD_DOMAINAD_DOMIAN,       info->domain},
        {CM_OMI_FIELD_DOMAINAD_STATE,       info->state},
    };
    cm_cnm_encode_str(item,field,cols_str,sizeof(cols_str)/sizeof(cm_cnm_map_value_str_t));
    return;
}
cm_omi_obj_t cm_cnm_domain_ad_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen)
{
    return cm_cnm_encode_comm_ext(pDecodeParam,pAckData,AckLen,
        sizeof(cm_cnm_domain_ad_info_t),cm_cnm_domain_ad_encode_each);
}
static sint32 cm_cnm_domain_ad_local_get_each(void *arg, sint8 **cols, uint32 col_num)
{
    cm_cnm_domain_ad_info_t *info = arg;
    const uint32 def_num = 3;       
    if(def_num != col_num)
    {
        CM_LOG_WARNING(CM_MOD_CNM,"col_num[%u] def_num[%u]",col_num,def_num);
        return CM_FAIL;
    }
    CM_VSPRINTF(info->domain,sizeof(info->domain),"%s",cols[0]);    
    CM_VSPRINTF(info->dc_name,sizeof(info->dc_name),"%s",cols[1]);
    CM_VSPRINTF(info->state,sizeof(info->state),"%s",cols[2]);
    return CM_OK;
}
void* cm_cnm_domain_check_info(void* arg)
{
    sint32 iRet = CM_OK;
    cm_cnm_domain_ad_info_t* info = (cm_cnm_domain_ad_info_t *)arg;
    iRet = cm_system("%s domain_config_set",g_cm_domain_ad_script);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"set_config fail");
        CM_FREE(info);
        return NULL;
    }
    
    iRet = cm_system("%s domain_ad_set %s %s %s",g_cm_domain_ad_script,
        info->domain,info->username,info->userpwd);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"username or pwd wrong");
    }
    CM_FREE(info);
    return NULL;
}
sint32  cm_cnm_domain_ad_insert(void *pDecodeParam,void **ppAckData,uint32 * pAckLen)
{
    sint32 iRet = CM_OK;
    cm_cnm_decode_info_t *decode = pDecodeParam;
    cm_cnm_domain_ad_info_t *info = NULL;
    cm_cnm_domain_ad_info_t *infox = NULL;
    sint8 buff[CM_STRING_2K] = {0};
    if(decode == NULL)
    {
        CM_LOG_ERR(CM_MOD_CNM,"decode param null");
        return CM_PARAM_ERR;
    }
    info = (const cm_cnm_domain_ad_info_t *)decode->data;
    if((CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_DOMAINAD_DOMIAN))
        &&(CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_DOMAINAD_DOMAINCTL))
        &&(CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_DOMAINAD_USERNAME))
        &&(CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_DOMAINAD_USERPWD)))
    {
        iRet = cm_system("%s domain_ad_set_krb5 %s %s",g_cm_domain_ad_script,info->domain,info->dc_name);
        if(CM_OK != iRet)
        {
            CM_LOG_ERR(CM_MOD_CNM,"set_krb5 fail");
            return CM_FAIL;
        }

        iRet = cm_system("%s domain_ad_set_local %s %s %s",g_cm_domain_ad_script,info->domain,info->username,info->userpwd);
        if(CM_OK != iRet)
        {
            CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]",iRet);
            return CM_FAIL;
        }
        infox = CM_MALLOC(sizeof(cm_cnm_domain_ad_info_t));
        if(NULL == infox)
        {
            CM_LOG_ERR(CM_MOD_CNM,"malloc fail");
            return CM_FAIL;
        }
        CM_MEM_CPY(infox,sizeof(cm_cnm_domain_ad_info_t),info,sizeof(cm_cnm_domain_ad_info_t));
        iRet = cm_thread_start(cm_cnm_domain_check_info,(void*)infox);
        if(CM_OK != iRet)
        {
            CM_FREE(infox);
            CM_LOG_ERR(CM_MOD_CNM,"cm_thread_start fail");
        }
        return iRet;
    }
    
    return CM_PARAM_ERR;
}    

sint32 cm_cnm_domain_ad_get(const void * pDecodeParam,void **ppAckData,uint32 * pAckLen)
{
    sint32 iRet = CM_OK;
    cm_cnm_domain_ad_info_t *pdata = CM_MALLOC(sizeof(cm_cnm_domain_ad_info_t));
    if(pdata == NULL)
    {
        CM_LOG_ERR(CM_MOD_CNM,"malloc fail");
        return CM_FAIL;
    }
    CM_MEM_ZERO(pdata,sizeof(cm_cnm_domain_ad_info_t));
    iRet = cm_cnm_exec_get_col(cm_cnm_domain_ad_local_get_each,pdata,
            "%s domain_ad_get",g_cm_domain_ad_script);    
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"iRet[%d] get_col_fail",iRet);
    }
    *ppAckData = pdata;
    *pAckLen = sizeof(cm_cnm_domain_ad_info_t);
    return CM_OK;
}


void* cm_cnm_domain_delete_thread(void* arg)
{
    sint32 iRet = CM_OK;
    iRet = cm_system("%s domain_cluster_exit",g_cm_domain_ad_script);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"exit fail");
        return CM_FAIL;
    }

    return NULL;
}


sint32 cm_cnm_domain_ad_delete(void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_thread_start(cm_cnm_domain_delete_thread,NULL);
}



