/******************************************************************************
 *          Copyright (c) 2017 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_sync.c
 * author     : wbn
 * create date: 2017Äê10ÔÂ27ÈÕ
 * description: TODO:
 *
 *****************************************************************************/
#include "cm_sync.h"
#include "cm_queue.h"
#include "cm_log.h"
#include "cm_node.h"
#include "cm_cmt.h"
#include "cm_htbt.h"
#include "cm_db.h"

#define CM_SYNC_DB_DIR CM_DATA_DIR
#define CM_SYNC_DB_NAME "cm_sync.db"

#define CM_SYNC_REQ_DELETE 1
#define CM_SYNC_REQ_UPDATE 0

extern const cm_sync_config_t g_cm_sync_config[CM_SYNC_OBJ_BUTT];

typedef struct
{
    uint64 step;
    uint64 data_id;
    uint32 obj_id;
    uint32 state;
}cm_sync_info_t;

typedef struct
{
    bool_t is_run;
    cm_mutex_t mutex;
    cm_db_handle_t db_handle;
    uint64 steps[CM_SYNC_OBJ_BUTT];
    cm_queue_t *queue;
}cm_sync_global_info_t;

typedef struct
{
    uint64 data_id;
    uint64 step;
    uint32 obj_id;
    uint32 data_len;
    uint64 is_delete;
    uint8 data[];
}cm_sync_obj_info_t;

typedef struct
{
    uint64 data_id;
    uint64 step;
    uint32 obj_id;
    uint64 is_delete;
}cm_sync_db_record_t;


typedef struct
{
    uint32 obj_id;
    uint64 step;
}cm_sync_msg_obj_info_t;

static cm_sync_global_info_t g_cm_sync_global;
static sint32 cm_sync_request_local(cm_sync_global_info_t *pglobal, cm_sync_obj_info_t 
*pReq);


static sint32 cm_sync_get_obj_info(
    void *arg, sint32 col_cnt, sint8 **col_vals, sint8 **col_names)
{
    cm_sync_db_record_t *db_record = arg;

    if(col_cnt < 3)
    {
        return CM_FAIL;
    }
    db_record->data_id = (uint64)atoll(col_vals[0]);
    db_record->step = (uint64)atoll(col_vals[1]);
    db_record->is_delete = (uint8)atoll(col_vals[2]);
    return CM_OK;
}

