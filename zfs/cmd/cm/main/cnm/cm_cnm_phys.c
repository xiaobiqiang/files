/******************************************************************************
 *          Copyright (c) 2018 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cnm_phys.c
 * author     : wbn
 * create date: 2018年3月16日
 * description: TODO:
 *
 *****************************************************************************/

#include "cm_cnm_phys.h"
#include "cm_node.h"
#include "cm_omi.h"
#include "cm_log.h"
#include "cm_cmt.h"

#define cm_cnm_phys_req(cmd,param,ppAck,plen)\
    cm_cnm_request_comm(CM_OMI_OBJECT_PHYS,cmd,sizeof(cm_cnm_phys_info_t),param,ppAck,plen)

extern const cm_omi_map_enum_t CmOmiMapEnumPortStateType;
extern const cm_omi_map_enum_t CmOmiMapEnumPortDuplexType;

const sint8* cm_cnm_phys_ip_sh = "/var/cm/script/cm_cnm_phys_ip.sh";


sint32 cm_cnm_phys_init(void)
{
    return CM_OK;
}

static sint32 cm_cnm_phys_decode_ext(
    const cm_omi_obj_t ObjParam,void* data,cm_omi_field_flag_t *set)
{
    sint32 iRet = CM_OK;
    cm_cnm_phys_info_t *info = data;
    cm_cnm_decode_param_t param_str[] = 
    {
        {CM_OMI_FIELD_PHYS_NAME,sizeof(info->name),info->name,NULL},
    };

    cm_cnm_decode_param_t param_num[] = 
    {
        {CM_OMI_FIELD_PHYS_MTU,sizeof(info->mtu),&info->mtu,NULL},
    };
    
    iRet = cm_cnm_decode_str(ObjParam,param_str,
        sizeof(param_str)/sizeof(cm_cnm_decode_param_t),set);
    if(iRet != CM_OK)
    {
        return iRet;
    }
    iRet = cm_cnm_decode_num(ObjParam,param_num,
        sizeof(param_num)/sizeof(cm_cnm_decode_param_t),set);
    
    return CM_OK;
}


sint32 cm_cnm_phys_decode(const cm_omi_obj_t ObjParam, void
**ppDecodeParam)
{
    return cm_cnm_decode_comm(ObjParam,sizeof(cm_cnm_phys_info_t),
        cm_cnm_phys_decode_ext,ppDecodeParam);
}

static void cm_cnm_phys_encode_each(cm_omi_obj_t item,void *eachdata,void *arg)
{
    cm_omi_field_flag_t *field = arg;
    cm_cnm_phys_info_t *info = eachdata;
    cm_cnm_map_value_str_t cols_str[] = 
    {
        {CM_OMI_FIELD_PHYS_NAME,   info->name},
        {CM_OMI_FIELD_PHYS_MAC,    info->mac},
    };
    cm_cnm_map_value_num_t cols_num[] = 
    {   
        {CM_OMI_FIELD_PHYS_NID,  info->nid},
        {CM_OMI_FIELD_PHYS_STATE,  info->state},
        {CM_OMI_FIELD_PHYS_SPEED,  info->speed},
        {CM_OMI_FIELD_PHYS_DUPLEX, info->duplex},
        {CM_OMI_FIELD_PHYS_MTU,    info->mtu},
    };
    cm_cnm_encode_str(item,field,cols_str,sizeof(cols_str)/sizeof(cm_cnm_map_value_str_t));
    cm_cnm_encode_num(item,field,cols_num,sizeof(cols_num)/sizeof(cm_cnm_map_value_num_t)); 
    return;
}

cm_omi_obj_t cm_cnm_phys_encode(const void *pDecodeParam, void *pAckData, uint32 AckLen)
{
    return cm_cnm_encode_comm_ext(pDecodeParam,pAckData,AckLen,
        sizeof(cm_cnm_phys_info_t),cm_cnm_phys_encode_each);
}

sint32 cm_cnm_phys_get(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_cnm_phys_req(CM_OMI_CMD_GET,pDecodeParam,ppAckData,pAckLen);
}
sint32 cm_cnm_phys_get_batch(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_cnm_phys_req(CM_OMI_CMD_GET_BATCH,pDecodeParam,ppAckData,pAckLen);
}
sint32 cm_cnm_phys_count(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_cnm_phys_req(CM_OMI_CMD_COUNT,pDecodeParam,ppAckData,pAckLen);
}

sint32 cm_cnm_phys_update(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    if(NULL == pDecodeParam)
    {
         return CM_PARAM_ERR;
    }
    return cm_cnm_phys_req(CM_OMI_CMD_MODIFY,pDecodeParam,ppAckData,pAckLen);
}


static sint32 cm_cnm_phys_local_get_each(void *arg, sint8 **cols, uint32 col_num)
{
    cm_cnm_phys_info_t *info = arg;
    const uint32 def_num = 7;
    /*
    LINK         MEDIA                STATE      SPEED  DUPLEX    DEVICE
    ixgbe0       Ethernet             up         10000  full      ixgbe0
    ixgbe1       Ethernet             up         10000  full      ixgbe1
    */
    if(col_num != def_num)
    {
        CM_LOG_ERR(CM_MOD_CNM,"col_num[%u] def_num[%u]",col_num,def_num);
        return CM_FAIL;
    }
    info->nid = cm_node_get_id();
    CM_VSPRINTF(info->name,sizeof(info->name),"%s",cols[0]);
    info->state = cm_cnm_get_enum(&CmOmiMapEnumPortStateType,cols[2],CM_PORT_STATE_UNKNOW);
    info->speed = (uint32)atoi(cols[3]);
    info->duplex = cm_cnm_get_enum(&CmOmiMapEnumPortDuplexType,cols[4],CM_PORT_DUPLEX_UNKNOW);
    if(atoi(cols[5]) != 0)
    {
        info->mtu = atoi(cols[5]);
    }
    if(strcmp(cols[6],"null") != 0)
    {
        CM_VSPRINTF(info->mac,sizeof(info->name),"%s",cols[6]);
    }
    return CM_OK;        
}

