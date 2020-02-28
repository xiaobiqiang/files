/******************************************************************************
 *          Copyright (c) 2017 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_omi_client.c
 * author     : wbn
 * create date: 2017年10月23日
 * description: TODO:
 *
 *****************************************************************************/
#include "cm_omi.h"
#include "cm_log.h"
#include "cm_rpc.h"

static cm_rpc_handle_t CmOmiHandle = NULL;
extern bool_t g_CmOmiCliMulti;
#define CM_OMI_REMOTE_MAX_NUM 256
static cm_rpc_handle_t CmOmiRemoteHandles[CM_OMI_REMOTE_MAX_NUM];


sint32 cm_omi_init_client(void)
{
    sint32 iRet = CM_FAIL;
    sint8 buff[CM_IP_LEN] = {0};

    CM_MEM_ZERO(CmOmiRemoteHandles,sizeof(CmOmiRemoteHandles));

    strcpy(buff,"127.0.0.1");
    
#ifndef CM_OMI_LOCAL
#ifndef CM_JNI_USED 
    iRet = cm_exec(buff,CM_IP_LEN,"ceres_cmd master |grep node_ip "
        "|awk -F':' '{print $2}' |awk -F' ' '{printf $1}'");
    if((CM_OK != iRet) || (0 == strlen(buff)))
    {
        CM_LOG_ERR(CM_MOD_OMI,"get master fail");
        strcpy(buff,"127.0.0.1");
    }    
#endif
#endif

    iRet = cm_rpc_connent(&CmOmiHandle,buff,CM_RPC_SERVER_PORT,g_CmOmiCliMulti);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_OMI,"conn to %s fail[%d]",buff,iRet);
    }
    return iRet;
}

sint32 cm_omi_close(void)
{
    sint32 iRet = CM_OK;

    if(NULL == CmOmiHandle)
    {
        return CM_OK;
    }
    iRet = cm_rpc_close(CmOmiHandle);
    if(CM_OK == iRet)
    {
        CmOmiHandle = NULL;
    }
    return iRet;
}

void cm_omi_reconnect(void)
{
    (void)cm_rpc_connect_reset(CmOmiHandle,"127.0.0.1",CM_RPC_SERVER_PORT);
    return;
}

sint32 cm_omi_request(cm_omi_obj_t req, cm_omi_obj_t *pAck, uint32 timeout)
{
    const sint8* pData = NULL;
    uint32 DataLen = 0;
    sint32 iRet = CM_FAIL;
    cm_rpc_msg_info_t *pRpcMsg = NULL;
    cm_rpc_msg_info_t *pRpcAck = NULL;

    if(NULL == CmOmiHandle)
    {
        CM_LOG_ERR(CM_MOD_OMI,"conn none!");
        return CM_ERR_NO_MASTER;
    }
    pData = cm_omi_obj_tostr(req);
    if(NULL == pData)
    {
        CM_LOG_ERR(CM_MOD_OMI,"to str fail!");
        return CM_FAIL;
    }

    DataLen = strlen(pData)+1;
    pRpcMsg = cm_rpc_msg_new(CM_MSG_OMI, DataLen);
    if(NULL == pRpcMsg)
    {
        CM_LOG_ERR(CM_MOD_OMI,"new msg len[%u] fail!",DataLen);
        return CM_FAIL;
    }
    CM_MEM_CPY(pRpcMsg->data,DataLen,pData,DataLen);
    iRet = cm_rpc_request(CmOmiHandle,pRpcMsg,&pRpcAck,timeout);
    cm_rpc_msg_delete(pRpcMsg);

    if(NULL != pRpcAck)
    {
        iRet = pRpcAck->result;
        if(pRpcAck->data_len > 0)
        {
            pData = (sint8*)((sint8*)pRpcAck + pRpcAck->head_len);
            *pAck = cm_omi_obj_parse(pData);
            if(NULL == *pAck)
            {
                CM_LOG_ERR(CM_MOD_OMI,"ack msg parse fail!p[%p]strlen[%u],data_len[%u]",
                    pRpcAck,DataLen,pRpcAck->data_len);
                iRet = CM_FAIL;
            }
        }        
        cm_rpc_msg_delete(pRpcAck);

    }
    return iRet;
}