sint32 cm_sync_cbk_cmt_obj_info(void *pMsg, uint32 Len, void **ppAck, uint32 *pAckLen)
{
    cm_sync_global_info_t *pglobal = &g_cm_sync_global;
    cm_sync_msg_obj_info_t *obj = (cm_sync_msg_obj_info_t*)pMsg;
    const cm_sync_config_t *cfg = NULL;
    cm_sync_db_record_t db_record = {0,0,0,0};
    cm_sync_obj_info_t *pAckInfo = NULL;
    uint32 count = 0;
    void *pAckData = NULL;
    uint32 AckLen = 0;
    sint32 iRet = CM_FAIL;
    uint64 step = 0;
    if(Len != sizeof(cm_sync_msg_obj_info_t))
    {
        CM_LOG_ERR(CM_MOD_SYNC,"len[%u]",Len);
        return CM_FAIL;
    }
    
    if(CM_SYNC_OBJ_BUTT <= obj->obj_id)
    {
        CM_LOG_ERR(CM_MOD_SYNC,"obj[%u]",obj->obj_id);
        return CM_ERR_NOT_SUPPORT;
    }

    step = obj->step;
    while(1)
    {
        count = cm_db_exec_get(pglobal->db_handle,cm_sync_get_obj_info,
            &db_record,1,sizeof(cm_sync_db_record_t),
            "SELECT data_id,step,is_delete FROM record_t WHERE obj_id=%u AND step>%llu"
            " ORDER BY step LIMIT 1",obj->obj_id,step);
        if(0 == count)
        {
            return CM_OK;
        }
        
        if(CM_SYNC_REQ_UPDATE == db_record.is_delete)
        {
            cfg = &g_cm_sync_config[obj->obj_id];
            if(NULL == cfg->cbk_get)
            {
                return CM_ERR_NOT_SUPPORT;
            }
            iRet = cfg->cbk_get(db_record.data_id,&pAckData,&AckLen);
            if(CM_OK != iRet)
            {
                CM_LOG_ERR(CM_MOD_SYNC,"get obj[%u] data_id[%llu] step[%llu] iRet[%d]",
                    obj->obj_id,db_record.data_id,db_record.step,iRet);
                step = db_record.step;
                continue;
            }
            if(NULL == pAckData)
            {
                CM_LOG_ERR(CM_MOD_SYNC,"get obj[%u] data_id[%llu] step[%llu] ack null",
                    obj->obj_id,db_record.data_id, db_record.step);
                step = db_record.step;
                continue;
            }
        }
        break;
    }
    
    do
    {
        count = sizeof(cm_sync_obj_info_t) + AckLen;
        pAckInfo = CM_MALLOC(count);
        if(NULL == pAckInfo)
        {
            CM_LOG_ERR(CM_MOD_SYNC,"malloc fail");
            iRet = CM_FAIL;
            break;
        }
        pAckInfo->data_id = db_record.data_id;
        pAckInfo->step = db_record.step;
        pAckInfo->obj_id = obj->obj_id;
        pAckInfo->data_len = AckLen;
        pAckInfo->is_delete = db_record.is_delete;
        if(0 < AckLen)
        {
            CM_MEM_CPY(pAckInfo->data,AckLen,pAckData,AckLen);
        }
        *ppAck = pAckInfo;
        *pAckLen = count;
        iRet = CM_OK;
    }while(0);
    
    if(NULL != pAckData)
    {
        CM_FREE(pAckData);
    }
    return iRet;
}

sint32 cm_sync_cbk_cmt_obj_step(void *pMsg, uint32 Len, void **ppAck, uint32 *pAckLen)
{
    uint32 *pobj_id = (uint32*)pMsg;
    cm_sync_global_info_t *pglobal = &g_cm_sync_global;
    uint64 *pstep = NULL;
    
    if(Len != sizeof(uint32))
    {
        CM_LOG_ERR(CM_MOD_SYNC,"len[%u]",Len);
        return CM_FAIL;
    }
    
    if(CM_SYNC_OBJ_BUTT <= *pobj_id)
    {
        CM_LOG_ERR(CM_MOD_SYNC,"obj[%u]",*pobj_id);
        return CM_ERR_NOT_SUPPORT;
    }

    pstep = CM_MALLOC(sizeof(uint64));
    if(NULL == pstep)
    {
        CM_LOG_ERR(CM_MOD_SYNC,"malloc fail");
        return CM_FAIL;
    }

    *ppAck = pstep;
    *pAckLen = sizeof(uint64);

    CM_MUTEX_LOCK(&pglobal->mutex);
    *pstep = pglobal->steps[*pobj_id];
    CM_MUTEX_UNLOCK(&pglobal->mutex);
    return CM_OK;
}

