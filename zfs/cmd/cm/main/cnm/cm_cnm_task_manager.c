#include "cm_cnm_task_manager.h"
#include "cm_cfg_omi.h"
#include "cm_log.h"
#include "cm_task.h"
#include "cm_sys.h"
#include "cm_omi.h"
#include "cm_node.h"
#include "cm_db.h"

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


#define CM_CNM_LOCALTASK_READY 0
#define CM_CNM_LOCALTASK_RUNING 1
#define CM_CNM_LOCALTASK_FINISH 2
#define CM_CNM_LOCALTASK_FINISH_FAIL 3
#define CM_CNM_LOCALTASK_EXCEPTTION 4

static cm_db_handle_t gCmLocalTaskHandle=NULL;
static cm_mutex_t gCmLocalTaskMutex;
sint32 cm_cnm_localtask_init(void)
{
    sint32 iRet = CM_FAIL;
    cm_db_handle_t handle = NULL;    
    const sint8* initdb="CREATE TABLE IF NOT EXISTS record_t ("
            "tid INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,"
            "nid INTEGER,"
            "progress INTEGER,"
            "status INTEGER,"
            "start INTEGER,"
            "end INTEGER,"
            "desc VARCHAR(256))";
    iRet = cm_db_open_ext(CM_DATA_DIR"cm_localtask.db",&handle);
    if(CM_OK != iRet)
    {
        return iRet;
    }
    iRet = cm_db_exec_ext(handle,initdb);
    (void)cm_db_exec_ext(handle,"UPDATE record_t SET status=%u,end=%u"
        " WHERE status<%u", CM_CNM_LOCALTASK_EXCEPTTION,
        (uint32)cm_get_time(),CM_CNM_LOCALTASK_FINISH);
    gCmLocalTaskHandle = handle;
    CM_MUTEX_INIT(&gCmLocalTaskMutex);
    return iRet;
}

void cm_cnm_localtask_thread(void)
{
    (void)cm_db_exec_ext(gCmLocalTaskHandle,
        "UPDATE record_t SET progress=progress+5"
        " WHERE progress<80 AND status<%u",CM_CNM_LOCALTASK_FINISH);
}

typedef struct
{
    uint32 tid;
    sint8 cmd[];
}cm_cnm_localtask_param_t;

static void* cm_cnm_localtask_exec(void* paramx)
{
    cm_cnm_localtask_param_t *param = paramx;
    sint32 iRet = CM_OK;
    
    if(NULL == param)
    {
        return NULL;
    }
    CM_LOG_WARNING(CM_MOD_CNM,"start[%u]%s",param->tid,param->cmd);
    (void)cm_db_exec_ext(gCmLocalTaskHandle,
                "UPDATE record_t SET progress=progress+5,status=%u"
                " WHERE tid=%u",CM_CNM_LOCALTASK_RUNING,param->tid);
                
    iRet = cm_system_no_tmout(param->cmd);
    CM_LOG_WARNING(CM_MOD_CNM,"end[%u] iret=%d",param->tid,iRet);

    if(iRet == CM_OK)
    {
        iRet = CM_CNM_LOCALTASK_FINISH;
    }
    else
    {
        iRet = CM_CNM_LOCALTASK_FINISH_FAIL;
    }
    (void)cm_db_exec_ext(gCmLocalTaskHandle,
                "UPDATE record_t SET progress=100,status=%d,end=%u"
                " WHERE tid=%u",iRet,(uint32)cm_get_time(),param->tid);
    CM_FREE(param);
    return NULL;
}

