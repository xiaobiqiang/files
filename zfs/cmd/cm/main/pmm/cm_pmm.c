/******************************************************************************
 *          Copyright (c) 2017 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_pmm.c
 * author     : wbn
 * create date: 2017年10月27日
 * description: TODO:
 *
 *****************************************************************************/
#include "cm_omi_types.h"
#include "cm_db.h"
#include "cm_cmt.h"
#include "cm_omi.h"
#include "cm_log.h"
#include "cm_pmm_types.h"
#include "cm_node.h"
#include "cm_pmm.h"
#include "cm_sync.h"
#include "cm_ini.h"
#include "cm_pmm_common.h"
#include "cm_pmm_pool.h"
#include "cm_cnm_pmm.h"
#include "cm_cnm_pmm_lun.h"

#define CM_PMM_DB_DIR CM_DATA_DIR
#define CM_PMM_DB_BACKUP_DIR CM_DATA_DIR"pmm_backup/"
#define CM_PMM_DB_NAME "cm_pmm.db"
#define CM_PMM_CFG_SECTION "PMM_CFG"
#define CM_PMM_CFG_SAVE_HOURS "save_hours"


#define CM_PMM_MAX_FIELDS_NUM 128
#define CM_PMM_TMOUT 2
#define CM_PMM_MAX_RECORD_CNT 100
#define CM_PMM_HISTORY_DATA_TIME 3600

typedef struct
{
    uint32 cols;
    uint32 rows;
    double data[];    
}cm_pmm_encode_t;

typedef struct
{
    uint32 id;
    uint8 data[];
}cm_pmm_msg_t;

typedef sint32 (*cm_pmm_local_func_t)(const void *pDecode,void **ppAckData, uint32 *pAckLen);

typedef struct
{
    uint32 id;
    uint32 min_len;
    cm_pmm_local_func_t cbk;
}cm_pmm_msg_cbk_func_t;

static sint32 cm_pmm_local_current(const cm_pmm_decode_t *pDecode,void **ppAckData, uint32 *pAckLen);
static sint32 cm_pmm_local_history(const cm_pmm_decode_t *pDecode,void **ppAckData, uint32 *pAckLen);
static sint32 cm_pmm_local_current_mem(const cm_pmm_decode_t *pDecode,void **ppAckData, uint32 *pAckLen);

const cm_pmm_msg_cbk_func_t g_cm_pmm_msg_cfg[CM_PMM_MSG_BUTT] = 
{
    {CM_PMM_MSG_GET_CURRENT, sizeof(cm_pmm_decode_t),(cm_pmm_local_func_t)cm_pmm_local_current},
    {CM_PMM_MSG_GET_HISTORY, sizeof(cm_pmm_decode_t),(cm_pmm_local_func_t)cm_pmm_local_history},
    {CM_PMM_MSG_GET_CURRENT_MEM,sizeof(cm_pmm_decode_t),(cm_pmm_local_func_t)cm_pmm_local_current_mem}
};

