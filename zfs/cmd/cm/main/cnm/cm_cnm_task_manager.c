#include "cm_cnm_task_manager.h"
#include "cm_cfg_omi.h"
#include "cm_log.h"
#include "cm_task.h"
#include "cm_sys.h"
#include "cm_omi.h"

#define CM_TASK_CMD_LEN 256
#define CM_TASK_DESC_LEN 256

sint32 cm_cnm_task_manager_init(void)
{
    return CM_OK;
}

static sint32 cm_cnm_task_manager_decode_ext
(const cm_omi_obj_t ObjParam, void* data, cm_omi_field_flag_t *set)
{
    sint32 iRet;
    cm_cnm_task_manager_info_t *pinfo = data;
    cm_cnm_decode_param_t num_params[] =
    {
        {CM_OMI_FIELD_TASK_MANAGER_TID, sizeof(pinfo->tid), &pinfo->tid, NULL},
        {CM_OMI_FIELD_TASK_MANAGER_NID, sizeof(pinfo->nid), &pinfo->nid, NULL},
        {CM_OMI_FIELD_TASK_MANAGER_TYPE, sizeof(pinfo->type), &pinfo->type, NULL},
        {CM_OMI_FIELD_TASK_MANAGER_STATE, sizeof(pinfo->state), &pinfo->state, NULL},
        {CM_OMI_FIELD_TASK_MANAGER_PROG, sizeof(pinfo->prog), &pinfo->prog, NULL},
        {CM_OMI_FIELD_TASK_MANAGER_STTM, sizeof(pinfo->start), &pinfo->start, NULL},
        {CM_OMI_FIELD_TASK_MANAGER_ENDTM, sizeof(pinfo->end), &pinfo->end, NULL}
    };
    return cm_cnm_decode_num(ObjParam, num_params, sizeof(num_params) / sizeof(cm_cnm_decode_param_t), set);
}

sint32 cm_cnm_task_manager_decode
(const cm_omi_obj_t ObjParam, void **ppDecodeParam)
{
    return cm_cnm_decode_comm(ObjParam, sizeof(cm_cnm_task_manager_info_t),
                              cm_cnm_task_manager_decode_ext, ppDecodeParam);
}

static void cm_cnm_task_manager_encode_each
(cm_omi_obj_t item, void *eachdata, void *arg)
{
    cm_task_info_t *pinfo = (cm_task_info_t *)eachdata;
    cm_omi_field_flag_t *field = (cm_omi_field_flag_t *)arg;
    cm_cnm_map_value_num_t num_params[] =
    {
        {CM_OMI_FIELD_TASK_MANAGER_TID, (uint32)pinfo->task_id},
        {CM_OMI_FIELD_TASK_MANAGER_PID, pinfo->work_id},
        {CM_OMI_FIELD_TASK_MANAGER_NID, pinfo->to_nid},
        {CM_OMI_FIELD_TASK_MANAGER_TYPE, pinfo->task_type},
        {CM_OMI_FIELD_TASK_MANAGER_STATE, pinfo->task_state},
        {CM_OMI_FIELD_TASK_MANAGER_PROG, pinfo->task_prog},
        {CM_OMI_FIELD_TASK_MANAGER_STTM, (uint32)pinfo->task_start},
        {CM_OMI_FIELD_TASK_MANAGER_ENDTM, (uint32)pinfo->task_end},
    };
    cm_cnm_map_value_str_t str_params[] =
    {
        {CM_OMI_FIELD_TASK_MANAGER_DESC, pinfo->task_desc},
    };

    (void)cm_cnm_encode_num(item, field,
                            num_params, sizeof(num_params) / sizeof(cm_cnm_map_value_num_t));
    (void)cm_cnm_encode_str(item, field,
                            str_params, sizeof(str_params) / sizeof(cm_cnm_map_value_str_t));
}

cm_omi_obj_t cm_cnm_task_manager_encode
(const void *pDecodeParam, void *pAckData, uint32 AckLen)
{
    return cm_cnm_encode_comm_ext
           (pDecodeParam, pAckData, AckLen, sizeof(cm_task_info_t), cm_cnm_task_manager_encode_each);
}

sint32 cm_cnm_task_manager_get
(const void *pDecodeParam, void **ppAckData, uint32 *pAckLen)
{
    sint32 iRet;
    uint32 ackLen = 0;
    cm_task_info_t *pinfo = NULL;
    cm_cnm_decode_info_t *pDecode = (cm_cnm_decode_info_t *)pDecodeParam;
    cm_cnm_task_manager_info_t *pminfo = (cm_cnm_task_manager_info_t *)pDecode->data;
    cm_omi_field_flag_t *set = &pDecode->set;

    if(!(CM_OMI_FIELDS_FLAG_ISSET(set, CM_OMI_FIELD_TASK_MANAGER_TID)))
    {
        CM_LOG_ERR(CM_MOD_CNM, "not set tid");
        return CM_FAIL;
    }

    iRet = cm_task_get(pminfo->tid, (void **)&pinfo, &ackLen);

    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM, "get task info fail[%d]", iRet);

        if(NULL != pinfo)
        {
            CM_FREE(pinfo);
        }

        *ppAckData = NULL;
        *pAckLen = 0;
        return CM_FAIL;
    }

    *ppAckData = pinfo;
    *pAckLen = ackLen;
    return CM_OK;
}

