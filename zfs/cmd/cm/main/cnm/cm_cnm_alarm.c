/******************************************************************************
 *          Copyright (c) 2018 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cnm_alarm.c
 * author     : wbn
 * create date: 2018年1月22日
 * description: TODO:
 *
 *****************************************************************************/
#include "cm_omi.h"
#include "cm_alarm.h"
#include "cm_db.h"
#include "cm_log.h"
#include "cm_sync.h"
#include "cm_cnm_common.h"
#include "cm_cnm_alarm.h"
#include "cm_ini.h"

typedef struct
{
    uint64 offset;
    uint64 total;
    uint64 id;
    cm_time_t time_start;
    cm_time_t time_end;

    uint8 type;
    uint8 lvl;
    uint8 is_current;
    uint8 is_getbatch;
    cm_omi_field_flag_t field;
}cm_cnm_alarm_query_t;

#define CM_ALARM_MAX_RECORD_CNT CM_CNM_MAX_RECORD

extern cm_db_handle_t cm_alarm_db_get(void);
sint32 cm_cnm_alarm_common_init(void);


static uint64 cm_cnm_alarm_record_count(const cm_cnm_alarm_query_t *query);

sint32 cm_cnm_alarm_init(void)
{
    return CM_OK;
}

sint32 cm_cnm_alarm_decode(const cm_omi_obj_t ObjParam, void **ppDecodeParam)
{
    cm_cnm_alarm_query_t *query = CM_MALLOC(sizeof(cm_cnm_alarm_query_t));
    uint32 tmp = 0;
    if(NULL == query)
    {
        CM_LOG_ERR(CM_MOD_CNM,"malloc fail");
        return CM_FAIL;
    }
    CM_MEM_ZERO(query,sizeof(cm_cnm_alarm_query_t));
    query->type= CM_ALATM_TYPE_BUTT;
    query->lvl = CM_ALATM_LVL_BUTT;
    query->total = CM_ALARM_MAX_RECORD_CNT; 
    
    cm_omi_decode_fields_flag(ObjParam,&query->field);
    
    (void)cm_omi_obj_key_get_u64_ex(ObjParam,CM_OMI_FIELD_ALARM_ID,&query->id);
    if(CM_OK == cm_omi_obj_key_get_u32_ex(ObjParam,CM_OMI_FIELD_ALARM_TYPE,&tmp))
    {
        query->type = (uint8)tmp;
    }
    if(CM_OK == cm_omi_obj_key_get_u32_ex(ObjParam,CM_OMI_FIELD_ALARM_LVL,&tmp))
    {
        query->lvl = (uint8)tmp;
    }
    
    (void)cm_omi_obj_key_get_u64_ex(ObjParam,CM_OMI_FIELD_FROM,&query->offset);
    (void)cm_omi_obj_key_get_u64_ex(ObjParam,CM_OMI_FIELD_TOTAL,&query->total);
    (void)cm_omi_obj_key_get_u32_ex(ObjParam,CM_OMI_FIELD_TIME_START,(uint32*)&query->time_start);
    (void)cm_omi_obj_key_get_u32_ex(ObjParam,CM_OMI_FIELD_TIME_END,(uint32*)&query->time_end);
    if((0 != query->time_start)
        && (0 != query->time_end)
        && (query->time_start > query->time_end))
    {
        CM_LOG_ERR(CM_MOD_CNM,"%lu %lu",query->time_start,query->time_end);
        CM_FREE(query);
        return CM_PARAM_ERR;
    }
    *ppDecodeParam = query;
    return CM_OK;
}

sint32 cm_cnm_alarm_get(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    cm_cnm_alarm_query_t *query = NULL;
    uint32 len = sizeof(cm_cnm_alarm_query_t);

    if(NULL == pDecodeParam)
    {
        return CM_PARAM_ERR;
    }
    query = CM_MALLOC(len);
    if(NULL == query)
    {
        CM_LOG_ERR(CM_MOD_CNM,"malloc fail");
        return CM_FAIL;
    }
    CM_MEM_CPY(query,len,pDecodeParam,len);
    
    query->is_getbatch = CM_FALSE;
    
    *ppAckData = query;
    *pAckLen = len;
    return CM_OK;
}

static sint32 cm_cnm_alarm_getbatch(const void *pDecodeParam,void **ppAckData, uint32 
*pAckLen, bool_t is_current)
{
    cm_cnm_alarm_query_t *query = NULL;
    uint32 len = sizeof(cm_cnm_alarm_query_t);

    query = CM_MALLOC(len);
    if(NULL == query)
    {
        CM_LOG_ERR(CM_MOD_CNM,"malloc fail");
        return CM_FAIL;
    }
    if(NULL != pDecodeParam)
    {
        CM_MEM_CPY(query,len,pDecodeParam,len);
    }
    else
    {
        CM_MEM_ZERO(query,len);
        query->type= CM_ALATM_TYPE_BUTT;
        query->lvl = CM_ALATM_LVL_BUTT;
        query->total = CM_ALARM_MAX_RECORD_CNT; 
        CM_OMI_FIELDS_FLAG_SET_ALL(&query->field);
    }
    
    query->is_current = is_current;
    query->is_getbatch = CM_TRUE;
    
    *ppAckData = query;
    *pAckLen = len;
    return CM_OK;
}

static sint32 cm_cnm_alarm_count(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen,bool_t is_current)
{
    cm_cnm_alarm_query_t *query = NULL;
    uint64 *pCnt = NULL;
    uint32 len = 0;
    sint32 iRet = cm_cnm_alarm_getbatch(pDecodeParam,(void**)&query,&len,is_current);
    
    if(CM_OK != iRet)
    {
        return iRet;
    }
    pCnt = CM_MALLOC(sizeof(uint64));
    if(NULL == pCnt)
    {
        CM_FREE(query);
        CM_LOG_ERR(CM_MOD_CNM,"malloc fail");
        return CM_FAIL;
    }
    *pCnt = cm_cnm_alarm_record_count(query);
    CM_FREE(query);

    *ppAckData = pCnt;
    *pAckLen = sizeof(uint64);
    return CM_OK;    
}

sint32 cm_cnm_alarm_current_getbatch(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_cnm_alarm_getbatch(pDecodeParam,ppAckData,pAckLen, CM_TRUE);
}
sint32 cm_cnm_alarm_current_count(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_cnm_alarm_count(pDecodeParam,ppAckData,pAckLen, CM_TRUE);
}

sint32 cm_cnm_alarm_history_getbatch(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_cnm_alarm_getbatch(pDecodeParam,ppAckData,pAckLen, CM_FALSE);
}
sint32 cm_cnm_alarm_history_count(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_cnm_alarm_count(pDecodeParam,ppAckData,pAckLen, CM_FALSE);
}

sint32 cm_cnm_alarm_history_delete(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    const cm_cnm_alarm_query_t *query = pDecodeParam;

    if(NULL == query)
    {
        CM_LOG_ERR(CM_MOD_CNM,"pDecodeParam null");
        return CM_PARAM_ERR;
    }
    
    CM_LOG_WARNING(CM_MOD_CNM,"id[%llu]",query->id);

    return cm_sync_delete(CM_SYNC_OBJ_ALARM_RECORD,(uint64)query->id);
}

static void cm_cnm_alarm_sql_common(const cm_cnm_alarm_query_t *query,
    sint8 *sql, uint32 buf_size)
{
    uint32 len = strlen(sql);

    CM_LOG_INFO(CM_MOD_CNM,"is_current[%u],type[%u],lvl[%u]",
        query->is_current,query->type,query->lvl);
    
    len += CM_VSPRINTF(sql+len,buf_size-len,
                " WHERE record_t.alarm_id=config_t.alarm_id"
                " AND record_t.alarm_id IN (SELECT alarm_id FROM config_t WHERE is_disable=0");
    if(CM_FALSE != query->is_current)
    {
        len += CM_VSPRINTF(sql+len,buf_size-len," AND type=%u",CM_ALATM_TYPE_FAULT);
    }
    else 
    {
        if(CM_ALATM_TYPE_BUTT > query->type)
        {
            len += CM_VSPRINTF(sql+len,buf_size-len," AND type=%u",query->type);
        }
    }
    
    if(CM_ALATM_LVL_BUTT > query->lvl)
    {
        len += CM_VSPRINTF(sql+len,buf_size-len," AND lvl=%u",query->lvl);
    }
    len += CM_VSPRINTF(sql+len,buf_size-len,")");

    if(CM_FALSE != query->is_current)
    {
        len += CM_VSPRINTF(sql+len,buf_size-len," AND record_t.recovery_time=0");
    }

    if(query->time_start > 0)
    {
        len += CM_VSPRINTF(sql+len,buf_size-len," AND record_t.report_time>=%u",(uint32)query->time_start);
    }

    if(query->time_end > 0)
    {
        len += CM_VSPRINTF(sql+len,buf_size-len," AND record_t.report_time<%u",(uint32)query->time_end);
    }

    return;
}

static uint64 cm_cnm_alarm_record_count(const cm_cnm_alarm_query_t *query)
{
    cm_db_handle_t handle=cm_alarm_db_get();
    sint8 sql[CM_STRING_512] = {0};
    uint64 count = 0;
    
    (void)CM_VSPRINTF(sql,CM_STRING_512,"SELECT COUNT(id) FROM record_t,config_t");
    cm_cnm_alarm_sql_common(query,sql,sizeof(sql));

    CM_LOG_INFO(CM_MOD_CNM,"%s",sql);
    
    (void)cm_db_exec_get_count(handle,&count,"%s",sql);
    return count;
}