extern sint32 cm_pmm_cluster_init(void);
extern void cm_pmm_cluster_main(struct tm *tin);
extern sint32 cm_pmm_cluster_current_mem(const void *pDecode,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_pmm_node_init(void);
extern void cm_pmm_node_main(struct tm *tin);
extern sint32 cm_pmm_node_current_mem(const void *pDecode,void **ppAckData, uint32 *pAckLen);


/* 越小的单元越排前面 */
const cm_pmm_obj_cfg_t g_cm_pmm_obj[] = 
{
    {
        CM_OMI_OBJECT_PMM_NODE,
        cm_pmm_node_init,
        cm_pmm_node_main,
        cm_pmm_node_current_mem
    },
    {
        CM_OMI_OBJECT_PMM_CLUSTER,
        cm_pmm_cluster_init,
        cm_pmm_cluster_main,
        cm_pmm_cluster_current_mem
    },
    {
        CM_OMI_OBJECT_PMM_POOL,
        NULL,
        NULL,
        cm_pmm_pool_current_mem
    },
	{
        CM_OMI_OBJECT_PMM_NAS,
            NULL,
            NULL,
       cm_pmm_nas_current_mem
    },
    {
        CM_OMI_OBJECT_PMM_SAS,
            NULL,
            NULL,
       cm_pmm_sas_current_mem
    },
    {
        CM_OMI_OBJECT_PMM_PROTO,
            NULL,
            NULL,
       cm_pmm_protocol_current_mem
    },
    {
        CM_OMI_OBJECT_PMM_CACHE,
            NULL,
            NULL,
       cm_pmm_cache_current_mem
    },
	{
        CM_OMI_OBJECT_PMM_LUN,
        NULL,
        NULL,
        cm_pmm_lun_current_mem
    },
    {
        CM_OMI_OBJECT_PMM_NIC,
        NULL,
        NULL,
        cm_pmm_nic_current_mem
    }, 
    {
        CM_OMI_OBJECT_PMM_DISK,
        NULL,
        NULL,
        cm_pmm_disk_current_mem
    },    
};

static cm_db_handle_t g_cm_pmm_db = NULL;
cm_mutex_t g_cm_pmm_mutex;
static cm_pmm_cfg_t g_cm_pmm_cfg;

sint32 cm_pmm_decode(uint32 obj,const cm_omi_obj_t ObjParam, void
**ppDecodeParam)
{
    cm_pmm_decode_t *pDecode = NULL;

    pDecode = CM_MALLOC(sizeof(cm_pmm_decode_t));
    if(NULL == pDecode)
    {
        CM_LOG_ERR(CM_MOD_PMM,"malloc fail");
        return CM_FAIL;
    }
    
    CM_MEM_ZERO(pDecode,sizeof(cm_pmm_decode_t));
    pDecode->obj = obj;

    /* 确保ID都是同一字段 */
    (void)cm_omi_obj_key_get_u32_ex(ObjParam,CM_OMI_FIELD_PMM_CLUSTER_ID,
        &pDecode->id);

    (void)cm_omi_obj_key_get_u32_ex(ObjParam,CM_OMI_FIELD_PARENT_ID,
        &pDecode->parent_id);

    (void)cm_omi_obj_key_get_str_ex(ObjParam,CM_OMI_FIELD_FIELDS,
        pDecode->fields, sizeof(pDecode->fields));
    (void)cm_omi_obj_key_get_str_ex(ObjParam,CM_OMI_FIELD_PARAM,
        pDecode->param, sizeof(pDecode->param));
    (void)cm_omi_obj_key_get_u32_ex(ObjParam,CM_OMI_FIELD_PMM_GRAIN,
        &pDecode->grain_size);  
    (void)cm_omi_obj_key_get_u32_ex(ObjParam,CM_OMI_FIELD_TOTAL,&pDecode->max_cnt);
    if((0 == pDecode->max_cnt) 
        || (pDecode->max_cnt > CM_PMM_MAX_RECORD_CNT))
    {
        pDecode->max_cnt = CM_PMM_MAX_RECORD_CNT;
    }
    (void)cm_omi_obj_key_get_u64_ex(ObjParam,CM_OMI_FIELD_TIME_START,&pDecode->start);
    (void)cm_omi_obj_key_get_u64_ex(ObjParam,CM_OMI_FIELD_TIME_END,&pDecode->end);
    CM_LOG_INFO(CM_MOD_PMM,"start[%llu] end[%llu]", pDecode->start, pDecode->end);
    if((0 != pDecode->start) 
        && (0 != pDecode->end)
        && (pDecode->end <= pDecode->start))
    {
        CM_LOG_ERR(CM_MOD_PMM,"start:%llu,end:%llu",pDecode->start,pDecode->end);
        return CM_PARAM_ERR;
    }
    *ppDecodeParam = pDecode;
    return CM_OK;
}

static cm_omi_obj_t cm_pmm_encode_all(
    const cm_omi_map_object_t* pObj,cm_pmm_encode_t *pEncode)
{
    uint32 row_index = 0;
    uint32 cols_max = 0;
    uint32 col_index = 0;
    cm_omi_obj_t items = NULL;
    cm_omi_obj_t item = NULL;
    double *pdata_row = pEncode->data;
    double *pdata_col = NULL;
    const cm_omi_map_object_field_t *pfield = NULL;
    
    items = cm_omi_obj_new_array();
    if(NULL == items)
    {
        CM_LOG_ERR(CM_MOD_CNM,"new items fail");
        return NULL;
    }

    if(pEncode->cols > pObj->field_num)
    {
        cols_max = pObj->field_num;
    }
    else
    {
        cols_max = pEncode->cols;
    }
    
    while(row_index < pEncode->rows)
    {
        item = cm_omi_obj_new();
        if(NULL == item)
        {
            CM_LOG_ERR(CM_MOD_CNM,"new item fail");
            return items;
        }
        
        pdata_col = pdata_row;
        col_index = 0;
        while(col_index < cols_max)
        {
            pfield = &pObj->fields[col_index];
            if(CM_OMI_DATA_DOUBLE == pfield->data.type)
            {
                (void)cm_omi_obj_key_set_double_ex(item,pfield->field.code,*pdata_col);
            }
            /* start 修改问题单6103 */
            else if(pfield->field.code == CM_OMI_FIELD_PMM_CLUSTER_TIMESTAMP)
            {
                uint32 def_val = (uint32)*pdata_col;
                uint32 val = def_val % 5;
                val = def_val-val;
                (void)cm_omi_obj_key_set_u32_ex(item,pfield->field.code,val);
            }
            /* end 修改问题单6103 */
            else
            {
                (void)cm_omi_obj_key_set_u32_ex(item,pfield->field.code,(uint32)*pdata_col);
            }
            col_index++;
            pdata_col++;
        }
        
        if(CM_OK != cm_omi_obj_array_add(items,item))
        {
            CM_LOG_ERR(CM_MOD_CNM,"add item[id:%u] fail",(uint32)*pdata_row);
            cm_omi_obj_delete(item);
        }
        
        pdata_row += pEncode->cols;
        row_index++;
    }
    return items;
}

static cm_omi_obj_t cm_pmm_encode_part(
    const cm_pmm_decode_t *pDecode,cm_pmm_encode_t *pEncode)
{
    uint32 row_index = 0;
    uint32 col_index = 0;
    cm_omi_obj_t items = NULL;
    cm_omi_obj_t item = NULL;
    double *pdata_row = pEncode->data;
    double *pdata_col = NULL;
    sint8 *fields[CM_PMM_MAX_FIELDS_NUM] = {NULL};
    uint32 col_id = 0;
    uint32 cols_max = cm_omi_get_fields(pDecode->fields,fields,CM_PMM_MAX_FIELDS_NUM);

    items = cm_omi_obj_new_array();
    if(NULL == items)
    {
        CM_LOG_ERR(CM_MOD_CNM,"new items fail");
        return NULL;
    }

    if(pEncode->cols < cols_max)
    {
        cols_max = pEncode->cols;
    }

    while(row_index < pEncode->rows)
    {
        item = cm_omi_obj_new();
        if(NULL == item)
        {
            CM_LOG_ERR(CM_MOD_CNM,"new item fail");
            return items;
        }
        
        pdata_col = pdata_row;
        col_index = 0;
        while(col_index < cols_max)
        {
            col_id = (uint32)atoi(fields[col_index]);
            /* 前两个固定为 ID ,时间戳 */
            if(col_id > CM_OMI_FIELD_PMM_CLUSTER_TIMESTAMP)
            {
                (void)cm_omi_obj_key_set_double_ex(item,col_id,*pdata_col);
            }
            /* start 修改问题单6103 */
            else if(col_id == CM_OMI_FIELD_PMM_CLUSTER_TIMESTAMP)
            {
                uint32 def_val = (uint32)*pdata_col;
                uint32 val = def_val % 5;
                val = def_val-val;
                (void)cm_omi_obj_key_set_u32_ex(item,col_id,val);
            }
            /* end 修改问题单6103 */
            else
            {
                (void)cm_omi_obj_key_set_u32_ex(item,col_id,(uint32)*pdata_col);
            }
            col_index++;
            pdata_col++;
        }
        
        if(CM_OK != cm_omi_obj_array_add(items,item))
        {
            CM_LOG_ERR(CM_MOD_CNM,"add item[id:%u] fail",(uint32)*pdata_row);
            cm_omi_obj_delete(item);
        }
        
        pdata_row += pEncode->cols;
        row_index++;
    }
    return items;
}

cm_omi_obj_t cm_pmm_encode(
    uint32 obj,const void *pDecodeParam, void *pAckData, uint32 AckLen)
{
    const cm_pmm_decode_t *pDecode = (const cm_pmm_decode_t *)pDecodeParam;
    const cm_omi_map_object_t *pObj = NULL;
    
    if((NULL == pAckData) || (0 == AckLen))
    {
        return NULL;
    }

    if((NULL != pDecode) && ('\0' != pDecode->fields[0]))
    {
        return cm_pmm_encode_part(pDecode,(cm_pmm_encode_t*)pAckData);
    }
    
    pObj = cm_omi_get_map_obj_cfg(obj);
    /* 不用考虑为空的情况，配置上会保证 */
    
    return cm_pmm_encode_all(pObj,(cm_pmm_encode_t*)pAckData);
}

static sint32 cm_pmm_db_get(
    void *arg, sint32 col_cnt, sint8 **col_vals, sint8 **col_names)
{
    cm_db_query_t *query = arg;
    cm_pmm_encode_t *pdata = query->data;
    double *col = NULL;
    sint32 iloop = 0;
    
    if(query->index >= query->max)
    {
        return CM_OK;
    }

    col = &pdata->data[pdata->cols * pdata->rows];
    if(col_cnt > (sint32)pdata->cols)
    {
        col_cnt = (sint32)pdata->cols;
    }
    while(iloop < col_cnt)
    {
        if(NULL == *col_vals)
        {
            return CM_OK;
        }
        else
        {
            *col = (double)atof(*col_vals);
        }
        col_vals++;
        col++;
        iloop++;
    }
    query->index++;
    pdata->rows++;
    return CM_OK;
}

static sint32 cm_pmm_get_data_from_backup(
    const sint8 *select_data,cm_time_t start, cm_time_t end,cm_db_query_t *query)
{
    const uint32 seconds_in_hour = 3600;
    cm_time_t now_hour = cm_get_time_hour_floor(cm_get_time());
    cm_time_t curr_end = 0;
    cm_db_handle_t handle = NULL;  
    sint32 iRet = CM_OK;
    sint8 db_file[CM_STRING_256] = {0};
    struct tm tmp;
    
    for(curr_end = cm_get_time_hour_floor(start + seconds_in_hour);
        (start < end) && (start < now_hour) && (query->index < query->max); 
        start = curr_end, curr_end += seconds_in_hour)
    {
        /* 从备份文件中取 */
        cm_get_datetime_ext(&tmp, start);

        (void)CM_VSPRINTF(db_file,sizeof(db_file),
            "%s.%04d%02d%02d_%02d",
            CM_PMM_DB_BACKUP_DIR
            CM_PMM_DB_NAME,
            tmp.tm_year,tmp.tm_mon,tmp.tm_mday,tmp.tm_hour);
            
        iRet = cm_db_open(db_file,&handle);
        if(CM_OK != iRet)
        {
            CM_LOG_INFO(CM_MOD_PMM,"open[%s] fail",db_file);
            continue;
        }

        curr_end = (curr_end < end)?curr_end:end;
        iRet=cm_db_exec(handle,cm_pmm_db_get, query,
            "%s WHERE f%u>=%u AND f%u<%u "
            "ORDER BY f%u LIMIT %u",select_data,
            CM_OMI_FIELD_PMM_CLUSTER_TIMESTAMP,start,
            CM_OMI_FIELD_PMM_CLUSTER_TIMESTAMP,curr_end,
            CM_OMI_FIELD_PMM_CLUSTER_TIMESTAMP,(query->max - query->index));
        cm_db_close(handle);
        handle = NULL;
        
        if(CM_OK != iRet)
        {
            CM_LOG_WARNING(CM_MOD_PMM,"db[%s] start[%u] end[%u] fail[%d]",
                db_file,start,curr_end,iRet);
        }        
    }

    if((query->index < query->max)
        && (end > start))
    {
        /* 从当前文件中取 */
        iRet=cm_db_exec(g_cm_pmm_db,cm_pmm_db_get, query,
            "%s WHERE f%u>=%u AND f%u<%u "
            "ORDER BY f%u LIMIT %u",select_data,
            CM_OMI_FIELD_PMM_CLUSTER_TIMESTAMP,start,
            CM_OMI_FIELD_PMM_CLUSTER_TIMESTAMP,end,
            CM_OMI_FIELD_PMM_CLUSTER_TIMESTAMP,(query->max - query->index));
    }
    return iRet;
}

static void cm_pmm_get_range(
    const cm_pmm_decode_t *pDecode,cm_time_t *pstart, cm_time_t *pend)
{
    cm_time_t start = (cm_time_t)pDecode->start;
    cm_time_t end = (cm_time_t)pDecode->end;
    cm_time_t now = cm_get_time();
    cm_time_t tmp_min = now;
    
    if((0 == end) || (end > now))
    {
        end = now;
    }

    if(0 == g_cm_pmm_cfg.save_hours)
    {
        tmp_min -= CM_PMM_HISTORY_DATA_TIME;
    }
    else
    {
        tmp_min -= (g_cm_pmm_cfg.save_hours * CM_PMM_HISTORY_DATA_TIME);
    }    
    
    if((0 == start) || (start < tmp_min))
    {
        start = tmp_min;
    }

    *pstart = start;
    *pend = end;
    return;
}

static void cm_pmm_get_fields(const cm_pmm_decode_t *pDecode, sint8 *field_out, 
    uint32 buf_len,uint32 *pfield_num,const cm_omi_map_object_t* pObj)
{
    uint32 field_num = pObj->field_num;
    uint32 iloop = 0;
    uint32 len = 0;
    uint32 tmpid = 0;
    sint8 *fields[CM_PMM_MAX_FIELDS_NUM] = {NULL};
    const sint8 *fields_str = pDecode->fields; 

    buf_len -= 1;
    if('\0' == fields_str[0])
    {
        if(pDecode->grain_size == CM_OMI_PMM_GRAIN_SEC)
        {
            field_out[0] = '*';
        }
        else
        {
            len = CM_VSPRINTF(field_out,buf_len,"f0",iloop,iloop);
            for(iloop=1;iloop<pObj->field_num;iloop++)
            {
                len += CM_VSPRINTF(field_out+len,buf_len-len,",AVG(f%u) as a%u",iloop,iloop);
            }
        }
        return;
    }
   
    field_num = cm_omi_get_fields(fields_str,fields,CM_PMM_MAX_FIELDS_NUM);

    if(pDecode->grain_size == CM_OMI_PMM_GRAIN_SEC)
    {
        for(iloop=0;iloop<field_num;iloop++)
        {
            tmpid = (uint32)atoi(fields[iloop]);
            if(iloop == 0)
            {
                len = CM_VSPRINTF(field_out,buf_len,"f%u",tmpid);
            }
            else
            {
                len += CM_VSPRINTF(field_out+len,buf_len-len,",f%u",tmpid);
            }
        }
    }
    else
    {
        for(iloop=0;iloop<field_num;iloop++)
        {
            tmpid = (uint32)atoi(fields[iloop]);
            if(tmpid == 0)
            {
                if(iloop == 0)
                {
                    len = CM_VSPRINTF(field_out,buf_len,"f0");
                }
                else
                {
                    len += CM_VSPRINTF(field_out+len,buf_len-len,",f0");
                }
            }
            else
            {
                if(iloop == 0)
                {
                    len = CM_VSPRINTF(field_out,buf_len,"AVG(f%u) as a%u",tmpid,tmpid);
                }
                else
                {
                    len += CM_VSPRINTF(field_out+len,buf_len-len,",AVG(f%u) as a%u",tmpid,tmpid);
                }
            }
            
        }
    }
    *pfield_num = field_num;
    return;
}


static sint32 cm_pmm_get_local(uint32 msg, const cm_pmm_decode_t *pDecode,void **ppAckData, uint32 *pAckLen)
{
    const cm_omi_map_object_t* pObj = cm_omi_get_map_obj_cfg(pDecode->obj);
    sint8 select_data[CM_STRING_512] = {0};
    uint32 field_num = pObj->field_num;
    uint32 max_cnt = (pDecode->max_cnt == 0)?CM_PMM_MAX_RECORD_CNT:pDecode->max_cnt;
    cm_db_query_t query = {NULL,0,0};
    cm_pmm_encode_t *pdata = NULL;
    sint32 iRet = CM_FAIL;
    uint32 len = 0;
    cm_time_t start = 0;
    cm_time_t end = 0;

    /* SELECT f0,f1 FROM table_3_t */
    len = CM_VSPRINTF(select_data,sizeof(select_data),"SELECT ");
    cm_pmm_get_fields(pDecode,select_data+len,sizeof(select_data)-len,&field_num,pObj);
    len = strlen(select_data);
    len += CM_VSPRINTF(select_data + len,sizeof(select_data)-len,
        " FROM table_%u_t",pObj->obj.code);
    
    query.max = (CM_PMM_MSG_GET_CURRENT == msg) ? 1:max_cnt;
   
    len = sizeof(cm_pmm_encode_t) + (sizeof(double)*field_num*query.max);
    pdata = CM_MALLOC(len);

    if(NULL == pdata)
    {
        CM_LOG_ERR(CM_MOD_PMM,"malloc fail");
        return CM_FAIL;
    }
    CM_MEM_ZERO(pdata,len);
    
    pdata->cols = field_num;
    query.data = pdata;

    CM_MUTEX_LOCK(&g_cm_pmm_mutex);
    if(CM_PMM_MSG_GET_CURRENT == msg)
    {
        /* 获取最新记录 */
        iRet=cm_db_exec(g_cm_pmm_db,cm_pmm_db_get,&query,
            "%s ORDER BY f%u DESC LIMIT 1",select_data,
            CM_OMI_FIELD_PMM_CLUSTER_TIMESTAMP);
    }
    else
    {
        cm_pmm_get_range(pDecode,&start,&end);
        if((0 == g_cm_pmm_cfg.save_hours)
            ||(start >= (cm_get_time() - CM_PMM_HISTORY_DATA_TIME)))
        {
            /* 从当前数据库中获取 */
            iRet=cm_db_exec(g_cm_pmm_db,cm_pmm_db_get,&query,
                "%s WHERE f%u>=%u AND f%u<%u "
                "ORDER BY f%u LIMIT %u",select_data,
                CM_OMI_FIELD_PMM_CLUSTER_TIMESTAMP,start,
                CM_OMI_FIELD_PMM_CLUSTER_TIMESTAMP,end,
                CM_OMI_FIELD_PMM_CLUSTER_TIMESTAMP,query.max);
        }
        else
        {
            /* 从备份数据库中获取 */
            iRet = cm_pmm_get_data_from_backup(select_data,start,end,&query);
        }
    }
    
    CM_MUTEX_UNLOCK(&g_cm_pmm_mutex);
    
    if(CM_OK != iRet)
    {
        CM_FREE(pdata);
        CM_LOG_ERR(CM_MOD_PMM,"select_data:%s, iRet:%d",select_data,iRet);
        return iRet;
    }
    
    if(0 == query.index)
    {
        CM_FREE(pdata);
        return CM_OK;
    }
    *ppAckData = pdata;
    *pAckLen = sizeof(cm_pmm_encode_t) + (sizeof(double) * field_num * pdata->rows);
    return CM_OK;
}

static sint32 cm_pmm_local_current(const cm_pmm_decode_t *pDecode,void **ppAckData, uint32 *pAckLen)
{
    return cm_pmm_get_local(CM_PMM_MSG_GET_CURRENT, pDecode,ppAckData,pAckLen);
}

static sint32 cm_pmm_local_history(const cm_pmm_decode_t *pDecode,void **ppAckData, uint32 *pAckLen)
{
    return cm_pmm_get_local(CM_PMM_MSG_GET_HISTORY , pDecode,ppAckData,pAckLen);
}

static sint32 cm_pmm_local_current_mem(const cm_pmm_decode_t *pDecode,void **ppAckData, uint32 *pAckLen)
{
    const cm_pmm_obj_cfg_t *pCfg = g_cm_pmm_obj;
    uint32 cnt = sizeof(g_cm_pmm_obj)/sizeof(cm_pmm_obj_cfg_t);

    while(cnt > 0)
    {
        cnt--;
        if(pCfg->obj_id == pDecode->obj)
        {
            return pCfg->get_current(pDecode,ppAckData,pAckLen);
        }
        pCfg++;
    }
    return CM_FAIL;
}

static sint32 cm_pmm_get(uint32 msg,uint32 obj,const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    cm_pmm_decode_t decode;
    const cm_pmm_decode_t *pDecode = pDecodeParam;
    uint32 nid = 0;
    
    if(NULL == pDecode)
    {
        pDecode = &decode;
        CM_MEM_ZERO(&decode,sizeof(cm_pmm_decode_t));
        decode.obj = obj;
        decode.max_cnt = CM_PMM_MAX_RECORD_CNT;
    }
    else
    {
        nid = pDecode->parent_id;
    }
    
    if((0 == nid) 
        || (cm_node_get_id() == nid))
    {
        switch(msg)
        {
            case CM_PMM_MSG_GET_CURRENT:
            case CM_PMM_MSG_GET_HISTORY:
                return cm_pmm_get_local(msg, pDecode,ppAckData,pAckLen);
            case CM_PMM_MSG_GET_CURRENT_MEM:
                return cm_pmm_local_current_mem(pDecode,ppAckData,pAckLen);
            default:
                break;
        }
        return CM_OK;
    }
    return cm_pmm_request(nid,msg,pDecode,sizeof(cm_pmm_decode_t),ppAckData,pAckLen);
}

sint32 cm_pmm_current_mem(uint32 obj,uint32 id,uint32 parent_id,void **ppAckData, uint32 *pAckLen)
{
    cm_pmm_decode_t decode;

    CM_MEM_ZERO(&decode,sizeof(cm_pmm_decode_t));
    decode.parent_id = parent_id;
    decode.id = id;
    decode.obj = obj;

    if((0 == parent_id) 
        || (cm_node_get_id() == parent_id))
    {
        return cm_pmm_local_current_mem(&decode,ppAckData,pAckLen);
    }
    return cm_pmm_request(parent_id,CM_PMM_MSG_GET_CURRENT_MEM,&decode,sizeof(cm_pmm_decode_t),ppAckData,pAckLen);
}

sint32 cm_pmm_current(uint32 obj,const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    cm_pmm_encode_t *data = NULL;
    cm_pmm_data_t *pStat = NULL;
    uint32 stat_cols = 0;  
    uint32 AckLen = 0;
    sint32 iRet = cm_pmm_get(CM_PMM_MSG_GET_CURRENT_MEM,obj,pDecodeParam,ppAckData,pAckLen);

    if((CM_OK == iRet) && (*pAckLen > 0))
    {
        stat_cols = (*pAckLen - sizeof(cm_pmm_data_t))/sizeof(double);
        AckLen = sizeof(cm_pmm_encode_t)+(stat_cols+2)*sizeof(double);
        
        data = CM_MALLOC(AckLen);
        if(NULL != data)
        {
            data->rows = 1;
            data->cols = stat_cols + 2; /* id tm */
            pStat = (cm_pmm_data_t*)*ppAckData;
            data->data[0] = (double)pStat->id;
            if(0 != pStat->tmstamp)
            {
                data->data[1] = (double)pStat->tmstamp;/*cm_get_time()*/;
            }
            else
            {
                data->data[1] = (double)cm_get_time();
            }
            stat_cols *= sizeof(double);
            CM_MEM_CPY(&data->data[2],stat_cols,pStat->val,stat_cols);            
            
            CM_FREE(*ppAckData);
            *ppAckData = data;
            *pAckLen = AckLen;
        }
        else
        {
            CM_FREE(*ppAckData);
            *ppAckData = NULL;
            *pAckLen = 0;
        }
    }
    return iRet;
}

sint32 cm_pmm_history(uint32 obj,const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    const cm_pmm_decode_t *pDecode = pDecodeParam;

    if((NULL != pDecode)
        && (0 != pDecode->start)
        && ((cm_time_t)pDecode->start > cm_get_time()))
    {
        return CM_OK;
    }
    return cm_pmm_get(CM_PMM_MSG_GET_HISTORY,obj,pDecodeParam,ppAckData,pAckLen);
}

sint32 cm_pmm_cbk_cmt(
    void *pMsg, uint32 Len, void** ppAck, uint32 *pAckLen)
{
    cm_pmm_msg_t *msg = (cm_pmm_msg_t*)pMsg;    
    const cm_pmm_msg_cbk_func_t *pcfg = NULL;
    
    if((NULL == msg) || (NULL == g_cm_pmm_db))
    {
        CM_LOG_ERR(CM_MOD_PMM,"pMsg null");
        return CM_FAIL;
    }
    
    if(msg->id >= CM_PMM_MSG_BUTT)
    {
        CM_LOG_ERR(CM_MOD_PMM,"msg[%u]",msg->id);
        return CM_FAIL;
    }

    if(Len < sizeof(cm_pmm_msg_t))
    {
        CM_LOG_ERR(CM_MOD_PMM,"len[%u]", Len);
        return CM_FAIL;
    }
    Len -= sizeof(cm_pmm_msg_t);
    pcfg = &g_cm_pmm_msg_cfg[msg->id];

    if(Len < pcfg->min_len)
    {
        CM_LOG_ERR(CM_MOD_PMM,"Len[%u] min_len[%u]",Len,pcfg->min_len);
        return CM_FAIL;
    }
    return pcfg->cbk(msg->data,ppAck,pAckLen);
}

sint32 cm_pmm_request(uint32 nid,uint32 msg,const void* req, uint32 len, void **ppAckData, uint32 *pAckLen)
{
    uint32 datalen = sizeof(cm_pmm_msg_t) + len;
    cm_pmm_msg_t *pmsg = CM_MALLOC(datalen);
    sint32 iRet = CM_FAIL;
    
    if(NULL == pmsg)
    {
        CM_LOG_ERR(CM_MOD_PMM,"malloc fail");
        return CM_FAIL;
    }
    pmsg->id = msg;
    CM_MEM_CPY(pmsg->data,len,req,len);
    iRet = cm_cmt_request(nid,CM_CMT_MSG_PMM,pmsg,datalen,ppAckData,pAckLen,CM_PMM_TMOUT);
    CM_FREE(pmsg);
    return iRet;
}

static const sint8* cm_pmm_db_field_get(const cm_omi_map_data_t *data)
{
    const sint8* res = NULL;
    switch(data->type)
    {
        case CM_OMI_DATA_STRING:
            res = "VARCHAR(255)";
            break;
        case CM_OMI_DATA_DOUBLE:
            res = "DOUBLE default 0.0";
            break;
        default:
            switch(data->size)
            {
                case 1:
                    res = "TINYINT default 0";
                    break;
                case 2:
                    res = "SMALLINT default 0";
                    break;
                case 4:
                    res = "INT default 0";
                    break;
                case 8:
                    res = "BIGINT default 0";
                    break;
                default:
                    res = "INT default 0";
                    break;
            }
            break;
    }
    return res;
}

static sint32 cm_pmm_db_init(const cm_pmm_obj_cfg_t* pcfg, uint32 cnt)
{
    cm_db_handle_t handle = NULL;
    uint32 field_num = 0;
    const cm_omi_map_object_field_t *pfield = NULL;
    const cm_omi_map_object_t* pObj = NULL;
    sint8 sqlbuf[CM_STRING_512+1] = {0};
    sint32 len = 0;
    sint32 iRet = cm_db_open_ext(CM_PMM_DB_DIR
                                CM_PMM_DB_NAME,&handle);
    
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_PMM,"open %s fail",CM_PMM_DB_NAME);
        return CM_FAIL;
    }

    while(cnt > 0)
    {
        cnt--;
        if(NULL != pcfg->init)
        {
            iRet = pcfg->init();
            if(CM_OK != iRet)
            {
                CM_LOG_ERR(CM_MOD_PMM,"init %u fail %d",pcfg->obj_id,iRet);
                return iRet; 
            }
        }
        field_num = 0;
        pObj = cm_omi_get_map_obj_cfg(pcfg->obj_id);
        pfield = pObj->fields;
        len = CM_VSPRINTF(sqlbuf,CM_STRING_512,"CREATE TABLE IF NOT EXISTS table_%u_t (",pObj->obj.code);
        while(field_num < pObj->field_num)
        {
            if(0 == field_num)
            {
                len += CM_VSPRINTF(sqlbuf+len,CM_STRING_512-len,"f%u %s",
                    pfield->field.code,cm_pmm_db_field_get(&pfield->data));
            }
            else
            {
                len += CM_VSPRINTF(sqlbuf+len,CM_STRING_512-len,",f%u %s",
                    pfield->field.code,cm_pmm_db_field_get(&pfield->data));
            }
            field_num++;
            pfield++;
        }
        len += CM_VSPRINTF(sqlbuf+len,CM_STRING_512-len,")");
        iRet = cm_db_exec_ext(handle,"%s",sqlbuf);
        if(CM_OK != iRet)
        {
            CM_LOG_ERR(CM_MOD_PMM,"exe %s fail(%d)",sqlbuf,iRet);
            cm_db_close(handle);
            return iRet;
        }
        pcfg++;
    }
    g_cm_pmm_db = handle;
    return CM_OK;
}

