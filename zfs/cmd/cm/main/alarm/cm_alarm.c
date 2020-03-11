/******************************************************************************
 *          Copyright (c) 2018 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_alarm.c
 * author     : wbn
 * create date: 2018年1月8日
 * description: TODO:
 *
 *****************************************************************************/
#include "cm_alarm.h"
#include "cm_log.h"
#include "cm_db.h"
#include "cm_queue.h"
#include "cm_omi.h"
#include "cm_sync.h"
#include "cm_node.h"
#include "cm_cmt.h"

#define CM_ALARM_DB_DIR CM_DATA_DIR
#define CM_ALARM_DB_CFG CM_CONFIG_DIR"comm/cm_alarm_cfg.db"
#define CM_ALARM_DB_RECORD "cm_alarm.db"

typedef struct
{
    cm_db_handle_t db_handle;
    cm_mutex_t mutex;
    cm_sem_t sem;
    uint64 global_id;
    uint64 cache_id;
}cm_alarm_global_t;

typedef struct
{
    const cm_alarm_config_t *pcfg;
    const cm_alarm_record_t *pnew;
    cm_alarm_record_t *pbuff;
    uint32 max;
    uint32 cnt;
}cm_alarm_match_t;

static cm_alarm_global_t g_cm_alarm_global;
static bool_t g_cm_alarm_is_init = CM_FALSE;

#define CM_ALARM_INFO_PRINTF(p) \
    CM_LOG_INFO(CM_MOD_ALARM,"i[%llu] a[%u] p[%s] t1[%u] t2[%u] ", \
                    (p)->global_id,(p)->alarm_id,(p)->param, \
                    (p)->report_time,(p)->recovery_time)

/*
CREATE TABLE config_t (
    alarm_id INT,
    match_bits INT,
    param_num TINYINT,
    type TINYINT, 
    lvl TINYINT,
    is_disable TINYINT);
CREATE TABLE record_t (
    id BIGINT,
    alarm_id INT,
    report_time INT,
    recovery_time INT,
    param VARCHAR(255));
CREATE TABLE cache_t (
    id BIGINT,
    alarm_id INT,
    report_time INT,
    recovery_time INT,
    param VARCHAR(255));
*/

static sint32 cm_alarm_get_config_each(
    void *arg, sint32 col_cnt, sint8 **col_vals, sint8 **col_names)
{
    cm_alarm_config_t *cfg = arg;

    if(col_cnt != CM_ALRAM_CFG_TABLE_COLS_NUM)
    {
        CM_LOG_ERR(CM_MOD_ALARM,"col_cnt[%d]",col_cnt);
        return CM_FAIL;
    }
    cfg->alarm_id = (uint32)atoi(col_vals[0]);
    cfg->match_bits = (uint32)atoi(col_vals[1]);
    cfg->param_num= (uint8)atoi(col_vals[2]);
    cfg->type = (uint8)atoi(col_vals[3]);
    cfg->lvl= (uint8)atoi(col_vals[4]);
    cfg->is_disable= (uint8)atoi(col_vals[5]);
    return CM_OK;
}

static sint32 cm_alarm_get_config(cm_alarm_global_t *pinfo,
    uint32 alarm_id, cm_alarm_config_t *cfg)
{
    uint32 cnt = cm_db_exec_get(pinfo->db_handle,cm_alarm_get_config_each,cfg,1,
        sizeof(cm_alarm_config_t),"SELECT * FROM config_t WHERE alarm_id=%u",alarm_id);

    return (1 == cnt)? CM_OK : CM_FAIL;
}

sint32 cm_alarm_get_threshold_each(
    void *arg, sint32 col_cnt, sint8 **col_vals, sint8 **col_names)
{
    cm_alarm_threshold_t *cfg = arg;

    if(col_cnt != 3)
    {
        CM_LOG_ERR(CM_MOD_ALARM,"col_cnt[%d]",col_cnt);
        return CM_FAIL;
    }
    cfg->alarm_id = (uint32)atoi(col_vals[0]);
    cfg->report = (uint32)atoi(col_vals[1]);
    cfg->recovery = (uint8)atoi(col_vals[2]);
    return CM_OK;
}

