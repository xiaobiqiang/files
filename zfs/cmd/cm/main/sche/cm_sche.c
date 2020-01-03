/******************************************************************************
 *          Copyright (c) 2018 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_sche.c
 * author     : wbn
 * create date: 2018.05.25
 * description: TODO:
 *
 *****************************************************************************/
#include "cm_sche.h"
#include "cm_sync.h"
#include "cm_db.h"
#include "cm_log.h"
#include "cm_node.h"

#define CM_SCHE_DB_FILE CM_DATA_DIR"cm_sche.db"
static cm_db_handle_t g_cm_sche_db_handle = NULL;
extern const cm_sche_config_t* g_cm_sche_config;

#define CM_SCHE_CHECK_MASTER() \
    do{ \
        if(CM_NODE_ID_NONE == cm_node_get_master()) \
        { \
            CM_LOG_WARNING(CM_MOD_SCHE,"master none"); \
            return CM_ERR_NO_MASTER; \
        } \
    }while(0)

static void* cm_sche_thread(void* arg);

static sint32 cm_sche_get_each(
    void *arg, sint32 col_cnt, sint8 **col_vals, sint8 **col_names)
{
    cm_sche_info_t *info = arg;
    const sint32 col_def = 10;

    if(col_def != col_cnt)
    {
        CM_LOG_WARNING(CM_MOD_SCHE,"def[%d] cnt[%d]",col_def,col_cnt);
        return CM_FAIL;
    }
    info->id = (uint64)atoll(col_vals[0]);
    info->obj = (uint32)atoi(col_vals[1]);
    CM_VSPRINTF(info->name,sizeof(info->name),"%s",col_vals[2]);
    CM_VSPRINTF(info->param,sizeof(info->param),"%s",col_vals[3]);
    info->expired = (uint32)atoi(col_vals[4]);
    info->type = (uint8)atoi(col_vals[5]);
    info->day_type= (uint8)atoi(col_vals[6]);
    info->minute = (uint8)atoi(col_vals[7]);
    info->hours = (uint32)atoi(col_vals[8]);
    info->days = (uint32)atoi(col_vals[9]);
    return CM_OK;
}

sint32 cm_sche_cbk_sync_request(uint64 data_id, void *pdata, uint32 len)
{
    cm_sche_info_t *info = pdata;
    uint64 cnt = 0;
    sint32 iRet = cm_db_exec_get_count(g_cm_sche_db_handle,&cnt,
        "SELECT COUNT(id) FROM record_t WHERE id=%llu",data_id);

    if(CM_OK != iRet)
    {
        CM_LOG_INFO(CM_MOD_SCHE,"get CNT id[%llu] fail[%d]",data_id,iRet);
        return iRet;
    }
    if(0 == cnt)
    {
        return cm_db_exec_ext(g_cm_sche_db_handle,
                "INSERT INTO record_t (id,obj,name,param,expired,type,day_type,minute,hours,days)"
                " VALUES (%llu,%u,'%s','%s',%u,%u,%u,%u,%u,%u)",
                data_id,info->obj,info->name,info->param,info->expired,
                info->type,info->day_type,info->minute,info->hours,info->days);
    }
    
    return cm_db_exec_ext(g_cm_sche_db_handle,
            "UPDATE record_t SET name='%s',param='%s',expired=%u,type=%u,"
            "day_type=%u,minute=%u,hours=%u,days=%u "
            "WHERE id=%llu",info->name,info->param,info->expired,info->type,
            info->day_type,info->minute,info->hours,info->days,data_id);
}

sint32 cm_sche_init(void)
{
    cm_thread_t handle;
    sint32 iRet = CM_OK;
    const sint8* table = "CREATE TABLE IF NOT EXISTS record_t ("
            "id BIGINT,"
            "obj INT,"
            "name VARCHAR(255),"
            "param TEXT,"
            "expired INT,"
            "type TINYINT,"            
            "day_type TINYINT,"
            "minute TINYINT,"
            "hours INT,"
            "days INT)";

    iRet = cm_db_open_ext(CM_SCHE_DB_FILE,&g_cm_sche_db_handle);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_SCHE,"open %s fail",CM_SCHE_DB_FILE);
        return iRet;
    }
    iRet = cm_db_exec_ext(g_cm_sche_db_handle,table);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_SCHE,"create table fail[%d]",iRet);
        return iRet;
    }
    iRet = CM_THREAD_CREATE(&handle,cm_sche_thread,g_cm_sche_db_handle);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_SCHE,"create thread fail[%d]",iRet);
        return iRet;
    }
    CM_THREAD_DETACH(handle);
    return CM_OK;
}