sint32 cm_cnm_phys_local_getbatch(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    sint32 iRet = CM_OK;
    sint8 cmd[CM_STRING_128] = {0};

    CM_VSPRINTF(cmd,sizeof(cmd),"%s physgetbatch",cm_cnm_phys_ip_sh);
    iRet = cm_cnm_exec_get_list(cmd,cm_cnm_phys_local_get_each,
        (uint32)offset,sizeof(cm_cnm_phys_info_t),ppAck,&total);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]",iRet);
        return iRet;
    }
    *pAckLen = total * sizeof(cm_cnm_phys_info_t);
    return CM_OK;
}

sint32 cm_cnm_phys_local_get(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    sint32 iRet = CM_OK;
    const cm_cnm_decode_info_t *req = param;
    const cm_cnm_phys_info_t *info = NULL;
    cm_cnm_phys_info_t *data = NULL;

    if(NULL == req)
    {
        return CM_PARAM_ERR;
    }
    info = (const cm_cnm_phys_info_t *)req->data;

    data = CM_MALLOC(sizeof(cm_cnm_phys_info_t));
    if(NULL == data)
    {
        CM_LOG_ERR(CM_MOD_CNM,"malloc fail");
        return CM_FAIL;
    }

    iRet = cm_cnm_exec_get_col(cm_cnm_phys_local_get_each,data,
        "%s physget %s",cm_cnm_phys_ip_sh,info->name);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]",iRet);
        CM_FREE(data);
        return CM_OK;
    }
    *ppAck = data;
    *pAckLen = sizeof(cm_cnm_phys_info_t);
    return CM_OK;
}

sint32 cm_cnm_phys_local_count(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    uint64 cnt = cm_exec_int("%s physcount",cm_cnm_phys_ip_sh);
    return cm_cnm_ack_uint64(cnt,ppAck,pAckLen);
}

sint32 cm_cnm_phys_local_update(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    cm_cnm_decode_info_t *decode = param;
    cm_cnm_phys_info_t *info = (cm_cnm_phys_info_t *)decode->data;

    return cm_system("%s physupdate %s %d",cm_cnm_phys_ip_sh,info->name,info->mtu);
}

sint32 cm_cnm_phys_cbk_cmt(
 void *pMsg, uint32 Len, void** ppAck, uint32 *pAckLen)
{
    return CM_ERR_NOT_SUPPORT;
}
/*********************************************************
                     route(路由管理)
**********************************************************/

#define cm_cnm_route_req(cmd,param,ppAck,plen)\
    cm_cnm_request_comm(CM_OMI_OBJECT_ROUTE,cmd,sizeof(cm_cnm_route_info_t),param,ppAck,plen)
const sint8* cm_cnm_route_sh = "/var/cm/script/cm_cnm_route.sh";



sint32 cm_cnm_route_init(void)
{
    return CM_OK;
}

static sint32 cm_cnm_route_decode_ext(
    const cm_omi_obj_t ObjParam,void* data,cm_omi_field_flag_t* set)
{
    sint32 iRet = CM_OK;
    cm_cnm_route_info_t* info = data;
    cm_cnm_decode_param_t param_str[] = 
    {
        {CM_OMI_FIELD_ROUTE_DESTINATION,sizeof(info->destination),info->destination,NULL},
        {CM_OMI_FIELD_ROUTE_NETMASK,sizeof(info->netmask),info->netmask,NULL},
        {CM_OMI_FIELD_ROUTE_GETEWAY,sizeof(info->geteway),info->geteway,NULL},
    };
    cm_cnm_decode_param_t param_num[] =
    {
        {CM_OMI_FIELD_ROUTE_NID,sizeof(info->nid),&info->nid,NULL},
    };
    
    iRet = cm_cnm_decode_str(ObjParam,param_str,
        sizeof(param_str)/sizeof(cm_cnm_decode_param_t),set);
    iRet = cm_cnm_decode_num(ObjParam,param_num,
        sizeof(param_num)/sizeof(cm_cnm_decode_param_t),set);

    return CM_OK;
}

sint32 cm_cnm_route_decode(const cm_omi_obj_t ObjParam,void **ppDecodeParam)
{
    return cm_cnm_decode_comm(ObjParam,sizeof(cm_cnm_route_info_t),cm_cnm_route_decode_ext,ppDecodeParam);
}

static void cm_cnm_route_encode_each(cm_omi_obj_t item,void* eachdata,void* arg)
{
    cm_omi_field_flag_t *field = arg;
    cm_cnm_route_info_t *info = eachdata;

    cm_cnm_map_value_str_t cols_str[] = 
    {
        {CM_OMI_FIELD_ROUTE_DESTINATION,info->destination},
        {CM_OMI_FIELD_ROUTE_NETMASK,info->netmask},
        {CM_OMI_FIELD_ROUTE_GETEWAY,info->geteway},
    };
    cm_cnm_map_value_num_t cols_num[] = 
    {
        {CM_OMI_FIELD_ROUTE_NID,info->nid},
    };
        
    cm_cnm_encode_str(item,field,cols_str,sizeof(cols_str)/sizeof(cm_cnm_map_value_str_t));
    cm_cnm_encode_num(item,field,cols_num,sizeof(cols_num)/sizeof(cm_cnm_map_value_num_t)); 
    return;
}