sint32 cm_cnm_localtask_create(
    const sint8* name, const sint8* cmd, uint32 *tid)
{
    uint64 taskid = 0;
    sint32 iRet = strlen(cmd)+1;
    cm_cnm_localtask_param_t *param = 
        CM_MALLOC(iRet+sizeof(cm_cnm_localtask_param_t));

    if(NULL == param)
    {
        CM_LOG_ERR(CM_MOD_CNM,"malloc fail");
        return CM_FAIL;
    }
    CM_VSPRINTF(param->cmd,iRet,"%s",cmd);
    CM_MUTEX_LOCK(&gCmLocalTaskMutex);
    (void)cm_db_exec_get_count(gCmLocalTaskHandle, &taskid,
        "SELECT seq+1 FROM sqlite_sequence WHERE name='record_t'"); 

    if(0 == taskid)
    {
        taskid = 1;
    }
    CM_LOG_WARNING(CM_MOD_CNM,"%s[%llu]%s",name,taskid,cmd);
    
    param->tid = (uint32)taskid;

    iRet = cm_db_exec_ext(gCmLocalTaskHandle,
        "INSERT INTO record_t (nid,progress,status,start,end,desc)"
        " VALUES(%u,0,%u,%u,0,'%s')",
        cm_node_get_id(),CM_CNM_LOCALTASK_READY,(uint32)cm_get_time(),name);
    if(CM_OK == iRet)
    {
        iRet = cm_thread_start(cm_cnm_localtask_exec,param);
        if(CM_OK != iRet)
        {
            CM_LOG_ERR(CM_MOD_CNM,"create thread[%s] fail",name);
            CM_FREE(param);
            (void)cm_db_exec_ext(gCmLocalTaskHandle,
                "DELETE FROM record_t WHERE tid=%llu",taskid);
        }
        else
        {
            *tid = (uint32)taskid;
        }
    }
    CM_MUTEX_UNLOCK(&gCmLocalTaskMutex);
    return iRet;
}

static sint32 cm_cnm_localtask_decode_ext
(const cm_omi_obj_t ObjParam, void* data, cm_omi_field_flag_t *set)
{
    sint32 iRet;
    cm_cnm_localtask_info_t *pinfo = data;
    cm_cnm_decode_param_t num_params[] =
    {
        {CM_OMI_FIELD_LOCALTASK_TID, sizeof(pinfo->tid), &pinfo->tid, NULL},
        {CM_OMI_FIELD_LOCALTASK_NID, sizeof(pinfo->nid), &pinfo->nid, NULL},
        {CM_OMI_FIELD_LOCALTASK_PROG, sizeof(pinfo->progress), &pinfo->progress, NULL},
        {CM_OMI_FIELD_LOCALTASK_STATUS, sizeof(pinfo->status), &pinfo->status, NULL},
        {CM_OMI_FIELD_LOCALTASK_START, sizeof(pinfo->start), &pinfo->start, NULL},
        {CM_OMI_FIELD_LOCALTASK_END, sizeof(pinfo->end), &pinfo->end, NULL},
    };
    return cm_cnm_decode_num(ObjParam, num_params, sizeof(num_params) / sizeof(cm_cnm_decode_param_t), set);
}

sint32 cm_cnm_localtask_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam)
{
    return cm_cnm_decode_comm(ObjParam, sizeof(cm_cnm_localtask_info_t),
                              cm_cnm_localtask_decode_ext, ppDecodeParam);
}

static void cm_cnm_localtask_encode_each
(cm_omi_obj_t item, void *eachdata, void *arg)
{
    cm_cnm_localtask_info_t *pinfo = (cm_cnm_localtask_info_t *)eachdata;
    cm_omi_field_flag_t *field = (cm_omi_field_flag_t *)arg;
    cm_cnm_map_value_num_t num_params[] =
    {
        {CM_OMI_FIELD_LOCALTASK_TID, (uint32)pinfo->tid},
        {CM_OMI_FIELD_LOCALTASK_NID, pinfo->nid},
        {CM_OMI_FIELD_LOCALTASK_PROG, pinfo->progress},
        {CM_OMI_FIELD_LOCALTASK_STATUS, pinfo->status},
        {CM_OMI_FIELD_LOCALTASK_START, pinfo->start},
        {CM_OMI_FIELD_LOCALTASK_END, pinfo->end},
    };
    cm_cnm_map_value_str_t str_params[] =
    {
        {CM_OMI_FIELD_LOCALTASK_DESC, pinfo->desc},
    };

    (void)cm_cnm_encode_num(item, field,
        num_params, sizeof(num_params) / sizeof(cm_cnm_map_value_num_t));
    (void)cm_cnm_encode_str(item, field,
        str_params, sizeof(str_params) / sizeof(cm_cnm_map_value_str_t));
    return;             
}  

