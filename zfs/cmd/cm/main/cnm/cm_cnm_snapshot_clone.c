#include "cm_cnm_snapshot_clone.h"
#include "cm_task.h"
#include "cm_cnm_common.h"
#include "cm_log.h"
#include "cm_cfg_global.h"
#include "cm_sys.h"
#include "cm_node.h"
#include "cm_common.h"
#include "cm_platform.h"
#include "cm_cmt.h"

/*
 *
 */
sint32 cm_cnm_snapshot_clone_init(void)
{
    return CM_OK;
}

static sint32 cm_cnm_snapshot_clone_decode_ext
(const cm_omi_obj_t ObjParam, void* info, cm_omi_field_flag_t *set)
{
    sint32 iRet;
    cm_cnm_snapshot_clone_info_t *pinfo = info;
    cm_cnm_decode_param_t params_num[] =
    {
        {CM_OMI_FIELD_SNAPSHOT_CLONE_NID, sizeof(pinfo->nid), &pinfo->nid, NULL},
    };
    cm_cnm_decode_param_t params_str[] =
    {
        {CM_OMI_FIELD_SNAPSHOT_CLONE_PATH, CM_CNM_SNAPSHOT_CLONE_PATH_LEN, pinfo->path, NULL},
        {CM_OMI_FIELD_SNAPSHOT_CLONE_DST, CM_CNM_SNAPSHOT_CLONE_PATH_LEN, pinfo->dst, NULL},
    };

    iRet = cm_cnm_decode_str(ObjParam, params_str,
                             sizeof(params_str) / sizeof(cm_cnm_decode_param_t), set);

    if(CM_OK != iRet)
    {
        return CM_FAIL;
    }

    return cm_cnm_decode_num(ObjParam, params_num,
                             sizeof(params_num) / sizeof(cm_cnm_decode_param_t), set);
}

sint32 cm_cnm_snapshot_clone_decode
(const cm_omi_obj_t ObjParam, void **ppDecodeParam)
{
    return cm_cnm_decode_comm(ObjParam, sizeof(cm_cnm_snapshot_clone_info_t),
                              cm_cnm_snapshot_clone_decode_ext, ppDecodeParam);
}

static sint32 cm_cnm_snapshot_clone_request
(const void *pDecodeParam, void **ppAckData, uint32 *pAckLen)
{
    cm_cnm_decode_info_t *pDecode = pDecodeParam;
    cm_cnm_snapshot_clone_info_t *pinfo = (cm_cnm_snapshot_clone_info_t *)pDecode->data;
    cm_cnm_req_param_t req_param;
    req_param.nid = pinfo->nid;
    req_param.obj = CM_OMI_OBJECT_SNAPSHOT_CLONE;
    req_param.cmd = CM_OMI_CMD_ADD;
    req_param.param = pDecodeParam;
    req_param.param_len = sizeof(cm_cnm_decode_info_t) + sizeof(cm_cnm_snapshot_clone_info_t);
    req_param.offset = 0;
    req_param.total = 0;
    req_param.ppack = ppAckData;
    req_param.acklen = pAckLen;
    req_param.fail_break = CM_FALSE;
    return cm_cnm_request(&req_param);
}

/*
 *
 */
sint32 cm_cnm_snapshot_clone_add
(const void *pDecodeParam, void **ppAckData, uint32 *pAckLen)
{
    if(NULL == pDecodeParam)
    {
        return CM_PARAM_ERR;
    }

    return cm_cnm_snapshot_clone_request(pDecodeParam, ppAckData, pAckLen);
}

/*
 *
 */
void cm_cnm_snapshot_clone_oplog_create
(const sint8 *sessionid, void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK) ? CM_ALARM_LOG_SNAPSHOT_CLONE_OK : CM_ALARM_LOG_SNAPSHOT_CLONE_FAIL;
    const cm_cnm_decode_info_t *req = pDecodeParam;
    const uint32 cnt = 3;

    if(NULL == pDecodeParam)
    {   
        cm_cnm_oplog_report(sessionid,alarmid,NULL,cnt,NULL);
    }    
    else    
    {   
        cm_cnm_snapshot_clone_info_t *info = (cm_cnm_snapshot_clone_info_t *)req->data;
        const sint8 *nodename = cm_node_get_name(info->nid);
        cm_cnm_oplog_param_t params[3] = {
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_SNAPSHOT_CLONE_NID,strlen(nodename),nodename},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_SNAPSHOT_CLONE_PATH,strlen(info->path),info->path},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_SNAPSHOT_CLONE_DST,strlen(info->dst),info->dst},
        };
        
        cm_cnm_oplog_report(sessionid,alarmid,params,cnt,&req->set);
    }   
    return;
}

