/******************************************************************************
 *          Copyright (c) 2018 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cnm_aggr.c
 * author     : xar
 * create date: 2018.09.18
 * description: TODO:
 *
 *****************************************************************************/
#include "cm_cnm_aggr.h"
#include "cm_log.h"
#include "cm_xml.h"
#include "cm_node.h"
#include "cm_omi.h"
#include "cm_db.h"

extern const cm_omi_map_enum_t CmOmiMapEnumPolicyType;
extern const cm_omi_map_enum_t CmOmiMapEnumLacpactivityType;
extern const cm_omi_map_enum_t CmOmiMapEnumLacptimerType;

#define cm_cnm_aggr_req(cmd,param,ppAck,plen) \
     cm_cnm_request_comm(CM_OMI_OBJECT_AGGR,cmd,sizeof(cm_cnm_aggr_info_t),param,ppAck,plen)
const sint8* cm_cnm_aggr_sh = "/var/cm/script/cm_cnm_aggr.sh";
 
sint32 cm_cnm_aggr_init(void)
{
    return CM_OK;
}
 
static sint32 cm_cnm_aggr_decode_ext(const cm_omi_obj_t ObjParam, void* data, cm_omi_field_flag_t *set)
{
    sint32 iRet = CM_OK;
    cm_cnm_aggr_info_t* info = data;
  
    cm_cnm_decode_param_t param_str[] =
    {
        {CM_OMI_FIELD_AGGR_NAME, sizeof(info->name), info->name, NULL},
        {CM_OMI_FIELD_AGGR_ADAPTER, sizeof(info->adapter), info->adapter, NULL},
        {CM_OMI_FIELD_AGGR_IP, sizeof(info->ip), info->ip, NULL},
        {CM_OMI_FIELD_AGGR_NETMASK, sizeof(info->netmask), info->netmask, NULL},
    };
 
    cm_cnm_decode_param_t param_num[] =
    {
        {CM_OMI_FIELD_AGGR_NID, sizeof(info->nid), &info->nid, NULL},
        {CM_OMI_FIELD_AGGR_POLICY, sizeof(info->policy), &info->policy, NULL},
        {CM_OMI_FIELD_AGGR_LACPACTIVITY, sizeof(info->lacpactivity), &info->lacpactivity, NULL},         
        {CM_OMI_FIELD_AGGR_LACPTIMER, sizeof(info->lacptimer), &info->lacptimer, NULL},
    };
    iRet = cm_cnm_decode_str(ObjParam, param_str,
        sizeof(param_str) / sizeof(cm_cnm_decode_param_t), set);
    if(CM_OK != iRet)
    {
        return iRet;
    }
 
    iRet = cm_cnm_decode_num(ObjParam, param_num,
        sizeof(param_num) / sizeof(cm_cnm_decode_param_t), set);
 
    return CM_OK;
}
 
sint32 cm_cnm_aggr_decode(const cm_omi_obj_t ObjParam, void **ppDecodeParam)
{
    return cm_cnm_decode_comm(ObjParam, sizeof(cm_cnm_aggr_info_t), cm_cnm_aggr_decode_ext, ppDecodeParam);
}


static void cm_cnm_aggr_encode_each(cm_omi_obj_t item, void* eachdata, void* arg)
{
    cm_omi_field_flag_t *field = arg;
    cm_cnm_aggr_info_t *info = eachdata;
    cm_cnm_map_value_str_t cols_str[] =
    {
        {CM_OMI_FIELD_AGGR_NAME, info->name}, 
        {CM_OMI_FIELD_AGGR_STATE, info->state},
        {CM_OMI_FIELD_AGGR_IP, info->ip},
        {CM_OMI_FIELD_AGGR_NETMASK, info->netmask},
        {CM_OMI_FIELD_AGGR_MTU, info->mtu},
        {CM_OMI_FIELD_AGGR_MAC, info->mac},
        {CM_OMI_FIELD_AGGR_ADAPTER, info->adapter},
    };

    cm_cnm_map_value_num_t cols_num[] = 
    {
        {CM_OMI_FIELD_AGGR_NID, info->nid},
        {CM_OMI_FIELD_AGGR_POLICY, info->policy},
        {CM_OMI_FIELD_AGGR_LACPACTIVITY,info->lacpactivity},
        {CM_OMI_FIELD_AGGR_LACPTIMER,info->lacptimer},
    };
    
 
    cm_cnm_encode_str(item,field,cols_str,sizeof(cols_str)/sizeof(cm_cnm_map_value_str_t));
    cm_cnm_encode_num(item,field,cols_num,sizeof(cols_num)/sizeof(cm_cnm_map_value_str_t));
    return;
}
 