cm_omi_obj_t cm_cnm_route_encode(const void *pDecodeParam,void *pAckData,uint32 AckLen)
{
    return cm_cnm_encode_comm_ext(pDecodeParam,pAckData,AckLen,
        sizeof(cm_cnm_route_info_t),cm_cnm_route_encode_each);
}

static sint32 cm_cnm_route_get_each(void *arg, sint8 **cols, uint32 col_num)
{
    cm_cnm_route_info_t *info = arg;
    if(col_num != 3)
    {
        CM_LOG_ERR(CM_MOD_CNM,"col_num %u",col_num);
        return CM_FAIL;
    }

    info->nid = cm_node_get_id();
    CM_VSPRINTF(info->destination,sizeof(info->destination),"%s",cols[0]);
    CM_VSPRINTF(info->netmask,sizeof(info->netmask),"%s",cols[1]);
    CM_VSPRINTF(info->geteway,sizeof(info->geteway),"%s",cols[2]);

    return CM_OK;
}

sint32 cm_cnm_route_getbatch(const void * pDecodeParam,void **ppAckData,uint32 *pAckLen)
{
    return cm_cnm_route_req(CM_OMI_CMD_GET_BATCH,pDecodeParam,ppAckData,pAckLen);
}

sint32 cm_cnm_route_count(const void *pDecodeParam,void **ppAckData,uint32 *pAckLen)
{
    if(NULL == pDecodeParam)
    {
        return CM_PARAM_ERR; 
    }
    return cm_cnm_route_req(CM_OMI_CMD_COUNT,pDecodeParam,ppAckData,pAckLen); 
}

sint32 cm_cnm_route_create(const void *pDecodeParam,void **ppAckData,uint32 *pAckLen)
{
    if(NULL == pDecodeParam)
    {
        return CM_PARAM_ERR; 
    }
    return cm_cnm_route_req(CM_OMI_CMD_ADD,pDecodeParam,ppAckData,pAckLen);
}

sint32 cm_cnm_route_delete(const void *pDecodeParam,void **ppAckData,uint32 *pAckLen)
{
    if(NULL == pDecodeParam)
    {
        return CM_PARAM_ERR; 
    }
    return cm_cnm_route_req(CM_OMI_CMD_DELETE,pDecodeParam,ppAckData,pAckLen);
}

sint32 cm_cnm_route_local_getbatch(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    const cm_cnm_decode_info_t *decode = param;
    const cm_cnm_route_info_t *info = (const cm_cnm_route_info_t *)decode->data;
    sint32 iRet = CM_OK;
    sint8 cmd[CM_STRING_128] = {0};

    CM_VSPRINTF(cmd,CM_STRING_128,"%s getbatch",cm_cnm_route_sh);
    iRet = cm_cnm_exec_get_list(cmd,cm_cnm_route_get_each,offset,sizeof(cm_cnm_route_info_t),ppAck,&total);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]",iRet);
        return iRet;
    }

    *pAckLen = total * sizeof(cm_cnm_route_info_t);
    return CM_OK;
}

sint32 cm_cnm_route_local_count(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    uint64 cut = 0;
    cut = cm_exec_int("%s count",cm_cnm_route_sh);

    return cm_cnm_ack_uint64(cut,ppAck,pAckLen);
}

sint32 cm_cnm_route_local_create(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    sint32 iRet = CM_OK;
    const cm_cnm_decode_info_t *decode = param;
    const cm_cnm_route_info_t *info = (const cm_cnm_route_info_t *)decode->data;
    
    iRet = cm_system("%s insert %s %s %s",cm_cnm_route_sh,
        info->destination,info->netmask,info->geteway);
    if (iRet != CM_OK)
    {
        return CM_FAIL;
    }
    return CM_OK;
}

sint32 cm_cnm_route_local_delete(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    const cm_cnm_decode_info_t *decode = param;
    const cm_cnm_route_info_t *info = (const cm_cnm_route_info_t *)decode->data;

    return cm_system("%s delete %s %s %s",cm_cnm_route_sh,
        info->destination,info->netmask,info->geteway);
}

static void cm_cnm_route_oplog_report(
    const sint8* sessionid, const void *pDecodeParam, uint32 alarmid)
{
    const cm_cnm_decode_info_t *req = pDecodeParam;
    const uint32 cnt = 4;

    if(NULL == pDecodeParam)
    {   
        cm_cnm_oplog_report(sessionid,alarmid,NULL,cnt,NULL);
    }    
    else    
    {       
        const cm_cnm_route_info_t *info = (const cm_cnm_route_info_t*)req->data;
        
        cm_cnm_oplog_param_t params[4] = {
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_ROUTE_DESTINATION,strlen(info->destination),info->destination},
            {CM_OMI_DATA_INT,CM_OMI_FIELD_ROUTE_NID,sizeof(info->nid),&info->nid},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_ROUTE_NETMASK,strlen(info->netmask),info->netmask},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_ROUTE_GETEWAY,strlen(info->geteway),info->geteway},
        };
        
        cm_cnm_oplog_report(sessionid,alarmid,params,cnt,&req->set);
    }    
    
    return;
}    

void cm_cnm_route_oplog_create(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK)? CM_ALARM_LOG_ROUTE_CREATE_OK : CM_ALARM_LOG_ROUTE_CREATE_FAIL;
    
    cm_cnm_route_oplog_report(sessionid,pDecodeParam,alarmid);
    return;
}

void cm_cnm_route_oplog_delete(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK)? CM_ALARM_LOG_ROUTE_DELETE_OK : CM_ALARM_LOG_ROUTE_DELETE_FAIL;
   
    cm_cnm_route_oplog_report(sessionid,pDecodeParam,alarmid);
    return; 
}   


/****************************************************
                    phys_ip
*****************************************************/