static sint32 cm_cnm_task_manager_cat_sql
(cm_omi_field_flag_t *set, sint8 *sql, cm_cnm_task_manager_info_t *pminfo)
{
    uint32 flag = 0;

    if(CM_OMI_FIELDS_FLAG_ISSET(set, CM_OMI_FIELD_TASK_MANAGER_TID))
    {
        CM_SNPRINTF_ADD(sql, CM_TASK_CMD_LEN, "id = %ld", pminfo->tid);
        flag = 1;
    }

    if(CM_OMI_FIELDS_FLAG_ISSET(set, CM_OMI_FIELD_TASK_MANAGER_NID))
    {
        if(flag == 0)
        {
            CM_SNPRINTF_ADD(sql, CM_TASK_CMD_LEN, "nid = %d", pminfo->nid);
            flag = 1;
        }
        else
        {
            CM_SNPRINTF_ADD(sql, CM_TASK_CMD_LEN, " AND nid = %d", pminfo->nid);
        }
    }

    if(CM_OMI_FIELDS_FLAG_ISSET(set, CM_OMI_FIELD_TASK_MANAGER_TYPE))
    {
        if(flag == 0)
        {
            CM_SNPRINTF_ADD(sql, CM_TASK_CMD_LEN, "type = %d",
                            pminfo->type);
            flag = 1;
        }
        else
        {
            CM_SNPRINTF_ADD(sql, CM_TASK_CMD_LEN, " AND type = %d", pminfo->type);
        }
    }

    if(CM_OMI_FIELDS_FLAG_ISSET(set, CM_OMI_FIELD_TASK_MANAGER_STATE))
    {
        if(flag == 0)
        {
            CM_SNPRINTF_ADD(sql, CM_TASK_CMD_LEN, "state = %d", pminfo->state);
            flag = 1;
        }
        else
        {
            CM_SNPRINTF_ADD(sql, CM_TASK_CMD_LEN, " AND state = %d", pminfo->state);
        }
    }

    if(CM_OMI_FIELDS_FLAG_ISSET(set, CM_OMI_FIELD_TASK_MANAGER_PROG))
    {
        if(flag == 0)
        {
            CM_SNPRINTF_ADD(sql, CM_TASK_CMD_LEN, "prog = %d", pminfo->prog);
            flag = 1;
        }
        else
        {
            CM_SNPRINTF_ADD(sql, CM_TASK_CMD_LEN, " AND prog = %d", pminfo->prog);
        }
    }

    if(CM_OMI_FIELDS_FLAG_ISSET(set, CM_OMI_FIELD_TASK_MANAGER_STTM))
    {
        if(flag == 0)
        {
            CM_SNPRINTF_ADD(sql, CM_TASK_CMD_LEN, "start = %ld", pminfo->start);
            flag = 1;
        }
        else
        {
            CM_SNPRINTF_ADD(sql, CM_TASK_CMD_LEN, " AND start = %ld", pminfo->start);
        }
    }

    if(CM_OMI_FIELDS_FLAG_ISSET(set, CM_OMI_FIELD_TASK_MANAGER_ENDTM))
    {
        if(flag == 0)
        {
            CM_SNPRINTF_ADD(sql, CM_TASK_CMD_LEN, "end = %ld", pminfo->end);
            flag = 1;
        }
        else
        {
            CM_SNPRINTF_ADD(sql, CM_TASK_CMD_LEN, " AND end = %ld", pminfo->end);
        }
    }

    return CM_OK;
}

sint32 cm_cnm_task_manager_getbatch
(const void *pDecodeParam, void **ppAckData, uint32 *pAckLen)
{
    uint32 tmp = 0;
    sint32 iRet;
    uint64 offset = 0;
    uint32 total = CM_CNM_MAX_RECORD;
    sint8 param[CM_TASK_CMD_LEN] = {0};
    uint32 cnt = 0;
    cm_cnm_decode_info_t *pDecode = (cm_cnm_decode_info_t *)pDecodeParam;
    cm_cnm_task_manager_info_t *pminfo = NULL;
    cm_task_info_t *pinfo = NULL;

    if(NULL != pDecodeParam)
    {
        pminfo = (cm_cnm_task_manager_info_t *)pDecode->data;
        (void)cm_cnm_task_manager_cat_sql(&pDecode->set, param, pminfo);
        offset = pDecode->offset;
        total = pDecode->total;
    }

    CM_LOG_INFO(CM_MOD_CNM, "sql_condi:%s, offset:%llu, total:%u",
                   param, offset, total);

    iRet = cm_task_getbatch((void *)param, strlen(param),
                            offset, total, &pinfo, &cnt);

    if(CM_OK != iRet)   //malloc fail.
    {
        CM_LOG_ERR(CM_MOD_CNM, "getbatch fail[%d]", iRet);
        return CM_FAIL;
    }

    if(0 == cnt)    //no record.
    {
        if(NULL != pinfo)
        {
            CM_FREE(pinfo);
        }

        *ppAckData = NULL;
        *pAckLen = 0;
    }
    else
    {
        *ppAckData = pinfo;
        *pAckLen = cnt * sizeof(cm_task_info_t);
    }

    return CM_OK;
}

