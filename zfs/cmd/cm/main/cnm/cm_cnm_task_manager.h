#ifndef MAIN_CNM_CM_CNM_TASK_MANAGER_H_
#define MAIN_CNM_CM_CNM_TASK_MANAGER_H_

#include "cm_omi.h"
#include "cm_cnm_common.h"
#include "cm_platform.h"

typedef struct
{
    uint64 tid;
    uint32 nid;
    uint32 type;
    uint32 state;
    uint32 prog;
    cm_time_t start;
    cm_time_t end;
} cm_cnm_task_manager_info_t;

/*
 *
 */
extern sint32 cm_cnm_task_manager_init(void);

/*
 *
 */
extern sint32 cm_cnm_task_manager_decode
(const cm_omi_obj_t ObjParam, void **ppDecodeParam);

/*
 *
 */
extern cm_omi_obj_t cm_cnm_task_manager_encode
(const void *pDecodeParam, void *pAckData, uint32 AckLen);

/*
 *
 */
extern sint32 cm_cnm_task_manager_get
(const void *pDecodeParam, void **ppAckData, uint32 *pAckLen);

/*
 *
 */
extern sint32 cm_cnm_task_manager_getbatch
(const void *pDecodeParam, void **ppAckData, uint32 *pAckLen);
/*
 *
 */
extern sint32 cm_cnm_task_manager_delete
(const void *pDecodeParam, void **ppAckData, uint32 *pAckLen);
/*
 *
 */
extern sint32 cm_cnm_task_manager_count
(const void *pDecodeParam, void **ppAckData, uint32 *pAckLen);

/*
 *
 */
extern void cm_cnm_task_manager_oplog_delete
(const sint8* sessionid, const void *pDecodeParam, sint32 Result);

/*
 *
 */
extern sint32 cm_cnm_task_manager_local_get
(void *param, uint32 len,
 uint64 offset, uint32 total,
 void **ppAck, uint32 *pAckLen);

/*
 *
 */
extern sint32 cm_cnm_task_manager_local_getbatch
(void *param, uint32 len,
 uint64 offset, uint32 total,
 void **ppAck, uint32 *pAckLen);

/*
 *
 */
extern sint32 cm_cnm_task_manager_local_delete
(void *param, uint32 len,
 uint64 offset, uint32 total,
 void **ppAck, uint32 *pAckLen);

/*
 *
 */
extern sint32 cm_cnm_task_manager_local_count
(void *param, uint32 len,
 uint64 offset, uint32 total,
 void **ppAck, uint32 *pAckLen);


#endif /* MAIN_CNM_CM_CNM_TASK_MANAGER_H_ */