static sint32 cm_alarm_get_threshold(cm_alarm_global_t *pinfo,
    uint32 alarm_id, cm_alarm_threshold_t *cfg)
{
    uint32 cnt = cm_db_exec_get(pinfo->db_handle,cm_alarm_get_threshold_each,cfg,1,
        sizeof(cm_alarm_threshold_t),"SELECT * FROM threshold_t WHERE alarm_id=%u",alarm_id);

    return (1 == cnt)? CM_OK : CM_FAIL;
}


static sint32 cm_alarm_get_record_each(
    void *arg, sint32 col_cnt, sint8 **col_vals, sint8 **col_names)
{
    cm_alarm_record_t *data= arg;

    if(col_cnt != CM_ALRAM_RECORD_TABLE_COLS_NUM)
    {
        CM_LOG_ERR(CM_MOD_ALARM,"col_cnt[%d]",col_cnt);
        return CM_FAIL;
    }
    data->global_id= (uint64)atoll(col_vals[0]);
    data->alarm_id= (uint64)atoll(col_vals[1]);
    data->report_time= (uint32)atoi(col_vals[2]);
    data->recovery_time= (uint32)atoi(col_vals[3]);
    CM_MEM_CPY(data->param,sizeof(data->param),col_vals[4],strlen(col_vals[4])+1);
    return CM_OK;
}

static sint32 cm_alarm_cache_get(cm_alarm_global_t *pinfo,
    cm_alarm_record_t *data)
{
    uint32 cnt = cm_db_exec_get(pinfo->db_handle,cm_alarm_get_record_each,data,1,
        sizeof(cm_alarm_record_t),"SELECT * FROM cache_t LIMIT 1");

    return (1 == cnt)? CM_OK : CM_FAIL;
}

static sint32 cm_alarm_cache_add(cm_alarm_global_t *pinfo,
    cm_alarm_record_t *data)
{
    uint64 id = 0;
    sint32 iRet = CM_OK;

    CM_MUTEX_LOCK(&pinfo->mutex);
    id = ++(pinfo->cache_id);
    CM_MUTEX_UNLOCK(&pinfo->mutex);
    iRet = cm_db_exec_ext(pinfo->db_handle,
                "INSERT INTO cache_t (id,alarm_id,report_time,recovery_time,param)"
                " VALUES (%llu,%u,%u,%u,'%s')",
                id,data->alarm_id,data->report_time,
                data->recovery_time,data->param);
    if(CM_OK == iRet)
    {
        CM_SEM_POST(&pinfo->sem);
    }
    return iRet;
}

static sint32 cm_alarm_cache_delete(cm_alarm_global_t *pinfo,uint64 id)
{
    return cm_db_exec_ext(pinfo->db_handle,
                "DELETE FROM cache_t WHERE id=%llu",id);
}

static sint32 cm_alarm_record_add(cm_alarm_global_t *pinfo, 
    cm_alarm_record_t *record)
{
    pinfo->global_id++;
    record->global_id = pinfo->global_id;

    return cm_sync_request(CM_SYNC_OBJ_ALARM_RECORD,
        record->global_id,record,sizeof(cm_alarm_record_t));
}