sint32 cm_sche_create(cm_sche_info_t *info)
{
    uint64 id = 0;
    sint32 iRet = CM_FAIL;

    CM_SCHE_CHECK_MASTER();
    
    iRet = cm_db_exec_get_count(g_cm_sche_db_handle,&id,
        "SELECT COUNT(id) FROM record_t WHERE obj=%u AND name='%s' AND param='%s'",
        info->obj,info->name,info->param);

    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_SCHE,"check fail[%d]",iRet);
        return iRet;
    }
    if(0 != id)
    {
        CM_LOG_ERR(CM_MOD_SCHE,"obj[%u],name[%s],param[%s]",info->obj,info->name,info->param);
        return CM_ERR_ALREADY_EXISTS;
    }
    
    iRet = cm_db_exec_get_count(g_cm_sche_db_handle,&id,
        "SELECT MAX(id) FROM record_t");
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_SCHE,"get maxid fail[%d]",iRet);
        return iRet;
    }
    return cm_sync_request(CM_SYNC_OBJ_SCHE,(id+1),info,sizeof(cm_sche_info_t));
}

sint32 cm_sche_getbatch(uint32 obj, const sint8* opt_param, 
    uint32 offset, uint32 total, 
    cm_sche_info_t **ppAck, uint32 *pCnt)
{
    cm_sche_info_t *info = CM_MALLOC(sizeof(cm_sche_info_t)*total);

    if(NULL == info)
    {
        CM_LOG_ERR(CM_MOD_SCHE,"malloc fail");
        return CM_FAIL;
    }
    
    if((NULL == opt_param) || (0 == strlen(opt_param)))
    {
        *pCnt = cm_db_exec_get(g_cm_sche_db_handle,cm_sche_get_each,
            info,total,sizeof(cm_sche_info_t),
            "SELECT * FROM record_t WHERE "
            "obj=%u "
            "LIMIT %u,%u",
            obj,offset,total);
    }
    else
    {
        *pCnt = cm_db_exec_get(g_cm_sche_db_handle,cm_sche_get_each,
            info,total,sizeof(cm_sche_info_t),
            "SELECT * FROM record_t WHERE "
            "obj=%u AND param='%s' "
            "LIMIT %u,%u",
            obj,opt_param,offset,total);
    }
    if(0 == *pCnt)
    {
        CM_FREE(info);
    }
    else
    {
        *ppAck = info;
    }
    return CM_OK;
}

uint64 cm_sche_count(uint32 obj, const sint8* opt_param)
{
    uint64 cnt = 0;
    
    if((NULL == opt_param) || (0 == strlen(opt_param)))
    {
        (void)cm_db_exec_get_count(g_cm_sche_db_handle,&cnt,
            "SELECT COUNT(id) FROM record_t WHERE obj=%u",obj);
    }
    else
    {
        (void)cm_db_exec_get_count(g_cm_sche_db_handle,&cnt,
            "SELECT COUNT(id) FROM record_t WHERE obj=%u"
            " AND param='%s'",obj,opt_param);
    }
    return cnt;
}

sint32 cm_sche_delete(uint64 id)
{
    uint64 cnt = 0;
    sint32 iRet = CM_OK;
    
    CM_SCHE_CHECK_MASTER();
    iRet = cm_db_exec_get_count(g_cm_sche_db_handle,&cnt,
        "SELECT COUNT(id) FROM record_t WHERE id=%llu",id);

    if((CM_OK != iRet) || (0 == cnt))
    {
        CM_LOG_INFO(CM_MOD_SCHE,"get id[%llu] fail[%d]",id,iRet);
        return iRet;
    }
    
    return cm_sync_delete(CM_SYNC_OBJ_SCHE,id);
}

