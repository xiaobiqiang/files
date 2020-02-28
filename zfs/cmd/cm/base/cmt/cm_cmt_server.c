/******************************************************************************
 *          Copyright (c) 2017 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cmt_server.c
 * author     : wbn
 * create date: 2017Äê10ÔÂ23ÈÕ
 * description: TODO:
 *
 *****************************************************************************/
#include "cm_log.h"

#include "cm_cmt.h"
#include "cm_rpc.h"

extern sint32 cm_cmt_requst_route(uint32 Nid, cm_rpc_msg_info_t *pReq, cm_rpc_msg_info_t
** ppAck, uint32 tm_out);

extern cm_rpc_msg_info_t* cm_cmt_get_rpc_msg(cm_cmt_msg_info_t *pmsg);

static sint32 cm_cmt_rpc_callback(cm_rpc_msg_info_t *pmsg, cm_rpc_msg_info_t
**pAckmsg);

extern cm_cmt_msg_info_t* cm_cmt_msg_new(uint32 msgType, uint32 data_len);

extern cm_cmt_msg_info_t* cm_cmt_get_msg_from_rpcmsg(cm_rpc_msg_info_t
*pRpcmsg);

extern uint32 g_CmCmtMaxId;
extern const cm_cmt_config_t* g_CmCmtCbksCfg;
extern cm_get_node_func_t g_CmGetNodeIdFunc;
extern cm_get_node_func_t g_CmGetMasterIdFunc;

sint32 cm_cmt_server_init(void)
{
    return cm_rpc_reg(CM_MSG_CMT, cm_cmt_rpc_callback, CM_CMT_THREAD_NUM);
}



static sint32 cm_cmt_rpc_callback(cm_rpc_msg_info_t *pmsg, cm_rpc_msg_info_t
**ppAckmsg)
{
    void *pAckData = NULL;
    uint32 AckLen = 0;
    sint32 iRet = CM_FAIL;
    cm_cmt_config_t *pCfg = NULL;
    cm_cmt_msg_info_t *pCmtAck = NULL;
    cm_cmt_msg_info_t *pCmtMsg = cm_cmt_get_msg_from_rpcmsg(pmsg);

    if((pCmtMsg->msg >= g_CmCmtMaxId)
        || (CM_CMT_MAGIC_NUM != pCmtMsg->magic_number))
    {
        CM_LOG_ERR(CM_MOD_CMT,"Node[%u] msg[%u] MagicNumber[0x%08X]Not Support!",
            pCmtMsg->from_id,pCmtMsg->msg,pCmtMsg->magic_number);
        return CM_ERR_NOT_SUPPORT;
    }

    if((CM_NODE_ID_NONE != pCmtMsg->to_id) 
        && (pCmtMsg->to_id != g_CmGetNodeIdFunc()))
    {
        CM_LOG_WARNING(CM_MOD_CMT,"Node[%u] msg[%u] to Node[%u]!",
            pCmtMsg->from_id,pCmtMsg->msg,pCmtMsg->to_id);
        return cm_cmt_requst_route(pCmtMsg->to_id,pmsg, ppAckmsg,pCmtMsg->tm_out);
    }

    pCfg = &g_CmCmtCbksCfg[pCmtMsg->msg];
    if(NULL == pCfg->cbk)
    {
        CM_LOG_ERR(CM_MOD_CMT,"Node[%u] msg[%u] cbk Null!",
            pCmtMsg->from_id,pCmtMsg->msg);
        return CM_ERR_NOT_SUPPORT;
    }

    iRet = pCfg->cbk(pCmtMsg->data, pCmtMsg->data_len, &pAckData,&AckLen);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CMT,"Node[%u] msg[%u] iRet[%d]",
            pCmtMsg->from_id,pCmtMsg->msg, iRet);
    }

    if((NULL == pAckData) || (0 == AckLen))
    {
        return iRet;
    }

    pCmtAck = cm_cmt_msg_new(pCmtMsg->msg,AckLen);
    if(NULL == pCmtAck)
    {
        CM_LOG_ERR(CM_MOD_CMT,"Node[%u] msg[%u] New fail",
            pCmtMsg->from_id,pCmtMsg->msg);
        CM_FREE(pAckData);
        return CM_FAIL;
    }
    pCmtAck->from_id = pCmtMsg->to_id;
    pCmtAck->to_id = pCmtMsg->from_id;
    CM_MEM_CPY(pCmtAck->data,AckLen,pAckData,AckLen);
    CM_FREE(pAckData);
    *ppAckmsg = cm_cmt_get_rpc_msg(pCmtAck);

    return iRet;
}

sint32 cm_cmt_request_master(cm_msg_type_e Msg,const void *pData, uint32 DataLen,
    void **ppAckData, uint32 *pAckDataLen, uint32 Timeout)
{
    uint32 myid = g_CmGetNodeIdFunc();
    uint32 submaster = g_CmGetMasterIdFunc();
    const cm_cmt_config_t *pCfg = &g_CmCmtCbksCfg[Msg];
    
    if(CM_NODE_ID_NONE == submaster)
    {
       CM_LOG_ERR(CM_MOD_SYNC,"submaster none"); 
       return CM_FAIL; 
    }
    
    if(myid == submaster)
    {
        return pCfg->cbk(pData,DataLen,ppAckData,pAckDataLen);
    }

    return cm_cmt_request(submaster,Msg,pData,DataLen,ppAckData,pAckDataLen,Timeout);
}