#define cm_cnm_phys_ip_req(cmd,param,ppAck,plen)\
    cm_cnm_request_comm(CM_OMI_OBJECT_PHYS_IP,cmd,sizeof(cm_cnm_phys_ip_info_t),param,ppAck,plen)
//const sint8* cm_cnm_phys_ip_sh = "/var/cm/script/cm_cnm_phys_ip.sh";


sint32 cm_cnm_phys_ip_init(void)
{
    return CM_OK;
}

sint32 cm_cnm_phys_ip_getbatch(const void * pDecodeParam,void **ppAckData,uint32 *pAckLen)
{
    if(NULL == pDecodeParam)
    {
        return CM_PARAM_ERR; 
    }
    return cm_cnm_phys_ip_req(CM_OMI_CMD_GET_BATCH,pDecodeParam,ppAckData,pAckLen);
}

sint32 cm_cnm_phys_ip_count(const void *pDecodeParam,void **ppAckData,uint32 *pAckLen)
{
    if(NULL == pDecodeParam)
    {
        return CM_PARAM_ERR; 
    }
    return cm_cnm_phys_ip_req(CM_OMI_CMD_COUNT,pDecodeParam,ppAckData,pAckLen); 
}

sint32 cm_cnm_phys_ip_create(const void * pDecodeParam,void * * ppAckData,uint32 * pAckLen)
{
    if(NULL == pDecodeParam)
    {
        return CM_PARAM_ERR; 
    }
    return cm_cnm_phys_ip_req(CM_OMI_CMD_ADD,pDecodeParam,ppAckData,pAckLen); 
}

sint32 cm_cnm_phys_ip_delete(const void * pDecodeParam,void * * ppAckData,uint32 * pAckLen)
{
    if(NULL == pDecodeParam)
    {
        return CM_PARAM_ERR; 
    }
    return cm_cnm_phys_ip_req(CM_OMI_CMD_DELETE,pDecodeParam,ppAckData,pAckLen); 
}


static sint32 cm_cnm_phys_ip_decode_ext(
    const cm_omi_obj_t ObjParam,void* data,cm_omi_field_flag_t* set)
{
    sint32 iRet = CM_OK;
    cm_cnm_phys_ip_info_t *info = data;
    cm_cnm_decode_param_t param_str[] = 
    {
        {CM_OMI_FIELD_PHYS_IP_ADAPTER,sizeof(info->adapter),info->adapter,NULL},
        {CM_OMI_FIELD_PHYS_IP_IP,sizeof(info->ip),info->ip,NULL},
        {CM_OMI_FIELD_PHYS_IP_NETMASK,sizeof(info->netmask),info->netmask,NULL},
    };
    cm_cnm_decode_param_t param_num[] =
    {
        {CM_OMI_FIELD_PHYS_IP_NID,sizeof(info->nid),&info->nid,NULL},
    };
    
    iRet = cm_cnm_decode_str(ObjParam,param_str,
        sizeof(param_str)/sizeof(cm_cnm_decode_param_t),set);
    iRet = cm_cnm_decode_num(ObjParam,param_num,
        sizeof(param_num)/sizeof(cm_cnm_decode_param_t),set);

    return CM_OK;
}


sint32 cm_cnm_phys_ip_decode(const cm_omi_obj_t ObjParam,void **ppDecodeParam)
{
    return cm_cnm_decode_comm(ObjParam,sizeof(cm_cnm_phys_ip_info_t),cm_cnm_phys_ip_decode_ext,ppDecodeParam);
}

static void cm_cnm_phys_ip_encode_each(cm_omi_obj_t item,void* eachdata,void* arg)
{
    cm_omi_field_flag_t *field = arg;
    cm_cnm_phys_ip_info_t *info = eachdata;

    cm_cnm_map_value_str_t cols_str[] = 
    {
        {CM_OMI_FIELD_PHYS_IP_ADAPTER,info->adapter},
        {CM_OMI_FIELD_PHYS_IP_IP,info->ip},
        {CM_OMI_FIELD_PHYS_IP_NETMASK,info->netmask},
    };

    
    cm_cnm_encode_str(item,field,cols_str,sizeof(cols_str)/sizeof(cm_cnm_map_value_str_t)); 
    return;
}

cm_omi_obj_t cm_cnm_phys_ip_encode(const void *pDecodeParam,void *pAckData,uint32 AckLen)
{
    return cm_cnm_encode_comm_ext(pDecodeParam,pAckData,AckLen,
        sizeof(cm_cnm_phys_ip_info_t),cm_cnm_phys_ip_encode_each);
}

static sint32 cm_cnm_phys_ip_get_each(void *arg, sint8 **cols, uint32 col_num)
{
    cm_cnm_phys_ip_info_t* info = arg;
    if(col_num != 3)
    {
        CM_LOG_ERR(CM_MOD_CNM,"col_num %u",col_num);
        return CM_FAIL;
    }

    CM_VSPRINTF(info->adapter,sizeof(info->adapter),"%s",cols[0]);
    CM_VSPRINTF(info->ip,sizeof(info->ip),"%s",cols[1]);
    CM_VSPRINTF(info->netmask,sizeof(info->netmask),"%s",cols[2]);

    return CM_OK;
}


sint32 cm_cnm_phys_ip_local_getbatch(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    cm_cnm_decode_info_t* decode = param;
    cm_cnm_phys_ip_info_t* info = (cm_cnm_phys_ip_info_t*)decode->data;
    sint8 cmd[CM_STRING_128] = {0};
    sint32 iRet = CM_OK;

    if(CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_PHYS_IP_ADAPTER))
    {
        CM_VSPRINTF(cmd,CM_STRING_128,"%s getbatch|grep %s",cm_cnm_phys_ip_sh,info->adapter);
    }else
    {
        CM_VSPRINTF(cmd,CM_STRING_128,"%s getbatch",cm_cnm_phys_ip_sh);
    }
    iRet = cm_cnm_exec_get_list(cmd,cm_cnm_phys_ip_get_each,offset,sizeof(cm_cnm_phys_ip_info_t),ppAck,&total);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]",iRet);
        return iRet;
    }

    *pAckLen = total * sizeof(cm_cnm_phys_ip_info_t);
    return CM_OK;
}