sint32 cm_sche_update(uint64 id, uint32 set_flag, cm_sche_info_t *info)
{
    uint32 cnt = 0;
    uint8 name_change = 0;
    cm_sche_info_t oldinfo;

    CM_SCHE_CHECK_MASTER();
    
    cnt = cm_db_exec_get(g_cm_sche_db_handle,cm_sche_get_each,
        &oldinfo,1,sizeof(cm_sche_info_t),
        "SELECT * FROM record_t WHERE id=%llu LIMIT 1",id);
    if(1 != cnt)
    {
        CM_LOG_INFO(CM_MOD_SCHE,"get id[%llu] fail",id);
        return CM_ERR_NOT_EXISTS;
    }
    
    cnt = 0;
    
    if((set_flag & CM_SCHE_FLAG_EXPIRED) && (oldinfo.expired != info->expired))
    {
        oldinfo.expired = info->expired;
        cnt++;
    }
    if((set_flag & CM_SCHE_FLAG_NAME) && (0 != strcmp(oldinfo.name,info->name)))
    {
        strcpy(oldinfo.name,info->name);
        cnt++;
        name_change++;
    }
    if((set_flag & CM_SCHE_FLAG_PARAM) && (0 != strcmp(oldinfo.param,info->param)))
    {
        strcpy(oldinfo.param,info->param);
        cnt++;
        name_change++;
    }
    if((set_flag & CM_SCHE_FLAG_TYPE) && (oldinfo.type != info->type))
    {
        oldinfo.type = info->type;
        cnt++;
    }
    if((set_flag & CM_SCHE_FLAG_DAYTYPE) && (oldinfo.day_type != info->day_type))
    {
        oldinfo.day_type = info->day_type;
        cnt++;
    }
    if((set_flag & CM_SCHE_FLAG_MINUTE) && (oldinfo.minute != info->minute))
    {
        oldinfo.minute = info->minute;
        cnt++;
    }
    if((set_flag & CM_SCHE_FLAG_HOURS) && (oldinfo.hours != info->hours))
    {
        oldinfo.hours = info->hours;
        cnt++;
    }
    if((set_flag & CM_SCHE_FLAG_DAYS) && (oldinfo.days != info->days))
    {
        oldinfo.days = info->days;
        cnt++;
    }
    if(0 == cnt)
    {
        return CM_OK;
    } 
    if(0 != name_change)
    {
        cnt = 0;
        (void)cm_db_exec_get_count(g_cm_sche_db_handle,&cnt,
            "SELECT COUNT(id) FROM record_t WHERE id<>%llu AND obj=%u AND name='%s' AND param='%s'",
            id,oldinfo.obj,oldinfo.name,oldinfo.param);

        if(0 != cnt)
        {
            CM_LOG_ERR(CM_MOD_SCHE,"obj[%u],name[%s],param[%s]",info->obj,info->name,info->param);
            return CM_ERR_ALREADY_EXISTS;
        }
    }
    
    return cm_sync_request(CM_SYNC_OBJ_SCHE,id,&oldinfo,sizeof(cm_sche_info_t));
}

sint32 cm_sche_cbk_sync_get(uint64 data_id, void **pdata, uint32 *plen)
{
    uint32 cnt = 0;
    cm_sche_info_t *info = CM_MALLOC(sizeof(cm_sche_info_t));

    if(NULL == info)
    {
        CM_LOG_INFO(CM_MOD_SCHE,"malloc fail");
        return CM_FAIL;
    }
    cnt = cm_db_exec_get(g_cm_sche_db_handle,cm_sche_get_each,
        info,1,sizeof(cm_sche_info_t),
        "SELECT * FROM record_t WHERE id=%llu LIMIT 1",data_id);
    if(0 == cnt)
    {
        CM_FREE(info);
        return CM_ERR_NOT_EXISTS;
    }
    *pdata = info;
    *plen = sizeof(cm_sche_info_t);
    return CM_OK;
}

sint32 cm_sche_cbk_sync_delete(uint64 data_id)
{
    return cm_db_exec_ext(g_cm_sche_db_handle,
            "DELETE FROM record_t WHERE id=%llu",data_id);
}

static sint32 cm_sche_get_each_id(
    void *arg, sint32 col_cnt, sint8 **col_vals, sint8 **col_names)
{
    uint64 *id = arg;
    if(1 != col_cnt)
    {
        CM_LOG_WARNING(CM_MOD_SCHE,"col_cnt[%d]",col_cnt);
        return CM_FAIL;
    }
    *id = (uint64)atoll(col_vals[0]);
    return CM_OK;    
}

