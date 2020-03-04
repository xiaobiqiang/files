/******************************************************************************
 *          Copyright (c) 2018 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cfg_cnm_threshold.c
 * author     : zry
 * create date: 2019.3.28
 * description: TODO:
 *
 *****************************************************************************/
#include "cm_omi_types.h"
#include "cm_cfg_cnm.h"


extern const cm_omi_map_enum_t CmOmiMapEnumThresholdType; 

const cm_omi_map_object_field_t CmOmiMapFieldsThreshold[] =
{
    {{"alarm_id", CM_OMI_FIELD_THRESHOLD_ALARM_ID}, "-aid", {CM_OMI_DATA_INT, 4, {NULL}}},
    {{"re_threshold", CM_OMI_FIELD_THRESHOLD_RECOVERVAL}, "-rval", {CM_OMI_DATA_INT, 4, {NULL}}},
    {{"threshold", CM_OMI_FIELD_THRESHOLD_VALUE}, "-val", {CM_OMI_DATA_INT, 4, {NULL}}},
    
};
const cm_omi_map_object_field_t* CmOmiMapThresholdCmdParamsGetBatch[]=
{
    &CmOmiMapCommFields[1], /*offset*/
    &CmOmiMapCommFields[2], /*total*/
};
const cm_omi_map_object_field_t* CmOmiMapThresholdCmdParamsDelete[]=
{
    &CmOmiMapFieldsThreshold[CM_OMI_FIELD_THRESHOLD_ALARM_ID],
};

const cm_omi_map_object_field_t* CmOmiMapThresholdCmdParamsUpdate[]=
{
    &CmOmiMapFieldsThreshold[CM_OMI_FIELD_THRESHOLD_VALUE],
    &CmOmiMapFieldsThreshold[CM_OMI_FIELD_THRESHOLD_RECOVERVAL],
};
const cm_omi_map_object_cmd_t CmOmiMapThresholdCmds[] =
{
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET_BATCH],
        0,
        NULL,
        2,
        CmOmiMapThresholdCmdParamsGetBatch,
    },
    {
        &CmOmiMapCmds[CM_OMI_CMD_COUNT],
        0,
        NULL,
        0,
        NULL,
    },  
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET],
        1,
        CmOmiMapThresholdCmdParamsDelete,
        0,
        NULL,
    },
    {
        &CmOmiMapCmds[CM_OMI_CMD_MODIFY],
        1,
        CmOmiMapThresholdCmdParamsDelete,
        2,
        CmOmiMapThresholdCmdParamsUpdate,
    },  
};
const cm_omi_map_object_t CmCnmThresholdCfg =
{
    {"threshold", CM_OMI_OBJECT_THRESHOLD},
    sizeof(CmOmiMapFieldsThreshold) / sizeof(cm_omi_map_object_field_t),
    CmOmiMapFieldsThreshold,
    sizeof(CmOmiMapThresholdCmds) / sizeof(cm_omi_map_object_cmd_t),
    CmOmiMapThresholdCmds
};