static void cm_sync_thread_once_exe(cm_sync_global_info_t *pglobal,uint32 obj)
{
    sint32 iRet = CM_FAIL;
    uint64 tmp_end = 0;
    uint64 tmp_curr = 0;
    uint64 count = 0;
    void *pAckData = NULL;
    uint32 AckLen = 0;
    cm_sync_msg_obj_info_t info = {obj,0};
    cm_sync_obj_info_t *pAckInfo = NULL;
    
    (void)cm_db_exec_get_count(pglobal->db_handle,&tmp_curr,
            "SELECT MAX(step) FROM cache_t WHERE obj_id=%u",obj);
    tmp_curr = (tmp_curr == 0)? pglobal->steps[obj] : tmp_curr;

    iRet = cm_cmt_request_master(CM_CMT_MSG_SYNC_GET_STEP,
        &obj,sizeof(obj),&pAckData,&AckLen,CM_SYNC_SEND_TMOUT);
    if(CM_OK != iRet)
    {
        return;
    }
    if(NULL == pAckData)
    {
        CM_LOG_ERR(CM_MOD_SYNC,"obj[%u] ack null",obj);
        return;
    }
    tmp_end = *((uint64*)pAckData);
    CM_FREE(pAckData);
    pAckData = NULL;
    AckLen = 0;
    CM_LOG_INFO(CM_MOD_SYNC,"obj[%u] step[%llu] end[%llu]",obj,tmp_curr,tmp_end);
    
    while(tmp_curr < tmp_end)
    {
        CM_LOG_INFO(CM_MOD_SYNC,"obj[%u] step[%llu]",obj,tmp_curr);
        info.step = tmp_curr;
        iRet = cm_cmt_request_master(CM_CMT_MSG_SYNC_GET_INFO,
            &info,sizeof(info),&pAckData,&AckLen,CM_SYNC_SEND_TMOUT);
        if(CM_OK != iRet)
        {
            CM_LOG_ERR(CM_MOD_SYNC,"obj[%u] tmp_curr[%llu]",obj,tmp_curr);
            return;
        }
        if(NULL == pAckData)
        {
            break;
        }
        pAckInfo = (cm_sync_obj_info_t*)pAckData;

        count = 0;
        (void)cm_db_exec_get_count(pglobal->db_handle,&count,
            "SELECT COUNT(data_id) FROM record_t WHERE obj_id=%u "
            "AND step=%llu",obj,pAckInfo->step);
        if(0 == count)
        {
            CM_MUTEX_LOCK(&pglobal->mutex);
            iRet = cm_sync_request_local(pglobal,pAckInfo);
            CM_MUTEX_UNLOCK(&pglobal->mutex);
            
            if(CM_OK != iRet)
            {            
                CM_LOG_ERR(CM_MOD_SYNC,"cbk_request obj[%u] data_id[%llu] fail[%d]",
                    obj,pAckInfo->data_id,iRet);
            }
        }
        else
        {
            CM_LOG_INFO(CM_MOD_SYNC,"skip:obj[%u] step[%llu]",obj,pAckInfo->step);
        }
        
        (void)cm_db_exec_ext(pglobal->db_handle,"UPDATE cache_t SET step=%llu "
            "WHERE obj_id=%u",pAckInfo->step,obj);
        tmp_curr = pAckInfo->step;
        CM_FREE(pAckData);
        pAckData = NULL;
        AckLen = 0;
    }
    
    (void)cm_db_exec_ext(pglobal->db_handle,"DELETE FROM cache_t "
            "WHERE obj_id=%u",obj);
    return;
}

static void* cm_sync_thread_once(void* arg)
{
    cm_sync_global_info_t *pglobal = arg;
    const cm_sync_config_t* cfg = g_cm_sync_config;
    uint32 iloop = 0;
    uint32 domain_id = cm_node_get_subdomain_id();
    
    pglobal->is_run = CM_TRUE;
    CM_LOG_INFO(CM_MOD_SYNC,"start");

    for( ; iloop<CM_SYNC_OBJ_BUTT; iloop++,cfg++)
    {
        if(cfg->mask & CM_SYNC_FLAG_REAL_TIME_ONLY)
        {
            continue;
        }
        if((cfg->mask & CM_SYNC_FLAG_MASTER_DOMAIN)
            && (CM_MASTER_SUBDOMAIN_ID != domain_id))
        {
            continue;
        }
        cm_sync_thread_once_exe(pglobal,iloop);
    }
    CM_LOG_INFO(CM_MOD_SYNC,"end");
    pglobal->is_run = CM_FALSE;
    return NULL;
}