sint32 cm_cnm_phys_ip_local_count(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    uint64 cut = 0;
    cut = cm_exec_int("%s count",cm_cnm_phys_ip_sh);

    return cm_cnm_ack_uint64(cut,ppAck,pAckLen);
}

sint32 cm_cnm_phys_ip_local_create(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    cm_cnm_decode_info_t *decode = param;
    cm_cnm_phys_ip_info_t *info = (cm_cnm_phys_ip_info_t *)decode->data;
    uint32 key = 0;
    sint32 iRet = CM_OK;
    sint8 adapter[CM_STRING_128] = {0};

    key = cm_exec_int("ifconfig -a | grep %s |wc -l",info->ip);
    if(0 != key)
    {
        CM_LOG_ERR(CM_MOD_CNM,"ip %s exists",info->ip);
        return CM_ERR_ALREADY_EXISTS;
    }
    
    key = cm_exec_int("ifconfig %s|grep 'inet '|wc -l",info->adapter);
    if(0 == key)
    {
        if(CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_PHYS_IP_NETMASK))
        {
            return cm_system("%s create %s %s %s",cm_cnm_phys_ip_sh,info->ip,info->netmask,info->adapter);
        }
        return cm_system("%s create %s null %s",cm_cnm_phys_ip_sh,info->ip,info->adapter);
    }
    
    key = cm_exec_int("ifconfig -a|grep %s|awk -F':' 'NF==2{print $1}  NF==3{print $1\":\"$2}'|awk -F':' '{print $2}'|awk 'BEGIN {cut=0}{if($1-cut==1) cut=$1} END{printf cut}'",
        info->adapter);
    CM_VSPRINTF(adapter,sizeof(adapter),"%s:%u",info->adapter,key+1);

    if(CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_PHYS_IP_NETMASK))
    {
        
        return cm_system("%s create %s %s %s",cm_cnm_phys_ip_sh,info->ip,info->netmask,adapter);
    }

    return cm_system("%s create %s null %s",cm_cnm_phys_ip_sh,info->ip,adapter);
}

sint32 cm_cnm_phys_ip_local_delete(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    cm_cnm_decode_info_t *decode = param;
    cm_cnm_phys_ip_info_t *info = (cm_cnm_phys_ip_info_t *)decode->data;
    sint32 iRet = CM_OK;
    if(CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_PHYS_IP_ADAPTER))
    {
    iRet = cm_system(CM_SHELL_EXEC" cm_phys_ip_delete %s %s",info->ip,info->adapter);
        if(CM_OK != iRet)
        {
            CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]",iRet);
            return iRet;
        }
    }
    else{
        iRet = cm_system(CM_SHELL_EXEC" cm_phys_ip_delete %s",info->ip);
        if(CM_OK != iRet)
        {
            CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]",iRet);
            return iRet;
        }

    }
    return CM_OK;
}

static void cm_cnm_phys_ip_oplog_report(
    const sint8* sessionid, const void *pDecodeParam, uint32 alarmid)
{
    const cm_cnm_decode_info_t *req = pDecodeParam;
    const uint32 cnt = 4;

    if(NULL == pDecodeParam)
    {   
        cm_cnm_oplog_report(sessionid,alarmid,NULL,cnt,NULL);
    }    
    else    
    {       
        const cm_cnm_phys_ip_info_t *info = (const cm_cnm_phys_ip_info_t*)req->data;
        
        cm_cnm_oplog_param_t params[4] = {
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_PHYS_IP_ADAPTER,strlen(info->adapter),info->adapter},
            {CM_OMI_DATA_INT,CM_OMI_FIELD_PHYS_IP_NID,sizeof(info->nid),&info->nid},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_PHYS_IP_IP,strlen(info->ip),info->ip},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_PHYS_IP_NETMASK,strlen(info->netmask),info->netmask},
        };
        
        cm_cnm_oplog_report(sessionid,alarmid,params,cnt,&req->set);
    }    
    
    return;
}    

void cm_cnm_phys_ip_oplog_create(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK)? CM_ALARM_LOG_PHYS_IP_CREATE_OK : CM_ALARM_LOG_PHYS_IP_CREATE_FAIL;
    
    cm_cnm_phys_ip_oplog_report(sessionid,pDecodeParam,alarmid);
    return;
}

void cm_cnm_phys_ip_oplog_delete(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK)? CM_ALARM_LOG_PHYS_IP_DELETE_OK : CM_ALARM_LOG_PHYS_IP_DELETE_FAIL;
   
    cm_cnm_phys_ip_oplog_report(sessionid,pDecodeParam,alarmid);
    return; 
}

/***********************************fcinfo*************************************/

sint32 cm_cnm_fcinfo_init(void)
{
    return CM_OK;
}

#define cm_cnm_fcinfo_request(cmd,param,ppAck,plen) \
    cm_cnm_request_comm(CM_OMI_OBJECT_FCINFO,cmd,sizeof(cm_cnm_fcinfo_info_t),param,ppAck,plen)

