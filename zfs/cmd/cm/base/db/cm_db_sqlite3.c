/******************************************************************************
 *          Copyright (c) 2017 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_db_sqlite3.c
 * author     : wbn
 * create date: 2017Äê11ÔÂ6ÈÕ
 * description: TODO:
 *
 *****************************************************************************/
#include <stdarg.h>
#include "cm_db.h"
#include "sqlite3.h"
#include "cm_log.h"

typedef struct
{
    uint64 *pcount;
    bool_t is_set;
}cm_db_count_t;

static sint32 cm_db_get_count(
    void *arg, sint32 col_cnt, sint8 **col_vals, sint8 **col_names)
{
    cm_db_count_t *pinfo = (cm_db_count_t*)arg;

    if(NULL == pinfo->pcount)
    {
        return CM_OK;
    }
    pinfo->is_set = CM_TRUE;
    if((0 == col_cnt) || (NULL == col_vals) || (NULL == col_vals[0]))
    {
        CM_LOG_INFO(CM_MOD_DB, "col_cnt:0");
        //*(pinfo->pcount) = 0;
        return CM_OK;
    }
    *(pinfo->pcount) = (uint64)atoll(col_vals[0]);
    return CM_OK;    
}

typedef struct
{
    void *pbuff;
    uint32 max_rows;
    uint32 each_size;
    uint32 get_rows;
    cm_db_record_cbk_t cbk;
}cm_db_get_t;

static sint32 cm_db_get_info(
    void *arg, sint32 col_cnt, sint8 **col_vals, sint8 **col_names)
{
    cm_db_get_t *get = (cm_db_get_t*)arg;
    uint8 *pbuff = (void*)get->pbuff;
    sint32 iRet = CM_FAIL;
    
    if(get->get_rows >= get->max_rows)
    {
        return CM_OK;
    }
    iRet = get->cbk((void*)pbuff,col_cnt,col_vals,col_names);
    if(CM_OK != iRet)
    {
        return iRet;
    }
    get->get_rows++;
    get->pbuff = pbuff + get->each_size;
    return CM_OK;
}

#if 1
sint32 cm_db_open(const sint8* pname, cm_db_handle_t *phandle)
{
    sint32 iRet = CM_FAIL;
    sqlite3 *psql = NULL;

    if(CM_FALSE == cm_file_exist(pname))
    {
        CM_LOG_INFO(CM_MOD_DB, "file:%s not exist",pname);
        return CM_FAIL;
    }
    
    iRet = sqlite3_open(pname, &psql);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_DB, "open:%s fail[%d]",pname,iRet);
        return iRet;
    }
    *phandle = (cm_db_handle_t)psql;
    return CM_OK;
}

sint32 cm_db_open_ext(const sint8* pname, cm_db_handle_t *phandle)
{
    sint32 iRet = CM_FAIL;
    sqlite3 *psql = NULL;
    
    iRet = sqlite3_open(pname, &psql);
    if(CM_OK != iRet)
    {
        cm_system("rm -f %s",pname);
        cm_system("touch %s",pname);
        iRet = sqlite3_open(pname, &psql);
        if(CM_OK != iRet)
        {
            CM_LOG_ERR(CM_MOD_DB, "open:%s fail[%d]",pname,iRet);
            return iRet;
        }
    }
    *phandle = (cm_db_handle_t)psql;
    return CM_OK;
}


sint32 cm_db_close(cm_db_handle_t handle)
{
    if(NULL == handle)
    {
        CM_LOG_ERR(CM_MOD_DB, "handle: null");
        return CM_FAIL;
    }
    return sqlite3_close((sqlite3*)handle);
}