static sint32 cm_cnm_snapshot_clone_create_lu
(const sint8 *path, const sint8 *dst)
{
    sint32 iRet;
    sint8 srcdir[CM_CNM_SNAPSHOT_CLONE_CMD_LEN] = {0};
    iRet = cm_exec_tmout(srcdir, sizeof(srcdir), CM_CNM_SNAPSHOT_CLONE_EXEC_TMOUT,
                         "echo %s | awk 'BEGIN{FS=\"@\"} {printf $1}'", path);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM, "get snapshot dir fail[%d]", iRet);
        return CM_ERR_VOLUME_CREATE_LU;
    }
    iRet = cm_exec_int("zfs list -H -t volume \"%s\" | wc -l", srcdir);
    if(0 != iRet)   // is volume's snapshot
    {
        iRet = cm_exec_tmout(NULL, 0, CM_CNM_SNAPSHOT_CLONE_EXEC_TMOUT,
                         "stmfadm create-lu /dev/zvol/rdsk/%s", dst);
        if(CM_OK != iRet)
        {
            CM_LOG_ERR(CM_MOD_CNM, "create-lu fail[%d]", iRet);
            return CM_ERR_VOLUME_CREATE_LU;
        }
        iRet = cm_system("zfs set zfs:single_data=1 %s 2>/dev/null", dst);
        if(CM_OK != iRet)
        {
            CM_LOG_ERR(CM_MOD_CNM, "plumb LU[%s] fail[%d]", dst, iRet);
            return CM_ERR_VOLUME_CREATE_LU;
        }
    }
    return CM_OK;
}

/*
 *
 */
sint32 cm_cnm_snapshot_clone_local_add
(void *param, uint32 len,
 uint64 offset, uint32 total,
 void **ppAck, uint32 *pAckLen
)
{
    sint32 iRet = CM_FAIL;
    uint32 size = sizeof(cm_cnm_decode_info_t) + sizeof(cm_cnm_snapshot_clone_info_t);
    cm_cnm_decode_info_t *pDecode = NULL;
    cm_cnm_snapshot_clone_info_t *pinfo = NULL;
    sint8 pool_1[CM_CNM_SNAPSHOT_CLONE_PATH_LEN] = {0};
    sint8 pool_2[CM_CNM_SNAPSHOT_CLONE_PATH_LEN] = {0};

    if(len != size)
    {
        CM_LOG_ERR(CM_MOD_CNM, "recv:%u, size:%u", len, size);
        return CM_PARAM_ERR;
    }

    pDecode = param;
    pinfo = (cm_cnm_snapshot_clone_info_t *)pDecode->data;

    (void)cm_exec_tmout(pool_1, CM_CNM_SNAPSHOT_CLONE_PATH_LEN, 5,
                        "echo '%s' | awk 'BEGIN{FS=\"/\"} {print $1}'", pinfo->path);
    (void)cm_exec_tmout(pool_2, CM_CNM_SNAPSHOT_CLONE_PATH_LEN, 5,
                        "echo '%s' | awk 'BEGIN{FS=\"/\"} {print $1}'", pinfo->dst);

    if(strcmp(pool_1, pool_2) != 0)
    {
        CM_LOG_ERR(CM_MOD_CNM, "not in the same pool");
        return CM_ERR_SNAPSHOT_CLONE_DIFF_POOL;
    }

    iRet = cm_exec_tmout(NULL, 0, CM_CNM_SNAPSHOT_CLONE_EXEC_TMOUT,
                         "zfs clone %s %s", pinfo->path, pinfo->dst);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM, "snapshot clone fail[%d]", iRet);
        return iRet;
    }

    return cm_cnm_snapshot_clone_create_lu(pinfo->path, pinfo->dst);
}
