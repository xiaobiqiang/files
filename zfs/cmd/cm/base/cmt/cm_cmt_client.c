/******************************************************************************
 *          Copyright (c) 2017 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cmt_client.c
 * author     : wbn
 * create date: 2017Äê10ÔÂ23ÈÕ
 * description: TODO:
 *
 *****************************************************************************/
#include "cm_cmt.h"
#include "cm_log.h"
#include "cm_rpc.h"
#include "cm_queue.h"

static cm_tree_node_t *g_pCmCmtLocalConn = NULL;
static cm_mutex_t g_CmCmtMutex;
static cm_rpc_handle_t cm_cmt_get_rpc_handle(cm_tree_node_t *pHead, uint32 Id);

extern const cm_cmt_config_t* g_CmCmtCbksCfg;

static cm_cmt_node_check_cbk_func_t g_CmCmtNodeCheck = NULL;
static uint8 g_CmCmtIsClient = CM_FALSE;
extern uint32 g_CmCmtMaxId;
extern uint32 cm_get_local_nid_x(void);

sint32 cm_cmt_client_init(bool_t isclient)
{
    cm_tree_node_t *pHead = cm_tree_node_new(0);
    g_CmCmtIsClient = isclient;
    if(NULL == pHead)
    {
        CM_LOG_ERR(CM_MOD_CMT,"New Head fail");
        return CM_FAIL;
    }
    CM_MUTEX_INIT(&g_CmCmtMutex);
    g_pCmCmtLocalConn = pHead;

    return CM_OK;
}

sint32 cm_cmt_node_add(uint32 Nid, const sint8 *pIpAddr, bool_t isMutli)
{
    cm_tree_node_t *pNode = NULL;
    sint32 iRet = CM_FAIL;

    CM_LOG_WARNING(CM_MOD_CMT,"Nid[%u] pIpAddr[%s] isMutli[%u]",Nid,pIpAddr,isMutli);

    CM_MUTEX_LOCK(&g_CmCmtMutex);
    pNode = cm_tree_node_addnew(g_pCmCmtLocalConn, Nid);
    if(NULL == pNode)
    {
        CM_LOG_ERR(CM_MOD_CMT,"Nid[%u] add new fail", Nid);
        CM_MUTEX_UNLOCK(&g_CmCmtMutex);
        return CM_FAIL;
    }
    iRet = cm_rpc_connent((cm_rpc_handle_t*)&pNode->pdata,
        pIpAddr,CM_RPC_SERVER_PORT,isMutli);

    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CMT,"Nid[%u] conn fail", Nid);
        (void)cm_tree_node_delete(g_pCmCmtLocalConn,pNode);
        CM_MUTEX_UNLOCK(&g_CmCmtMutex);
        return iRet;
    }
    CM_MUTEX_UNLOCK(&g_CmCmtMutex);
    return CM_OK;
}

sint32 cm_cmt_node_delete(uint32 Nid)
{
    cm_tree_node_t *pNode = NULL;
    sint32 iRet = CM_OK;

    CM_LOG_WARNING(CM_MOD_CMT,"Nid[%u]",Nid);
    CM_MUTEX_LOCK(&g_CmCmtMutex);
    pNode = cm_tree_node_find(g_pCmCmtLocalConn, Nid);
    if(NULL != pNode)
    {
        (void)cm_rpc_close((cm_rpc_handle_t)pNode->pdata);
        pNode->pdata = NULL;
        iRet = cm_tree_node_delete(g_pCmCmtLocalConn,pNode);
    }
    CM_MUTEX_UNLOCK(&g_CmCmtMutex);
    return iRet;
}