sint32 cm_pmm_db_update(uint32 obj,cm_pmm_data_t *pdata)
{
    const cm_omi_map_object_t *pObj = cm_omi_get_map_obj_cfg(obj);
    uint32 field_num = pObj->field_num;
    const cm_omi_map_object_field_t *pfield = pObj->fields;
    double *pval = pdata->val;
    sint8 cols[CM_STRING_512+1] = {0};
    sint8 vals[CM_STRING_512+1] = {0};
    sint32 iRet = CM_FAIL;
    sint32 len_col = CM_VSPRINTF(cols,CM_STRING_512,"f%u,f%u",pfield[0].field.code,pfield[1].field.code);
    sint32 len_val = CM_VSPRINTF(vals,CM_STRING_512,"%u,%u",pdata->id,(uint32)cm_get_time());

    pfield += 2;
    field_num -= 2;
    while(field_num > 0)
    {
        len_col += CM_VSPRINTF(cols+len_col,CM_STRING_512-len_col,",f%u",pfield->field.code);
        len_val += CM_VSPRINTF(vals+len_val,CM_STRING_512-len_val,",%.02lf",*pval);
        field_num--;
        pfield++;
        pval++;
    }
    CM_MUTEX_LOCK(&g_cm_pmm_mutex);
    iRet = cm_db_exec_ext(g_cm_pmm_db,"INSERT INTO table_%u_t (%s) VALUES (%s)",
        pObj->obj.code,cols,vals);
    CM_MUTEX_UNLOCK(&g_cm_pmm_mutex);
    return iRet; 
}