cm_omi_obj_t cm_cnm_aggr_encode(const void *pDecodeParam, void *pAckData, uint32 AckLen)
{
    return cm_cnm_encode_comm_ext(pDecodeParam, pAckData, AckLen,
        sizeof(cm_cnm_aggr_info_t), cm_cnm_aggr_encode_each);
}

sint32 cm_cnm_aggr_getbatch(const void *pDecodeParam, void **ppAckData, uint32 *pAckLen)
{
    return cm_cnm_aggr_req(CM_OMI_CMD_GET_BATCH, pDecodeParam, ppAckData, pAckLen);
}

sint32 cm_cnm_aggr_count(const void *pDecodeParam,void **ppAckData,uint32 *pAckLen)
{
    if(NULL == pDecodeParam)
    {
        return CM_PARAM_ERR; 
    }
    return cm_cnm_aggr_req(CM_OMI_CMD_COUNT,pDecodeParam,ppAckData,pAckLen);
}

sint32 cm_cnm_aggr_update(const void *pDecodeParam,void **ppAckData,uint32 *pAckLen)
{
    if(NULL == pDecodeParam)
    {
        return CM_PARAM_ERR; 
    }
    return cm_cnm_aggr_req(CM_OMI_CMD_MODIFY,pDecodeParam,ppAckData,pAckLen);
}


sint32 cm_cnm_aggr_create(const void *pDecodeParam,void **ppAckData,uint32 *pAckLen)
{
    if(NULL == pDecodeParam)
    {
        return CM_PARAM_ERR; 
    }
    return cm_cnm_aggr_req(CM_OMI_CMD_ADD,pDecodeParam,ppAckData,pAckLen);
}

sint32 cm_cnm_aggr_delete(const void *pDecodeParam,void **ppAckData,uint32 *pAckLen)
{
    if(NULL == pDecodeParam)
    {
        return CM_PARAM_ERR; 
    }
    return cm_cnm_aggr_req(CM_OMI_CMD_DELETE,pDecodeParam,ppAckData,pAckLen);
}


static sint32 cm_cnm_aggr_get_each(void *arg, sint8 **cols, uint32 col_num)
{
    cm_cnm_aggr_info_t* info = arg;
    if(col_num != 9)
    {
        CM_LOG_ERR(CM_MOD_CNM,"col_num %u",col_num);
        return CM_FAIL;
    }

    info->nid = cm_node_get_id();    
    CM_VSPRINTF(info->name,sizeof(info->name),"%s",cols[0]);
    CM_VSPRINTF(info->state,sizeof(info->state),"%s",cols[1]);
    if(strcmp(cols[2],"null") == 0)
    {
        CM_VSPRINTF(info->ip,sizeof(info->ip),"%s","");
    }else
    {
        CM_VSPRINTF(info->ip,sizeof(info->ip),"%s",cols[2]); 
    }
    CM_VSPRINTF(info->netmask,sizeof(info->netmask),"%s",cols[3]);
    CM_VSPRINTF(info->mtu,sizeof(info->mtu),"%s",cols[4]);
    if(strcmp(cols[5],"null") == 0)
    {
        CM_VSPRINTF(info->mac,sizeof(info->mac),"%s","");
    }else
    {
        CM_VSPRINTF(info->mac,sizeof(info->mac),"%s",cols[5]); 
    }
    info->policy= cm_cnm_get_enum(&CmOmiMapEnumPolicyType,cols[6],0);
    info->lacpactivity= cm_cnm_get_enum(&CmOmiMapEnumLacpactivityType,cols[7],0);
    info->lacptimer= cm_cnm_get_enum(&CmOmiMapEnumLacptimerType,cols[8],0);
    (void)cm_exec_tmout(info->adapter,CM_STRING_64,CM_CMT_REQ_TMOUT,
        "dladm show-link -o over %s 2>>/dev/null|awk 'NR>1{printf}'",info->name);

    return CM_OK;
}