static void cm_cnm_alarm_record_sql(const cm_cnm_alarm_query_t *query,
    sint8 *sql, uint32 buf_size)
{
    uint32 len = 0;
    uint64 cnt = CM_MIN(query->total,CM_ALARM_MAX_RECORD_CNT);
    /* f<x>中 x和cm_omi_field_alarm_e中的定义一致 */
    const sint8* fields[] = {
        "record_t.id AS f0",
        "record_t.alarm_id AS f1",
        "config_t.type AS f2",
        "config_t.lvl AS f3",
        "record_t.report_time AS f4",
        "record_t.recovery_time AS f5",
        "record_t.param AS f6"
        };
    
    cm_omi_make_select_field(fields,sizeof(fields)/sizeof(sint8*),
        &query->field,sql,buf_size);

    len = strlen(sql);
    
    len += CM_VSPRINTF(sql+len,buf_size-len," FROM record_t,config_t");

    if(CM_FALSE == query->is_getbatch)
    {
        len += CM_VSPRINTF(sql+len,buf_size-len,
            " WHERE record_t.id=%llu"
            " AND config_t.alarm_id IN "
            "(SELECT alarm_id FROM record_t WHERE id=%llu)",
            query->id,query->id);
        return;
    }

    cm_cnm_alarm_sql_common(query,sql,buf_size);

    len = strlen(sql);
    
    len += CM_VSPRINTF(sql+len,buf_size-len," ORDER BY record_t.id DESC"
        " LIMIT %llu,%llu",query->offset,cnt);

    CM_LOG_INFO(CM_MOD_CNM,"%s",sql);
    return;
}

cm_omi_obj_t cm_cnm_alarm_encode(const void *pDecodeParam, void *pAckData, uint32 AckLen)
{
    cm_db_handle_t handle=cm_alarm_db_get();
    sint8 sql[CM_STRING_1K] = {0};
    cm_omi_obj_t items = NULL;
    cm_cnm_alarm_query_t *query = pAckData;
    
    if(NULL == pAckData)
    {
        CM_LOG_ERR(CM_MOD_CNM,"pAckData null");
        return NULL;
    }    
    
    items = cm_omi_obj_new_array();
    if(NULL == items)
    {
        CM_LOG_ERR(CM_MOD_CNM,"new items fail");
        return NULL;
    }

    cm_cnm_alarm_record_sql(query,sql,sizeof(sql));
    (void)cm_db_exec(handle,cm_omi_encode_db_record_each,items,"%s",sql);
    return items;
}


typedef struct
{
    cm_alarm_config_t cfg;
    cm_omi_field_flag_t field;
    cm_omi_field_flag_t set;
    uint64 offset;
    uint64 total;
    uint8 is_getbatch;
}cm_cnm_alarm_cfg_t;

sint32 cm_cnm_alarm_cfg_decode(const cm_omi_obj_t ObjParam, void
**ppDecodeParam)
{
    uint32 tmp = 0;
    cm_alarm_config_t *cfg = NULL;
    cm_cnm_alarm_cfg_t *deocde = CM_MALLOC(sizeof(cm_cnm_alarm_cfg_t));
    
    if(NULL == deocde)
    {
        CM_LOG_ERR(CM_MOD_CNM,"malloc fail");
        return CM_FAIL;
    }
    cfg = &deocde->cfg;
    CM_MEM_ZERO(deocde,sizeof(cm_cnm_alarm_cfg_t));
    cm_omi_decode_fields_flag(ObjParam,&deocde->field);
    deocde->total = CM_ALARM_MAX_RECORD_CNT;

    (void)cm_omi_obj_key_get_u32_ex(ObjParam,CM_OMI_FIELD_ALARM_CFG_ID,&cfg->alarm_id);
    CM_OMI_FIELDS_FLAG_SET(&deocde->set,CM_OMI_FIELD_ALARM_CFG_ID);
    if(CM_OK == cm_omi_obj_key_get_u32_ex(ObjParam,CM_OMI_FIELD_ALARM_CFG_LVL,&tmp))
    {
        if(tmp < CM_ALATM_LVL_BUTT)
        {
            cfg->lvl = (uint8)tmp;
            CM_OMI_FIELDS_FLAG_SET(&deocde->set,CM_OMI_FIELD_ALARM_CFG_LVL);
        }
    }
    if(CM_OK == cm_omi_obj_key_get_u32_ex(ObjParam,CM_OMI_FIELD_ALARM_CFG_TYPE,&tmp))
    {
        if(tmp < CM_ALATM_TYPE_BUTT)
        {
            cfg->type= (uint8)tmp;
            CM_OMI_FIELDS_FLAG_SET(&deocde->set,CM_OMI_FIELD_ALARM_CFG_TYPE);
        }
    }
    if(CM_OK == cm_omi_obj_key_get_u32_ex(ObjParam,CM_OMI_FIELD_ALARM_CFG_IS_DISABLE,&tmp))
    {
        if(tmp <= CM_TRUE)
        {
            cfg->is_disable = (uint8)tmp;
            CM_OMI_FIELDS_FLAG_SET(&deocde->set,CM_OMI_FIELD_ALARM_CFG_IS_DISABLE);
        }
    }

    (void)cm_omi_obj_key_get_u64_ex(ObjParam,CM_OMI_FIELD_FROM,&deocde->offset);
    (void)cm_omi_obj_key_get_u64_ex(ObjParam,CM_OMI_FIELD_TOTAL,&deocde->total);

    CM_LOG_INFO(CM_MOD_CNM,"id[%u] type[%u] lvl[%u] offset[%llu] t[%llu]",
        cfg->alarm_id,cfg->type,cfg->lvl,deocde->offset,deocde->total);
    *ppDecodeParam = deocde;

    return CM_OK;
}

static void cm_cnm_alarm_cfg_sql_where(const cm_cnm_alarm_cfg_t *cfg,
    sint8 *sql, uint32 buf_size)
{
    uint32 len = strlen(sql);
    uint32 cnt = 3;
    uint8 index[] = {
        CM_OMI_FIELD_ALARM_CFG_TYPE,
        CM_OMI_FIELD_ALARM_CFG_LVL,
        CM_OMI_FIELD_ALARM_CFG_IS_DISABLE};
    uint8 value[] = {cfg->cfg.type,cfg->cfg.lvl,cfg->cfg.is_disable};
    const sint8* names[] = {"type","lvl","is_disable"};
    bool_t is_first = CM_TRUE;
    
    while(cnt > 0)
    {
        cnt--;
        if(!CM_OMI_FIELDS_FLAG_ISSET(&cfg->set,index[cnt]))
        {
            continue;
        }
        if(CM_FALSE != is_first)
        {
            is_first = CM_FALSE;
            len += CM_VSPRINTF(sql+len,buf_size-len," WHERE %s=%u",
                names[cnt],value[cnt]);
        }
        else
        {
            len += CM_VSPRINTF(sql+len,buf_size-len," AND %s=%u",
                names[cnt],value[cnt]);
        }
    }
    return;
}

cm_omi_obj_t cm_cnm_alarm_cfg_encode(const void *pDecodeParam, void *pAckData, uint32 AckLen)
{
    cm_cnm_alarm_cfg_t *cfg = pAckData;
    sint8 sql[CM_STRING_512] = {0};
    uint32 buf_size = sizeof(sql);
    uint32 len = 0;
    /* f<x>中 x和cm_omi_field_alarm_e中的定义一致 */
    const sint8* fields[] = {
        "alarm_id AS f0",
        "match_bits AS f1",
        "param_num AS f2",
        "type AS f3",
        "lvl AS f4",
        "is_disable AS f5"
        };
    cm_omi_obj_t items = NULL;
    cm_db_handle_t handle=cm_alarm_db_get();
    
    if(NULL == pAckData)
    {
        CM_LOG_ERR(CM_MOD_CNM,"pAckData null");
        return NULL;
    }    
    
    items = cm_omi_obj_new_array();
    if(NULL == items)
    {
        CM_LOG_ERR(CM_MOD_CNM,"new items fail");
        return NULL;
    }
    
    cm_omi_make_select_field(fields,sizeof(fields)/sizeof(sint8*),
        &cfg->field,sql,buf_size);
    len = strlen(sql);
    
    len += CM_VSPRINTF(sql+len,buf_size-len," FROM config_t");

    if(CM_FALSE == cfg->is_getbatch)
    {
        len += CM_VSPRINTF(sql+len,buf_size-len," WHERE alarm_id=%u",cfg->cfg.alarm_id);
    }
    else 
    {
        cm_cnm_alarm_cfg_sql_where(cfg,sql,buf_size);
        len = strlen(sql);
        cfg->total = CM_MIN(cfg->total,CM_ALARM_MAX_RECORD_CNT);
        len += CM_VSPRINTF(sql+len,buf_size-len," LIMIT %llu,%llu",cfg->offset,cfg->total);
    }
    CM_LOG_INFO(CM_MOD_CNM,"%s",sql);
    (void)cm_db_exec(handle,cm_omi_encode_db_record_each,items,"%s",sql);
    return items;
}