static const cm_sync_config_t* cm_sync_get_config(uint32 obj_id)
{
    if(obj_id >= CM_SYNC_OBJ_BUTT)
    {
        return NULL;
    }
    return &g_cm_sync_config[obj_id];
}

static sint32 cm_sync_request_local(cm_sync_global_info_t *pglobal, cm_sync_obj_info_t *pReq)
{
    const cm_sync_config_t *cfg = cm_sync_get_config(pReq->obj_id);
    sint32 iRet = CM_ERR_NOT_SUPPORT;
    uint64 count = 0;
    if(NULL == cfg)
    {
        CM_LOG_ERR(CM_MOD_SYNC,"obj[%u] not support",pReq->obj_id);
        return CM_ERR_NOT_SUPPORT;
    }

    if(CM_SYNC_REQ_UPDATE == pReq->is_delete)
    {
        if(NULL != cfg->cbk_request)
        {
            iRet = cfg->cbk_request(pReq->data_id,pReq->data,pReq->data_len);
        }        
    }    
    else
    {
        if(NULL != cfg->cbk_delete)
        {
            iRet = cfg->cbk_delete(pReq->data_id);
        }        
    }
    
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_SYNC,"req:obj[%u] data_id[%llu] fail[%d]",
            pReq->obj_id,pReq->data_id,iRet);
        return iRet;
    }
    
    if(CM_SYNC_FLAG_REAL_TIME_ONLY & cfg->mask)
    {
        return CM_OK;
    }
    
    CM_LOG_INFO(CM_MOD_SYNC,"obj[%u] data_id[%llu] step[%llu]",
        pReq->obj_id,pReq->data_id,pReq->step);
    (void)cm_db_exec_get_count(pglobal->db_handle,&count,
            "SELECT COUNT(step) FROM record_t WHERE obj_id=%u "
            "AND data_id=%llu",pReq->obj_id,pReq->data_id);
    if(0 == count)
    {
        (void)cm_db_exec_ext(pglobal->db_handle,
            "INSERT INTO record_t (obj_id,data_id,step,is_delete) "
            "VALUES (%u,%llu,%llu,%u)",
            pReq->obj_id,pReq->data_id,pReq->step,pReq->is_delete);
    }
    else
    {
        (void)cm_db_exec_ext(pglobal->db_handle,
            "UPDATE record_t SET step=%llu,is_delete=%u WHERE obj_id=%u AND data_id=%llu",
            pReq->step,pReq->is_delete,pReq->obj_id,pReq->data_id);
    }
    pglobal->steps[pReq->obj_id] = pReq->step;
    return CM_OK;
}


static sint32 cm_sync_request_each_node(cm_node_info_t *pNode, void *pArg)
{
    sint32 iRet = CM_OK;
    cm_db_query_t *query = (cm_db_query_t*)pArg;
    cm_sync_obj_info_t *pReq = (cm_sync_obj_info_t*)query->data;
    uint32 Len = pReq->data_len + sizeof(cm_sync_obj_info_t);

    query->max++;
    if(pNode->id == cm_node_get_id())
    {
        iRet = cm_sync_request_local(&g_cm_sync_global,pReq);
    }
    else
    {
        iRet = cm_cmt_request(pNode->id,CM_CMT_MSG_SYNC_REQUEST,
            pReq,Len,NULL,NULL,CM_SYNC_SEND_TMOUT);
    }
    
    if(CM_OK == iRet)
    {
        query->index++;
    }
    else
    {
        CM_LOG_INFO(CM_MOD_SYNC,"nid[%u] obj[%u] data[%llu] fail[%d]",
            pNode->id,pReq->obj_id,pReq->data_id, iRet);
    }
    
    return iRet;    
}