sint32 cm_db_exec(
    cm_db_handle_t handle,
    cm_db_record_cbk_t cbk,
    void *arg,
    const sint8* sqlformat,...)
{
    sint32 iRet = CM_FAIL;
    va_list args;
    sint8 sqlcmd[CM_STRING_1K+1] = {0};
    char *perrmsg = NULL;

    va_start(args, sqlformat);
    (void)vsnprintf(sqlcmd,CM_STRING_1K,sqlformat,args);
    va_end(args);
    
    iRet = sqlite3_exec((sqlite3*)handle,sqlcmd,cbk,arg,&perrmsg);

    if(NULL != perrmsg)
    {
        CM_LOG_ERR(CM_MOD_DB, "exec:%s,errmsg:%s",sqlcmd,perrmsg);
        sqlite3_free(perrmsg);
    }

    return iRet;    
}



sint32 cm_db_exec_get_count(cm_db_handle_t handle,uint64 *pcnt,
    const sint8* sqlformat,...)
{
    cm_db_count_t info = {pcnt,CM_FALSE};
    sint32 iRet = CM_FAIL;
    va_list args;
    sint8 sqlcmd[CM_STRING_1K+1] = {0};
    char *perrmsg = NULL;

    va_start(args, sqlformat);
    (void)vsnprintf(sqlcmd,CM_STRING_1K,sqlformat,args);
    va_end(args);

    iRet = sqlite3_exec((sqlite3*)handle,sqlcmd,cm_db_get_count,&info,&perrmsg);

    if(NULL != perrmsg)
    {
        CM_LOG_ERR(CM_MOD_DB, "exec:%s,errmsg:%s",sqlcmd,perrmsg);
        sqlite3_free(perrmsg);
    }
    return iRet;
}

uint32 cm_db_exec_get(cm_db_handle_t handle, cm_db_record_cbk_t cbk, 
    void *pbuff, uint32 max_rows, uint32 each_size, const sint8* sqlformat,...)
{
    cm_db_get_t get = {pbuff,max_rows,each_size,0,cbk};
    sint32 iRet = CM_FAIL;
    va_list args;
    sint8 sqlcmd[CM_STRING_1K+1] = {0};
    char *perrmsg = NULL;

    va_start(args, sqlformat);
    (void)vsnprintf(sqlcmd,CM_STRING_1K,sqlformat,args);
    va_end(args);

    iRet = sqlite3_exec((sqlite3*)handle,sqlcmd,cm_db_get_info,&get,&perrmsg);

    if(NULL != perrmsg)
    {
        CM_LOG_ERR(CM_MOD_DB, "exec:%s,errmsg:%s",sqlcmd,perrmsg);
        sqlite3_free(perrmsg);
        return 0;
    }
    return get.get_rows;
}


sint32 cm_db_exec_file_count(const sint8* file,uint64 *pcnt,
    const sint8* sqlformat,...)
{
    cm_db_count_t info = {pcnt,CM_FALSE};
    sint32 iRet = CM_FAIL;
    va_list args;
    sint8 sqlcmd[CM_STRING_1K+1] = {0};
    char *perrmsg = NULL;
    cm_db_handle_t handle = NULL;
    
    iRet = cm_db_open(file,&handle);
    if(CM_OK != iRet)
    {
        return iRet;
    }
    va_start(args, sqlformat);
    (void)vsnprintf(sqlcmd,CM_STRING_1K,sqlformat,args);
    va_end(args);

    iRet = sqlite3_exec((sqlite3*)handle,sqlcmd,cm_db_get_count,&info,&perrmsg);

    if(NULL != perrmsg)
    {
        CM_LOG_ERR(CM_MOD_DB, "exec:%s,errmsg:%s",sqlcmd,perrmsg);
        sqlite3_free(perrmsg);
    }
    cm_db_close(handle);
    return iRet;
}

