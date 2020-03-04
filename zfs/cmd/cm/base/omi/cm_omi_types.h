/******************************************************************************
 *          Copyright (c) 2017 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_omi_types.h
 * author     : wbn
 * create date: 2017��10��26��
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

/* �������� */
typedef struct
{
    /* ����ID */
    const uint32 cmd;

    /* Ȩ������ */
    const uint32 permission;

    /* ���뺯�� */
    cm_omi_cbk_decode_t decode_func;

    /* ������ */
    cm_omi_cbk_process_t process_func;

    /* ���뺯�� */
    cm_omi_cbk_encode_t encode_func;

    /* ������־�ϱ����� */
    cm_omi_cbk_oplog_t oplog_func;
}cm_omi_object_cmd_cfg_t;

/* ������ */
typedef struct
{
    /* ����ID */
    const uint32 object;

    /* �������� */
    const uint32 cmd_cnt;

    /* �����б�*/
    const cm_omi_object_cmd_cfg_t *pcmd_list;

    /* ��ʼ������ */
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

/* ���ƺ����ӳ�� */
typedef struct
{
    /* ���� */
    const sint8* pname;
    /* ��� */
    const uint32 code;
}cm_omi_map_cfg_t;

/* ö�ٽṹ */
typedef struct
{
    /* ��Ա�� */
    const uint32 num;
    /* ӳ���б� */
    const cm_omi_map_cfg_t *value;
}cm_omi_map_enum_t;

/* �������Ͷ���*/
typedef struct
{
    /* ��������: �ַ��������֣�ö�� */
    const cm_omi_map_data_e type;
    /* ���� */
    const uint32 size;

    /* У�鷽�� */
    union
    {
        /* ������ʽУ�� */
        const sint8* pregex;
        /* ö��ֵУ�� */
        const cm_omi_map_enum_t *penum;
    }check;
}cm_omi_map_data_t;

/* �ֶζ��� */
typedef struct
{
    /* ���ƣ�ID */
    const cm_omi_map_cfg_t field;

    /* ��д */
    const sint8* short_name;

    /* ���ݸ�ʽ */
    const cm_omi_map_data_t data;
}cm_omi_map_object_field_t;

/* ���� */
typedef struct
{
    /* ���ƣ�ID */
    const cm_omi_map_cfg_t* pcmd;

    /* ��ѡ����(�����ֶ�) */
    const uint32 param_num;
    const cm_omi_map_object_field_t **params;

    /* ��ѡ���� */
    const uint32 opt_param_num;
    const cm_omi_map_object_field_t **opt_params;
}cm_omi_map_object_cmd_t;

/* ������ */
typedef struct
{
    /* ���ƣ�ID */
    const cm_omi_map_cfg_t obj;

    /* �ֶ�����*/
    const uint32 field_num;
    const cm_omi_map_object_field_t *fields;

    /* ��������*/
    const uint32 cmd_num;
    const cm_omi_map_object_cmd_t *cmds;
}cm_omi_map_object_t;

typedef struct
{
    uint8 bits[CM_OMI_FIELD_MAX_NUM_BYTE];
}cm_omi_field_flag_t;


#endif /* BASE_OMI_CM_OMI_TYPES_H_ */