static sint32 cm_alarm_record_match_check(
    const cm_alarm_config_t *cfg, const sint8 *param_new, const sint8 *param_old)
{
    uint8 iloop = 0;
    sint8 *pnew[CM_STRING_256] = {NULL};
    sint8 *pold[CM_STRING_256] = {NULL};
    sint8 *pchr_new = NULL;
    sint8 *pchr_old = NULL;
    
    if(0 == strcmp(param_new,param_old))
    {
        return CM_OK;
    }
    
    (void)cm_omi_get_fields(param_new,pnew,CM_STRING_256);
    (void)cm_omi_get_fields(param_old,pold,CM_STRING_256);
    
    for(iloop = 0;iloop<cfg->param_num;iloop++)
    {
        if(!((1<<iloop) & cfg->match_bits))
        {
            continue;
        }
        if((NULL == pnew[iloop]) && (NULL == pold[iloop]))
        {
            continue;
        }
        if(!((NULL != pnew[iloop]) && (NULL != pold[iloop])))
        {
            return CM_FAIL;
        }
        for(pchr_new = pnew[iloop],pchr_old = pold[iloop];
            (*pchr_new != '\0') && (*pchr_new != ',')
            && (*pchr_old != '\0') && (*pchr_old != ',');
            pchr_new++,pchr_old++)
        {
            if(*pchr_new != *pchr_old)
            {
                return CM_FAIL;
            }
        }
        if(*pchr_new != *pchr_old)
        {
            return CM_FAIL;
        }
    }
    return CM_OK;
}

static sint32 cm_alarm_record_match_each(
    void *arg, sint32 col_cnt, sint8 **col_vals, sint8 **col_names)
{
    cm_alarm_match_t *match= arg;
    cm_alarm_record_t *pold = match->pbuff;

    if(col_cnt != CM_ALRAM_RECORD_TABLE_COLS_NUM)
    {
        CM_LOG_ERR(CM_MOD_ALARM,"col_cnt[%d]",col_cnt);
        return CM_FAIL;
    }
    if(match->cnt >= match->max)
    {
        return CM_OK;
    }
    if(CM_OK != cm_alarm_record_match_check(match->pcfg,match->pnew->param,col_vals[4]))
    {
        return CM_OK;
    }
    pold += match->cnt;
    pold->global_id= (uint64)atoll(col_vals[0]);
    pold->alarm_id= (uint64)atoll(col_vals[1]);
    pold->report_time= (uint32)atoi(col_vals[2]);
    pold->recovery_time= (uint32)atoi(col_vals[3]);
    CM_MEM_CPY(pold->param,sizeof(pold->param),col_vals[4],strlen(col_vals[4])+1);
    match->cnt++;
    return CM_OK;
}

static sint32 cm_alarm_record_match(cm_alarm_global_t *pinfo, 
    const cm_alarm_config_t *cfg,
    const cm_alarm_record_t *pnew, cm_alarm_record_t *pold)
{
    cm_alarm_match_t match = {cfg,pnew,pold,1,0};
    (void)cm_db_exec(pinfo->db_handle,cm_alarm_record_match_each,&match,
        "SELECT * FROM record_t WHERE alarm_id=%u AND recovery_time=0",pnew->alarm_id);
    return (match.cnt == 1) ? CM_OK : CM_FAIL;
}

static sint32 cm_alarm_process_local(cm_alarm_global_t *pinfo, 
    cm_alarm_record_t *record)
{
    cm_alarm_config_t cfg;
    sint32 iRet = CM_FAIL;
    cm_alarm_record_t old;
    
    CM_ALARM_INFO_PRINTF(record);

    iRet = cm_alarm_get_config(pinfo,record->alarm_id,&cfg);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_ALARM,"alarm_id[%u] not support",record->alarm_id);
        return CM_OK;
    }

    if(CM_ALATM_TYPE_FAULT != cfg.type)
    {
        return cm_alarm_record_add(pinfo,record);
    }

    iRet = cm_alarm_record_match(pinfo,&cfg,record,&old);
    
    if(0 == record->recovery_time)
    {
        /* report */
        if(CM_OK == iRet)
        {
            return CM_OK;
        }
        else
        {
            CM_LOG_INFO(CM_MOD_ALARM,"alarm_id[%u] new!",record->alarm_id);
            return cm_alarm_record_add(pinfo,record);
        }
    }
    
    /* recovery */
    if(CM_OK != iRet)
    {
        return CM_OK;
    }
    
    CM_LOG_INFO(CM_MOD_ALARM,"global_id[%llu] recovery",old.global_id);
    old.recovery_time = record->recovery_time;
    
    CM_ALARM_INFO_PRINTF(&old);
    return cm_sync_request(CM_SYNC_OBJ_ALARM_RECORD,
        old.global_id,&old,sizeof(cm_alarm_record_t));
}