static sint32 cm_cnm_fcinfo_decode_ext(
    const cm_omi_obj_t ObjParam,void* data,cm_omi_field_flag_t *set)
{
    sint32 iRet = CM_OK;
    cm_cnm_fcinfo_info_t *info = data;
    cm_cnm_decode_param_t param_str[] = 
    {
        {CM_OMI_FIELD_FCINFO_PORT_WWN,sizeof(info->port_wwn),info->port_wwn,NULL},
        {CM_OMI_FIELD_FCINFO_PORT_MODE,sizeof(info->port_mode),info->port_mode,NULL},
        {CM_OMI_FIELD_FCINFO_DRIVER_NAME,sizeof(info->driver_name),info->driver_name,NULL},
        {CM_OMI_FIELD_FCINFO_STATE,sizeof(info->state),info->state,NULL},
        {CM_OMI_FIELD_FCINFO_SPEED,sizeof(info->speed),info->speed,NULL},
        {CM_OMI_FIELD_FCINFO_CURSPEED,sizeof(info->cur_speed),info->cur_speed,NULL},
    };
    cm_cnm_decode_param_t param_num[] = 
    {
        {CM_OMI_FIELD_FCINFO_NID,sizeof(info->nid),&info->nid,NULL},
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
sint32 cm_cnm_fcinfo_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam)
{
    return cm_cnm_decode_comm(ObjParam,sizeof(cm_cnm_fcinfo_info_t),
        cm_cnm_fcinfo_decode_ext,ppDecodeParam);
}
static void cm_cnm_fcinfo_encode_each(cm_omi_obj_t item,void *eachdata,void *arg)
{
    cm_omi_field_flag_t *field = arg;
    cm_cnm_fcinfo_info_t *info = eachdata;
    cm_cnm_map_value_str_t cols_str[] = 
    {
        {CM_OMI_FIELD_FCINFO_PORT_WWN,      info->port_wwn},
        {CM_OMI_FIELD_FCINFO_PORT_MODE,     info->port_mode},
        {CM_OMI_FIELD_FCINFO_DRIVER_NAME,   info->driver_name},
        {CM_OMI_FIELD_FCINFO_STATE,         info->state},
        {CM_OMI_FIELD_FCINFO_SPEED,         info->speed},
        {CM_OMI_FIELD_FCINFO_CURSPEED,      info->cur_speed},
    };
    cm_cnm_map_value_num_t cols_num[] = 
    {
        {CM_OMI_FIELD_FCINFO_NID,          info->nid},
    };
    cm_cnm_encode_str(item,field,cols_str,sizeof(cols_str)/sizeof(cm_cnm_map_value_str_t));
    cm_cnm_encode_num(item,field,cols_num,sizeof(cols_num)/sizeof(cm_cnm_map_value_num_t)); 
    return;
}
cm_omi_obj_t cm_cnm_fcinfo_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen)
{
    return cm_cnm_encode_comm_ext(pDecodeParam,pAckData,AckLen,
        sizeof(cm_cnm_fcinfo_info_t),cm_cnm_fcinfo_encode_each);
}
static sint32 cm_cnm_fcinfo_local_get_each(void *arg, sint8 **cols, uint32 col_num)
{
    cm_cnm_fcinfo_info_t *info = arg;
    const uint32 def_num = 6;       
    if(def_num != col_num)
    {
        CM_LOG_WARNING(CM_MOD_CNM,"col_num[%u] def_num[%u]",col_num,def_num);
        return CM_FAIL;
    }
    CM_MEM_ZERO(info,sizeof(cm_cnm_fcinfo_info_t));
    CM_VSPRINTF(info->port_wwn,sizeof(info->port_wwn),"%s",cols[0]);    
    CM_VSPRINTF(info->port_mode,sizeof(info->port_mode),"%s",cols[1]);
    CM_VSPRINTF(info->driver_name,sizeof(info->driver_name),"%s",cols[2]);
    CM_VSPRINTF(info->state,sizeof(info->state),"%s",cols[3]);
    CM_VSPRINTF(info->speed,sizeof(info->speed),"%s",cols[4]);
    if(NULL != strstr(cols[5],"not"))
    {
        CM_VSPRINTF(info->cur_speed,sizeof(info->cur_speed),"%s established",cols[5]);    
    } 
    else
    {
        CM_VSPRINTF(info->cur_speed,sizeof(info->cur_speed),"%s",cols[5]);
    }
    info->nid = cm_node_get_id();
    return CM_OK;
}
sint32  cm_cnm_fcinfo_local_getbatch(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    sint32 iRet = CM_OK;
    iRet = cm_cnm_exec_get_list("/var/cm/script/cm_shell_exec.sh cm_cnm_fcinfo_getbatch",
        cm_cnm_fcinfo_local_get_each,(uint32)offset,sizeof(cm_cnm_fcinfo_info_t),ppAck,&total);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]",iRet);
        return iRet;
    }
    *pAckLen = total * sizeof(cm_cnm_fcinfo_info_t);
    return CM_OK;
}    
sint32 cm_cnm_fcinfo_local_count(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    uint64 cnt = 0;
    cnt = cm_exec_int("fcinfo hba-port|grep -w 'HBA Port WWN'|wc -l");
    return cm_cnm_ack_uint64(cnt,ppAck,pAckLen);
}
sint32 cm_cnm_fcinfo_count(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_cnm_fcinfo_request(CM_OMI_CMD_COUNT,pDecodeParam,ppAckData,pAckLen);
}    
sint32 cm_cnm_fcinfo_getbatch(const void * pDecodeParam,void **ppAckData,uint32 *pAckLen)
{
    return cm_cnm_fcinfo_request(CM_OMI_CMD_GET_BATCH,pDecodeParam,ppAckData,pAckLen);
}

