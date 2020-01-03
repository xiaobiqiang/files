/******************************************************************************
 *          Copyright (c) 2017 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cmt.h
 * author     : wbn
 * create date: 2017Äê10ÔÂ23ÈÕ
 * description: TODO:
 *
 *****************************************************************************/
#ifndef BASE_CMT_CM_CMT_H_
#define BASE_CMT_CM_CMT_H_
#include "cm_common.h"

typedef struct
{
    uint32 magic_number;
    uint32 msg;
    uint32 from_id;
    uint32 to_id;
    uint32 tm_out;
    uint32 data_len;
    uint8 data[];
}cm_cmt_msg_info_t;

typedef sint32 (*cm_cmt_msg_cbk_func_t)(void *pMsg, uint32 Len, void**
ppAck, uint32 *pAckLen);

typedef struct
{
    cm_msg_type_e msg;
    cm_cmt_msg_cbk_func_t cbk;
}cm_cmt_config_t;

typedef sint32 (*cm_cmt_node_check_cbk_func_t)(uint32 nid);

extern sint32 cm_cmt_init(bool_t isclient);

extern sint32 cm_cmt_node_add(uint32 Nid, const sint8 *pIpAddr, bool_t isMutli);

extern sint32 cm_cmt_node_delete(uint32 Nid);

extern void cm_cmt_node_state_cbk_reg(cm_cmt_node_check_cbk_func_t cbk);

extern sint32 cm_cmt_request(uint32 Nid,cm_msg_type_e Msg,const void *pData, uint32 DataLen,
    void **ppAckData, uint32 *pAckDataLen, uint32 Timeout);

extern sint32 cm_cmt_request_master(cm_msg_type_e Msg,const void *pData, uint32 DataLen,
    void **ppAckData, uint32 *pAckDataLen, uint32 Timeout);

#endif /* BASE_CMT_CM_CMT_H_ */
