/******************************************************************************
 *          Copyright (c) 2018 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_alarm_types.h
 * author     : wbn
 * create date: 2018年1月8日
 * description: TODO:
 *
 *****************************************************************************/
#ifndef MAIN_ALARM_CM_ALARM_TYPES_H_
#define MAIN_ALARM_CM_ALARM_TYPES_H_

#include "cm_common.h"

#define CM_ALARM_MAX_PARAM_LEN 256
#define CM_ALARM_CHECK_CNT 5

typedef enum
{
    CM_ALATM_TYPE_FAULT = 0,
    CM_ALATM_TYPE_EVENT,
    CM_ALATM_TYPE_LOG,
    CM_ALATM_TYPE_BUTT
}cm_alarm_type_e;

typedef enum
{
    CM_ALATM_LVL_CRITICAL = 0,
    CM_ALATM_LVL_MAJOR,
    CM_ALATM_LVL_MINOR,
    CM_ALATM_LVL_TRIVIAL,
    CM_ALATM_LVL_BUTT
}cm_alarm_level_e;

/*
CREATE TABLE config_t (
    alarm_id INT,
    match_bits INT,
    param_num TINYINT, 
    lvl TINYINT default 0,
    is_disable TINYINT default 0);
如果修改需要同步修改cm_alarm_get_config_each()
*/
typedef struct
{
    uint32 alarm_id;
    uint32 match_bits; /* 针对故障类型 */
    uint8 param_num;
    uint8 type; /*fault, event, log*/
    uint8 lvl; 
    uint8 is_disable;
}cm_alarm_config_t;
#define CM_ALRAM_CFG_TABLE_COLS_NUM 6
/*
CREATE TABLE record_t (
    global_id BIGINT,
    alarm_id INT,
    report_time INT,
    recovery_time INT,
    param VARCHAR(255));
    
*/
typedef struct
{
    uint64 global_id;
    uint32 alarm_id; 
    cm_time_t report_time;
    cm_time_t recovery_time; 
    sint8 param[CM_ALARM_MAX_PARAM_LEN];
}cm_alarm_record_t;

typedef struct
{
    uint32 alarm_id;
    uint32 report;
    uint32 recovery;
}cm_alarm_threshold_t;

#define CM_ALRAM_RECORD_TABLE_COLS_NUM 5

#endif /* MAIN_ALARM_CM_ALARM_TYPES_H_ */