sint32 cm_omi_request_str_in(cm_rpc_handle_t handle,const sint8 *pReq, sint8 **ppAckData, uint32 *pAckLen, uint32 timeout)
{
    uint32 DataLen = strlen(pReq)+1;
    sint32 iRet = CM_FAIL;
    cm_rpc_msg_info_t *pRpcMsg = NULL;
    cm_rpc_msg_info_t *pRpcAck = NULL;

    if(NULL == handle)
    {
        CM_LOG_ERR(CM_MOD_OMI,"conn none!");
        return CM_ERR_NO_MASTER;
    }
    pRpcMsg = cm_rpc_msg_new(CM_MSG_OMI, DataLen);
    if(NULL == pRpcMsg)
    {
        CM_LOG_ERR(CM_MOD_OMI,"new msg len[%u] fail!",DataLen);
        return CM_FAIL;
    }

    CM_MEM_CPY(pRpcMsg->data,DataLen,pReq,DataLen);
    iRet = cm_rpc_request(handle,pRpcMsg,&pRpcAck,timeout);
    cm_rpc_msg_delete(pRpcMsg);
    if(NULL != pRpcAck)
    {
        if(pRpcAck->data_len > 0)
        {
            *ppAckData = (sint8*)((sint8*)pRpcAck + pRpcAck->head_len);
            *pAckLen = pRpcAck->data_len;
            /* 后面调用 cm_omi_free 释放*/
        }
        else
        {
            *ppAckData = NULL;
            *pAckLen = 0;
            cm_rpc_msg_delete(pRpcAck);
        }
    }
    return iRet;
}

sint32 cm_omi_request_str(const sint8 *pReq, sint8 **ppAckData, uint32 *pAckLen, uint32 timeout)
{
    return cm_omi_request_str_in(CmOmiHandle,pReq,ppAckData,pAckLen,timeout);
}

void cm_omi_free(sint8 *pAckData)
{
    cm_rpc_msg_delete((cm_rpc_msg_info_t*)(pAckData - sizeof(cm_rpc_msg_info_t)));
}



sint32 cm_omi_remote_connect(const sint8* ipaddr)
{
    cm_rpc_handle_t *pHandle=NULL;
    cm_rpc_handle_t *ptmp=CmOmiRemoteHandles;
    sint32 iloop=0;
    sint32 freeindex=0;
    sint32 iRet = CM_OK;
    const sint8* ipconn=NULL;
    
    for(iloop=0;iloop<CM_OMI_REMOTE_MAX_NUM;iloop++,ptmp++)
    {
        if(*ptmp == NULL)
        {
            if(pHandle == NULL)
            {
                /* get free handle */
                pHandle = ptmp;
                freeindex = iloop;
            }
            continue;
        }
        ipconn = cm_rpc_get_ipaddr(*ptmp);
        if(NULL == ipconn)
        {
            continue;
        }
        if(0 == strcmp(ipconn,ipaddr))
        {
            return iloop;
        }
    }

    if(pHandle == NULL)
    {
        CM_LOG_ERR(CM_MOD_OMI,"conn to %s no free",ipaddr);
        return -1;
    }
    
    iRet = cm_rpc_connent(pHandle,ipaddr,CM_RPC_SERVER_PORT,g_CmOmiCliMulti);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_OMI,"conn to %s fail[%d]",ipaddr,iRet);
        return -2;
    }
    return freeindex;
}

sint32 cm_omi_remote_request(sint32 handle,const sint8 *pReq, 
    sint8 **ppAckData, uint32 *pAckLen, uint32 timeout)
{
    cm_rpc_handle_t rpchanlde=NULL;
    if((0>handle) || (handle>=CM_OMI_REMOTE_MAX_NUM))
    {
        return CM_PARAM_ERR;
    }
    rpchanlde = CmOmiRemoteHandles[handle];
    
    return cm_omi_request_str_in(rpchanlde,pReq,ppAckData,pAckLen,timeout);
}

sint32 cm_omi_remote_close(sint32 handle)
{
    cm_rpc_handle_t rpchanlde=NULL;
    if((0>handle) || (handle>=CM_OMI_REMOTE_MAX_NUM))
    {
        return CM_PARAM_ERR;
    }
    rpchanlde=CmOmiRemoteHandles[handle];
    if(CmOmiRemoteHandles[handle] == NULL)
    {
        return CM_OK;
    }
    CmOmiRemoteHandles[handle] = NULL;
    (void)cm_rpc_close(rpchanlde);
    return CM_OK;
}