static sint32 cm_sync_request_each_subdomain(cm_subdomain_info_t *pinfo, void *pArg)
{
    sint32 iRet = CM_OK;
    cm_db_query_t *query = (cm_db_query_t*)pArg;
    cm_sync_obj_info_t *pReq = (cm_sync_obj_info_t*)query->data;
    uint32 Len = pReq->data_len + sizeof(cm_sync_obj_info_t);

    if(pinfo->id == cm_node_get_subdomain_id())
    {
        return CM_OK;
    }
    query->max++;
    iRet = cm_cmt_request(pinfo->submaster_id,CM_CMT_MSG_SYNC_REQUEST,
            pReq,Len,NULL,NULL,CM_SYNC_SEND_TMOUT);
    if(CM_OK == iRet)
    {
        query->index++;
    }
    else
    {
        CM_LOG_ERR(CM_MOD_SYNC,"sub[%u] nid[%u] obj[%u] data[%llu] fail[%d]",
            pinfo->id,pinfo->submaster_id,pReq->obj_id,pReq->data_id, iRet);
    }
    
    return iRet;    
}

sint32 cm_sync_cbk_cmt_request(void *pMsg, uint32 Len, void **ppAck, uint32 *pAckLen)
{
    cm_sync_obj_info_t *pReq = (cm_sync_obj_info_t*)pMsg;
    cm_sync_global_info_t *pglobal = &g_cm_sync_global;
    uint32 subdomain = cm_node_get_subdomain_id();
    sint32 iRet = CM_OK;
    if(cm_node_get_id() != cm_node_get_submaster_current())
    {
        CM_MUTEX_LOCK(&pglobal->mutex);
        iRet = cm_sync_request_local(pglobal, pReq);
        CM_MUTEX_UNLOCK(&pglobal->mutex);
        return iRet;
    }

    if(CM_MASTER_SUBDOMAIN_ID == subdomain)
    {
        return CM_OK;
    }
    pReq = CM_MALLOC(Len);
    if(NULL == pReq)
    {
        CM_LOG_ERR(CM_MOD_SYNC,"malloc fail");
        return CM_FAIL;
    }
    CM_MEM_CPY(pReq,Len,pMsg,Len);
    if(CM_OK != cm_queue_add(pglobal->queue,pReq))
    {
        CM_FREE(pReq);
        return CM_FAIL;
    }
    return CM_OK;
}

static void* cm_sync_thread_requst(void *arg)
{
    cm_sync_global_info_t *pglobal = arg;
    cm_sync_obj_info_t *pReq = NULL;
    uint32 subdomain = cm_node_get_subdomain_id();
    sint32 iRet = CM_FAIL;
    cm_db_query_t query = {NULL,0,0};
    
    while(1)
    {
        iRet = cm_queue_get(pglobal->queue,(void**)&pReq);
        if(CM_OK != iRet)
        {
            continue;
        }
        query.index = 0;
        query.max = 0;
        query.data = pReq;
        CM_LOG_INFO(CM_MOD_SYNC,"obj[%u] data_id[%llu] step[%llu] len[%u]",
            pReq->obj_id,pReq->data_id,pReq->step,pReq->data_len);
        if(CM_MASTER_SUBDOMAIN_ID == subdomain)
        {
            (void)cm_node_traversal_subdomain((cm_node_trav_func_t)cm_sync_request_each_subdomain,
                (void*)&query, CM_FALSE);
        }
        else
        {
            (void)cm_node_traversal_node(cm_node_get_subdomain_id(),
                    (cm_node_trav_func_t)cm_sync_request_each_node, (void*)&query, CM_FALSE);
        }
        CM_FREE(pReq);
    }
    return NULL;
}

