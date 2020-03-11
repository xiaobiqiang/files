/******************************************************************************
 *          Copyright (c) 2018 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cnm_common.h
 * author     : wbn
 * create date: 2018年3月16日
 * description: TODO:
 *
 *****************************************************************************/
#ifndef MAIN_CNM_CM_CNM_COMMON_H_
#define MAIN_CNM_CM_CNM_COMMON_H_

#include "cm_omi_types.h"
#include "cm_cfg_cnm.h"

#define CM_CNM_PING_TMOUT 3
#define CM_CNM_TRIM_LEFG "sed 's/^[ \t]*//g'"
#define CM_CNM_DEL_NULL "sed -e '/^$/d'"

/* 获取记录回调函数
arg: 记录指针
cols: 列值列表
col_num: 列数
typedef sint32 (*cm_cnm_exec_cbk_col_t)(void *arg, sint8 **cols, uint32 col_num);
*/
    
typedef cm_exec_cbk_col_t cm_cnm_exec_cbk_col_t;

typedef sint32 (*cm_cnm_exec_cbk_row_t)(void *arg, const sint8 *row_val, uint32 row_index);

typedef sint32 (*cm_cnm_query_getbatch_cbk_t)(
    uint32 nid, void *param, uint64 offset, uint32 total,void **ppAck, uint32 *pAckLen);
typedef uint64 (*cm_cnm_query_count_cbk_t)(uint32 nid, void *param);

typedef sint32 (*cm_cnm_local_cbk_t)(
    void *param, uint32 len,
    uint64 offset, uint32 total, /* 这两个参数仅批量查询使用 */
    void **ppAck, uint32 *pAckLen
    );
typedef sint32 (*cm_cnm_decode_check_func_t)(void *data);    

typedef struct
{
    const uint32 cmd;
    cm_cnm_local_cbk_t cbk;
}cm_cnm_cfg_cmd_t;

typedef struct
{
    /* 对象ID */
    const uint32 obj;

    /* 单条数据长度 */
    const uint32 each_size;

    /* 参数数据长度 */
    const uint32 param_size;

    /* 支持的操作个数 */
    const uint32 cmd_num;
    const cm_cnm_cfg_cmd_t *cmds;
}cm_cnm_cfg_obj_t;

/* 通用请求数据结构 */
typedef struct
{
    /* 目的节点, 0表示所有节点 */
    uint32 nid; 
    
    /* 对象ID */
    uint32 obj; 

    /* 操作 */
    uint32 cmd;

    /* 参数地址 */
    const void* param;

    /* 参数长度 */
    uint32 param_len;

    /* 批量查询: 开始偏移位置 */
    uint64 offset;
    
    /* 批量查询: 返回记录总数 */
    uint32 total;

    /* 返回数据地址 */
    void **ppack;

    /* 返回数据长度 */
    uint32 *acklen;

    /* 执行失败是否继续执行，对于非查询类操作 */
    bool_t fail_break;

    /* 仅在sbb中的一个节点执行 */
    bool_t sbb_only;
}cm_cnm_req_param_t;

typedef struct
{
    uint32 id;
    uint32 value;
}cm_cnm_map_value_num_t;

typedef struct
{
    uint32 id;
    sint8* value;
}cm_cnm_map_value_str_t;

typedef struct
{
    uint32 id;
    double value;
}cm_cnm_map_value_double_t;


typedef struct
{
    uint64 offset;
    uint32 total;
    uint32 nid; /* 固定采用1号参数*/
    cm_omi_field_flag_t field;
    cm_omi_field_flag_t set;
    uint8 data[];
}cm_cnm_decode_info_t;

typedef struct
{
    uint32 type; /* cm_cnm_query_ids_e */
    sint8 param[CM_STRING_256];
}cm_cnm_query_ids_t;

typedef struct
{
    uint32 id;
    uint32 size;
    void *val;
    cm_cnm_decode_check_func_t check;
}cm_cnm_decode_param_t;

extern uint32 cm_cnm_get_enum(const cm_omi_map_enum_t *pEnum, const sint8* 
pName, uint32 default_val);

extern sint32 cm_cnm_get_errcode(const cm_omi_map_enum_t *pEnum, const sint8* 
pErrorStr, sint32 default_val);


extern const sint8* cm_cnm_get_enum_str(const cm_omi_map_enum_t *pEnum, uint32 val);


/* 处理如下命令:
>zpool list
NAME        SIZE  ALLOC   FREE    CAP  DEDUP  HEALTH  ALTROOT
pool       3.62T  94.8M  3.62T     0%  1.00x  UNAVAIL  -
tank       3.62T  11.1G  3.61T     0%  1.00x  ONLINE  -
test       2.72T  10.1G  2.71T     0%  1.00x  ONLINE  -
test-pool  2.72T  37.0G  2.68T     1%  1.00x  UNAVAIL  -

参数说明:
cmd: 执行的命令
cbk: 每条记录的回调函数 cm_cnm_exec_cbk_t
row_offset: 从第多少条记录开始取 ,从0开始计数
each_size: 每条记录需要的空间Byte;
ppbuff: 输出内存地址，函数内申请，外面用了之后需要释放
row_cnt: 输入输出参数，输入返回最大条数，输出实际条数;
*/
extern sint32 cm_cnm_exec_get_list_old(const sint8 *cmd, cm_cnm_exec_cbk_col_t cbk, 
    uint32 row_offset, uint32 each_size,void **ppbuff,uint32 *row_cnt);

#define cm_cnm_exec_get_list(cmd,cbk,row_offset,each_size,ppbuff,row_cnt) \
    cm_exec_get_list((cbk),(row_offset),(each_size),(ppbuff),(row_cnt),(cmd))

