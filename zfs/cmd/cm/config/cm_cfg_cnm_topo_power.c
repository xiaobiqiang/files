/******************************************************************************
 *          Copyright (c) 2018 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cfg_cnm_topo_power.c
 * author     : xar
 * create date: 2018.08.24
 * description: TODO:
 *
 *****************************************************************************/
#include "cm_omi_types.h"
#include "cm_cfg_cnm.h"

const cm_omi_map_cfg_t CmOmiMapTopoStatusType[] =
{
    {"OK",0},
    {"FAIL",1},
};

const cm_omi_map_enum_t CmOmiMapEnumTopoStatusType =
{
    sizeof(CmOmiMapTopoStatusType)/sizeof(cm_omi_map_cfg_t),
    CmOmiMapTopoStatusType
};

const cm_omi_map_object_field_t CmOmiMapFieldsTopoPower[] =
{
    {{"enid", CM_OMI_FIRLD_TOPO_POWER_ENID}, "-en", {CM_OMI_DATA_INT, 4, {NULL}}},
    {{"power", CM_OMI_FIRLD_TOPO_POWER}, "-power", {CM_OMI_DATA_STRING, CM_STRING_256, {NULL}}},
    {{"volt", CM_OMI_FIRLD_TOPO_POWER_VOLT}, "-v", {CM_OMI_DATA_DOUBLE, 8, {NULL}}},
    {{"status", CM_OMI_FIRLD_TOPO_POWER_STATUS}, "-status", {CM_OMI_DATA_ENUM, 4, {&CmOmiMapEnumTopoStatusType}}},
};

const cm_omi_map_object_field_t* CmOmiMapTopoCmdParamsGetBatchVar[] =
{
    &CmOmiMapFieldsTopoPower[CM_OMI_FIRLD_TOPO_POWER_ENID], 
};

const cm_omi_map_object_cmd_t CmOmiMapTopoPowerCmds[] =
{
    /* getbatch */
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET_BATCH],
        0,
        NULL,
        1,
        CmOmiMapTopoCmdParamsGetBatchVar,
    },
    /* count */
    {
        &CmOmiMapCmds[CM_OMI_CMD_COUNT],
        0,
        NULL,
        1,
        CmOmiMapTopoCmdParamsGetBatchVar,
    },    
};

const cm_omi_map_object_t CmCnmTopoPowerCfg =
{
    {"power", CM_OMI_OBJECT_TOPO_POWER},
    sizeof(CmOmiMapFieldsTopoPower) / sizeof(cm_omi_map_object_field_t),
    CmOmiMapFieldsTopoPower,
    sizeof(CmOmiMapTopoPowerCmds) / sizeof(cm_omi_map_object_cmd_t),
    CmOmiMapTopoPowerCmds
};

/*  fan  */

const cm_omi_map_object_field_t CmOmiMapFieldsTopoFan[] =
{
    {{"enid", CM_OMI_FIRLD_TOPO_FAN_ENID}, "-en", {CM_OMI_DATA_INT, 4, {NULL}}},
    {{"nid", CM_OMI_FIRLD_TOPO_FAN_NID}, "-n", {CM_OMI_DATA_INT, 4, {NULL}}},
    {{"fan", CM_OMI_FIRLD_TOPO_FAN}, "-fan", {CM_OMI_DATA_STRING, CM_STRING_256, {NULL}}},
    {{"rotate", CM_OMI_FIRLD_TOPO_FAN_ROTATE}, "-r", {CM_OMI_DATA_INT, 4, {NULL}}},
    {{"status", CM_OMI_FIRLD_TOPO_FAN_STATUS}, "-status", {CM_OMI_DATA_ENUM, 4, {&CmOmiMapEnumTopoStatusType}}},
};

const cm_omi_map_object_field_t* CmOmiMapTopoFanCmdParamsGetBatchVar[] =
{
    &CmOmiMapFieldsTopoFan[CM_OMI_FIRLD_TOPO_FAN_ENID], 
    &CmOmiMapFieldsTopoFan[CM_OMI_FIRLD_TOPO_FAN_NID],
};

const cm_omi_map_object_cmd_t CmOmiMapTopoFanCmds[] =
{
    /* getbatch */
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET_BATCH],
        0,
        NULL,
        2,
        CmOmiMapTopoFanCmdParamsGetBatchVar,
    },
    /* count */
    {
        &CmOmiMapCmds[CM_OMI_CMD_COUNT],
        0,
        NULL,
        2,
        CmOmiMapTopoFanCmdParamsGetBatchVar,
    },    
};

