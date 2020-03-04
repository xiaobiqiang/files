/******************************************************************************
 *          Copyright (c) 2018 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cmd_server.h
 * author     : wbn
 * create date: 2018Äê1ÔÂ8ÈÕ
 * description: TODO:
 *
 *****************************************************************************/
#ifndef MAIN_CMD_CM_CMD_SERVER_H_
#define MAIN_CMD_CM_CMD_SERVER_H_

#include "cm_types.h"

typedef struct
{
    const sint8 *pname;
    sint32 (*cbk)(const sint8 **ppParam, uint32 ParamNum, void **ppAckData, uint32 *pAckLen);
}cm_cmd_cbk_cfg_t;

extern sint32 cm_cmd_init(void);

extern sint32 cm_cmd_cbk_cmt(
 void *pMsg, uint32 Len, void** ppAck, uint32 *pAckLen);
 

#endif /* MAIN_CMD_CM_CMD_SERVER_H_ */