/*  
处理如下命令:

LU Name: 600144F00100000000005AC3351E0003
Operational Status: Online
Provider Name     : sbd
Alias             : /dev/zvol/rdsk/tank/lun1
View Entry Count  : 0
Data File         : /dev/zvol/rdsk/tank/lun1
Meta File         : not set
Size              : 10737418240
Block Size        : 512
Management URL    : not set
Vendor ID         : CERESDAT
Product ID        : DJET6000        
Serial Num        : 1028
Write Protect     : Disabled
Writeback Cache   : Enabled
Access State      : Active->Standby
对于获取多个属性时使用该方法，如果只获取一条直接执行命令获取即可
*/
extern sint32 cm_cnm_exec_get_row(
    const sint8 **row_names, uint32 row_cnt,
    cm_cnm_exec_cbk_row_t cbk, void *info,
    const sint8 *cmd_format, ...);

/*
处理如下命令，获取单条记录信息
Prodigy-root#zfs list test
NAME   USED  AVAIL  REFER  MOUNTPOINT
test  84.8M  2.68T  42.2M  /test
*/
extern sint32 cm_cnm_exec_get_col(
    cm_cnm_exec_cbk_col_t cbk, void *info,
    const sint8 *cmd_format, ...);

extern sint32 cm_cnm_ack_uint64(uint64 val, void** ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_cmt_cbk(void *pMsg, uint32 Len, void** ppAck, uint32 
*pAckLen);

extern sint32 cm_cnm_request(cm_cnm_req_param_t *param);

extern bool_t cm_cnm_check_exist(cm_cnm_req_param_t *param);

typedef void (*cm_cnm_encode_each_cbk_t)(cm_omi_obj_t item,void *eachdata,void *arg);
extern cm_omi_obj_t cm_cnm_encode_comm(void *pAckData, uint32 AckLen, uint32 eachsize,
    cm_cnm_encode_each_cbk_t cbk, void *arg);

typedef sint32 (*cm_cnm_decode_cbk_t)(const cm_omi_obj_t ObjParam,void* info,cm_omi_field_flag_t *set);

extern sint32 cm_cnm_decode_comm(const cm_omi_obj_t ObjParam, uint32 eachsize,
cm_cnm_decode_cbk_t cbk, void** ppAck);

extern cm_omi_obj_t cm_cnm_encode_comm_ext(const void* pDecodeParam, void *pAckData, uint32 AckLen, 
    uint32 eachsize,cm_cnm_encode_each_cbk_t cbk);

extern void cm_cnm_encode_str(cm_omi_obj_t item,const cm_omi_field_flag_t *field,
    const cm_cnm_map_value_str_t *map, uint32 count);  
extern void cm_cnm_encode_num(cm_omi_obj_t item,const cm_omi_field_flag_t *field,
    const cm_cnm_map_value_num_t *map, uint32 count);

extern void cm_cnm_encode_double(cm_omi_obj_t item, const cm_omi_field_flag_t *field,
    const cm_cnm_map_value_double_t *map, uint32 count);

extern sint32 cm_cnm_request_comm(uint32 obj, uint32 cmd,
    uint32 eachsize, const void* pDecodeParam, void **ppAck, uint32 *pAckLen);

/* 将逗号换为空格 */
extern void cm_cnm_comma2blank(sint8 *str);

#define cm_cnm_get_cols(col_line,cols,max_cols) cm_get_cols((col_line),(cols),(max_cols))

extern const sint8* cm_cnm_get_enum_str(const cm_omi_map_enum_t *pEnum, uint32 val);

extern sint32 cm_cnm_exec_local(void *pMsg, uint32 Len, void** ppAck, uint32 
*pAckLen);

extern sint32 cm_cnm_exec_remote(uint32 nid, bool_t fail_break,sint8 *buff, uint32 size,
    const sint8* format, ...);
extern uint64 cm_cnm_exec_count(uint32 nid,const sint8* format, ...);

extern uint32 cm_cnm_exec_exited_nid(const sint8* format, ...);

extern sint32 cm_cnm_decode_str(const cm_omi_obj_t ObjParam,
    cm_cnm_decode_param_t *param, uint32 cnt,cm_omi_field_flag_t *set);
extern sint32 cm_cnm_decode_num(const cm_omi_obj_t ObjParam,
    cm_cnm_decode_param_t *param, uint32 cnt,cm_omi_field_flag_t *set);

#define cm_cnm_check_blocksize(n) ((4<=(n)) && (512>=(n)) && (!((n)&((n)-1))))

extern sint32 cm_cnm_decode_check_bool(void *val);

extern sint32 cm_cnm_exec_ping(sint8 *ipaddr);

typedef struct
{
    cm_omi_map_data_e type;
    uint32 id;
    uint32 size;
    const void* data;
}cm_cnm_oplog_param_t;

extern void cm_cnm_oplog_report(const sint8 *sessionid,uint32 alarmid,
    const cm_cnm_oplog_param_t* params, uint32 cnt, const cm_omi_field_flag_t* 
    set);

extern sint32 cm_cnm_localtask_create(
    const sint8* name, const sint8* cmd, uint32 *tid);

extern void cm_cnm_mkparam_str(sint8* buff,sint32 len, 
    const cm_omi_field_flag_t *field,
    const cm_cnm_map_value_str_t *map, uint32 count);

extern void cm_cnm_mkparam_num(sint8* buff,sint32 len,
    const cm_omi_field_flag_t *field,
    const cm_cnm_map_value_num_t *map, uint32 count);

#endif /* MAIN_CNM_CM_CNM_COMMON_H_ */

