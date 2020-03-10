/******************************************************************************
 *          Copyright (c) 2017 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cnm_demo.h
 * author     : wbn
 * create date: 2017Äê10ÔÂ26ÈÕ
 * description: TODO:
 *
 *****************************************************************************/
#ifndef MAIN_CNM_CM_CNM_DEMO_H_
#define MAIN_CNM_CM_CNM_DEMO_H_
#include "cm_omi_types.h"
#include "cm_sync_types.h"

extern sint32 cm_cnm_session_init(void);

extern sint32 cm_cnm_session_decode(const cm_omi_obj_t ObjParam, void
**ppDecodeParam);
extern cm_omi_obj_t cm_cnm_session_encode(const void *pDecodeParam, void *pAckData, uint32 AckLen);

extern sint32 cm_cnm_session_get(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
extern sint32 cm_cnm_session_delete(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

/*============================================================================*/

extern sint32 cm_cnm_remote_init(void);

extern sint32 cm_cnm_remote_decode(const cm_omi_obj_t ObjParam, void **ppDecodeParam);
extern cm_omi_obj_t cm_cnm_remote_encode(const void *pDecodeParam, void *pAckData, uint32 AckLen);

extern sint32 cm_cnm_remote_getbatch(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_remote_count(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);


#endif /* MAIN_CNM_CM_CNM_DEMO_H_ */