static void* cm_alarm_thread(void *arg)
{
    cm_alarm_global_t *pinfo = arg;
    uint32 master= CM_NODE_ID_NONE;
    uint32 myid = cm_node_get_id();
    sint32 iRet = CM_OK;
    uint64 cache_id = 0;
    cm_alarm_record_t cache;
    
    while(1)
    {
        CM_SEM_WAIT_TIMEOUT(&pinfo->sem,5,iRet); 
        master = cm_node_get_master();
        if(CM_NODE_ID_NONE == master)
        {
            continue;
        }
        
        do
        {
            iRet = cm_alarm_cache_get(pinfo,&cache);
            if(CM_OK != iRet)
            {
                break;
            }
            cache_id = cache.global_id;
            if(myid == master)
            {
                iRet = cm_alarm_process_local(pinfo, &cache);
            }
            else
            {
                iRet = cm_cmt_request_master(CM_CMT_MSG_ALARM,&cache,
                    sizeof(cache),NULL,NULL,10);
            }
            
            if(CM_OK != iRet)
            {
                CM_LOG_ERR(CM_MOD_ALARM,"id[%llu] aid[%u] param[%s] fail[%d]",
                    cache_id,cache.alarm_id,cache.param,iRet);
                break;
            }
            
            (void)cm_alarm_cache_delete(pinfo,cache_id);
        }while(1);
    }
    return NULL;
}

static sint32 cm_alarm_init_config_each(
    void *arg, sint32 col_cnt, sint8 **col_vals, sint8 **col_names)
{
    cm_db_handle_t handle = arg;

    if(col_cnt != CM_ALRAM_CFG_TABLE_COLS_NUM)
    {
        CM_LOG_ERR(CM_MOD_ALARM,"col_cnt[%d]",col_cnt);
        return CM_FAIL;
    }
    return cm_db_exec_ext(handle,
                "INSERT INTO config_t (alarm_id,match_bits,param_num,type,lvl,is_disable)"
                " VALUES (%s,%s,%s,%s,%s,%s)",
                col_vals[0],col_vals[1],col_vals[2],
                col_vals[3],col_vals[4],col_vals[5]);
}

static sint32 cm_alarm_init_threshold_each(
    void *arg, sint32 col_cnt, sint8 **col_vals, sint8 **col_names)
{
    cm_db_handle_t handle = arg;

    if(col_cnt != 3)
    {
        CM_LOG_ERR(CM_MOD_CNM,"col_cnt[%d]",col_cnt);
        return CM_FAIL;
    }
    return cm_db_exec_ext(handle,
                "INSERT INTO threshold_t (alarm_id,threshold,recoverval)"
                " VALUES (%s,%s,%s)",
                col_vals[0],col_vals[1],col_vals[2]);
}