const cm_omi_map_object_t CmCnmTopoFanCfg =
{
    {"fan", CM_OMI_OBJECT_TOPO_FAN},
    sizeof(CmOmiMapFieldsTopoFan) / sizeof(cm_omi_map_object_field_t),
    CmOmiMapFieldsTopoFan,
    sizeof(CmOmiMapTopoFanCmds) / sizeof(cm_omi_map_object_cmd_t),
    CmOmiMapTopoFanCmds
};
extern const cm_omi_map_enum_t CmOmiMapEnumDevType; 
const cm_omi_map_object_field_t CmOmiMapFieldsTopoTable[] =
{
    {{"name", CM_OMI_FIELD_TOPO_TABLE_NAME}, "-name", {CM_OMI_DATA_STRING, CM_STRING_64, {NULL}}},
    {{"set", CM_OMI_FIELD_TOPO_TABLE_SET}, "-set", {CM_OMI_DATA_INT, 4, {NULL}}},
    {{"enid", CM_OMI_FIELD_TOPO_TABLE_ENID}, "-enid", {CM_OMI_DATA_INT, 4, {NULL}}},
    {{"type", CM_OMI_FIELD_TOPO_TABLE_TYPE}, "-type", {CM_OMI_DATA_ENUM, 4, {&CmOmiMapEnumDevType}}},
    {{"u_num", CM_OMI_FIELD_TOPO_TABLE_UNUM}, "-u", {CM_OMI_DATA_INT, 4, {NULL}}},
    {{"slot", CM_OMI_FIELD_TOPO_TABLE_SLOT}, "-slot", {CM_OMI_DATA_INT, 4, {NULL}}},
    {{"sn", CM_OMI_FIELD_TOPO_TABLE_SN}, "-sn", {CM_OMI_DATA_STRING, CM_STRING_64, {NULL}}},
};
const cm_omi_map_object_field_t* CmOmiMapTopoTableCmdParamsGetBatch[]=
{
    &CmOmiMapFieldsTopoTable[CM_OMI_FIELD_TOPO_TABLE_NAME],
    &CmOmiMapCommFields[1], /*offset*/
    &CmOmiMapCommFields[2], /*total*/
};
const cm_omi_map_object_field_t* CmOmiMapTopoTableCmdParamsDelete[]=
{
    &CmOmiMapFieldsTopoTable[CM_OMI_FIELD_TOPO_TABLE_ENID],
};
const cm_omi_map_object_field_t* CmOmiMapTopoTableCmdParamsGet[]=
{
    &CmOmiMapFieldsTopoTable[CM_OMI_FIELD_TOPO_TABLE_NAME],
};
const cm_omi_map_object_field_t* CmOmiMapTopoTableCmdParamsUpdate[]=
{
    &CmOmiMapFieldsTopoTable[CM_OMI_FIELD_TOPO_TABLE_NAME],
    &CmOmiMapFieldsTopoTable[CM_OMI_FIELD_TOPO_TABLE_SET],
    &CmOmiMapFieldsTopoTable[CM_OMI_FIELD_TOPO_TABLE_SN],
};
const cm_omi_map_object_field_t* CmOmiMapTopoTableCmdParamsAdd[]=
{
    &CmOmiMapFieldsTopoTable[CM_OMI_FIELD_TOPO_TABLE_NAME],
    &CmOmiMapFieldsTopoTable[CM_OMI_FIELD_TOPO_TABLE_SN],
    &CmOmiMapFieldsTopoTable[CM_OMI_FIELD_TOPO_TABLE_UNUM],
    &CmOmiMapFieldsTopoTable[CM_OMI_FIELD_TOPO_TABLE_SLOT],
    &CmOmiMapFieldsTopoTable[CM_OMI_FIELD_TOPO_TABLE_SET],
    &CmOmiMapFieldsTopoTable[CM_OMI_FIELD_TOPO_TABLE_ENID],
};
const cm_omi_map_object_cmd_t CmOmiMapTopoTableCmds[] =
{
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET_BATCH],
        0,
        NULL,
        3,
        CmOmiMapTopoTableCmdParamsGetBatch,
    },
    {
        &CmOmiMapCmds[CM_OMI_CMD_COUNT],
        0,
        NULL,
        1,
        CmOmiMapTopoTableCmdParamsGet,
    },  
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET],
        1,
        CmOmiMapTopoTableCmdParamsDelete,
        0,
        NULL,
    },
    {
        &CmOmiMapCmds[CM_OMI_CMD_DELETE],
        1,
        CmOmiMapTopoTableCmdParamsDelete,
        0,
        NULL,
    },  
    {
        &CmOmiMapCmds[CM_OMI_CMD_MODIFY],
        1,
        CmOmiMapTopoTableCmdParamsDelete,
        3,
        CmOmiMapTopoTableCmdParamsUpdate,
    },  
    {
        &CmOmiMapCmds[CM_OMI_CMD_ADD],
        6,
        CmOmiMapTopoTableCmdParamsAdd,
        0,
        NULL,
    },
    {
        &CmOmiMapCmds[CM_OMI_CMD_OFF],
        1,
        CmOmiMapTopoTableCmdParamsDelete,
        0,
        NULL,
    },
};
const cm_omi_map_object_t CmCnmTopoTableCfg =
{
    {"device", CM_OMI_OBJECT_TOPO_TABLE},
    sizeof(CmOmiMapFieldsTopoTable) / sizeof(cm_omi_map_object_field_t),
    CmOmiMapFieldsTopoTable,
    sizeof(CmOmiMapTopoTableCmds) / sizeof(cm_omi_map_object_cmd_t),
    CmOmiMapTopoTableCmds
};

