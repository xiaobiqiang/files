/******************************************************************************
 *          Copyright (c) 2017 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_omi_types.h
 * author     : wbn
 * create date: 2017年10月26日
 * description: TODO:
 *
 *****************************************************************************/
#ifndef BASE_OMI_CM_OMI_TYPES_H_
#define BASE_OMI_CM_OMI_TYPES_H_

#include "cm_common.h"


#define CM_OMI_PERMISSION_ALL   0xFFFFFFFF
#define CM_CNM_FILED_ALL        0xFFFFFFFF
#define CM_CNM_MAX_RECORD       100
#define CM_OMI_FIELD_MAX_NUM    256
#define CM_OMI_FIELD_MAX_NUM_BYTE 32


typedef void* cm_omi_obj_t;

/**** define OMI CFG types ***************************************************/
typedef sint32 (*cm_omi_cbk_decode_t)(const cm_omi_obj_t ObjParam, void
**ppDecodeParam);
typedef sint32 (*cm_omi_cbk_process_t)(const void *pDecodeParam,void **ppAckData, uint32 *
pAckLen);
typedef cm_omi_obj_t (*cm_omi_cbk_encode_t)(const void *pDecodeParam, void *pAckData, uint32 AckLen);
typedef void (*cm_omi_cbk_oplog_t)(const sint8* sessionid, void *pDecodeParam, sint32 Result);

typedef sint32 (*cm_omi_cbk_init_t)(void);

typedef sint32 (*cm_omi_cbk_session_t)(const sint8* sessionid,uint32 *level);

typedef sint32 (*cm_omi_cbk_level_t)(uint32 obj,uint32 cmd,uint32 level);

#define IP_FORMAT "((1[0-9][0-9]\.)|(2[0-4][0-9]\.)|(25[0-5]\.)|([1-9][0-9]\.)|([1-9]\.))((1[0-9][0-9]\.)|(2[0-4][0-9]\.)|(25[0-5]\.)|([1-9][0-9]\.)|([0-9]\.)){2}((1[0-9][0-9])|(2[0-4][0-9])|(25[0-5])|([1-9][0-9])|([0-9]))"

/* 操作定义 */
typedef struct
{
    /* 操作ID */
    const uint32 cmd;

    /* 权限配置 */
    const uint32 permission;

    /* 解码函数 */
    cm_omi_cbk_decode_t decode_func;

    /* 处理函数 */
    cm_omi_cbk_process_t process_func;

    /* 编码函数 */
    cm_omi_cbk_encode_t encode_func;

    /* 操作日志上报函数 */
    cm_omi_cbk_oplog_t oplog_func;
}cm_omi_object_cmd_cfg_t;

/* 对象定义 */
typedef struct
{
    /* 对象ID */
    const uint32 object;

    /* 操作数量 */
    const uint32 cmd_cnt;

    /* 操作列表*/
    const cm_omi_object_cmd_cfg_t *pcmd_list;

    /* 初始化函数 */
    cm_omi_cbk_init_t init_func;
}cm_omi_object_cfg_t;

typedef enum
{
    CM_OMI_DATA_STRING = 0,
    CM_OMI_DATA_INT,
    CM_OMI_DATA_ENUM,
    CM_OMI_DATA_DOUBLE,
    CM_OMI_DATA_TIME,
    CM_OMI_DATA_BITS,
    CM_OMI_DATA_PWD,
    CM_OMI_DATA_ENUM_OBJ,
    CM_OMI_DATA_BITS_LEVELS,
}cm_omi_map_data_e;

/* 名称和序号映射 */
typedef struct
{
    /* 名称 */
    const sint8* pname;
    /* 序号 */
    const uint32 code;
}cm_omi_map_cfg_t;

/* 枚举结构 */
typedef struct
{
    /* 成员数 */
    const uint32 num;
    /* 映射列表 */
    const cm_omi_map_cfg_t *value;
}cm_omi_map_enum_t;

/* 数据类型定义*/
typedef struct
{
    /* 数据类型: 字符串，数字，枚举 */
    const cm_omi_map_data_e type;
    /* 长度 */
    const uint32 size;

    /* 校验方法 */
    union
    {
        /* 正则表达式校验 */
        const sint8* pregex;
        /* 枚举值校验 */
        const cm_omi_map_enum_t *penum;
    }check;
}cm_omi_map_data_t;

/* 字段定义 */
typedef struct
{
    /* 名称，ID */
    const cm_omi_map_cfg_t field;

    /* 缩写 */
    const sint8* short_name;

    /* 数据格式 */
    const cm_omi_map_data_t data;
}cm_omi_map_object_field_t;

/* 操作 */
typedef struct
{
    /* 名称，ID */
    const cm_omi_map_cfg_t* pcmd;

    /* 必选参数(或者字段) */
    const uint32 param_num;
    const cm_omi_map_object_field_t **params;

    /* 可选参数 */
    const uint32 opt_param_num;
    const cm_omi_map_object_field_t **opt_params;
}cm_omi_map_object_cmd_t;

/* 对象定义 */
typedef struct
{
    /* 名称，ID */
    const cm_omi_map_cfg_t obj;

    /* 字段配置*/
    const uint32 field_num;
    const cm_omi_map_object_field_t *fields;

    /* 操作配置*/
    const uint32 cmd_num;
    const cm_omi_map_object_cmd_t *cmds;
}cm_omi_map_object_t;

typedef struct
{
    uint8 bits[CM_OMI_FIELD_MAX_NUM_BYTE];
}cm_omi_field_flag_t;


#endif /* BASE_OMI_CM_OMI_TYPES_H_ */