static void cm_pmm_db_delete(cm_pmm_obj_cfg_t *pcfg, uint32 cnt)
{
    cm_time_t now = cm_get_time();

    now -= CM_PMM_HISTORY_DATA_TIME;
    CM_MUTEX_LOCK(&g_cm_pmm_mutex);
    while(cnt > 0)
    {
        cnt--;
        (void)cm_db_exec_ext(g_cm_pmm_db,
            "DELETE FROM table_%u_t WHERE f%u<%u",
            pcfg->obj_id,CM_OMI_FIELD_PMM_CLUSTER_TIMESTAMP,now);
        pcfg++;
    }
    CM_MUTEX_UNLOCK(&g_cm_pmm_mutex);
    return;
}

static void cm_pmm_dir_check(const sint8 *dir, sint32 max)
{
    sint32 cnt = 0;

    cnt = cm_exec_int("df -h %s |sed -n 2p |awk '{print $5}' |sed 's/%//g'",dir);

    if(cnt >= 90)
    {
        cm_system("cd %s;"
            "rm -f `ls -l |grep '^-' |sed -n '1p' |awk '{print $9}'`;",dir);
    }
    
    cnt = cm_exec_int("ls -l %s |grep '^-' |wc -l", dir);

    if(cnt <= max) 
    {
        return;
    }

    cnt -= max;
    cm_system("cd %s;"
        "rm -f `ls -l |grep '^-' |sed -n '1,%dp' |awk '{print $9}'`;"
        ,dir, cnt);
    return;
}