sint32 cm_cnm_alarm_cfg_get(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    cm_cnm_alarm_cfg_t *query = NULL;
    uint32 len = sizeof(cm_cnm_alarm_cfg_t);

    if(NULL == pDecodeParam)
    {
        return CM_PARAM_ERR;
    }
    query = CM_MALLOC(len);
    if(NULL == query)
    {
        CM_LOG_ERR(CM_MOD_CNM,"malloc fail");
        return CM_FAIL;
    }
    CM_MEM_CPY(query,len,pDecodeParam,len);
    
    query->is_getbatch = CM_FALSE;
    
    *ppAckData = query;
    *pAckLen = len;
    return CM_OK;
}
sint32 cm_cnm_alarm_cfg_getbatch(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{

    cm_cnm_alarm_cfg_t *query = NULL;
    uint32 len = sizeof(cm_cnm_alarm_cfg_t);
    
    query = CM_MALLOC(len);
    if(NULL == query)
    {
        CM_LOG_ERR(CM_MOD_CNM,"malloc fail");
        return CM_FAIL;
    }
    if(NULL != pDecodeParam)
    {
        CM_MEM_CPY(query,len,pDecodeParam,len);
    }
    else
    {
        CM_MEM_ZERO(query,len);
        query->total = CM_ALARM_MAX_RECORD_CNT; 
        CM_OMI_FIELDS_FLAG_SET_ALL(&query->field);
    }

    query->is_getbatch = CM_TRUE;
    
    *ppAckData = query;
    *pAckLen = len;
    return CM_OK;
}

sint32 cm_cnm_alarm_cfg_count(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    sint8 sql[CM_STRING_512] = {0};
    uint32 buf_size = sizeof(sql);
    const cm_cnm_alarm_cfg_t *query = pDecodeParam;
    uint64 *pCnt = CM_MALLOC(sizeof(uint64));
    
    if(NULL == pCnt)
    {
        CM_LOG_ERR(CM_MOD_CNM,"malloc fail");
        return CM_FAIL;
    }
    CM_VSPRINTF(sql,buf_size,"SELECT COUNT(alarm_id) FROM config_t");
    if(NULL != query)
    {
        cm_cnm_alarm_cfg_sql_where(query,sql,buf_size);
    }
    (void)cm_db_exec_get_count(cm_alarm_db_get(),pCnt,"%s",sql);

    *ppAckData = pCnt;
    *pAckLen = sizeof(uint64);
    return CM_OK;
}

sint32 cm_cnm_alarm_cfg_update(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    const cm_cnm_alarm_cfg_t *query = pDecodeParam;
    cm_alarm_config_t cfg;
    
    if(NULL == query)
    {
        CM_LOG_ERR(CM_MOD_CNM,"pDecodeParam null");
        return CM_FAIL;
    }
    if(CM_OK != cm_alarm_cfg_get((uint32)query->cfg.alarm_id,&cfg))
    {
        CM_LOG_ERR(CM_MOD_CNM,"alarm_id[%u] null",query->cfg.alarm_id);
        return CM_FAIL;
    }
    if(CM_OMI_FIELDS_FLAG_ISSET(&query->set,CM_OMI_FIELD_ALARM_CFG_LVL))
    {
        cfg.lvl = query->cfg.lvl;
    }
    if(CM_OMI_FIELDS_FLAG_ISSET(&query->set,CM_OMI_FIELD_ALARM_CFG_IS_DISABLE))
    {
        cfg.is_disable = query->cfg.is_disable;
    }
    return cm_sync_request(CM_SYNC_OBJ_ALARM_CONFIG,
        (uint64)query->cfg.alarm_id,&cfg,sizeof(cfg));
}

void cm_cnm_alarm_cfg_oplog_update(const sint8* sessionid,const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK) ? CM_ALARM_LOG_ALARM_CFG_MODIFY_OK : 
        CM_ALARM_LOG_ALARM_CFG_MODIFY_FAIL;
    const cm_cnm_alarm_cfg_t *req = pDecodeParam;
    const uint32 cnt = 3;

    if(NULL == pDecodeParam)
    {   
        cm_cnm_oplog_report(sessionid,alarmid,NULL,cnt,NULL);
    }    
    else    
    {   
        cm_alarm_config_t *info = &req->cfg;
        cm_cnm_oplog_param_t params[3] = {
            {CM_OMI_DATA_INT,CM_OMI_FIELD_ALARM_CFG_ID,sizeof(info->alarm_id),&info->alarm_id},
            {CM_OMI_DATA_INT,CM_OMI_FIELD_ALARM_CFG_LVL,sizeof(info->lvl),&info->lvl},
            {CM_OMI_DATA_ENUM,CM_OMI_FIELD_ALARM_CFG_IS_DISABLE,sizeof(info->is_disable),&info->is_disable},
        };
        
        cm_cnm_oplog_report(sessionid,alarmid,params,cnt,&req->set);
    }   
    return;
}

sint32 cm_cnm_alarm_cfg_sync_request(uint64 data_id, void *pdata, uint32 len)
{
    cm_alarm_config_t *cfg = pdata;
    cm_db_handle_t handle=cm_alarm_db_get();
    
    if(len != sizeof(cm_alarm_config_t))
    {
        CM_LOG_ERR(CM_MOD_CNM,"len[%u]",len);
        return CM_FAIL;
    }
    /* 仅支持修改 */
    return cm_db_exec_ext(handle,"UPDATE config_t SET lvl=%u,is_disable=%u"
        " WHERE alarm_id=%u",cfg->lvl,cfg->is_disable,(uint32)data_id);
}
sint32 cm_cnm_alarm_cfg_sync_get(uint64 data_id, void **pdata, uint32 *plen)
{
    sint32 iRet = CM_FAIL;
    cm_alarm_config_t *cfg = CM_MALLOC(sizeof(cm_alarm_config_t));
    
    if(NULL == cfg)
    {
        CM_LOG_ERR(CM_MOD_CNM,"malloc fail");
        return CM_FAIL;
    }
    iRet = cm_alarm_cfg_get((uint32)data_id,cfg);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"get %llu fail",data_id);
        CM_FREE(cfg);
        return iRet;
    }
    *pdata = cfg;
    *plen = sizeof(cfg);
    return CM_OK;
}
sint32 cm_cnm_alarm_cfg_sync_delete(uint64 data_id)
{
    return CM_OK;
}


/******************************************************************************/

static sint32 cm_cnm_threshold_decode_ext(
    const cm_omi_obj_t ObjParam,void* data,cm_omi_field_flag_t *set)
{
    sint32 iRet = CM_OK;
    cm_alarm_threshold_t *info = data;
    
    cm_cnm_decode_param_t param_num[] = 
    {
        {CM_OMI_FIELD_THRESHOLD_ALARM_ID,sizeof(info->alarm_id),&info->alarm_id,NULL},
        {CM_OMI_FIELD_THRESHOLD_RECOVERVAL,sizeof(info->recovery),&info->recovery,NULL},
        {CM_OMI_FIELD_THRESHOLD_VALUE,sizeof(info->report),&info->report,NULL},
    };
    iRet = cm_cnm_decode_num(ObjParam,param_num,
        sizeof(param_num)/sizeof(cm_cnm_decode_param_t),set);
    if(iRet != CM_OK)
    {
        return iRet;
    }
    return CM_OK;
}
sint32 cm_cnm_threshold_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam)
{
    return cm_cnm_decode_comm(ObjParam,sizeof(cm_alarm_threshold_t),
        cm_cnm_threshold_decode_ext,ppDecodeParam);
}
static void cm_cnm_threshold_encode_each(cm_omi_obj_t item,void *eachdata,void *arg)
{
    cm_omi_field_flag_t *field = arg;
    cm_alarm_threshold_t *info = eachdata;
    cm_cnm_map_value_num_t cols_num[] = 
    {
        {CM_OMI_FIELD_THRESHOLD_RECOVERVAL,      info->recovery},
        {CM_OMI_FIELD_THRESHOLD_ALARM_ID,        info->alarm_id},
        {CM_OMI_FIELD_THRESHOLD_VALUE,           info->report},
    };
    cm_cnm_encode_num(item,field,cols_num,sizeof(cols_num)/sizeof(cm_cnm_map_value_num_t)); 
    return;
}
cm_omi_obj_t cm_cnm_threshold_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen)
{
    return cm_cnm_encode_comm_ext(pDecodeParam,pAckData,AckLen,
        sizeof(cm_alarm_threshold_t),cm_cnm_threshold_encode_each);
}

extern sint32 cm_alarm_get_threshold_each(
    void *arg, sint32 col_cnt, sint8 **col_vals, sint8 **col_names);


sint32  cm_cnm_threshold_update(const void *pDecodeParam,void **ppAckData,uint32 * pAckLen)
{
    const cm_cnm_decode_info_t *decode = pDecodeParam;
    cm_alarm_threshold_t info = {0,0,0};
    const cm_alarm_threshold_t *req= NULL;
    sint32 iRet = CM_FAIL;
    if(decode == NULL)
    {
        CM_LOG_ERR(CM_MOD_CNM,"param null");
        return CM_PARAM_ERR;
    }
    req = (const cm_alarm_threshold_t *)decode->data;
    if(!CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_THRESHOLD_ALARM_ID))
    {
        CM_LOG_ERR(CM_MOD_CNM,"alarm id null");
        return CM_PARAM_ERR;
    }
    info.alarm_id = req->alarm_id;
    iRet = cm_alarm_threshold_get(req->alarm_id, &info.report,&info.recovery);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"aid:%u",req->alarm_id);
        return CM_ERR_NOT_EXISTS;
    }
    
    if(CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_THRESHOLD_RECOVERVAL))
    {
        info.recovery = req->recovery;
    }
    if(CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_THRESHOLD_VALUE))
    {
        info.report= req->report;
    }
    return cm_sync_request(CM_SYNC_OBJ_THRESHOLD,(uint64)info.alarm_id,&info,sizeof(cm_alarm_threshold_t));
}    

sint32 cm_cnm_threshold_get(const void * pDecodeParam,void **ppAckData,uint32 * pAckLen)
{
    const cm_cnm_decode_info_t *decode = pDecodeParam;
    const cm_alarm_threshold_t *req= NULL;
    
    if(decode == NULL)
    {
        CM_LOG_ERR(CM_MOD_CNM,"param null");
        return CM_PARAM_ERR;
    }
    req = (const cm_alarm_threshold_t *)decode->data;
    if(!CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_THRESHOLD_ALARM_ID))
    {
        CM_LOG_ERR(CM_MOD_CNM,"alarm id null");
        return CM_PARAM_ERR;
    }
    return cm_cnm_threshold_sync_get(req->alarm_id,ppAckData,pAckLen);
}
sint32 cm_cnm_threshold_getbatch(const void * pDecodeParam,void **ppAckData,uint32 * pAckLen)
{
    const cm_cnm_decode_info_t *dec = pDecodeParam;
    cm_alarm_threshold_t *info = NULL;
    uint32 offset = 0;
    uint32 total = CM_CNM_MAX_RECORD;
    if(dec != NULL && dec->offset != 0)
    {
        offset = (uint32)dec->offset;
    }
    if(dec != NULL && dec->total != 0)
    {
        total = dec->total;
    }
    info = CM_MALLOC(sizeof(cm_alarm_threshold_t)*total);
    total = cm_db_exec_get(cm_alarm_db_get(), cm_alarm_get_threshold_each, info,
                               total, sizeof(cm_alarm_threshold_t),
                               "SELECT * FROM threshold_t ORDER BY alarm_id DESC LIMIT %u,%u",offset,total);
    if(total == 0)
    {
        CM_LOG_ERR(CM_MOD_CNM,"threshold getbatch fail");
        CM_FREE(info);
        return CM_OK;
    }
    *ppAckData = info;
    *pAckLen = total * sizeof(cm_alarm_threshold_t);
    return CM_OK;
}