sint32 cm_cnm_task_manager_delete
(const void *pDecodeParam, void **ppAckData, uint32 *pAckLen)
{
    sint32 iRet;
    cm_cnm_decode_info_t *pDecode = (cm_cnm_decode_info_t *)pDecodeParam;
    cm_cnm_task_manager_info_t *pinfo = (cm_cnm_task_manager_info_t *)pDecode->data;
    cm_omi_field_flag_t *set = &pDecode->set;

    if(!(CM_OMI_FIELDS_FLAG_ISSET(set, CM_OMI_FIELD_TASK_MANAGER_TID)))
    {
        CM_LOG_ERR(CM_MOD_CNM, "not set tid");
        return CM_FAIL;
    }

    iRet = cm_task_delete(pinfo->tid);

    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM, "delete task fail[%d]", pinfo->tid);
        *ppAckData = NULL;
        *pAckLen = 0;
        return CM_FAIL;
    }

    *ppAckData = NULL;
    *pAckLen = 0;
    return CM_OK;
}

sint32 cm_cnm_task_manager_count
(const void *pDecodeParam, void **ppAckData, uint32 *pAckLen)
{
    sint32 iRet;
    sint8 param[CM_TASK_CMD_LEN];
    uint64 *pAck = NULL;
    cm_cnm_decode_info_t *pDecode = (cm_cnm_decode_info_t *)pDecodeParam;
    cm_cnm_task_manager_info_t *pinfo;

    param[0] = '\0';

    if(NULL != pDecodeParam)
    {
        pinfo = (cm_cnm_task_manager_info_t *)pDecode->data;
        (void)cm_cnm_task_manager_cat_sql(&pDecode->set, param, pinfo);
    }

    pAck = CM_MALLOC(sizeof(uint64));

    if(NULL == pAck)
    {
        CM_LOG_ERR(CM_MOD_CNM, "malloc fail");
        *ppAckData = NULL;
        *pAckLen = 0;
        return CM_FAIL;
    }

    iRet = cm_task_count((void *)param, strlen(param), pAck);

    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM, "get count fail");
        *ppAckData = NULL;
        *pAckLen = 0;
        CM_FREE(pAck);
        return CM_FAIL;
    }

    *ppAckData = pAck;
    *pAckLen = sizeof(uint64);
    return CM_OK;
}

void cm_cnm_task_manager_oplog_delete
(const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK) ? CM_ALARM_LOG_TASK_DELETE_OK : CM_ALARM_LOG_TASK_DELETE_FAIL;
    const cm_cnm_decode_info_t *req = pDecodeParam;
    const uint32 cnt = 1;

    if(NULL == pDecodeParam)
    {   
        cm_cnm_oplog_report(sessionid,alarmid,NULL,cnt,NULL);
    }    
    else    
    {   
        cm_cnm_task_manager_info_t *info = (cm_cnm_task_manager_info_t *)req->data;
        cm_cnm_oplog_param_t params[1] = {
            {CM_OMI_DATA_INT,CM_OMI_FIELD_TASK_MANAGER_TID,sizeof(info->tid),&info->tid},
        };
        
        cm_cnm_oplog_report(sessionid,alarmid,params,cnt,&req->set);
    }   
    return;
}

sint32 cm_cnm_task_manager_local_get
(void *param, uint32 len,
 uint64 offset, uint32 total,
 void **ppAck, uint32 *pAckLen)
{
    return CM_OK;
}

sint32 cm_cnm_task_manager_local_getbatch
(void *param, uint32 len,
 uint64 offset, uint32 total,
 void **ppAck, uint32 *pAckLen)
{
    return CM_OK;
}

sint32 cm_cnm_task_manager_local_delete
(void *param, uint32 len,
 uint64 offset, uint32 total,
 void **ppAck, uint32 *pAckLen)
{
    return CM_OK;
}

sint32 cm_cnm_task_manager_local_count
(void *param, uint32 len,
 uint64 offset, uint32 total,
 void **ppAck, uint32 *pAckLen)
{
    return CM_OK;
}