static sint32 cm_sync_request_master(uint32 obj_id, uint64 data_id,void *pdata, uint32 len)
{
    const cm_sync_config_t* cfg = cm_sync_get_config(obj_id);
    cm_sync_global_info_t *pglobal = &g_cm_sync_global;
    cm_sync_obj_info_t *pReq = NULL;
    uint32 ReqLen = 0;
    cm_db_query_t query = {NULL,0,0};
    if(NULL == cfg)
    {
        CM_LOG_ERR(CM_MOD_SYNC,"obj_id[%u] not support",obj_id);
        return CM_ERR_NOT_SUPPORT;
    }
    
    cm_htbt_get_local_node(&query.max,&query.index);
    if((query.index <= (query.max >> 1)) && !(cfg->mask & CM_SYNC_FLAG_REAL_TIME_ONLY)
        && !(cfg->mask & CM_SYNC_FLAG_ALWAYLS))
    {
        CM_LOG_ERR(CM_MOD_SYNC,"tatal[%u] online[%u]",query.max,query.index);
        return CM_FAIL;
    }
    ReqLen = sizeof(cm_sync_obj_info_t) + len;
    pReq = CM_MALLOC(ReqLen);
    if(NULL == pReq)
    {
        CM_LOG_ERR(CM_MOD_SYNC,"malloc fail");
        return CM_FAIL;
    }
    CM_MUTEX_LOCK(&pglobal->mutex);
    pReq->obj_id = obj_id;
    pReq->step = pglobal->steps[obj_id] + 1;
    
    if((NULL == pdata) || (0 == len))
    {
        pReq->is_delete = CM_SYNC_REQ_DELETE;
    }
    else
    {
        pReq->is_delete = CM_SYNC_REQ_UPDATE;
        CM_MEM_CPY(pReq->data,len,pdata,len);
    }
    
    pReq->data_id = data_id;
    pReq->data_len = len;
    query.data = pReq;
    query.max = 0;
    query.index = 0;
    
    (void)cm_node_traversal_node(CM_MASTER_SUBDOMAIN_ID,
       (cm_node_trav_func_t)cm_sync_request_each_node, &query, CM_FALSE);
    CM_MUTEX_UNLOCK(&pglobal->mutex);
	
    if((query.index <= (query.max >> 1)) && !(cfg->mask & CM_SYNC_FLAG_REAL_TIME_ONLY)
        && !(cfg->mask & CM_SYNC_FLAG_ALWAYLS))
    {
        CM_FREE(pReq);
        CM_LOG_ERR(CM_MOD_SYNC,"reqnum[%u] oknum[%u]",query.max,query.index);
        return CM_FAIL;
    }
    if(cfg->mask & CM_SYNC_FLAG_MASTER_DOMAIN)
    {
        CM_FREE(pReq);
        return CM_OK;
    }
    if(CM_OK != cm_queue_add(pglobal->queue,pReq))
    {
        CM_FREE(pReq);
    }
    return CM_OK;
}

sint32 cm_sync_delete(uint32 obj_id, uint64 data_id)
{
    return cm_sync_request(obj_id,data_id,NULL,0);
}

void cm_sync_master_change(uint32 old_id, uint32 new_id)
{
    cm_thread_t handle;
    cm_sync_global_info_t *pglobal = &g_cm_sync_global;
    
    if((CM_NODE_ID_NONE == new_id)
        || (new_id == cm_node_get_id()))
    {
        return;
    }
    if(CM_TRUE == pglobal->is_run)
    {
        return;
    }
    if(CM_OK != CM_THREAD_CREATE(&handle,cm_sync_thread_once,pglobal))
    {
        CM_LOG_ERR(CM_MOD_SYNC,"create thread fail");
        return;
    }
    CM_THREAD_DETACH(handle);
    return;
}