sint32 cm_cnm_threshold_count(const void * pDecodeParam,void **ppAckData,uint32 *pAckLen)
{
    uint64 cnt = 0;
    sint32 iRet = CM_OK;
    iRet = cm_db_exec_get_count(cm_alarm_db_get(),&cnt,"SELECT COUNT(*) FROM threshold_t");
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM, "get count fail[%d]", iRet);
        return CM_FAIL;
    }
    return cm_cnm_ack_uint64(cnt,ppAckData,pAckLen);
}

sint32 cm_cnm_threshold_sync_request(uint64 data_id, void *pdata, uint32 len)
{
    cm_alarm_threshold_t *info = (cm_alarm_threshold_t*)pdata;

    /* 仅支持修改 */
    return cm_db_exec_ext(cm_alarm_db_get(),"UPDATE threshold_t SET threshold=%u,recoverval=%u"
        " WHERE alarm_id=%u",info->report,info->recovery,(uint32)data_id);;
}
sint32 cm_cnm_threshold_sync_get(uint64 data_id, void **pdata, uint32 *plen)
{
    sint32 iRet = CM_OK;
    cm_alarm_threshold_t *info = CM_MALLOC(sizeof(cm_alarm_threshold_t));
    if(NULL == info)
    {
        return CM_FAIL;
    }
    iRet = cm_alarm_threshold_get((uint32)data_id, &info->report,&info->recovery);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"aid:%llu",data_id);
        CM_FREE(info);
        return CM_ERR_NOT_EXISTS;
    }
    
    info->alarm_id = (uint32)data_id;
    *pdata = info;
    *plen = sizeof(cm_alarm_threshold_t);
    return CM_OK;
}


/* lun nas pool 
*  alarm and event
*  if the capacoty exceed the limit
*/

#define CM_CNM_ALARM_REPORT 10000014
#define CM_CNM_ALARM_EVENT 40000004

typedef struct cm_cnm_storage_alarm_node_tt
{
    struct cm_cnm_storage_alarm_node_tt* pnext;
    uint8 status;
    uint8 flag; /*whether or not node is find*/
    uint8 new_flag;  /*whether or not node is new add*/
    sint8 name[CM_STRING_128];
}cm_cnm_storage_alarm_node_t;

typedef struct
{
    uint32 type;
    cm_cnm_storage_alarm_node_t* head;
}cm_cnm_storage_alarm_head_t;

typedef enum
{
    CM_CNM_ALARM_STATUS_NORMAL=0,
    CM_CNM_ALARM_STATUS_EVENT,
    CM_CNM_ALARM_STATUS_ALARM
}cm_cnm_alarm_status_e;

typedef enum
{
    CM_CNM_ALARMA_STORAGE_POOL=0,
    CM_CNM_ALARMA_STORAGE_NAS,
    CM_CNM_ALARMA_STORAGE_LUN
}cm_cnm_alarm_storage_type_e;



cm_cnm_storage_alarm_head_t* cm_cnm_alarm_pool_nood_root = NULL;
cm_cnm_storage_alarm_head_t* cm_cnm_alarm_nas_nood_root = NULL;
cm_cnm_storage_alarm_head_t* cm_cnm_alarm_lun_nood_root = NULL;
sint8 *cm_cnm_storage_alarm_sh = "/var/cm/script/cm_cnm_alarm_storage.sh";

uint32 cm_storage_alarm_value=0;
uint32 cm_storage_event_value=0;
uint32 cm_storage_alarm_recovery=0;
uint32 cm_storage_event_recovery=0;


static void cm_cnm_storage_get_alarm_param(char* param,char* name,uint32 type,uint32 used)
{
    if(type == CM_CNM_ALARMA_STORAGE_POOL)
    {
        CM_VSPRINTF(param,CM_STRING_128,"POOL,%s,%u",name,used);
    }else if(type == CM_CNM_ALARMA_STORAGE_NAS)
    {
        CM_VSPRINTF(param,CM_STRING_128,"NAS,%s,%u",name,used);
    }else
    {
        CM_VSPRINTF(param,CM_STRING_128,"LUN,%s,%u",name,used);
    }

    return;
}

static cm_cnm_storage_alarm_head_t* cm_cnm_storage_link_head_init(uint32 type)
{
    cm_cnm_storage_alarm_head_t* pnode = (cm_cnm_storage_alarm_head_t* )CM_MALLOC(sizeof(cm_cnm_storage_alarm_head_t));

    pnode->type = type;
    pnode->head = NULL;

    return pnode;
}

static sint32 cm_cnm_storage_link_add(cm_cnm_storage_alarm_head_t* root,const sint8 *name,uint32 status)
{
    cm_cnm_storage_alarm_node_t* pnode = root->head;
    cm_cnm_storage_alarm_node_t* node = NULL;

    node = (cm_cnm_storage_alarm_node_t* )CM_MALLOC(sizeof(cm_cnm_storage_alarm_node_t));
    if(node == NULL)
    {
        return 1;
    }
    node->status = status;
    node->pnext = NULL;
    node->flag = 0;
    node->new_flag = 0;
    CM_MEM_CPY(node->name,sizeof(node->name),name,sizeof(node->name));
    
    if(pnode == NULL)
    {
        root->head = node;
        return 0;
    }

    while(pnode->pnext != NULL)
    {
        pnode = pnode->pnext;
    }

    pnode->pnext = node;
    return 0;
}

static void cm_cnm_storage_link_delete(cm_cnm_storage_alarm_node_t** root,uint32 type)
{
    cm_cnm_storage_alarm_node_t* pnode = *root;
    cm_cnm_storage_alarm_node_t* tmp = *root;
    sint8 param[128] = {0};

    while(*root!=NULL)
    {
        pnode = *root;
        if(pnode->flag == 0 && pnode->new_flag!=0){
            *root = pnode->pnext;
            if(pnode->status == CM_CNM_ALARM_STATUS_ALARM)
            {
                cm_cnm_storage_get_alarm_param(param,pnode->name,type,0);
                CM_ALARM_RECOVERY(CM_CNM_ALARM_REPORT,param);
            }
            CM_LOG_WARNING(CM_MOD_CNM,"delete node name:%s",pnode->name);
            CM_FREE(pnode); 
            continue;
        }
        break;
    }

    if(pnode == NULL)
    {
        return;
    }
    
    while(pnode->pnext != NULL)
    {
        if(pnode->pnext->flag == 0 && pnode->pnext->new_flag!=0)
        {
            tmp = pnode->pnext;
            pnode->pnext = tmp->pnext;
            if(tmp->status == CM_CNM_ALARM_STATUS_ALARM)
            {
                cm_cnm_storage_get_alarm_param(param,tmp->name,type,0);
                CM_ALARM_RECOVERY(CM_CNM_ALARM_REPORT,param);
            }
            CM_LOG_WARNING(CM_MOD_CNM,"delete node name:%s",tmp->name);
            CM_FREE(tmp);
            continue;
        }

        pnode = pnode->pnext;
    }

    return ;
}

static cm_cnm_storage_alarm_node_t* cm_cnm_storage_link_find(cm_cnm_storage_alarm_head_t* root,const sint8* name)
{
    cm_cnm_storage_alarm_node_t* pnode = root->head;

    while(pnode != NULL)
    {
        if(0 == strcmp(pnode->name,name))
        {
            pnode->flag = 1;
            return pnode;
        }
         
        pnode = pnode->pnext;
    }

    return NULL;
}

static void cm_cnm_storage_link_newflag_set(void)
{
    cm_cnm_storage_alarm_node_t* ppool = cm_cnm_alarm_pool_nood_root->head;
    cm_cnm_storage_alarm_node_t* pnas = cm_cnm_alarm_nas_nood_root->head;
    cm_cnm_storage_alarm_node_t* plun = cm_cnm_alarm_lun_nood_root->head;

    while(ppool != NULL)
    {
        ppool->new_flag = 1;
        ppool->flag = 0;
        ppool = ppool->pnext;
    }

    while(pnas != NULL)
    {
        pnas->new_flag = 1;
        pnas->flag = 0;
        pnas = pnas->pnext;
    }

    while(plun != NULL)
    {
        plun->new_flag = 1;
        plun->flag = 0;
        plun = plun->pnext;
    }

    return;
}


static cm_cnm_storage_alarm_node_t* cm_cnm_storage_alarm_find(const sint8* name,uint32 type)
{
    cm_cnm_storage_alarm_node_t* p = NULL;
    if(type == CM_CNM_ALARMA_STORAGE_POOL)
    {
        p = cm_cnm_storage_link_find(cm_cnm_alarm_pool_nood_root,name);
    }else if(type == CM_CNM_ALARMA_STORAGE_NAS)
    {
        p = cm_cnm_storage_link_find(cm_cnm_alarm_nas_nood_root,name);
    }else
    {
        p = cm_cnm_storage_link_find(cm_cnm_alarm_lun_nood_root,name);
    }

    return p;
}


static void cm_cnm_storage_alarm_delete(void)
{
    cm_cnm_storage_link_delete(&(cm_cnm_alarm_pool_nood_root->head),cm_cnm_alarm_pool_nood_root->type);
    cm_cnm_storage_link_delete(&(cm_cnm_alarm_nas_nood_root->head),cm_cnm_alarm_nas_nood_root->type);
    cm_cnm_storage_link_delete(&(cm_cnm_alarm_lun_nood_root->head),cm_cnm_alarm_lun_nood_root->type);

    return;
}