sint32 cm_cnm_aggr_local_getbatch(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    sint32 iRet = CM_OK;
    sint8 cmd[CM_STRING_256] = {0};

    CM_VSPRINTF(cmd,CM_STRING_256,"%s getbatch",cm_cnm_aggr_sh);
    iRet = cm_cnm_exec_get_list(cmd,cm_cnm_aggr_get_each,offset,sizeof(cm_cnm_aggr_info_t),ppAck,&total);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]",iRet);
        return iRet;
    }

    *pAckLen = total * sizeof(cm_cnm_aggr_info_t);
    return CM_OK;
}

sint32 cm_cnm_aggr_local_count(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    uint64 cut = 0;
    cut = cm_exec_int("%s count",cm_cnm_aggr_sh);
    return cm_cnm_ack_uint64(cut, ppAck, pAckLen);
}

static sint32 cm_cnm_aggr_make_adapter(sint8* cmd,uint32 length,sint8* adapter)
{
    uint32 cut = 0;
    sint8* ptr = strtok(adapter,",");
    while(NULL != ptr)
    {
        cut = cm_exec_int("dladm show-phys %s 2>>/dev/null|wc -l",ptr);
        if(0 == cut)
        {
            return CM_PARAM_ERR;
        }
        
        CM_SNPRINTF_ADD(cmd,length," -d %s",ptr);
        ptr = strtok(NULL,",");
    }
    return CM_OK;
}

sint32 cm_cnm_aggr_local_create(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    const cm_cnm_decode_info_t *decode = param;
    const cm_cnm_aggr_info_t *info = (const cm_cnm_aggr_info_t *)decode->data;
    sint8 cmd[CM_STRING_256] = {0};
    uint32 length = (uint32)sizeof(cmd);
    sint8 adapter[CM_STRING_256] = {0};
    sint32 iRet = CM_OK;
    uint32 key = 0;
    const sint8 *lacpactivity = cm_cnm_get_enum_str(&CmOmiMapEnumLacpactivityType,info->lacpactivity);
    const sint8 *lacptimer = cm_cnm_get_enum_str(&CmOmiMapEnumLacptimerType,info->lacptimer);
    const sint8 *policy = cm_cnm_get_enum_str(&CmOmiMapEnumPolicyType,info->policy);
    CM_VSPRINTF(cmd,CM_STRING_256,"dladm create-aggr");

    CM_MEM_CPY(adapter,sizeof(adapter),info->adapter,sizeof(info->adapter));
    iRet = cm_cnm_aggr_make_adapter(cmd,length,adapter);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"adapter not exists");
        return CM_PARAM_ERR; 
    }

    if(CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_AGGR_POLICY))
    {
        CM_SNPRINTF_ADD(cmd,CM_STRING_256," -P %s",policy);
    }
    if(CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_AGGR_LACPACTIVITY))
    {
        CM_SNPRINTF_ADD(cmd,CM_STRING_256," -L %s",lacpactivity);
    }
    if(CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_AGGR_LACPTIMER))
    {
        CM_SNPRINTF_ADD(cmd,CM_STRING_256," -T %s",lacptimer);
    }

    key = cm_exec_int("dladm show-aggr -o LINK|egrep -v LINK|awk -F'r' '{print $2}'|awk 'BEGIN {cut=0}{if($1-cut==1) cut=$1} END{print cut}'");
    key = key + 1;

    CM_SNPRINTF_ADD(cmd,CM_STRING_256," %u",key);

    iRet = cm_system(cmd);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"%s fail",cmd);
        return CM_FAIL; 
    }

    if(CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_AGGR_IP))
    {
        if(CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_AGGR_NETMASK))
        {
            return cm_system("/var/cm/script/cm_cnm_phys_ip.sh create %s %s aggr%u",info->ip,info->netmask,key);
        }
        return cm_system("/var/cm/script/cm_cnm_phys_ip.sh create %s null aggr%u",info->ip,key);
    }
    
    return CM_OK;
}

