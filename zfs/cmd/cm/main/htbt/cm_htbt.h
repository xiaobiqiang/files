/******************************************************************************
 *          Copyright (c) 2017 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_htbt.h
 * author     : wbn
 * create date: 2017Äê10ÔÂ26ÈÕ
 * description: TODO:
 *
 *****************************************************************************/
#ifndef MAIN_HTBT_CM_HTBT_H_
#define MAIN_HTBT_CM_HTBT_H_

#include "cm_common.h"
#include "cm_cfg_local.h"

extern sint32 cm_htbt_init(void);

extern sint32 cm_htbt_cbk_recv(void *pMsg, uint32 Len, void**ppAck, uint32 *pAckLen);

extern void cm_htbt_get_local_node(uint32 *ptotal, uint32 *ponline);

extern sint32 cm_htbt_add(uint32 subdomain,uint32 nid);

extern sint32 cm_htbt_delete(uint32 subdomain,uint32 nid);

#endif /* MAIN_HTBT_CM_HTBT_H_ */
