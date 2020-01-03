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
sint32 cm_omi_init_client(void)
{
    sint32 iRet = CM_FAIL;
    sint8 buff[CM_IP_LEN] = {0};

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

sint32 cm_omi_request_str(const sint8 *pReq, sint8 **ppAckData, uint32 *pAckLen, uint32 timeout)
{
    uint32 DataLen = strlen(pReq)+1;
    sint32 iRet = CM_FAIL;
    cm_rpc_msg_info_t *pRpcMsg = NULL;
    cm_rpc_msg_info_t *pRpcAck = NULL;

    if(NULL == CmOmiHandle)
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
    iRet = cm_rpc_request(CmOmiHandle,pRpcMsg,&pRpcAck,timeout);
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

void cm_omi_free(sint8 *pAckData)
{
    cm_rpc_msg_delete((cm_rpc_msg_info_t*)(pAckData - sizeof(cm_rpc_msg_info_t)));
}