static void cm_pmm_db_backup(struct tm *now)
{
    uint32 cnt = 0;
    
    CM_MUTEX_LOCK(&g_cm_pmm_mutex);
    cnt = g_cm_pmm_cfg.save_hours;
    
    if(0 == cnt)
    {
        cm_pmm_dir_check(CM_PMM_DB_BACKUP_DIR,0);
        CM_MUTEX_UNLOCK(&g_cm_pmm_mutex);
        return;
    }
    
    (void)cm_system("cp %s %s.%04d%02d%02d_%02d",
        CM_PMM_DB_DIR
        CM_PMM_DB_NAME,
        CM_PMM_DB_BACKUP_DIR
        CM_PMM_DB_NAME,
        now->tm_year,now->tm_mon,now->tm_mday,now->tm_hour
        );

    cm_pmm_dir_check(CM_PMM_DB_BACKUP_DIR,cnt);
    CM_MUTEX_UNLOCK(&g_cm_pmm_mutex);
    return;
}

static void cm_pmm_db_backup_once(void)
{
    /* 处理进程因为停止错过数据备份问题 */
    uint64 last_time = 0;
    sint32 iRet = CM_OK;
    struct tm last_tm;
    cm_time_t last_backup = cm_get_time_hour_floor(cm_get_time());
    
    CM_MUTEX_LOCK(&g_cm_pmm_mutex);
    do
    {
        if(0 == g_cm_pmm_cfg.save_hours)
        {
            break;
        }
        iRet = cm_db_exec_get_count(g_cm_pmm_db,&last_time,
            "SELECT MAX(f%d) FROM table_%d_t",
            CM_OMI_FIELD_PMM_CLUSTER_TIMESTAMP,
            CM_OMI_OBJECT_PMM_CLUSTER);
        if((CM_OK != iRet) || (0 == last_time))
        {
            break;
        }
        
        if(((cm_time_t)last_time) >= last_backup)
        {
            break;
        }

        cm_get_datetime_ext(&last_tm,(cm_time_t)last_time);
        (void)cm_system("cp %s %s.%04d%02d%02d_%02d",
            CM_PMM_DB_DIR
            CM_PMM_DB_NAME,
            CM_PMM_DB_BACKUP_DIR
            CM_PMM_DB_NAME,
            last_tm.tm_year,last_tm.tm_mon,last_tm.tm_mday,last_tm.tm_hour
            );
        cm_pmm_dir_check(CM_PMM_DB_BACKUP_DIR,g_cm_pmm_cfg.save_hours);
    }while(0);
    
    CM_MUTEX_UNLOCK(&g_cm_pmm_mutex);
    return;
}