static void cm_cnm_storage_alarm_add(const sint8* name,uint32 type,sint32 used)
{
    uint32 status = 0;
    sint8 param[128] = {0};

    cm_cnm_storage_get_alarm_param(param,name,type,used);
    if(used>=cm_storage_alarm_value)
    {
        CM_ALARM_REPORT(CM_CNM_ALARM_REPORT,param);
        status = CM_CNM_ALARM_STATUS_ALARM;
    }else if(used>=cm_storage_event_value&&used<cm_storage_alarm_value)
    {
        CM_ALARM_REPORT(CM_CNM_ALARM_EVENT,param);
        status = CM_CNM_ALARM_STATUS_EVENT;
    }else
    {
        status = CM_CNM_ALARM_STATUS_NORMAL;
    }
    if(type == CM_CNM_ALARMA_STORAGE_POOL)
    {
        cm_cnm_storage_link_add(cm_cnm_alarm_pool_nood_root,name,status);
    }else if(type == CM_CNM_ALARMA_STORAGE_NAS)
    {
        cm_cnm_storage_link_add(cm_cnm_alarm_nas_nood_root,name,status);
    }else
    {
        cm_cnm_storage_link_add(cm_cnm_alarm_lun_nood_root,name,status);
    }

    return;
}

static void cm_cnm_storage_alarm_process(const sint8* name,uint32 type,const sint8* percent)
{
    cm_cnm_storage_alarm_node_t *pnode = NULL;
    sint32 used = atoi(percent);
    sint8 param[128] = {0};

    pnode = cm_cnm_storage_alarm_find(name,type);
    if(pnode == NULL)
    {
        cm_cnm_storage_alarm_add(name,type,used);
        return;
    }

    cm_cnm_storage_get_alarm_param(param,name,type,used);
    if(pnode->status == CM_CNM_ALARM_STATUS_NORMAL)
    {
        if(used >= cm_storage_alarm_value)
        {
            pnode->status = CM_CNM_ALARM_STATUS_ALARM;
            CM_ALARM_REPORT(CM_CNM_ALARM_REPORT,param);
            return;
        }

        if(used >= cm_storage_event_value)
        {
            pnode->status = CM_CNM_ALARM_STATUS_EVENT;
            CM_ALARM_REPORT(CM_CNM_ALARM_EVENT,param);
            return;
        }

        return;
    }

    if(pnode->status == CM_CNM_ALARM_STATUS_EVENT)
    {
       if(used < cm_storage_event_value)
       {
            pnode->status = CM_CNM_ALARM_STATUS_NORMAL;
            return;
       }

       if(used >= cm_storage_alarm_value)
       {
            pnode->status = CM_CNM_ALARM_STATUS_ALARM;
            CM_ALARM_REPORT(CM_CNM_ALARM_REPORT,param);
            return;
       }

       return;
    }

    if(pnode->status == CM_CNM_ALARM_STATUS_ALARM)
    {
       if(used < cm_storage_event_value)
       {
           pnode->status = CM_CNM_ALARM_STATUS_NORMAL;
           CM_ALARM_RECOVERY(CM_CNM_ALARM_REPORT,param);
           return;
       }

       if(used < cm_storage_alarm_recovery)
       {
           pnode->status = CM_CNM_ALARM_STATUS_EVENT;
           CM_ALARM_RECOVERY(CM_CNM_ALARM_REPORT,param);
           return;
       }
    }

    return;
}

static void cm_cnm_storage_alarm_fenge(char* node)
{
    sint8 *p = NULL;
    sint8 name[32] = {'\0'};
    sint8 type[32] = {'\0'};
    sint8 percent[32] = {'\0'};
    sint32 j = 0;
    sint32 douhao=0;
    uint32 type_s = 0;
    
    p = node;
    for(;*p != '\0';p++){
        if(*p == ','){
            douhao++;
            j=0;
            continue;
        }
        if(douhao==0){
            name[j]=*p;
        }
        if(douhao==1){
            type[j]=*p;
        }
        if(douhao==2){
            percent[j]=*p;
        }
        j++;
    }

    type_s = atoi(type);
    cm_cnm_storage_alarm_process(name,type_s,percent);
    return;
}



void cm_cnm_storage_alarm_thread(void)
{
    sint8 list[CM_STRING_512] = {0};
    sint8* node;
    sint32 iRet = CM_OK;
    
    cm_cnm_storage_link_newflag_set();
    iRet = cm_alarm_threshold_get(CM_CNM_ALARM_REPORT,&cm_storage_alarm_value,&cm_storage_alarm_recovery);
    if(iRet != CM_OK)
    {
        CM_LOG_ERR(CM_MOD_CNM,"database get fail");
        return;
    }
    iRet = cm_alarm_threshold_get(CM_CNM_ALARM_EVENT,&cm_storage_event_value,&cm_storage_event_recovery);
    if(iRet != CM_OK)
    {
        CM_LOG_ERR(CM_MOD_CNM,"database get fail");
        return;
    }
    
    iRet = cm_exec_tmout(list,CM_STRING_512,CM_CMT_REQ_TMOUT,"%s",cm_cnm_storage_alarm_sh);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]",iRet);
        return ;
    }

    if(strlen(list) == 0)
    {
        return;
    }
    node = strtok(list,"\n");
    while(node != NULL)
    {
        cm_cnm_storage_alarm_fenge(node);      
        node = strtok(NULL,"\n");
    }
    cm_cnm_storage_alarm_delete();
    return;
}


sint32 cm_cnm_storage_alarm_init(void)
{
    
    cm_cnm_alarm_pool_nood_root = cm_cnm_storage_link_head_init(CM_CNM_ALARMA_STORAGE_POOL);
    cm_cnm_alarm_nas_nood_root = cm_cnm_storage_link_head_init(CM_CNM_ALARMA_STORAGE_NAS);
    cm_cnm_alarm_lun_nood_root = cm_cnm_storage_link_head_init(CM_CNM_ALARMA_STORAGE_LUN);

    cm_cnm_alarm_common_init();
    return CM_OK;
}

/*
*  alarm memory and sas.
*  ........
*/

#define CM_CNM_ALARM_MEMORY  10000015
#define CM_CNM_ALARM_SAS  10000016

sint8* cm_cnm_alarm_config_file = "/var/cm/data/cm_alarm.ini";
sint8* cm_cnm_alarm_sas_py = "/var/cm/script/cm_cnm_alarm_sas.py";
uint32 cm_cnm_alarm_memory = 0;
sint8 cm_cnm_alarm_sas[CM_STRING_512] = {'\0'};

extern uint32 cm_cnm_disk_enid_exchange(uint32 enid);

static void cm_cnm_alarm_common_get(void)
{
    cm_ini_get_ext_uint32(cm_cnm_alarm_config_file,"config","memory",&cm_cnm_alarm_memory);
    cm_ini_get_ext(cm_cnm_alarm_config_file,"config","sas",cm_cnm_alarm_sas,sizeof(cm_cnm_alarm_sas));
}

static void cm_cnm_alarm_commom_sas_get_param(const sint8* param,uint32 enid,sint8* hostname)
{
    sint8 sn[CM_STRING_128] = {0};

    cm_exec_tmout(sn,sizeof(sn),CM_CMT_REQ_TMOUT,
        "sqlite3 /var/cm/data/cm_topo.db  \"select sn from  topo_t where enid=%u\"|awk '{printf}'",enid);
    
    CM_VSPRINTF(param,CM_STRING_128,"%s,%s",hostname,sn);

    return;
}
/*
static void cm_cnm_alarm_commom_sas_fenge(uint32 sas_val[],sint8 sas[])
{
    sint8* p = NULL;
    sint8 sas_backup[CM_STRING_128] = {0};
    uint32 i = 0;

    if(sas == NULL)
    {
        return;
    }
    CM_VSPRINTF(sas_backup,sizeof(sas_backup),sas);
    p = strtok(sas_backup," ");
    while(p != NULL)
    {
        sas_val[i] = atoi(p);
        p = strtok(NULL," ");
        i++;
    }

    return;
}

static void cm_cnm_alarm_commom_sas_compare(uint32 old_sas[],uint32 new_sas[],uint32 new_len)
{
    uint32 i = 0;
    uint32 j = 0;
    uint32 enid = 0;
    sint8 param[CM_STRING_128] = {0};
    sint8 hostname[CM_STRING_128] = {0};
    cm_exec_tmout(hostname,sizeof(hostname),CM_CMT_REQ_TMOUT,"hostname|awk '{printf}'");
    for(;i<CM_STRING_64;i++)
    {
        if(old_sas[i] == 0)
        {
            break;
        }
        
        for(j=0;j<new_len;j++)
        {
            if(old_sas[i] == new_sas[j])
            {
                new_sas[j] = 0;
                break;
            }
        }

        if(j == new_len)
        {
            enid = cm_cnm_disk_enid_exchange(old_sas[i]);
            cm_cnm_alarm_commom_sas_get_param(param,enid,hostname);
            CM_ALARM_REPORT(CM_CNM_ALARM_SAS,param);
        }
    }

    for(j=0;j<new_len;j++)
    {
        if(new_sas[j] != 0)
        {
            enid = cm_cnm_disk_enid_exchange(new_sas[j]);
            cm_cnm_alarm_commom_sas_get_param(param,enid,hostname);
            CM_ALARM_RECOVERY(CM_CNM_ALARM_SAS,param);
        }
    }

    return;
}

static uint32 cm_cnm_alarm_sas_get_len(sint8* sas)
{
    uint32 len = 0;
    sint8* p = NULL;

    if(sas == NULL)
    {
        return 0;
    }
    p = sas;
    
    while(*p != '\0')
    {
        if(*p == ' ')
        {
            len++;
        }
        p++;
    }

    return len;
}

static void cm_cnm_alarm_commom_sas(void)
{
    sint8 sas[CM_STRING_512] = {0};
    uint32 old_sas[CM_STRING_64] = {0};
    uint32 new_sas[CM_STRING_64] = {0};
    uint32 len = 0;
    uint32 cut = 0;
    sint32 iRet = CM_OK;

    iRet = cm_exec_tmout(sas,sizeof(sas),CM_CMT_REQ_TMOUT,
    "sqlite3 "CM_CNM_DISK_FILE" "
    "\"select distinct enid from record_t where enid < 1000 and enid>22 \"|awk '{printf $1\" \"}'");

    if(iRet != CM_OK)
    {
        return;
    }

    if(strcmp(cm_cnm_alarm_sas,sas) == 0)
    {
        return;
    }

    len = cm_cnm_alarm_sas_get_len(sas);
    if(len > CM_STRING_64)
    {
        CM_LOG_INFO(CM_MOD_CNM, "sas id more than 64");    
        return;
    }

    if(len == 0)
    {
        cut = cm_exec_int("ls /dev/es|wc -l");
        if(cut > 1)
        {
            CM_LOG_ERR(CM_MOD_CNM, "sas id get error");    
            return;   
        }
    }

    cm_cnm_alarm_commom_sas_fenge(old_sas,cm_cnm_alarm_sas);
    cm_cnm_alarm_commom_sas_fenge(new_sas,sas);

    cm_cnm_alarm_commom_sas_compare(old_sas,new_sas,len);

    CM_VSPRINTF(cm_cnm_alarm_sas,sizeof(cm_cnm_alarm_sas),sas);
    cm_ini_set_ext(cm_cnm_alarm_config_file,"config","sas",cm_cnm_alarm_sas); 
    return;
}
*/
void cm_cnm_alarm_common_thread(void)
{
    uint32 count = 0;
    
    count = cm_exec_int("sqlite3 /var/cm/data/cm_topo.db  \"select COUNT(enid) from  topo_t \"");
    if(count == 0)
    {
        return;
    }

    //cm_cnm_alarm_commom_sas();
    cm_system("%s thread",cm_cnm_alarm_sas_py);
    return;
}