static void cm_sche_clear_expired(cm_db_handle_t db_handle)
{
    const uint32 max_cnt = 10;
    uint64 ids[10] = {0};
    uint32 cnt = max_cnt;
    uint32 iloop = 0;
    cm_time_t now = cm_get_time()-60;

    while(cnt == max_cnt)
    {
        /* 这里不能直接从数据库删除，需要通过同步删除，不然会造成不一致 */
        cnt = cm_db_exec_get(db_handle,cm_sche_get_each_id,
            ids,max_cnt,sizeof(uint64),
            "SELECT id FROM record_t WHERE expired>0 AND expired<%u LIMIT %u",
            (uint32)now,max_cnt);
        for(iloop=0; iloop<cnt;iloop++)
        {
            (void)cm_sync_delete(CM_SYNC_OBJ_SCHE,ids[iloop]);
        }
    }
    return;
}

static cm_sche_cbk_t cm_sche_exec_cbk(uint32 obj)
{
    const cm_sche_config_t* cfg = g_cm_sche_config;

    while(NULL != cfg->cbk)
    {
        if(cfg->obj == obj)
        {
            return cfg->cbk;
        }
        cfg++;
    }
    return NULL;
}

static void cm_sche_exec(cm_sche_info_t *info, uint32 cnt)
{
    cm_sche_cbk_t cbk = NULL;
    
    for(;cnt>0;cnt--,info++)
    {
        cbk = cm_sche_exec_cbk(info->obj);
        if(NULL == cbk)
        {
            CM_LOG_WARNING(CM_MOD_SCHE,"obj[%u] cbk null",info->obj);
            continue;
        }
        (void)cbk(info->name,info->param);
    }
    return;
}
static void cm_sche_check(struct tm *tmnow,cm_db_handle_t db_handle)
{
    const uint32 max_cnt = 10;
    cm_sche_info_t infos[10];
    uint32 cnt = max_cnt;
    uint8 minute = 0;
    uint32 hour = 0;
    uint32 week_day = 0;
    uint32 month_day = 0;
    cm_time_t now = 0;

    minute = tmnow->tm_min;
    hour = 1<<tmnow->tm_hour;
    week_day = 1<<tmnow->tm_wday;
    month_day = 1<<tmnow->tm_mday;
    
    do
    {
        cnt = cm_db_exec_get(db_handle,cm_sche_get_each,
            infos,max_cnt,sizeof(cm_sche_info_t),
            "SELECT * FROM record_t WHERE "
            "type=1 AND minute=%u "
            "AND (hours=0 OR hours&%u) "
            "AND (days=0 OR (day_type=1 AND days&%u) OR (day_type=2 AND days&%u)) "
            "LIMIT %u",
            minute,hour,month_day,week_day,max_cnt);
        cm_sche_exec(infos, cnt);
    }while(cnt == max_cnt);
    
    do
    {
        now = cm_get_time();
        cnt = cm_db_exec_get(db_handle,cm_sche_get_each,
            infos,max_cnt,sizeof(cm_sche_info_t),
            "SELECT * FROM record_t WHERE "
            "type=0 AND expired<=%u AND expired>%u "
            "LIMIT %u",
            (uint32)now,(uint32)(now-60),max_cnt);
        cm_sche_exec(infos, cnt);
    }while(cnt == max_cnt);
    
    return;
}

static void* cm_sche_thread(void* arg)
{
    cm_db_handle_t db_handle = arg;
    struct tm tmnow;
    uint32 master = CM_NODE_ID_NONE;
    uint32 myid = cm_node_get_id();
    const uint8 waittime = 10;
    while(1)
    {
        master = cm_node_get_master();
        if(master != myid)
        {
            /* 仅在主节点上运行 */
            CM_SLEEP_S(waittime);
            continue;
        }
        cm_get_datetime(&tmnow);
        if(tmnow.tm_sec > waittime)
        {
            CM_SLEEP_S(waittime);
            continue;
        }
        /* 在前10秒内执行 */
        cm_sche_clear_expired(db_handle);
        
        cm_sche_check(&tmnow,db_handle);
        CM_SLEEP_S(waittime);   
    }
    return NULL;
}

