#ifndef MAIN_CNM_CM_CNM_SNAPSHOT_CLONE_H_
#define MAIN_CNM_CM_CNM_SNAPSHOT_CLONE_H_

#include "cm_cfg_public.h"
#include "cm_cnm_common.h"

#define CM_CNM_SNAPSHOT_CLONE_PATH_LEN 64
#define CM_CNM_SNAPSHOT_CLONE_CMD_LEN CM_STRING_256
#define CM_CNM_SNAPSHOT_CLONE_EXEC_TMOUT 20

typedef struct
{
    uint32 nid;
    sint8 path[CM_CNM_SNAPSHOT_CLONE_PATH_LEN];
    sint8 dst[CM_CNM_SNAPSHOT_CLONE_PATH_LEN];
} cm_cnm_snapshot_clone_info_t;

/*
 *
 */
extern sint32 cm_cnm_snapshot_clone_init(void);

/*
 *
 */
extern sint32 cm_cnm_snapshot_clone_decode
(const cm_omi_obj_t ObjParam, void **ppDecodeParam);

/*
 *
 */
extern sint32 cm_cnm_snapshot_clone_add
(const void *pDecodeParam, void **ppAckData, uint32 *pAckLen);

/*
 *
 */
extern void cm_cnm_snapshot_clone_oplog_create
(const sint8 *sessionid, void *pDecodeParam, sint32 Result);

/*
 *
 */
extern sint32 cm_cnm_snapshot_clone_local_add
(void *param, uint32 len,
 uint64 offset, uint32 total,
 void **ppAck, uint32 *pAckLen
);

#endif /* MAIN_CNM_CM_CNM_SNAPSHOT_CLONE_H_ */