void cm_cnm_alarm_get_memory_param(sint8* param,sint8* hostname,uint32 new_val,uint32 old_val)
{
    uint32 new_g = 0;
    uint32 old_g = 0;

    old_g = (old_val+1023)>>10;
    new_g = (new_val+1023)>>10;
   
    CM_VSPRINTF(param,CM_STRING_128,"%s,%u,%u",hostname,old_g,new_g);
}

sint32 cm_cnm_alarm_common_init(void)
{
    sint32 iRet = CM_OK;
    uint32 memory = 0;
    sint8 hostname[CM_STRING_128] = {0};
    sint8 param[CM_STRING_128] = {0};
    void* handle;
    
    handle = cm_ini_open(CM_CLUSTER_INI);
    if(NULL == handle)
    {
        iRet = cm_system("touch %s",cm_cnm_alarm_config_file);
        if(CM_OK != iRet)
        {
            CM_LOG_ERR(CM_MOD_CNM, "touch alarm_ini fail[%d]", iRet);
            return CM_FAIL;
        }
    }else
    {
        cm_ini_free(handle);
    }

    cm_cnm_alarm_common_get();
    cm_system("%s init",cm_cnm_alarm_sas_py);
    #ifdef __linux__
    memory = cm_exec_int("cat /proc/meminfo | grep MemTotal| awk '{print $2}'");
    memory /= 1024;
    #else
    memory = cm_exec_int("prtconf -p | grep 'Memory' |awk '{printf $3}'");
    #endif
    if(cm_cnm_alarm_memory == 0)
    {
        cm_ini_set_ext_uint32(cm_cnm_alarm_config_file,"config","memory",memory);
        cm_cnm_alarm_memory = memory;
        return;
    }

    cm_exec_tmout(hostname,sizeof(hostname),CM_CMT_REQ_TMOUT,"hostname|awk '{printf $1}'");

    if(memory < cm_cnm_alarm_memory)
    {
        cm_cnm_alarm_get_memory_param(param,hostname,memory,cm_cnm_alarm_memory);
        CM_ALARM_REPORT(CM_CNM_ALARM_MEMORY,param);
    }else
    {
        CM_ALARM_RECOVERY(CM_CNM_ALARM_MEMORY,hostname);
        cm_ini_set_ext_uint32(cm_cnm_alarm_config_file,"config","memory",memory);
        cm_cnm_alarm_memory = memory;
    }

    return CM_OK;
}

/****************************mail**********************************************/

#define CM_MAIL_DB_DIR "/var/cm/data/cm_mail.db"
extern const cm_omi_map_enum_t CmOmiMapEnumAlarmLvl;
extern const cm_omi_map_enum_t CmOmiMapEnumLanguage;
extern const cm_omi_map_enum_t CmOmiMapEnumSwitchType;
static cm_db_handle_t mail_db_handle = NULL;
sint32 cm_cnm_mailsend_init(void)
{
    uint64 cnt=0;
    sint32 iRet = CM_OK;
    const sint8* table = "CREATE TABLE IF NOT EXISTS server_t ("
            "state INT,"
            "useron INT,"
            "sender VARCHAR(128),"
            "server VARCHAR(128),"
            "port INT,"
            "language INT,"
            "user VARCHAR(128),"
            "passwd VARCHAR(128))";
    iRet = cm_db_open_ext(CM_MAIL_DB_DIR,&mail_db_handle);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"open %s fail",CM_MAIL_DB_DIR);
        return iRet;
    }
    iRet = cm_db_exec_ext(mail_db_handle, table);
    if(iRet != CM_OK)
    {
        CM_LOG_ERR(CM_MOD_CNM, "create server_t fail[%d]", iRet);
        return iRet;
    }

    (void)cm_db_exec_get_count(mail_db_handle,&cnt,
        "SELECT COUNT(*) FROM server_t");
    if(0 == cnt)
    {
        (void)cm_db_exec_ext(mail_db_handle,"INSERT INTO server_t"
            " (state,useron,sender,server,port,language,user,passwd)"
            " VALUES (0,0,'','',0,0,'','')");
    }
    return CM_OK;
}

static sint32 cm_cnm_mailsend_local_get_each(
    void *arg, sint32 col_cnt, sint8 **col_vals, sint8 **col_names)
{
    cm_cnm_mailsend_info_t *info = arg;
    if(col_cnt != 8)
    {
        CM_LOG_ERR(CM_MOD_CNM,"col_cnt[%d]",col_cnt);
        return CM_FAIL;
    }
    CM_MEM_ZERO(info,sizeof(cm_cnm_mailsend_info_t));
    info->state = (uint8)atoi(col_vals[0]);
    info->useron= (uint8)atoi(col_vals[1]);
    CM_VSPRINTF(info->sender,sizeof(info->sender),"%s",col_vals[2]);
    CM_VSPRINTF(info->server,sizeof(info->server),"%s",col_vals[3]);
    info->port = (uint32)atoi(col_vals[4]);
    info->language = (uint8)atoi(col_vals[5]);
    CM_VSPRINTF(info->user,sizeof(info->user),"%s",col_vals[6]);
    CM_VSPRINTF(info->passwd,sizeof(info->passwd),"%s",col_vals[7]);
    return CM_OK;
}

sint32 cm_cnm_mailsend_update(const void *pDecodeParam,void **ppAckData,uint32 * pAckLen)
{
    const cm_cnm_decode_info_t *decode = pDecodeParam;
    cm_cnm_mailsend_info_t data;
    const cm_cnm_mailsend_info_t *info = NULL;

    if(decode == NULL)
    {
        CM_LOG_ERR(CM_MOD_CNM,"decode param null");
        return CM_PARAM_ERR;
    }
    
    CM_MEM_ZERO(&data,sizeof(data));
    (void)cm_db_exec_get(mail_db_handle,cm_cnm_mailsend_local_get_each,&data,1,
        sizeof(cm_cnm_mailsend_info_t),"SELECT * FROM server_t LIMIT 1");
    
    info = (const cm_cnm_mailsend_info_t *)decode->data;
    if(CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_MAILSEND_STATE))
    {
        data.state = info->state;
    }
        
    if(CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_MAILSEND_SENDER))
    {
        CM_VSPRINTF(data.sender,sizeof(data.sender),"%s",info->sender);
    }
    
    if(CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_MAILSEND_SERVER))
    {
        CM_VSPRINTF(data.server,sizeof(data.server),"%s",info->server);
    }
    
    if(CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_MAILSEND_PORT))
    {
        data.port = info->port;
    }

    if(CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_MAILSEND_LANGUAGE))
    {
        data.language = info->language;
    }

    if(CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_MAILSEND_USERON))
    {
        data.useron= info->useron;
    }
    
    if(CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_MAILSEND_USER))
    {
        CM_VSPRINTF(data.user,sizeof(data.user),"%s",info->user);
    }
    
    if(CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_MAILSEND_PASSWD))
    {
        CM_VSPRINTF(data.passwd,sizeof(data.passwd),"%s",info->passwd);
    }
    
    return cm_sync_request(CM_SYNC_OBJ_MAILSEND,0,&data,sizeof(cm_cnm_mailsend_info_t));
}  

sint32 cm_cnm_mailsend_test(
        const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    const cm_cnm_decode_info_t *decode = pDecodeParam;
    const cm_cnm_mailsend_info_t *info = NULL;

    if(decode == NULL)
    {
        CM_LOG_ERR(CM_MOD_CNM,"decode param null");
        return CM_PARAM_ERR;
    }
    
    info = (const cm_cnm_mailsend_info_t*)decode->data;
    return cm_system(CM_SCRIPT_DIR"cm_mail.py test_server '%s' '%u' '%s' '%s'",
        info->server,info->port,info->user,info->passwd);
}

sint32 cm_cnm_mailsend_get(const void * pDecodeParam,void **ppAckData,uint32 * pAckLen)
{
    uint32 cnt = 0;
    cm_cnm_mailsend_info_t *data = CM_MALLOC(sizeof(cm_cnm_mailsend_info_t));
    
    if(data == NULL)
    {
        CM_LOG_ERR(CM_MOD_CNM,"malloc fail");
        return CM_FAIL;
    }
    cnt = cm_db_exec_get(mail_db_handle,cm_cnm_mailsend_local_get_each,data,1,
        sizeof(cm_cnm_mailsend_info_t),"SELECT * FROM server_t LIMIT 1");
    if(cnt == 0)
    {
        CM_LOG_ERR(CM_MOD_CNM,"no server_t cfg");
        CM_FREE(data);
        return CM_ERR_NOT_EXISTS;
    }
    *ppAckData = data;
    *pAckLen = sizeof(cm_cnm_mailsend_info_t);
    return CM_OK;
}