sint32 cm_cnm_aggr_local_update(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    const cm_cnm_decode_info_t *decode = param;
    const cm_cnm_aggr_info_t *info = (const cm_cnm_aggr_info_t *)decode->data;
    sint8 cmd[CM_STRING_256] = {0};
    uint32 key = 0;
    sint8 netmask[CM_STRING_64] = {0};
    sint8 ip[CM_STRING_64] = {0};

    if(!CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_AGGR_POLICY)&&
        !CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_AGGR_LACPACTIVITY)&&
        !CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_AGGR_LACPTIMER)&&
        !CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_AGGR_IP)&&
        !CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_AGGR_NETMASK))
    {
        CM_LOG_ERR(CM_MOD_CNM,"aggr update not param");
        return CM_PARAM_ERR;
    }

    const sint8 *lacpactivity = cm_cnm_get_enum_str(&CmOmiMapEnumLacpactivityType,info->lacpactivity);
    const sint8 *lacptimer = cm_cnm_get_enum_str(&CmOmiMapEnumLacptimerType,info->lacptimer);
    const sint8 *policy = cm_cnm_get_enum_str(&CmOmiMapEnumPolicyType,info->policy);
    
    CM_VSPRINTF(cmd,CM_STRING_256,"dladm modify-aggr");
    if(CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_AGGR_POLICY))
    {
        CM_SNPRINTF_ADD(cmd,CM_STRING_256," -P %s",policy);
    }
    if(CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_AGGR_LACPACTIVITY))
    {
        CM_SNPRINTF_ADD(cmd,CM_STRING_256," -L %s",lacpactivity);
    }
    if(CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_AGGR_LACPTIMER))
    {
        CM_SNPRINTF_ADD(cmd,CM_STRING_256," -T %s",lacptimer);
    }

    key = cm_exec_int("%s getkey %s",cm_cnm_aggr_sh,info->name);
    CM_SNPRINTF_ADD(cmd,CM_STRING_256," %u",key);

    if(CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_AGGR_IP))
    {
        CM_VSPRINTF(ip,sizeof(ip),info->ip);
    }else
    {
        CM_VSPRINTF(ip,sizeof(ip),"null");
    }

    if(CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_AGGR_NETMASK))
    {
        CM_VSPRINTF(netmask,sizeof(netmask),info->netmask);
    }else
    {
        CM_VSPRINTF(netmask,sizeof(netmask),"null");
    }

    cm_system("%s update %s %s %s",cm_cnm_aggr_sh,ip,netmask,info->name);

    if(!CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_AGGR_POLICY)&&
        !CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_AGGR_LACPACTIVITY)&&
        !CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_AGGR_LACPTIMER))
    {
        return CM_OK;
    }
    
    return cm_system(cmd);
}