static sint32 cm_alarm_init_db(cm_db_handle_t handle)
{
    const sint8* create_tables[]=
    {
        "CREATE TABLE IF NOT EXISTS config_t ("
            "alarm_id INT,"
            "match_bits INT,"
            "param_num TINYINT,"
            "type TINYINT, "
            "lvl TINYINT,"
            "is_disable TINYINT)",
        "CREATE TABLE IF NOT EXISTS record_t ("
            "id BIGINT,"
            "alarm_id INT,"
            "report_time INT,"
            "recovery_time INT,"
            "param VARCHAR(255))",
        "CREATE TABLE IF NOT EXISTS cache_t ("
            "id BIGINT,"
            "alarm_id INT,"
            "report_time INT,"
            "recovery_time INT,"
            "param VARCHAR(255))",
       "CREATE TABLE IF NOT EXISTS threshold_t ("
            "alarm_id INT,"
            "threshold INT," 
            "recoverval INT)"
    };
    uint32 iloop = 0;
    uint32 table_cnt = sizeof(create_tables)/(sizeof(sint8*));
    sint32 iRet = CM_OK;
    uint64 count = 0;
    cm_db_handle_t config= NULL;
    
    while(iloop < table_cnt)
    {
        iRet = cm_db_exec_ext(handle,"%s",create_tables[iloop]);
        if(CM_OK != iRet)
        {
            return iRet;
        }
        iloop++;
    } 

    (void)cm_db_exec_get_count(handle,&count,
            "SELECT COUNT(alarm_id) FROM threshold_t");
    if(count == 0)
    {
        iRet = cm_db_open(CM_ALARM_DB_CFG,&config);
        if(CM_OK != iRet)
        {
            CM_LOG_ERR(CM_MOD_ALARM,"open %s fail",CM_ALARM_DB_CFG);
            return iRet;
        }
        (void)cm_db_exec(config,cm_alarm_init_threshold_each,handle,
        "SELECT * FROM threshold_t");
    }
    (void)cm_db_exec_get_count(handle,&count,
            "SELECT COUNT(alarm_id) FROM config_t");
    if(count > 0)
    {
        if(NULL != config)
        {
            cm_db_close(config);
        }        
        return CM_OK;
    }
    if(NULL == config)
    {
       iRet = cm_db_open(CM_ALARM_DB_CFG,&config);
        if(CM_OK != iRet)
        {
            CM_LOG_ERR(CM_MOD_ALARM,"open %s fail",CM_ALARM_DB_CFG);
            return iRet;
        }
    }
    
    iRet = cm_db_exec(config,cm_alarm_init_config_each,handle,
        "SELECT * FROM config_t");
    cm_db_close(config);
    return iRet;
}
sint32 cm_alarm_init(void)
{
    sint32 iRet = CM_OK;
    cm_thread_t handle;
    cm_alarm_global_t *pinfo = &g_cm_alarm_global;
    
    CM_MEM_ZERO(pinfo,sizeof(cm_alarm_global_t));
    CM_MUTEX_INIT(&pinfo->mutex);
    CM_SEM_INIT(&pinfo->sem,0);
    iRet = cm_db_open_ext(CM_ALARM_DB_DIR
        CM_ALARM_DB_RECORD,&pinfo->db_handle);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_ALARM,"open %s fail",CM_ALARM_DB_RECORD);
        return iRet;
    }
    iRet = cm_alarm_init_db(pinfo->db_handle);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_ALARM,"init db fail");
        return iRet;
    }
    (void)cm_db_exec_get_count(pinfo->db_handle,&pinfo->global_id,
            "SELECT MAX(id) FROM record_t");
    (void)cm_db_exec_get_count(pinfo->db_handle,&pinfo->cache_id,
            "SELECT MAX(id) FROM cache_t");
    
    iRet = CM_THREAD_CREATE(&handle,cm_alarm_thread,pinfo);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_ALARM,"create thread fail");
        return CM_FAIL;
    }
    CM_THREAD_DETACH(handle);
    g_cm_alarm_is_init = CM_TRUE;
    return CM_OK;
}