cm_omi_obj_t cm_cnm_localtask_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen)
{
    return cm_cnm_encode_comm_ext(pDecodeParam, pAckData, AckLen, 
           sizeof(cm_cnm_localtask_info_t), cm_cnm_localtask_encode_each);
}

static void cm_cnm_localtask_makecond(const void *pDecodeParam,
    sint8 *cond, sint32 len)
{
    const cm_cnm_decode_info_t *decode = pDecodeParam;
    const cm_cnm_localtask_info_t *info = NULL;
    const sint8 *paramdef = "null";
    const cm_omi_field_flag_t *set = NULL;
    if(NULL == decode)
    {
        CM_VSPRINTF(cond,len,"%s,%s,%s,%s,%s",
            paramdef,paramdef,paramdef,paramdef,paramdef);
        return;
    }
    
    info = (const cm_cnm_localtask_info_t *)decode->data;
    set = &decode->set;
    if(CM_OMI_FIELDS_FLAG_ISSET(set, CM_OMI_FIELD_LOCALTASK_NID))
    {
        CM_VSPRINTF(cond,len,"%u",info->nid);
    }
    else
    {
        CM_VSPRINTF(cond,len,"%s",paramdef);
    }

    if(CM_OMI_FIELDS_FLAG_ISSET(set, CM_OMI_FIELD_LOCALTASK_PROG))
    {
        CM_SNPRINTF_ADD(cond,len,",%u",info->progress);
    }
    else
    {
        CM_SNPRINTF_ADD(cond,len,",%s",paramdef);
    }

    if(CM_OMI_FIELDS_FLAG_ISSET(set, CM_OMI_FIELD_LOCALTASK_STATUS))
    {
        CM_SNPRINTF_ADD(cond,len,",%u",info->status);
    }
    else
    {
        CM_SNPRINTF_ADD(cond,len,",%s",paramdef);
    }

    if(CM_OMI_FIELDS_FLAG_ISSET(set, CM_OMI_FIELD_LOCALTASK_START))
    {
        CM_SNPRINTF_ADD(cond,len,",%u",info->start);
    }
    else
    {
        CM_SNPRINTF_ADD(cond,len,",%s",paramdef);
    }

    if(CM_OMI_FIELDS_FLAG_ISSET(set, CM_OMI_FIELD_LOCALTASK_END))
    {
        CM_SNPRINTF_ADD(cond,len,",%u",info->end);
    }
    else
    {
        CM_SNPRINTF_ADD(cond,len,",%s",paramdef);
    }
    return;
}


static sint32 cm_cnm_localtask_local_get_each(void *arg, sint8 **cols, uint32 col_num)
{
    cm_cnm_localtask_info_t *info=arg;
    if(col_num < 7)
    {
        return CM_FAIL;
    }
    /* tid|nid|progress|status|start|end|desc */
    CM_MEM_ZERO(info,sizeof(cm_cnm_localtask_info_t));
    info->tid = atoi(cols[0]);
    info->nid = atoi(cols[1]);
    info->progress= atoi(cols[2]);
    info->status= atoi(cols[3]);
    info->start= atoi(cols[4]);
    info->end= atoi(cols[5]);
    CM_SNPRINTF_ADD(info->desc,sizeof(info->desc),"%s",cols[6]);
    for(cols+=7,col_num-=7;col_num>0;col_num--,cols++)
    {
        CM_SNPRINTF_ADD(info->desc,sizeof(info->desc)," %s",*cols);
    }
    return CM_OK;
}