sint32 cm_cnm_mailsend_sync_request(uint64 id, void *pdata, uint32 len)
{
    cm_cnm_mailsend_info_t *info = (cm_cnm_mailsend_info_t*)pdata;

    return cm_db_exec_ext(mail_db_handle,
                "UPDATE server_t SET state=%u,useron=%u,port=%u,language=%u," 
                "sender='%s',server='%s',user='%s',passwd='%s'",
                info->state,info->useron,info->port,info->language,
                info->sender,info->server,info->user,info->passwd);
}

sint32 cm_cnm_mailsend_sync_get(uint64 id, void **pdata, uint32 *plen)
{
    return cm_cnm_mailsend_get(NULL,pdata,plen);
}

static sint32 cm_cnm_mailsend_decode_check_language(void *val)
{
    uint8 language = *((uint8*)val);
    const sint8* str = cm_cnm_get_enum_str(&CmOmiMapEnumLanguage,language);
    if(NULL == str)
    {
        CM_LOG_ERR(CM_MOD_CNM,"language[%u]",language);
        return CM_PARAM_ERR;
    }
    return CM_OK;
}
static sint32 cm_cnm_mailsend_decode_check_button(void *val)
{
    uint8 button = *((uint8*)val);
    const sint8* str = cm_cnm_get_enum_str(&CmOmiMapEnumSwitchType,button);
    if(NULL == str)
    {
        CM_LOG_ERR(CM_MOD_CNM,"button[%u]",button);
        return CM_PARAM_ERR;
    }
    return CM_OK;
}
static sint32 cm_cnm_mailsend_decode_ext(
    const cm_omi_obj_t ObjParam,void* data,cm_omi_field_flag_t *set)
{
    sint32 iRet = CM_OK;
    cm_cnm_mailsend_info_t *info = data;
    cm_cnm_decode_param_t param_str[] = 
    {
        {CM_OMI_FIELD_MAILSEND_SENDER,sizeof(info->sender),info->sender,NULL},
        {CM_OMI_FIELD_MAILSEND_SERVER,sizeof(info->server),info->server,NULL},
        {CM_OMI_FIELD_MAILSEND_USER,sizeof(info->user),info->user,NULL},
        {CM_OMI_FIELD_MAILSEND_PASSWD,sizeof(info->passwd),info->passwd,NULL},
    };
    cm_cnm_decode_param_t param_num[] = 
    {
        {CM_OMI_FIELD_MAILSEND_USERON,sizeof(info->useron),&info->useron,cm_cnm_mailsend_decode_check_button},
        {CM_OMI_FIELD_MAILSEND_STATE,sizeof(info->state),&info->state,cm_cnm_mailsend_decode_check_button},    
        {CM_OMI_FIELD_MAILSEND_PORT,sizeof(info->port),&info->port,NULL},
        {CM_OMI_FIELD_MAILSEND_LANGUAGE,sizeof(info->language),&info->language,cm_cnm_mailsend_decode_check_language}, 
    };
    iRet = cm_cnm_decode_str(ObjParam,param_str,
        sizeof(param_str)/sizeof(cm_cnm_decode_param_t),set);
    if(iRet != CM_OK)
    {
        return iRet;
    }
    iRet = cm_cnm_decode_num(ObjParam,param_num,
        sizeof(param_num)/sizeof(cm_cnm_decode_param_t),set);
    if(iRet != CM_OK)
    {
        return iRet;
    }
    return CM_OK;
}
sint32 cm_cnm_mailsend_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam)
{
    return cm_cnm_decode_comm(ObjParam,sizeof(cm_cnm_mailsend_info_t),
        cm_cnm_mailsend_decode_ext,ppDecodeParam);
}
static void cm_cnm_mailsend_encode_each(cm_omi_obj_t item,void *eachdata,void *arg)
{
    cm_omi_field_flag_t *field = arg;
    cm_cnm_mailsend_info_t *info = eachdata;
    cm_cnm_map_value_str_t cols_str[] = 
    {
        {CM_OMI_FIELD_MAILSEND_SENDER,  info->sender},
        {CM_OMI_FIELD_MAILSEND_SERVER,  info->server},
        {CM_OMI_FIELD_MAILSEND_USER,    info->user},
        {CM_OMI_FIELD_MAILSEND_PASSWD,  info->passwd},
    };
    cm_cnm_map_value_num_t cols_num[] = 
    {
        {CM_OMI_FIELD_MAILSEND_USERON,           (uint32)info->useron},
        {CM_OMI_FIELD_MAILSEND_STATE,       (uint32)info->state},
        {CM_OMI_FIELD_MAILSEND_PORT,         (uint32)info->port},
        {CM_OMI_FIELD_MAILSEND_LANGUAGE,     (uint32)info->language}, 
    };
    cm_cnm_encode_str(item,field,cols_str,sizeof(cols_str)/sizeof(cm_cnm_map_value_str_t));
    cm_cnm_encode_num(item,field,cols_num,sizeof(cols_num)/sizeof(cm_cnm_map_value_num_t)); 
    return;
}

cm_omi_obj_t cm_cnm_mailsend_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen)
{
    return cm_cnm_encode_comm_ext(pDecodeParam,pAckData,AckLen,
        sizeof(cm_cnm_mailsend_info_t),cm_cnm_mailsend_encode_each);
}

sint32 cm_cnm_mailrecv_init(void)
{
    sint32 iRet = CM_OK;
    const sint8* table = "CREATE TABLE IF NOT EXISTS receiver_t ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,"
        "receiver VARCHAR(128),"
        "level INT)";
    iRet = cm_db_open_ext(CM_MAIL_DB_DIR,&mail_db_handle);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"open %s fail",CM_MAIL_DB_DIR);
        return iRet;
    }
    iRet = cm_db_exec_ext(mail_db_handle, table);
    if(iRet != CM_OK)
    {
        CM_LOG_ERR(CM_MOD_CNM, "create receiver_t fail[%d]", iRet);
        return iRet;
    }
    return CM_OK;
}
sint32 cm_cnm_mailrecv_local_get_each(
    void *arg, sint32 col_cnt, sint8 **col_vals, sint8 **col_names)
{
    cm_cnm_mailrecv_info_t *info = arg;
    if(col_cnt != 3)
    {
        CM_LOG_ERR(CM_MOD_CNM,"col_cnt[%d]",col_cnt);
        return CM_FAIL;
    }
    CM_MEM_ZERO(info,sizeof(cm_cnm_mailrecv_info_t));
    info->id = (uint32)atoi(col_vals[0]);
    CM_VSPRINTF(info->receiver,sizeof(info->receiver),"%s",col_vals[1]);
    info->level= (uint8)atoi(col_vals[2]);
    return CM_OK;
}
sint32  cm_cnm_mailrecv_insert(void *pDecodeParam,void **ppAckData,uint32 * pAckLen)
{
    sint32 iRet = CM_OK;
    uint64 cnt = 0;
    const cm_cnm_decode_info_t *decode = pDecodeParam;
    const cm_cnm_mailrecv_info_t *info = NULL;
    if(decode == NULL)
    {
        CM_LOG_ERR(CM_MOD_CNM,"decode param null");
        return CM_PARAM_ERR;
    }
    info = (const cm_cnm_mailrecv_info_t *)decode->data;
    if((!CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_MAILRECV_RECEIVER))
        || (!CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_MAILRECV_LEVEL)))
    {           
        return CM_PARAM_ERR;
    }
    iRet = cm_db_exec_get_count(mail_db_handle, &cnt,
        "SELECT COUNT(receiver) FROM receiver_t WHERE receiver='%s'",info->receiver);                                 
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM, "get receiver[%s] count fail",info->receiver);
        return CM_FAIL;
    }
    
    if(cnt != 0)
    {
        CM_LOG_ERR(CM_MOD_CNM,"this receiver[%s] exist",info->receiver);
        return CM_ERR_ALREADY_EXISTS;
    }
    
    (void)cm_db_exec_get_count(mail_db_handle, &cnt,
        "SELECT seq+1 FROM sqlite_sequence WHERE name='receiver_t'"); 
    if(cnt == 0)
    {
        cnt = 1;
    }
    CM_LOG_WARNING(CM_MOD_CNM,"%llu,%s,%u",cnt,info->receiver,info->level); 
    return cm_sync_request(CM_SYNC_OBJ_MAILRECV,cnt,info,sizeof(cm_cnm_mailrecv_info_t));     
}

sint32  cm_cnm_mailrecv_update(const void *pDecodeParam,void **ppAckData,uint32 * pAckLen)
{
    uint32 cnt = 0;
    const cm_cnm_decode_info_t *decode = pDecodeParam;
    const cm_cnm_mailrecv_info_t *info = NULL;
    cm_cnm_mailrecv_info_t data;

    if(decode == NULL)
    {
        CM_LOG_ERR(CM_MOD_CNM,"decode param null");
        return CM_PARAM_ERR;
    }
    info = (const cm_cnm_mailrecv_info_t *)decode->data;
    if((!CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_MAILRECV_ID))
        || (!CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_MAILRECV_LEVEL)))
    {
        return CM_PARAM_ERR;
    }

    cnt= cm_db_exec_get(mail_db_handle,cm_cnm_mailrecv_local_get_each,&data,1,
        sizeof(cm_cnm_mailrecv_info_t),
        "SELECT * FROM receiver_t WHERE id=%u LIMIT 1",info->id);
    if (0 == cnt)
    {
        return CM_ERR_NOT_EXISTS;
    }

    if (info->level == data.level)
    {
        return CM_OK;
    }
    data.level = info->level;
    CM_LOG_WARNING(CM_MOD_CNM,"%u,%s,%u",info->id,data.receiver,info->level); 
    return cm_sync_request(CM_SYNC_OBJ_MAILRECV,info->id,&data,sizeof(cm_cnm_mailrecv_info_t));
}

