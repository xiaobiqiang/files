/******************************************************************************
 *          Copyright (c) 2017 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_rpc.h
 * author     : wbn
 * create date: 2017Äê10ÔÂ23ÈÕ
 * description: TODO:
 *
 *****************************************************************************/
#ifndef BASE_RPC_CM_RPC_H_
#define BASE_RPC_CM_RPC_H_
#include "cm_common.h"

typedef enum
{
    CM_MSG_OMI=0,
    CM_MSG_CMT=1,
    CM_MSG_LMT=2,
    CM_MSG_BUTT
}cm_rpc_msg_e;

typedef struct
{
    uint32 magic_number;
    cm_rpc_msg_e msg_type;
    uint32 msg_id;
    uint32 head_len;
    uint32 data_len;
    /* ------------------opt----------------------*/
    uint32 result; /* Response Msg use */
    /* ------------------opt------------------------*/
    uint8 data[];
}cm_rpc_msg_info_t;

typedef sint32 (*cm_rpc_server_cbk_func_t)(cm_rpc_msg_info_t *pMsg, cm_rpc_msg_info_t**pAckMsg);

typedef void* cm_rpc_handle_t;


extern sint32 cm_rpc_init(bool_t isclient);


extern sint32 cm_rpc_reg(cm_rpc_msg_e MsgType, cm_rpc_server_cbk_func_t cbk, uint32 ThreadNum);


extern sint32 cm_rpc_connent(
    cm_rpc_handle_t *pHandle, const sint8 *pIpAddr, uint16 Port, bool_t isMutli);

extern sint32 cm_rpc_connect_reset(cm_rpc_handle_t Handle, const sint8 *pIpAddr, uint16
Port);

extern sint32 cm_rpc_request(
    cm_rpc_handle_t Handle, cm_rpc_msg_info_t *pData,
    cm_rpc_msg_info_t **ppAckData,uint32 Timeout);

extern sint32 cm_rpc_close(cm_rpc_handle_t Handle);


extern cm_rpc_msg_info_t* cm_rpc_msg_new(cm_rpc_msg_e MsgType, uint32 DataLen);

extern void cm_rpc_msg_delete(cm_rpc_msg_info_t *pMsg);

extern const sint8* cm_rpc_get_ipaddr(cm_rpc_handle_t Handle);


#endif /* BASE_RPC_CM_RPC_H_ */