sint32 cm_alarm_report(uint32 alarm_id,const sint8 *param, bool_t is_recovery)
{
    cm_alarm_global_t *pinfo = &g_cm_alarm_global;
    cm_alarm_record_t data;
    cm_alarm_config_t cfg;
    sint32 iRet = CM_FAIL;
    sint8 *pfield[CM_STRING_256] = {NULL};
    uint32 param_num = 0;
    
    if(CM_FALSE == g_cm_alarm_is_init)
    {
        CM_LOG_ERR(CM_MOD_ALARM,"not init");
        return CM_FAIL;
    }
    
    iRet = cm_alarm_get_config(pinfo,alarm_id,&cfg);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_ALARM,"alarm_id[%u] not support",alarm_id);
        return CM_ERR_NOT_SUPPORT;
    }

    if(CM_FALSE == is_recovery)
    {
        if(CM_TRUE == cfg.is_disable)
        {
            return CM_FAIL;
        }
    }
    else
    {
        param_num = cm_omi_get_fields(param,pfield,CM_STRING_256);
        if(param_num != cfg.param_num)
        {
            CM_LOG_ERR(CM_MOD_ALARM,"alarm_id[%u] type[%u] pnumc[%u] curr[%u]",
                alarm_id,cfg.type,cfg.param_num,param_num);
            return CM_PARAM_ERR;
        }
        if(CM_ALATM_TYPE_FAULT != cfg.type)
        {
            CM_LOG_ERR(CM_MOD_ALARM,"alarm_id[%u] type[%u]",alarm_id,cfg.type);
            return CM_FAIL;
        }
    }
    
    CM_MEM_ZERO(&data,sizeof(data));
    data.alarm_id = alarm_id;
    if(CM_FALSE == is_recovery)
    {
        data.report_time = cm_get_time();
        data.recovery_time = 0;
    }
    else
    {
        data.report_time = 0;
        data.recovery_time = cm_get_time();
    }
    CM_MEM_CPY(data.param,sizeof(data.param),param,strlen(param));
    return cm_alarm_cache_add(pinfo,&data);
}

sint32 cm_alarm_cbk_cmt(
 void *pMsg, uint32 Len, void** ppAck, uint32 *pAckLen)
{
    uint32 myid = cm_node_get_id();
    uint32 master = cm_node_get_master();
    uint32 submaster = cm_node_get_submaster_current();
    cm_alarm_record_t *data = pMsg;
    cm_alarm_global_t *pinfo = &g_cm_alarm_global;
    
    if(CM_FALSE == g_cm_alarm_is_init)
    {
        CM_LOG_ERR(CM_MOD_ALARM,"not init");
        return CM_FAIL;
    }
    
    if(CM_NODE_ID_NONE == master)
    {
        CM_LOG_ERR(CM_MOD_ALARM,"master none");
        return CM_FAIL;
    }
    
    if(Len != sizeof(cm_alarm_record_t))
    {
        CM_LOG_ERR(CM_MOD_ALARM,"len[%u]",Len);
        return CM_FAIL;
    }
    CM_ALARM_INFO_PRINTF(data);
    if(myid == master)
    {
        return cm_alarm_cache_add(pinfo,data);
    }

    if(myid == submaster)
    {
        return cm_cmt_request(master,CM_CMT_MSG_ALARM,pMsg,Len,ppAck,pAckLen,10);
    }
    CM_LOG_ERR(CM_MOD_ALARM,"myid[%u] submaster[%u]",myid,submaster);
    return CM_FAIL;
}

static void* cm_alarm_mail_thread(void *arg)
{
    sint32 iRet = CM_OK;
    cm_alarm_record_t *info = (cm_alarm_record_t *)arg;
    
    iRet = cm_system(CM_SCRIPT_DIR"cm_mail.py %u '%s' %lu",info->alarm_id,info->param,info->recovery_time);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_ALARM,"alarm_id[%u],param[%s] %d",
            info->alarm_id,info->param,iRet);
    }
    CM_FREE(info);
    return NULL;
}

static void cm_alarm_mail_report(const cm_alarm_record_t *info)
{
    cm_alarm_record_t *data=NULL;
    sint32 iRet = CM_OK;
    
    if(cm_node_get_id() != cm_node_get_master())
    {
        return;
    }
    
    /* only master report */
    data = (cm_alarm_record_t*)CM_MALLOC(sizeof(cm_alarm_record_t));
    if(NULL == data)
    {
        CM_LOG_ERR(CM_MOD_ALARM,"malloc fail");
        return;
    }
    CM_MEM_CPY(data,sizeof(cm_alarm_record_t),info,sizeof(cm_alarm_record_t));

    iRet = cm_thread_start(cm_alarm_mail_thread,data);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_ALARM,"start fail");
        CM_FREE(data);
    }
    return;
}