sint32 cm_cnm_localtask_getbatch(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    sint32 iRet = CM_OK;
    const cm_cnm_decode_info_t *decode = pDecodeParam;
    uint32 offset = 0;
    uint32 total = CM_CNM_MAX_RECORD;
    sint8 cond[CM_STRING_128] = {0};

    if(NULL != decode)
    {
        offset = (uint32)decode->offset;
        total = decode->total;
    }
    cm_cnm_localtask_makecond(pDecodeParam,cond,sizeof(cond));
    iRet = cm_exec_get_list(cm_cnm_localtask_local_get_each,
        (uint32)offset,sizeof(cm_cnm_localtask_info_t),ppAckData,&total,
        CM_SCRIPT_DIR"cm_cnm.sh localtask_getbatch '%s'",cond);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]",iRet);
        return iRet;
    }
    *pAckLen = total * sizeof(cm_cnm_localtask_info_t);
    return CM_OK;
}

  
sint32 cm_cnm_localtask_count(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    sint8 cond[CM_STRING_128] = {0};
    uint64 cnt = 0;
    
    cm_cnm_localtask_makecond(pDecodeParam,cond,sizeof(cond));
    cnt = cm_exec_int(CM_SCRIPT_DIR"cm_cnm.sh localtask_count '%s'",cond);

    return cm_cnm_ack_uint64(cnt,ppAckData,pAckLen);
}
    

sint32 cm_cnm_localtask_get(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    const cm_cnm_decode_info_t *decode = pDecodeParam;

    if((NULL == decode) || (decode->nid == 0))
    {
        return CM_PARAM_ERR;
    }
    return cm_cnm_request_comm(CM_OMI_OBJECT_LOCAL_TASK,CM_OMI_CMD_GET,sizeof(cm_cnm_localtask_info_t),
        pDecodeParam, ppAckData, pAckLen);
}

sint32 cm_cnm_localtask_delete(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    const cm_cnm_decode_info_t *decode = pDecodeParam;

    if((NULL == decode) || (decode->nid == 0))
    {
        return CM_PARAM_ERR;
    }
    return cm_cnm_request_comm(CM_OMI_OBJECT_LOCAL_TASK,CM_OMI_CMD_DELETE,sizeof(cm_cnm_localtask_info_t),
        pDecodeParam, ppAckData, pAckLen);
}


static sint32 cm_cnm_localtask_db_get_each(
    void *arg, sint32 col_cnt, sint8 **col_vals, sint8 **col_names)
{
    cm_cnm_localtask_info_t *info = arg;

    if(6 != col_cnt)
    {
        CM_LOG_ERR(CM_MOD_CNM,"col_cnt %u",col_cnt);
        return CM_FAIL;
    }
    info->nid = cm_node_get_id();
    info->tid = atoi(col_vals[0]);
    info->progress = atoi(col_vals[1]);
    info->status= atoi(col_vals[2]);
    info->start= atoi(col_vals[3]);
    info->end= atoi(col_vals[4]);
    CM_VSPRINTF(info->desc,sizeof(info->desc),"%s",col_vals[5]);
    return CM_OK;
}


sint32 cm_cnm_localtask_local_get(void *param, uint32 len,
    uint64 offset, uint32 total,
    void **ppAck, uint32 *pAckLen)
{
    uint32 cnt = 0;
    const cm_cnm_decode_info_t *decode = param;
    const cm_cnm_localtask_info_t *infoin = (const cm_cnm_localtask_info_t *)decode->data;
    cm_cnm_localtask_info_t *info = CM_MALLOC(sizeof(cm_cnm_localtask_info_t));

    if(NULL == info)
    {
        CM_LOG_ERR(CM_MOD_CNM,"malloc fail");
        return CM_FAIL;
    }
    cnt = cm_db_exec_get(gCmLocalTaskHandle,cm_cnm_localtask_db_get_each,
        info,1,sizeof(cm_cnm_localtask_info_t),
        "SELECT tid,progress,status,start,end,desc FROM record_t WHERE tid=%u LIMIT 1",infoin->tid);
    if(0 == cnt)
    {
        CM_LOG_ERR(CM_MOD_CNM,"id=%u",infoin->tid);
        CM_FREE(info);
        return CM_FAIL;
    }
    *ppAck = info;
    *pAckLen = sizeof(cm_cnm_localtask_info_t);
    return CM_OK;
}

sint32 cm_cnm_localtask_local_delete(void *param, uint32 len,
    uint64 offset, uint32 total,
    void **ppAck, uint32 *pAckLen)
{
    const cm_cnm_decode_info_t *decode = param;
    const cm_cnm_localtask_info_t *infoin = (const cm_cnm_localtask_info_t *)decode->data;
    
    return cm_db_exec_ext(gCmLocalTaskHandle,
                "DELETE FROM record_t WHERE tid=%u",infoin->tid);
}