static void* cm_pmm_thread(void* arg)
{
    cm_pmm_obj_cfg_t *pcfg = NULL;
    uint32 cnt_max = sizeof(g_cm_pmm_obj)/sizeof(cm_pmm_obj_cfg_t);
    uint32 cnt = 0;    
    struct tm pre;
    struct tm now;

    cm_pmm_db_backup_once();
    
    cm_get_datetime(&pre);
    while(1)
    {
        pcfg = g_cm_pmm_obj;
        cnt = cnt_max;
        cm_get_datetime(&now);
        
        while(cnt > 0)
        {
            cnt--;
            if(NULL != pcfg->task)
            {
                pcfg->task(&now);
            }
            pcfg++;
        }
        if(0 == now.tm_sec % 5)
        {
            cm_pmm_tree_node_update(cm_pmm_node_root);
        }
        
        if(pre.tm_min != now.tm_min)
        {
            /* 每分钟执行一次删除 */
            cm_pmm_db_delete(g_cm_pmm_obj,cnt_max);
        }
        
        if(pre.tm_hour != now.tm_hour)
        {
            /* 每小时备份一次历史数据 */
            cm_pmm_db_backup(&pre);
        }
        
        CM_MEM_CPY(&pre,sizeof(pre),&now,sizeof(pre));
        CM_SLEEP_S(1);
    }
    
    return NULL;
}