/***********************************ipfilter***********************************/
const sint8* g_cm_cnm_ipf_script = "/var/cm/script/cm_cnm_ipf.sh";
extern const cm_omi_map_enum_t CmOmiMapIpfStatusEnumBoolType;
extern const cm_omi_map_enum_t CmOmiMapIpfopEnumBoolType;
extern const cm_omi_map_enum_t CmOmiMapIpfoptionEnumBoolType;
sint32 cm_cnm_ipf_init(void)
{
    return CM_OK;
}
#define cm_cnm_ipf_request(cmd,param,ppAck,plen) \
    cm_cnm_request_comm(CM_OMI_OBJECT_IPF,cmd,sizeof(cm_cnm_ipf_info_t),param,ppAck,plen)
static sint32 cm_cnm_ipf_decode_check_status(void *val)
{
    uint8 status = *((uint8*)val);
    const sint8* str = cm_cnm_get_enum_str(&CmOmiMapIpfStatusEnumBoolType,status);
    if(NULL == str)
    {
        CM_LOG_ERR(CM_MOD_CNM,"status[%u]",status);
        return CM_PARAM_ERR;
    }
    return CM_OK;
}
static sint32 cm_cnm_ipf_decode_check_operate(void *val)
{
    uint8 operate = *((uint8*)val);
    const sint8* str = cm_cnm_get_enum_str(&CmOmiMapIpfopEnumBoolType,operate);
    if(NULL == str)
    {
        CM_LOG_ERR(CM_MOD_CNM,"operate[%u]",operate);
        return CM_PARAM_ERR;
    }
    return CM_OK;
}
static sint32 cm_cnm_ipf_decode_ext(
    const cm_omi_obj_t ObjParam,void* data,cm_omi_field_flag_t *set)
{
    sint32 iRet = CM_OK;
    cm_cnm_ipf_info_t *info = data;
    cm_cnm_decode_param_t param_str[] = 
    {
        {CM_OMI_FIELD_IPF_NIC,sizeof(info->nic),info->nic,NULL},
        {CM_OMI_FIELD_IPF_IP,sizeof(info->ip),info->ip,NULL},            
    };
    cm_cnm_decode_param_t param_num[] = 
    {
        {CM_OMI_FIELD_IPF_NID,sizeof(info->nid),&info->nid,NULL},
        {CM_OMI_FIELD_IPF_OPERATE,sizeof(info->operate),&info->operate,cm_cnm_ipf_decode_check_operate},
        {CM_OMI_FIELD_IPF_STATE,sizeof(info->status),&info->status,cm_cnm_ipf_decode_check_status},
        {CM_OMI_FIELD_IPF_PORT,sizeof(info->port),&info->port,NULL},
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
sint32 cm_cnm_ipf_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam)
{
    return cm_cnm_decode_comm(ObjParam,sizeof(cm_cnm_ipf_info_t),
        cm_cnm_ipf_decode_ext,ppDecodeParam);
}
static void cm_cnm_ipf_encode_each(cm_omi_obj_t item,void *eachdata,void *arg)
{
    cm_omi_field_flag_t *field = arg;
    cm_cnm_ipf_info_t *info = eachdata;
    cm_cnm_map_value_str_t cols_str[] = 
    {
        {CM_OMI_FIELD_IPF_NIC,              info->nic},
        {CM_OMI_FIELD_IPF_IP,               info->ip},
    };
    cm_cnm_map_value_num_t cols_num[] = 
    {
        {CM_OMI_FIELD_IPF_NID,          info->nid},
        {CM_OMI_FIELD_IPF_OPERATE,      info->operate},
        {CM_OMI_FIELD_IPF_PORT,         info->port},
        {CM_OMI_FIELD_IPF_STATE,        info->status},
    };
    cm_cnm_encode_str(item,field,cols_str,sizeof(cols_str)/sizeof(cm_cnm_map_value_str_t));
    cm_cnm_encode_num(item,field,cols_num,sizeof(cols_num)/sizeof(cm_cnm_map_value_num_t)); 
    return;
}
cm_omi_obj_t cm_cnm_ipf_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen)
{
    return cm_cnm_encode_comm_ext(pDecodeParam,pAckData,AckLen,
        sizeof(cm_cnm_ipf_info_t),cm_cnm_ipf_encode_each);
}

static sint32 cm_cnm_ipf_local_get_each(void *arg, sint8 **cols, uint32 col_num)
{
    cm_cnm_ipf_info_t *info = arg;
    sint8 buff[CM_STRING_64] = {0};
    sint8 buff_port[CM_STRING_32] = {0};
    if(col_num != 5)
    {
        CM_LOG_WARNING(CM_MOD_CNM,"col_num[%u]",col_num);
        return CM_FAIL;
    }
    CM_MEM_ZERO(info,sizeof(cm_cnm_ipf_info_t));
    
    info->operate = (0 == strcmp(cols[0],"pass")) ? CM_TRUE : CM_FALSE;
    CM_VSPRINTF(info->nic,sizeof(info->nic),"%s",cols[1]);
    CM_VSPRINTF(info->ip,sizeof(info->ip),"%s",cols[2]);  
    info->port = (0 == strcmp(cols[3],"any")) ? CM_FALSE : atoi(cols[3]);
    info->status = (0 == strcmp(cols[4],"online")) ? CM_TRUE : CM_FALSE;
    info->nid = cm_node_get_id();
    return CM_OK;
}

sint32  cm_cnm_ipf_local_getbatch(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    sint32 iRet = CM_OK;
    sint8 cmd[CM_STRING_512] = {0};
    CM_VSPRINTF(cmd,sizeof(cmd),"%s getbatch",g_cm_cnm_ipf_script);
    iRet = cm_cnm_exec_get_list(cmd,cm_cnm_ipf_local_get_each,
        (uint32)offset,sizeof(cm_cnm_ipf_info_t),ppAck,&total);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]",iRet);
        return iRet;
    }
    *pAckLen = total * sizeof(cm_cnm_ipf_info_t);
    return CM_OK;
}    