sint32 cm_cnm_aggr_local_delete(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    const cm_cnm_decode_info_t *decode = param;
    const cm_cnm_aggr_info_t *info = (const cm_cnm_aggr_info_t *)decode->data;
    sint32 iRet = CM_OK;
    uint32 cut = 0;
    uint32 key = 0;
    
    cut = cm_exec_int("dladm show-aggr|grep %s|wc -l",info->name);
    if(0 == cut)
    {
        CM_LOG_ERR(CM_MOD_CNM,"aggr not exists");
        return CM_ERR_NOT_EXISTS;
    }

    key = cm_exec_int("%s getkey %s",cm_cnm_aggr_sh,info->name);
    
    iRet = cm_system("ifconfig %s unplumb",info->name);
    if(CM_OK != iRet)
    {
        CM_LOG_WARNING(CM_MOD_CNM,"%s unplnmb ip fail",info->name);
    }

    cm_system("rm /etc/hostname.aggr%u",key);
    return cm_system("dladm delete-aggr %u",key);
}

static void cm_cnm_aggr_oplog_report(
    const sint8* sessionid, const void *pDecodeParam, uint32 alarmid)
{
    const cm_cnm_decode_info_t *req = pDecodeParam;
    const uint32 cnt = 9;

    if(NULL == pDecodeParam)
    {   
        cm_cnm_oplog_report(sessionid,alarmid,NULL,cnt,NULL);
    }    
    else    
    {       
        const cm_cnm_aggr_info_t *info = (const cm_cnm_aggr_info_t*)req->data;
        const sint8* nodename = cm_node_get_name(info->nid);
        cm_cnm_oplog_param_t params[9] = {
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_AGGR_NAME,strlen(info->name),info->name},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_AGGR_NID,strlen(nodename),nodename},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_AGGR_STATE,strlen(info->state),info->state},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_AGGR_IP,strlen(info->ip),info->ip},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_AGGR_NETMASK,strlen(info->netmask),info->netmask},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_AGGR_ADAPTER,strlen(info->adapter),info->adapter},
            {CM_OMI_DATA_INT,CM_OMI_FIELD_AGGR_POLICY,sizeof(info->policy),&info->policy},
            {CM_OMI_DATA_INT,CM_OMI_FIELD_AGGR_LACPACTIVITY,sizeof(info->lacpactivity),&info->lacpactivity},
            {CM_OMI_DATA_INT,CM_OMI_FIELD_AGGR_LACPTIMER,sizeof(info->lacptimer),&info->lacptimer},
        };
        
        cm_cnm_oplog_report(sessionid,alarmid,params,cnt,&req->set);
    }    
    
    return;
}    

void cm_cnm_aggr_oplog_update(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK)? CM_ALARM_LOG_AGGR_UPDATE_OK : CM_ALARM_LOG_AGGR_UPDATE_FAIL;
    
    cm_cnm_aggr_oplog_report(sessionid,pDecodeParam,alarmid);
    return;
}
void cm_cnm_aggr_oplog_create(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{  
    uint32 alarmid = (Result == CM_OK)? CM_ALARM_LOG_AGGR_CREATR_OK : CM_ALARM_LOG_AGGR_CREATR_FAIL;
    
    cm_cnm_aggr_oplog_report(sessionid,pDecodeParam,alarmid);
    return;
}

void cm_cnm_aggr_oplog_delete(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK)? CM_ALARM_LOG_AGGR_DELETE_OK : CM_ALARM_LOG_AGGR_DELETE_FAIL;
    const cm_cnm_decode_info_t *req = pDecodeParam;
    const uint32 cnt = 2;
    
    if(NULL == pDecodeParam)
    {   
        cm_cnm_oplog_report(sessionid,alarmid,NULL,cnt,NULL);
    }    
    else    
    {       
        const cm_cnm_aggr_info_t *info = (const cm_cnm_aggr_info_t*)req->data;
        const sint8* nodename = cm_node_get_name(info->nid);
        cm_cnm_oplog_param_t params[2] = {
              {CM_OMI_DATA_STRING,CM_OMI_FIELD_AGGR_NAME,strlen(info->name),info->name},
              {CM_OMI_DATA_STRING,CM_OMI_FIELD_AGGR_NID,strlen(nodename),nodename},
        };
        cm_cnm_oplog_report(sessionid,alarmid,params,cnt,&req->set);
    }    
      
    return;
}    

 