sint32 cm_cmt_request(uint32 Nid,cm_msg_type_e Msg,const void *pData, uint32 DataLen,
    void **ppAckData, uint32 *pAckDataLen, uint32 Timeout)
{
    sint32 iRet = CM_FAIL;
    void *pAckData = NULL;
    cm_cmt_msg_info_t *pMsg = NULL;
    cm_cmt_msg_info_t *pCmtAck = NULL;
    cm_rpc_msg_info_t *pAckMsg = NULL;
    cm_rpc_handle_t Handle = NULL;
    const cm_cmt_config_t *cfg = NULL;
    if(g_CmCmtIsClient == CM_FALSE)
    {        
        if(g_CmCmtMaxId <= Msg)
        {
            CM_LOG_ERR(CM_MOD_CMT,"Msg[%u]",Msg);
            return CM_ERR_NOT_SUPPORT;
        }
        
        if((Nid == cm_get_local_nid_x())||(Nid == CM_NODE_ID_LOCAL))
        {
            cfg = &g_CmCmtCbksCfg[Msg];
            return cfg->cbk(pData,DataLen,ppAckData,pAckDataLen);
        }

        if((NULL != g_CmCmtNodeCheck) 
            && ((CM_CMT_MSG_CNM == Msg) || (CM_CMT_MSG_EXEC == Msg))
            && (CM_OK != g_CmCmtNodeCheck(Nid)))
        {
            CM_LOG_INFO(CM_MOD_CMT,"node[%u]",Nid);
            return CM_ERR_CONN_FAIL;
        }
    }

    Handle = cm_cmt_get_rpc_handle(g_pCmCmtLocalConn,Nid);
    if(NULL == Handle)
    {
        CM_LOG_ERR(CM_MOD_CMT,"Nid(%u) Msg(%u) Handle NULL",Nid, Msg);
        return CM_FAIL;
    }

    pMsg = cm_cmt_msg_new((uint32)Msg, DataLen);
    if(NULL == pMsg)
    {
        CM_LOG_ERR(CM_MOD_CMT,"Nid(%u) Msg(%u) New NULL",Nid, Msg);
        return CM_FAIL;
    }
    
    if(DataLen > 0)
    {
        CM_MEM_CPY(pMsg->data,DataLen,pData,DataLen);
    }
    
    pMsg->to_id = Nid;
    pMsg->from_id = cm_get_local_nid_x();
    pMsg->tm_out = Timeout;

    iRet = cm_rpc_request(Handle, cm_cmt_get_rpc_msg(pMsg), &pAckMsg, Timeout);
    cm_cmt_msg_delete(pMsg);

    if(NULL == pAckMsg)
    {
        return iRet;
    }

    if(CM_OK != iRet)
    {
        CM_LOG_INFO(CM_MOD_CMT,"Nid(%u) Msg(%u) fail iRet=%d",Nid, Msg,iRet);
    }

    iRet = pAckMsg->result;
    
    if((NULL == ppAckData) || (NULL == pAckDataLen))
    {
        cm_rpc_msg_delete(pAckMsg);
        return iRet;
    }

    *ppAckData = NULL;
    *pAckDataLen = 0;

    pCmtAck = cm_cmt_get_msg_from_rpcmsg(pAckMsg);

    if((0 == pAckMsg->data_len) || (0 == pCmtAck->data_len))
    {
        cm_rpc_msg_delete(pAckMsg);
        return iRet;
    }

    pAckData = CM_MALLOC(pCmtAck->data_len);
    if(NULL == pAckData)
    {
        cm_rpc_msg_delete(pAckMsg);
        CM_LOG_ERR(CM_MOD_CMT,"Nid(%u) Msg(%u) Malloc Fail",Nid, Msg);
        return CM_FAIL;
    }

    CM_MEM_CPY(pAckData,pCmtAck->data_len,pCmtAck->data,pCmtAck->data_len);
    cm_rpc_msg_delete(pAckMsg);
    *ppAckData = pAckData;
    *pAckDataLen = pCmtAck->data_len;
    return iRet;
}

static cm_rpc_handle_t cm_cmt_get_rpc_handle(cm_tree_node_t *pHead, uint32 Id)
{
    cm_rpc_handle_t Handle = NULL;
    cm_tree_node_t *pNode = NULL;

    CM_MUTEX_LOCK(&g_CmCmtMutex);
    pNode = cm_tree_node_find(pHead, Id);
    if(NULL != pNode)
    {
        Handle = (cm_tree_node_t*)pNode->pdata;
    }
    CM_MUTEX_UNLOCK(&g_CmCmtMutex);
    return Handle;
}

sint32 cm_cmt_requst_route(uint32 Nid, cm_rpc_msg_info_t *pReq, cm_rpc_msg_info_t
** ppAck, uint32 Tmout)
{
    sint32 iRet = CM_FAIL;
    cm_cmt_msg_info_t *pCmtMsg = cm_cmt_get_msg_from_rpcmsg(pReq);
    cm_rpc_handle_t Handle = cm_cmt_get_rpc_handle(g_pCmCmtLocalConn,Nid);

    if(NULL == Handle)
    {
        CM_LOG_ERR(CM_MOD_CMT,"ToNid(%u) Msg(%u) Handle NULL",Nid, pCmtMsg->msg);
        return CM_FAIL;
    }

    iRet = cm_rpc_request(Handle, pReq, ppAck, Tmout);

    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CMT,"ToNid(%u) Msg(%u) fail iRet=%d",Nid, pCmtMsg->msg,iRet);
    }
    return iRet;
}

void cm_cmt_node_state_cbk_reg(cm_cmt_node_check_cbk_func_t cbk)
{
    g_CmCmtNodeCheck = cbk;
}


