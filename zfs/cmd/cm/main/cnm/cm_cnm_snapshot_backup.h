#ifndef MAIN_CNM_CM_CNM_SNAPSHOT_BACKUP_H_
#define MAIN_CNM_CM_CNM_SNAPSHOT_BACKUP_H_

#include "cm_cfg_public.h"
#include "cm_cnm_common.h"
#include "cm_task.h"

#define CM_CNM_SNAPSHOT_BACKUP_PATH_LEN 64

typedef struct
{
    uint32 src_nid;
    sint8 src_path[CM_CNM_SNAPSHOT_BACKUP_PATH_LEN];
    sint8 dst_ipaddr[CM_IP_LEN];
    sint8 dst_dir[CM_CNM_SNAPSHOT_BACKUP_PATH_LEN];
    sint8 sec_path[CM_CNM_SNAPSHOT_BACKUP_PATH_LEN];
} cm_cnm_snapshot_backup_info_t;

typedef struct
{
    sint8 ipaddr[CM_IP_LEN];
} cm_cnm_ping_info_t;

/*
 *
 */
extern sint32 cm_cnm_snapshot_backup_init(void);

/*
 *
 */
extern sint32 cm_cnm_ping_init(void);

/*
 *
 */
extern sint32 cm_cnm_snapshot_backup_decode
(const cm_omi_obj_t ObjParam, void **ppDecodeParam);

/*
 *
 */
extern sint32 cm_cnm_ping_decode
(const cm_omi_obj_t ObjParam, void **ppDecodeParam);

/*
 *
 */
extern sint32 cm_cnm_snapshot_backup_add
(const void *pDecodeParam, void **ppAckData, uint32 *pAckLen);

/*
 *
 */
extern sint32 cm_cnm_ping_add
(const void *pDecodeParam, void **ppAckData, uint32 *pAckLen);

/*
 *
 */
extern void cm_cnm_snapshot_backup_oplog_create
(const sint8 *sessionid, void *pDecodeParam, sint32 Result);

extern void cm_cnm_ping_oplog_create
(const sint8 *sessionid, void *pDecodeParam, sint32 Result);

/*
 *
 */
extern sint32 cm_cnm_snapshot_backup_local_add
(void *param, uint32 len,
 uint64 offset, uint32 total,
 void **ppAck, uint32 *pAckLen
);

/*
 *
 */
extern sint32 cm_cnm_ping_local_add
(void *param, uint32 len,
 uint64 offset, uint32 total,
 void **ppAck, uint32 *pAckLen
);

extern sint32 cm_cnm_snapshot_backup_cbk_task_pre
(sint8 *pParam, uint32 len, uint32 tmout);

extern sint32 cm_cnm_snapshot_backup_cbk_task_report
(cm_task_send_state_t *pSnd, cm_task_cmt_echo_t **pproc);

extern sint32 cm_cnm_snapshot_backup_cbk_task_aft
(sint8 *pParam, uint32 len, uint32 tmout);

#endif /* MAIN_CNM_CM_CNM_SNAPSHOT_BACKUP_H_ */