static void cm_pmm_cfg_init(cm_pmm_cfg_t *cfg)
{
    sint32 iRet = cm_ini_get_ext_uint32(CM_CLUSTER_INI,CM_PMM_CFG_SECTION,
        CM_PMM_CFG_SAVE_HOURS, &cfg->save_hours);

    if(CM_OK != iRet)
    {
        cfg->save_hours = 0;
    }
    return;
}

sint32 cm_pmm_cfg_update(cm_pmm_cfg_t *cfg)
{
    return cm_sync_request(CM_SYNC_OBJ_PMM_CONFIG,0,cfg,sizeof(cm_pmm_cfg_t));
}

sint32 cm_pmm_cfg_sync_request(uint64 data_id, void *pdata, uint32 len)
{
    sint32 iRet = CM_OK;
    cm_pmm_cfg_t *cfg = pdata;

    if(len != sizeof(cm_pmm_cfg_t))
    {   
        CM_LOG_ERR(CM_MOD_PMM,"len[%u]",len);
        return CM_FAIL;
    }
    CM_MUTEX_LOCK(&g_cm_pmm_mutex);
    g_cm_pmm_cfg.save_hours = cfg->save_hours;
    iRet = cm_ini_set_ext_uint32(CM_CLUSTER_INI,
        CM_PMM_CFG_SECTION,CM_PMM_CFG_SAVE_HOURS, g_cm_pmm_cfg.save_hours);
    CM_MUTEX_UNLOCK(&g_cm_pmm_mutex);
    return iRet;
}

