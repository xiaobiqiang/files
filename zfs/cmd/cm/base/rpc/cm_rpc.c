/******************************************************************************
 *          Copyright (c) 2017 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_rpc.c
 * author     : wbn
 * create date: 2017Äê10ÔÂ23ÈÕ
 * description: TODO:
 *
 *****************************************************************************/
#include "cm_rpc.h"
#include "cm_log.h"

#ifdef CM_RPC_USE_NANOMSG
#include <nanomsg/nn.h>
#include <nanomsg/pair.h>
#include <nanomsg/reqrep.h>
#endif


extern sint32 cm_rpc_client_init(void);

extern sint32 cm_rpc_server_init(void);

sint32 cm_rpc_init(bool_t isclient)
{
    sint32 iRet = CM_FAIL;

    iRet = cm_rpc_client_init();

    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_RPC,"Init Client fail");
        return CM_FAIL;
    }

    if(CM_FALSE == isclient)
    {
        iRet = cm_rpc_server_init();

        if(CM_OK != iRet)
        {
            CM_LOG_ERR(CM_MOD_RPC,"Init Server fail");
            return CM_FAIL;
        }
    }

    return CM_OK;
}

cm_rpc_msg_info_t* cm_rpc_msg_new(cm_rpc_msg_e MsgType, uint32 DataLen)
{
    cm_rpc_msg_info_t *pMsg = NULL;
#ifndef CM_RPC_USE_NANOMSG

    if((sizeof(cm_rpc_msg_e) + DataLen) > CM_RPC_MAX_MSG_LEN)
    {
        CM_LOG_ERR(CM_MOD_RPC,"DataLen[%u] too long!",DataLen);
        return NULL;
    }

    pMsg = CM_MALLOC(sizeof(cm_rpc_msg_info_t) + DataLen);
#else

    pMsg = nn_allocmsg(sizeof(cm_rpc_msg_info_t) + DataLen,0);
#endif

    if(NULL == pMsg)
    {
        CM_LOG_ERR(CM_MOD_RPC,"Malloc Fail!");
        return NULL;
    }
    pMsg->head_len = sizeof(cm_rpc_msg_info_t);
    pMsg->msg_type = MsgType;
    pMsg->data_len = DataLen;
    pMsg->magic_number = CM_RPC_MAGIC_NUM;

    return pMsg;
}

void cm_rpc_msg_delete(cm_rpc_msg_info_t *pMsg)
{
#ifndef CM_RPC_USE_NANOMSG
    CM_FREE(pMsg);
#else
    nn_freemsg(pMsg);
#endif
}