sint32 cm_alarm_cbk_sync_request(uint64 data_id, void *pdata, uint32 len)
{
    cm_alarm_record_t *data = pdata;
    uint64 cnt = 0;
    sint32 iRet= CM_OK;
    cm_alarm_global_t *pinfo = &g_cm_alarm_global;
    
    if(len != sizeof(cm_alarm_record_t))
    {
        CM_LOG_ERR(CM_MOD_ALARM,"len[%u]",len);
        return CM_FAIL;
    }
    
    CM_ALARM_INFO_PRINTF(data);

    if(data->global_id > pinfo->global_id)
    {
        pinfo->global_id = data->global_id;
    }
    
    (void)cm_db_exec_get_count(pinfo->db_handle,&cnt,
            "SELECT COUNT(id) FROM record_t WHERE id=%llu",data_id);
    if(0 == cnt)
    {
        iRet = cm_db_exec_ext(pinfo->db_handle,
                "INSERT INTO record_t (id,alarm_id,report_time,recovery_time,param)"
                " VALUES (%llu,%u,%u,%u,'%s')",
                data->global_id,data->alarm_id,data->report_time,
                data->recovery_time,data->param);
    }
    
    iRet = cm_db_exec_ext(pinfo->db_handle,
            "UPDATE record_t SET recovery_time=%u WHERE id=%llu",
            data->recovery_time,data->global_id);
    cm_alarm_mail_report(data);
    return iRet;
}

sint32 cm_alarm_cbk_sync_get(uint64 data_id, void **pdata, uint32 *plen)
{
    cm_alarm_global_t *pinfo = &g_cm_alarm_global;
    cm_alarm_record_t *data = CM_MALLOC(sizeof(cm_alarm_record_t));
    uint32 cnt = 0;
    
    if(NULL == data)
    {
        CM_LOG_ERR(CM_MOD_ALARM,"malloc fail");
        return CM_FAIL;
    }
    cnt = cm_db_exec_get(pinfo->db_handle,cm_alarm_get_record_each,data,1,
        sizeof(cm_alarm_record_t),
        "SELECT * FROM record_t WHERE id=%llu LIMIT 1",data_id);

    if(0 == cnt)
    {
        CM_FREE(data);
        *pdata = NULL;
        *plen = 0;
        CM_LOG_ERR(CM_MOD_ALARM,"id[%llu] none",data_id);
        return CM_FAIL;
    }
    *pdata = data;
    *plen = sizeof(cm_alarm_record_t);
    return CM_OK;
}

sint32 cm_alarm_cbk_sync_delete(uint64 data_id)
{
    cm_alarm_global_t *pinfo = &g_cm_alarm_global;
    
    return cm_db_exec_ext(pinfo->db_handle,
            "DELETE FROM record_t WHERE id=%llu",data_id);
}

cm_db_handle_t cm_alarm_db_get(void)
{
    return g_cm_alarm_global.db_handle;
}

sint32 cm_alarm_cfg_get(uint32 alarm_id, cm_alarm_config_t *cfg)
{
    return cm_alarm_get_config(&g_cm_alarm_global,alarm_id,cfg);
}

sint32 cm_alarm_threshold_get(uint32 alarm_id, uint32 *report, uint32 *recovery)
{
    cm_alarm_threshold_t cfg = {alarm_id,0,0};
    sint32 iret = cm_alarm_get_threshold(&g_cm_alarm_global,alarm_id,&cfg);

    if(CM_OK == iret)
    {
        if(NULL != report)
        {
            *report = cfg.report;
        }
        if(NULL != recovery)
        {
            *recovery = cfg.recovery;
        }
    }
    return iret;
}
sint32 cm_alarm_oplog(uint32 alarm_id,const sint8 *param, const sint8 *sessionid)
{
    /* 该函数用于往参数中填充 管理员名称,IP信息 */
    return CM_OK;
}


