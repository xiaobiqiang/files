/******************************************************************************
 *          Copyright (c) 2017 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_db.h
 * author     : wbn
 * create date: 2017年11月6日
 * description: TODO:
 *
 *****************************************************************************/
#ifndef BASE_DB_CM_DB_H_
#define BASE_DB_CM_DB_H_
#include "cm_common.h"

typedef void* cm_db_handle_t;

typedef sint32 (*cm_db_record_cbk_t)(
    void *arg, sint32 col_cnt, sint8 **col_vals, sint8 **col_names);

typedef struct
{
    void *data;
    uint32 max;
    uint32 index;
}cm_db_query_t;

/******************************************************************************
 * function     : cm_db_open
 * description  : 打开数据库
 *
 * parameter in : const sint8* pname
 *   
 * parameter out: cm_db_handle_t *phandle
 *
 * return type  : sint32
 *
 * author       : wbn
 * create date  : 2017.11.6
 *****************************************************************************/
extern sint32 cm_db_open(const sint8* pname, cm_db_handle_t *phandle);

/******************************************************************************
 * function     : cm_db_open_ext
 * description  : 打开数据库(如果不存在，或者打开失败重新创建)
 *
 * parameter in : const sint8* pname
 *   
 * parameter out: cm_db_handle_t *phandle
 *
 * return type  : sint32
 *
 * author       : wbn
 * create date  : 2017.12.11
 *****************************************************************************/
extern sint32 cm_db_open_ext(const sint8* pname, cm_db_handle_t *phandle);


/******************************************************************************
 * function     : cm_db_close
 * description  : 关闭数据库
 *
 * parameter in : cm_db_handle_t handle
 *   
 * parameter out: 无
 *
 * return type  : cm_ini_handle_t
 *
 * author       : wbn
 * create date  : 2017.11.6
 *****************************************************************************/
extern sint32 cm_db_close(cm_db_handle_t handle);

/******************************************************************************
 * function     : cm_db_exec
 * description  : 执行SQL命令
 *
 * parameter in : cm_db_handle_t handle
 *                cm_db_record_cbk_t cbk
 *                void *arg
 *                const sint8* sqlformat ...
 * parameter out: 无
 *
 * return type  : sint32
 *
 * author       : wbn
 * create date  : 2017.11.6
 *****************************************************************************/
extern sint32 cm_db_exec(
    cm_db_handle_t handle,
    cm_db_record_cbk_t cbk,
    void *arg,
    const sint8* sqlformat,...);

/* 非查询类操作执行 */
#define cm_db_exec_ext(handle,...) \
    cm_db_exec(handle, NULL,NULL,__VA_ARGS__)

/******************************************************************************
 * function     : cm_db_exec_get_count
 * description  : 执行获取记录数SQL
 *
 * parameter in : cm_db_handle_t handle
 *                const sint8* sqlformat ...
 * parameter out: uint64 *pcnt
 *
 * return type  : sint32
 *
 * author       : wbn
 * create date  : 2017.11.6
 *****************************************************************************/
extern sint32 cm_db_exec_get_count(cm_db_handle_t handle,uint64 *pcnt,
    const sint8* sqlformat,...);

/******************************************************************************
 * function     : cm_db_exec_get
 * description  : 执行获取记录SQL
 *
 * parameter in : cm_db_handle_t handle
 *                const sint8* sqlformat ...
 * parameter out: uint64 *pcnt
 *
 * return type  : uint32
 *
 * author       : wbn
 * create date  : 2018.01.16
 *****************************************************************************/

extern uint32 cm_db_exec_get(cm_db_handle_t handle, cm_db_record_cbk_t cbk, void *pbuff, uint32 
max_rows, uint32 each_size, const sint8* sqlformat,...); 

extern sint32 cm_db_exec_file_count(const sint8* file,uint64 *pcnt,
    const sint8* sqlformat,...);

extern uint32 cm_db_exec_file_get(const sint8* file,cm_db_record_cbk_t cbk, void *pbuff, uint32 
max_rows, uint32 each_size, const sint8* sqlformat,...);  


#endif /* BASE_DB_CM_DB_H_ */

