/******************************************************************************
 *          Copyright (c) 2017 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cmt.c
 * author     : wbn
 * create date: 2017Äê10ÔÂ23ÈÕ
 * description: TODO:
 *
 *****************************************************************************/
#include "cm_cmt.h"
#include "cm_log.h"
#include "cm_rpc.h"

extern sint32 cm_cmt_server_init(void);
extern sint32 cm_cmt_client_init(bool_t isclient);

sint32 cm_cmt_init(bool_t isclient)
{
    sint32 iRet = CM_FAIL;

    if(isclient != CM_TRUE)
    {
        iRet = cm_cmt_server_init();
        if(CM_OK != iRet)
        {
            CM_LOG_ERR(CM_MOD_CMT,"Init Server Fail[%d]",iRet);
            return iRet;
        }
    }

    iRet = cm_cmt_client_init(isclient);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CMT,"Init Client Fail[%d]",iRet);
        return iRet;
    }
    return CM_OK;
}

cm_cmt_msg_info_t* cm_cmt_get_msg_from_rpcmsg(cm_rpc_msg_info_t *pRpcMsg)
{
    return (cm_cmt_msg_info_t*)(((uint8*)pRpcMsg) + pRpcMsg->head_len);
}

cm_cmt_msg_info_t* cm_cmt_msg_new(uint32 MsgType, uint32 DataLen)
{
    cm_rpc_msg_info_t *pRpcMsg = NULL;
    cm_cmt_msg_info_t *pCmtMsg = NULL;

    pRpcMsg = cm_rpc_msg_new(CM_MSG_CMT, sizeof(cm_cmt_msg_info_t) + DataLen);

    if(NULL == pRpcMsg)
    {
        CM_LOG_ERR(CM_MOD_CMT,"New (%u) fail",MsgType);
        return NULL;
    }

    pCmtMsg = (cm_cmt_msg_info_t*)pRpcMsg->data;
    pCmtMsg->data_len = DataLen;
    pCmtMsg->msg = MsgType;
    pCmtMsg->magic_number = CM_CMT_MAGIC_NUM;
    return pCmtMsg;
}

sint32 cm_cmt_msg_delete(cm_cmt_msg_info_t *pMsg)
{
    cm_rpc_msg_info_t *pRpcMsg = NULL;

    pRpcMsg = (cm_rpc_msg_info_t*)((uint8*)pMsg - sizeof(cm_rpc_msg_info_t));

    cm_rpc_msg_delete(pRpcMsg);
    return CM_OK;
}

cm_rpc_msg_info_t* cm_cmt_get_rpc_msg(cm_cmt_msg_info_t *pMsg)
{
    return (cm_rpc_msg_info_t*)((uint8*)pMsg - sizeof(cm_rpc_msg_info_t));
}