uint32 cm_db_exec_file_get(const sint8* file,cm_db_record_cbk_t cbk, void *pbuff, uint32 
max_rows, uint32 each_size, const sint8* sqlformat,...)
{
    cm_db_get_t get = {pbuff,max_rows,each_size,0,cbk};
    sint32 iRet = CM_FAIL;
    va_list args;
    sint8 sqlcmd[CM_STRING_1K+1] = {0};
    char *perrmsg = NULL;
    cm_db_handle_t handle = NULL;
    
    iRet = cm_db_open(file,&handle);
    if(CM_OK != iRet)
    {
        return iRet;
    }

    va_start(args, sqlformat);
    (void)vsnprintf(sqlcmd,CM_STRING_1K,sqlformat,args);
    va_end(args);

    iRet = sqlite3_exec((sqlite3*)handle,sqlcmd,cm_db_get_info,&get,&perrmsg);

    if(NULL != perrmsg)
    {
        CM_LOG_ERR(CM_MOD_DB, "exec:%s,errmsg:%s",sqlcmd,perrmsg);
        sqlite3_free(perrmsg);
        cm_db_close(handle);
        return 0;
    }
    cm_db_close(handle);
    return get.get_rows;
}
#else
sint32 cm_db_open(const sint8* pname, cm_db_handle_t *phandle)
{
    if(CM_FALSE == cm_file_exist(pname))
    {
        CM_LOG_INFO(CM_MOD_DB, "file:%s not exist",pname);
        return CM_FAIL;
    }
    *phandle = (cm_db_handle_t)pname;
    return CM_OK;
}

sint32 cm_db_open_ext(const sint8* pname, cm_db_handle_t *phandle)
{
    if(CM_FALSE == cm_file_exist(pname))
    {
        cm_system("rm -f %s",pname);
        cm_system("touch %s",pname);
    }
    *phandle = (cm_db_handle_t)pname;
    return CM_OK;
}

sint32 cm_db_close(cm_db_handle_t handle)
{
    return CM_OK;
}

sint32 cm_db_exec(
    cm_db_handle_t handle,
    cm_db_record_cbk_t cbk,
    void *arg,
    const sint8* sqlformat,...)
{
    sint32 iRet = CM_FAIL;
    va_list args;
    sint8 sqlcmd[CM_STRING_1K+1] = {0};
    char *perrmsg = NULL;
    sqlite3 *psql = NULL;
    
    iRet = sqlite3_open((const char*)handle, &psql);
    if(CM_OK != iRet)
    {
        CM_LOG_INFO(CM_MOD_DB, "file:%s not exist",(const char*)handle);
        return CM_FAIL;
    }
    va_start(args, sqlformat);
    (void)vsnprintf(sqlcmd,CM_STRING_1K,sqlformat,args);
    va_end(args);
    
    iRet = sqlite3_exec((sqlite3*)psql,sqlcmd,cbk,arg,&perrmsg);

    if(NULL != perrmsg)
    {
        CM_LOG_ERR(CM_MOD_DB, "exec:%s,errmsg:%s",sqlcmd,perrmsg);
        sqlite3_free(perrmsg);
    }
    sqlite3_close((sqlite3*)psql);
    return iRet;    
}

sint32 cm_db_exec_get_count(cm_db_handle_t handle,uint64 *pcnt,
    const sint8* sqlformat,...)
{
    cm_db_count_t info = {pcnt,CM_FALSE};
    sint32 iRet = CM_FAIL;
    va_list args;
    sint8 sqlcmd[CM_STRING_1K+1] = {0};
    char *perrmsg = NULL;
    sqlite3 *psql = NULL;
    
    iRet = sqlite3_open((const char*)handle, &psql);
    if(CM_OK != iRet)
    {
        CM_LOG_INFO(CM_MOD_DB, "file:%s not exist",(const char*)handle);
        return CM_FAIL;
    }
    va_start(args, sqlformat);
    (void)vsnprintf(sqlcmd,CM_STRING_1K,sqlformat,args);
    va_end(args);

    iRet = sqlite3_exec((sqlite3*)psql,sqlcmd,cm_db_get_count,&info,&perrmsg);

    if(NULL != perrmsg)
    {
        CM_LOG_ERR(CM_MOD_DB, "exec:%s,errmsg:%s",sqlcmd,perrmsg);
        sqlite3_free(perrmsg);
    }
    sqlite3_close((sqlite3*)psql);
    return iRet;
}