sint32 cm_cnm_mailrecv_get(const void * pDecodeParam,void **ppAckData,uint32 * pAckLen)
{
    uint32 cnt = 0;
    const cm_cnm_decode_info_t *decode = pDecodeParam;
    const cm_cnm_mailrecv_info_t *info = NULL;
    cm_cnm_mailrecv_info_t *data = NULL;
    
    if(decode == NULL)
    {
        CM_LOG_ERR(CM_MOD_CNM,"param null");
        return CM_PARAM_ERR;
    }
    if(!CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_MAILRECV_ID))
    {
        CM_LOG_ERR(CM_MOD_CNM,"id null");
        return CM_PARAM_ERR;
    }

    info = (const cm_cnm_mailrecv_info_t *)decode->data;
    
    data = CM_MALLOC(sizeof(cm_cnm_mailrecv_info_t));
    if(data == NULL)
    {
        CM_LOG_ERR(CM_MOD_CNM,"malloc fail");
        return CM_FAIL;
    }

    cnt= cm_db_exec_get(mail_db_handle,cm_cnm_mailrecv_local_get_each,data,1,
        sizeof(cm_cnm_mailrecv_info_t),
        "SELECT * FROM receiver_t WHERE id=%u LIMIT 1",info->id);
    if (0 == cnt)
    {
        CM_FREE(data);
        return CM_OK;
    }
    
    *ppAckData = data;
    *pAckLen = sizeof(cm_cnm_mailrecv_info_t);
    return CM_OK;
}

sint32 cm_cnm_mailrecv_getbatch(const void * pDecodeParam,void **ppAckData,uint32 * pAckLen)
{
    const cm_cnm_decode_info_t *dec = pDecodeParam;
    cm_cnm_mailrecv_info_t *info = NULL;
    uint32 offset = 0;
    uint32 total = CM_CNM_MAX_RECORD;

    if(dec != NULL)
    {
        offset = (uint32)dec->offset;
        total = dec->total;
    }
    
    info = CM_MALLOC(sizeof(cm_cnm_mailrecv_info_t)*total);
    if(NULL == info)
    {
        CM_LOG_ERR(CM_MOD_CNM,"malloc fail");
        return CM_FAIL;
    }
    total = cm_db_exec_get(mail_db_handle,cm_cnm_mailrecv_local_get_each, info,
                total, sizeof(cm_cnm_mailrecv_info_t),
                "SELECT * FROM receiver_t ORDER BY id DESC LIMIT %u,%u",offset,total);
    if(total == 0)
    {
        CM_FREE(info);
        return CM_OK;
    }
    *ppAckData = info;
    *pAckLen = total * sizeof(cm_cnm_mailrecv_info_t);
    return CM_OK;
}

sint32 cm_cnm_mailrecv_delete(const void * pDecodeParam,void **ppAckData,uint32 * pAckLen)
{
    uint32 cnt = 0;
    const cm_cnm_decode_info_t *decode = pDecodeParam;
    const cm_cnm_mailrecv_info_t *info = NULL;
    
    if(decode == NULL)
    {
        CM_LOG_ERR(CM_MOD_CNM,"param null");
        return CM_PARAM_ERR;
    }
    if(!CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_MAILRECV_ID))
    {
        CM_LOG_ERR(CM_MOD_CNM,"id null");
        return CM_PARAM_ERR;
    }
    info = (const cm_cnm_mailrecv_info_t *)decode->data;
    CM_LOG_WARNING(CM_MOD_CNM,"%u",info->id); 
    
    return cm_sync_delete(CM_SYNC_OBJ_MAILRECV,info->id);   
}

sint32 cm_cnm_mailrecv_count(const void * pDecodeParam,void **ppAckData,uint32 *pAckLen)
{
    uint64 cnt = 0;
    sint32 iRet = CM_OK;
    iRet = cm_db_exec_get_count(mail_db_handle,&cnt,"SELECT COUNT(*) FROM receiver_t");
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM, "get all receivers fail[%d]", iRet);
        return CM_FAIL;
    }
    return cm_cnm_ack_uint64(cnt,ppAckData,pAckLen);
}

sint32 cm_cnm_mailrecv_test(
    const void * pDecodeParam,void **ppAckData,uint32 *pAckLen)
{
    const cm_cnm_decode_info_t *decode = pDecodeParam;
    const cm_cnm_mailrecv_info_t *info = NULL;

    if(decode == NULL)
    {
        CM_LOG_ERR(CM_MOD_CNM,"param null");
        return CM_PARAM_ERR;
    }
    info = (const cm_cnm_mailrecv_info_t *)decode->data;
    return cm_system(CM_SCRIPT_DIR"cm_mail.py test_send '%s'",
        info->receiver);
}

sint32 cm_cnm_mailrecv_sync_request(uint64 id, void *pdata, uint32 len)
{
    sint32 iRet = CM_OK;
    uint64 count = 0;
    cm_cnm_mailrecv_info_t *info = (cm_cnm_mailrecv_info_t*)pdata;
    iRet = cm_db_exec_get_count(mail_db_handle, &count,
        "SELECT COUNT(receiver) FROM receiver_t WHERE id=%llu",id);                                 
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM, "get receiver count fail[%d]", iRet);
        return CM_FAIL;
    }
    if(count > 0) 
    {
       return cm_db_exec_ext(mail_db_handle,
                "UPDATE receiver_t SET level=%u WHERE id=%llu",info->level, 
                info->id);
    }
    return cm_db_exec_ext(mail_db_handle,
            "INSERT INTO receiver_t (receiver,level) "
            "VALUES('%s',%u)",info->receiver,info->level);
}

sint32 cm_cnm_mailrecv_sync_get(uint64 id, void **pdata, uint32 *plen)
{
    cm_cnm_mailrecv_info_t *info = CM_MALLOC(sizeof(cm_cnm_mailrecv_info_t));
    uint32 cnt = 0;
    if(info == NULL)
    {
        CM_LOG_ERR(CM_MOD_CNM,"malloc fail");
        return CM_FAIL;
    }

    cnt = cm_db_exec_get(mail_db_handle,cm_cnm_mailrecv_local_get_each,info,1,
        sizeof(cm_cnm_mailrecv_info_t),"SELECT * FROM receiver_t WHERE id=%u",(uint32)id);
    if(cnt == 0)
    {
        CM_LOG_ERR(CM_MOD_CNM,"id[%u] dosen't exit",id);
        CM_FREE(info);
        return CM_ERR_NOT_EXISTS;
    }
    *pdata = info;
    *plen = sizeof(cm_cnm_mailrecv_info_t);
    return CM_OK;
}

sint32 cm_cnm_mailrecv_sync_delete(uint64 id)
{
    return cm_db_exec_ext(mail_db_handle,"DELETE FROM receiver_t WHERE id=%llu",id);
}

static sint32 cm_cnm_mailrecv_decode_check_level(void *val)
{
    uint8 level = *((uint8*)val);
    const sint8* str = cm_cnm_get_enum_str(&CmOmiMapEnumAlarmLvl,level);
    if(NULL == str)
    {
        CM_LOG_ERR(CM_MOD_CNM,"level[%u]",level);
        return CM_PARAM_ERR;
    }
    return CM_OK;
}
static sint32 cm_cnm_mailrecv_decode_ext(
    const cm_omi_obj_t ObjParam,void* data,cm_omi_field_flag_t *set)
{
    sint32 iRet = CM_OK;
    cm_cnm_mailrecv_info_t *info = data;
    cm_cnm_decode_param_t param_str[] = 
    {
        {CM_OMI_FIELD_MAILRECV_RECEIVER,sizeof(info->receiver),info->receiver,NULL},  
    };
    cm_cnm_decode_param_t param_num[] = 
    {
        {CM_OMI_FIELD_MAILRECV_ID,sizeof(info->id),&info->id,NULL},
        {CM_OMI_FIELD_MAILRECV_LEVEL,sizeof(info->level),&info->level,cm_cnm_mailrecv_decode_check_level},   
    };
    iRet = cm_cnm_decode_str(ObjParam,param_str,
        sizeof(param_str)/sizeof(cm_cnm_decode_param_t),set);
    if(iRet != CM_OK)
    {
        return iRet;
    }
    iRet = cm_cnm_decode_num(ObjParam,param_num,
        sizeof(param_num)/sizeof(cm_cnm_decode_param_t),set);
    if(iRet != CM_OK)
    {
        return iRet;
    }
    return CM_OK;
}
sint32 cm_cnm_mailrecv_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam)
{
    return cm_cnm_decode_comm(ObjParam,sizeof(cm_cnm_mailrecv_info_t),
        cm_cnm_mailrecv_decode_ext,ppDecodeParam);
}
static void cm_cnm_mailrecv_encode_each(cm_omi_obj_t item,void *eachdata,void *arg)
{
    cm_omi_field_flag_t *field = arg;
    cm_cnm_mailrecv_info_t *info = eachdata;
    cm_cnm_map_value_str_t cols_str[] = 
    {
        {CM_OMI_FIELD_MAILRECV_RECEIVER,info->receiver},  
    };
    cm_cnm_map_value_num_t cols_num[] = 
    {
        {CM_OMI_FIELD_MAILRECV_ID,           (uint32)info->id},
        {CM_OMI_FIELD_MAILRECV_LEVEL,        (uint32)info->level},
    };
    cm_cnm_encode_str(item,field,cols_str,sizeof(cols_str)/sizeof(cm_cnm_map_value_str_t));
    cm_cnm_encode_num(item,field,cols_num,sizeof(cols_num)/sizeof(cm_cnm_map_value_num_t)); 
    return;
}
cm_omi_obj_t cm_cnm_mailrecv_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen)
{
    return cm_cnm_encode_comm_ext(pDecodeParam,pAckData,AckLen,
        sizeof(cm_cnm_mailrecv_info_t),cm_cnm_mailrecv_encode_each);
}