sint32 cm_cnm_ipf_local_count(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    uint64 cnt = 0;
    cnt = cm_exec_int("%s count",g_cm_cnm_ipf_script);
    return cm_cnm_ack_uint64(cnt,ppAck,pAckLen);
}

sint32  cm_cnm_ipf_local_insert(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    uint32 cnt = 0;
    const cm_cnm_decode_info_t *decode = param;
    cm_cnm_ipf_info_t *info = (const cm_cnm_ipf_info_t *)decode->data;
    const sint8* str = cm_cnm_get_enum_str(&CmOmiMapIpfopEnumBoolType,info->operate);
    if(CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_IPF_NIC)
        && (0 != strcmp(info->nic,"any")))
    {
        cnt = cm_exec_int("dladm show-link|awk '{print $1}'|grep -w '%s'|wc -l",info->nic);
        if(cnt == 0)
        {
            return CM_ERR_NOT_EXISTS;
        }
    }
    else
    {
        CM_VSPRINTF(info->nic,sizeof(info->nic),"any");
    }
    if(!CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_IPF_IP))
    {
        CM_VSPRINTF(info->ip,sizeof(info->ip),"any");
    }
    if(!CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_IPF_PORT))
    {
        info->port = 0;
    }
    return cm_system("%s %s %s %s %u",g_cm_cnm_ipf_script,str,info->nic,info->ip,info->port);
}

sint32  cm_cnm_ipf_local_update(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    const sint8* str = NULL;
    const cm_cnm_decode_info_t *decode = param;
    const cm_cnm_ipf_info_t *info = (const cm_cnm_ipf_info_t *)decode->data;
    str = cm_cnm_get_enum_str(&CmOmiMapIpfStatusEnumBoolType,info->status);
    return cm_system("%s update %s",g_cm_cnm_ipf_script,str);
}    

sint32  cm_cnm_ipf_local_delete(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    sint8 buff[CM_STRING_128] = {0};
    uint32 record = 0; 
    const cm_cnm_decode_info_t *decode = param;
    cm_cnm_ipf_info_t *info = (const cm_cnm_ipf_info_t *)decode->data;
    const sint8* op_str = cm_cnm_get_enum_str(&CmOmiMapIpfoptionEnumBoolType,info->operate);
    
    return cm_system("%s delete %s %s %s %u",g_cm_cnm_ipf_script,
            op_str,info->nic,info->ip,info->port);    
}   

sint32 cm_cnm_ipf_count(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_cnm_ipf_request(CM_OMI_CMD_COUNT,pDecodeParam,ppAckData,pAckLen);
}  

sint32 cm_cnm_ipf_getbatch(const void * pDecodeParam,void **ppAckData,uint32 *pAckLen)
{
    return cm_cnm_ipf_request(CM_OMI_CMD_GET_BATCH,pDecodeParam,ppAckData,pAckLen);
}

sint32 cm_cnm_ipf_insert(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_cnm_ipf_request(CM_OMI_CMD_ADD,pDecodeParam,ppAckData,pAckLen);
} 

sint32 cm_cnm_ipf_update(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_cnm_ipf_request(CM_OMI_CMD_MODIFY,pDecodeParam,ppAckData,pAckLen);
}    

sint32 cm_cnm_ipf_delete(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_cnm_ipf_request(CM_OMI_CMD_DELETE,pDecodeParam,ppAckData,pAckLen);
}  

static void cm_cnm_ipf_oplog_report(
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
        const cm_cnm_ipf_info_t *info = (const cm_cnm_ipf_info_t*)req->data;
        const sint8* nodename = cm_node_get_name(info->nid);
        cm_cnm_oplog_param_t params[5] = {
            {CM_OMI_DATA_INT,   CM_OMI_FIELD_IPF_OPERATE,sizeof(info->operate),&info->operate},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_IPF_NIC,strlen(info->nic),info->nic},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_IPF_IP,strlen(info->ip),info->ip},
            {CM_OMI_DATA_INT,   CM_OMI_FIELD_IPF_PORT,sizeof(info->port),&info->port},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_IPF_NID,strlen(nodename),nodename},
        };
        cm_cnm_oplog_report(sessionid,alarmid,params,cnt,&req->set);
    }    
    return;
}    
void cm_cnm_ipf_oplog_delete(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK)? CM_ALARM_LOG_IPF_DELETE_OK : CM_ALARM_LOG_IPF_DELETE_FAIL;
    cm_cnm_ipf_oplog_report(sessionid,pDecodeParam,alarmid);
    return;
}
void cm_cnm_ipf_oplog_insert(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK)? CM_ALARM_LOG_IPF_INSERT_OK : CM_ALARM_LOG_IPF_INSERT_FAIL;
    cm_cnm_ipf_oplog_report(sessionid,pDecodeParam,alarmid);
    return;
}
void cm_cnm_ipf_oplog_update(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK)? CM_ALARM_LOG_IPF_UPDATE_OK : CM_ALARM_LOG_IPF_UPDATE_FAIL;
    const cm_cnm_decode_info_t *req = pDecodeParam;
    const uint32 cnt = 2;
    if(NULL == pDecodeParam)
    {   
        cm_cnm_oplog_report(sessionid,alarmid,NULL,cnt,NULL);
    }    
    else    
    {       
        const cm_cnm_ipf_info_t *info = (const cm_cnm_ipf_info_t*)req->data;
        const sint8* nodename = cm_node_get_name(info->nid);
        cm_cnm_oplog_param_t params[2] = {
            {CM_OMI_DATA_INT,   CM_OMI_FIELD_IPF_STATE,sizeof(info->status),&info->status},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_IPF_NID,strlen(nodename),nodename},         
        };
        cm_cnm_oplog_report(sessionid,alarmid,params,cnt,&req->set);
    }   
    return; 
}    