uint32 cm_db_exec_get(cm_db_handle_t handle, cm_db_record_cbk_t cbk, 
    void *pbuff, uint32 max_rows, uint32 each_size, const sint8* sqlformat,...)
{
    cm_db_get_t get = {pbuff,max_rows,each_size,0,cbk};
    sint32 iRet = CM_FAIL;
    va_list args;
    sint8 sqlcmd[CM_STRING_1K+1] = {0};
    char *perrmsg = NULL;
    sqlite3 *psql = NULL;
    
    iRet = sqlite3_open((const char*)handle, &psql);
    if(CM_OK != iRet)
    {
        CM_LOG_INFO(CM_MOD_DB, "file:%s not exist",(const char*)handle);
        return CM_FAIL;
    }
    va_start(args, sqlformat);
    (void)vsnprintf(sqlcmd,CM_STRING_1K,sqlformat,args);
    va_end(args);

    iRet = sqlite3_exec((sqlite3*)psql,sqlcmd,cm_db_get_info,&get,&perrmsg);

    if(NULL != perrmsg)
    {
        CM_LOG_ERR(CM_MOD_DB, "exec:%s,errmsg:%s",sqlcmd,perrmsg);
        sqlite3_free(perrmsg);
        return 0;
    }
    sqlite3_close((sqlite3*)psql);
    return get.get_rows;
}


sint32 cm_db_exec_file_count(const sint8* file,uint64 *pcnt,
    const sint8* sqlformat,...)
{
    cm_db_count_t info = {pcnt,CM_FALSE};
    sint32 iRet = CM_FAIL;
    va_list args;
    sint8 sqlcmd[CM_STRING_1K+1] = {0};
    char *perrmsg = NULL;
    sqlite3 *psql = NULL;
    
    iRet = sqlite3_open((const char*)file, &psql);
    if(CM_OK != iRet)
    {
        CM_LOG_INFO(CM_MOD_DB, "file:%s not exist",file);
        return CM_FAIL;
    }
    va_start(args, sqlformat);
    (void)vsnprintf(sqlcmd,CM_STRING_1K,sqlformat,args);
    va_end(args);

    iRet = sqlite3_exec((sqlite3*)psql,sqlcmd,cm_db_get_count,&info,&perrmsg);

    if(NULL != perrmsg)
    {
        CM_LOG_ERR(CM_MOD_DB, "exec:%s,errmsg:%s",sqlcmd,perrmsg);
        sqlite3_free(perrmsg);
    }
    sqlite3_close((sqlite3*)psql);
    return iRet;
}

uint32 cm_db_exec_file_get(const sint8* file,cm_db_record_cbk_t cbk, void *pbuff, uint32 
max_rows, uint32 each_size, const sint8* sqlformat,...)
{
    cm_db_get_t get = {pbuff,max_rows,each_size,0,cbk};
    sint32 iRet = CM_FAIL;
    va_list args;
    sint8 sqlcmd[CM_STRING_1K+1] = {0};
    char *perrmsg = NULL;
    cm_db_handle_t handle = NULL;
    sqlite3 *psql = NULL;
    
    iRet = sqlite3_open((const char*)file, &psql);
    if(CM_OK != iRet)
    {
        CM_LOG_INFO(CM_MOD_DB, "file:%s not exist",file);
        return CM_FAIL;
    }
    va_start(args, sqlformat);
    (void)vsnprintf(sqlcmd,CM_STRING_1K,sqlformat,args);
    va_end(args);

    iRet = sqlite3_exec((sqlite3*)psql,sqlcmd,cm_db_get_info,&get,&perrmsg);

    if(NULL != perrmsg)
    {
        CM_LOG_ERR(CM_MOD_DB, "exec:%s,errmsg:%s",sqlcmd,perrmsg);
        sqlite3_free(perrmsg);
        sqlite3_close((sqlite3*)psql);
        return 0;
    }
    sqlite3_close((sqlite3*)psql);
    return get.get_rows;
}
#endif