sint32 cm_pmm_cfg_sync_get(uint64 data_id, void **pdata, uint32 *plen)
{
    cm_pmm_cfg_t *cfg = CM_MALLOC(sizeof(cm_pmm_cfg_t));

    if(NULL == cfg)
    {
        CM_LOG_ERR(CM_MOD_PMM,"malloc fail");
        return CM_FAIL;
    }
    CM_MUTEX_LOCK(&g_cm_pmm_mutex);
    cfg->save_hours = g_cm_pmm_cfg.save_hours;
    CM_MUTEX_UNLOCK(&g_cm_pmm_mutex);

    *pdata = cfg;
    *plen = sizeof(cm_pmm_cfg_t);
    return CM_OK;
}

sint32 cm_pmm_init(void)
{
    sint32 iRet = CM_FAIL;
    cm_thread_t handle;

    cm_system("mkdir -p %s 2>/dev/null",CM_PMM_DB_BACKUP_DIR);
    CM_MEM_ZERO(&g_cm_pmm_cfg,sizeof(cm_pmm_cfg_t));
    CM_MUTEX_INIT(&g_cm_pmm_mutex);
    CM_MUTEX_INIT(&g_cm_pmm_common_mutex);
    cm_pmm_node_root = cm_pmm_tree_node_new("root");

    cm_pmm_cfg_init(&g_cm_pmm_cfg);
    iRet = cm_pmm_db_init(g_cm_pmm_obj, sizeof(g_cm_pmm_obj)/sizeof(cm_pmm_obj_cfg_t));
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_PMM,"init db iret:%d",iRet);
        return iRet;
    }

    iRet = CM_THREAD_CREATE(&handle,cm_pmm_thread,NULL);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_PMM,"init thread iret:%d",iRet);
        return iRet;
    }
    CM_THREAD_DETACH(handle);
    
    return CM_OK;
}