sint32 cm_sync_init(void)
{
    sint32 iRet = CM_FAIL;
    cm_sync_global_info_t *pglobal = &g_cm_sync_global;
    const cm_sync_config_t* cfg = g_cm_sync_config;
    const sint8* create_tables[]=
    {
        "CREATE TABLE IF NOT EXISTS record_t "
            "(obj_id INT, data_id BIGINT, step BIGINT, is_delete TINYINT)",
        "CREATE TABLE IF NOT EXISTS cache_t "
            "(obj_id INT, step BIGINT)",
    };
    uint32 iloop = 0;
    uint32 table_cnt = sizeof(create_tables)/(sizeof(sint8*));
    cm_thread_t handle;

    CM_MUTEX_INIT(&pglobal->mutex);
    
    iRet = cm_queue_init(&pglobal->queue,CM_SYNC_RECV_QUEUE_LEN);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_SYNC,"init recv queue fail[%d]",iRet);
        return iRet;
    }
    iRet = cm_db_open_ext(CM_SYNC_DB_DIR
        CM_SYNC_DB_NAME, &pglobal->db_handle);

    if(CM_OK != iRet)
    {
        return CM_FAIL;
    }

    while(iloop < table_cnt)
    {
        iRet = cm_db_exec_ext(pglobal->db_handle,"%s",create_tables[iloop]);
        if(CM_OK != iRet)
        {
            return iRet;
        }
        iloop++;
    } 
    
    iloop = CM_SYNC_OBJ_BUTT;
    
    while(iloop > 0)
    {
        iloop--;
        pglobal->steps[iloop] = 0;
        if(cfg[iloop].mask & CM_SYNC_FLAG_REAL_TIME_ONLY)
        {
            continue;
        }
        (void)cm_db_exec_get_count(pglobal->db_handle,&pglobal->steps[iloop],
            "SELECT MAX(step) FROM record_t WHERE obj_id=%u",iloop);
    }

    iRet = CM_THREAD_CREATE(&handle,cm_sync_thread_requst,pglobal);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_SYNC,"create request thread fail[%d]",iRet);
        return CM_FAIL;
    }
    CM_THREAD_DETACH(handle);
    return CM_OK;
}

typedef struct
{
    uint64 data_id;
    uint32 obj_id;
    uint32 len;
    sint8 data[];
}cm_sync_msg_t;

sint32 cm_sync_request(uint32 obj_id, uint64 data_id,void *pdata, uint32 len)
{
    uint32 myid = cm_node_get_id();
    uint32 master = cm_node_get_master();
    cm_sync_msg_t *pMsg = NULL;
    sint32 iRet = CM_OK;

    if(CM_NODE_ID_NONE == master)
    {
        CM_LOG_ERR(CM_MOD_SYNC,"no master");
        return CM_ERR_NO_MASTER;
    }
    
    if(myid == master)
    {
        return cm_sync_request_master(obj_id,data_id,pdata,len);
    }

    pMsg = CM_MALLOC(sizeof(cm_sync_msg_t) + len);
    if(NULL == pMsg)
    {
        CM_LOG_ERR(CM_MOD_SYNC,"malloc fail");
        return CM_FAIL;
    }
    pMsg->data_id = data_id;
    pMsg->obj_id = obj_id;
    pMsg->len = len;
    
    if(len > 0)
    {
        CM_MEM_CPY(pMsg->data,len,pdata,len);
    }
    
    iRet = cm_cmt_request(master,CM_CMT_MSG_SYNC_TOMASTER,pMsg,
        (sizeof(cm_sync_msg_t) + len),NULL,NULL,CM_CMT_REQ_TMOUT);
    CM_FREE(pMsg);
    return iRet;
}

sint32 cm_sync_cbk_cmt_tomaster(void *pMsg, uint32 Len, void **ppAck, uint32 *pAckLen)
{
    cm_sync_msg_t *pMsgX = pMsg;

    if(pMsgX == NULL)
    {
        CM_LOG_ERR(CM_MOD_SYNC,"pMsg NULL");
        return CM_FAIL;
    }
    if(Len < sizeof(cm_sync_msg_t))
    {
        CM_LOG_ERR(CM_MOD_SYNC,"len:%u",Len);
        return CM_FAIL;
    }
    return cm_sync_request_master(pMsgX->obj_id,pMsgX->data_id,pMsgX->data,pMsgX->len);
}

