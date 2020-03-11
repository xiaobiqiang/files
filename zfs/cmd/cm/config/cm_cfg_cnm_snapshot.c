/******************************************************************************
 *          Copyright (c) 2018 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cfg_cnm_snapshot.c
 * author     : wbn
 * create date: 2018.05.24
 * description: TODO:
 *
 *****************************************************************************/

#include "cm_omi_types.h"
#include "cm_cfg_cnm.h"

const cm_omi_map_cfg_t CmOmiMapSnapshotType[] =
{
    {"NAS",0},
    {"LUN",1},
};

const cm_omi_map_enum_t CmOmiMapEnumSnapshotType =
{
    sizeof(CmOmiMapSnapshotType)/sizeof(cm_omi_map_cfg_t),
    CmOmiMapSnapshotType
};


const cm_omi_map_object_field_t CmOmiMapFieldsSnapshot[] =
{
    {{"name", CM_OMI_FIELD_SNAPSHOT_NAME}, "-n",{CM_OMI_DATA_STRING, CM_NAME_LEN_SNAPSHOT, {"((([a-z0-9A-Z_-]+)[.]{0,1})+)[a-z0-9A-Z_-]$"}}},
    {{"nid", CM_OMI_FIELD_SNAPSHOT_NID}, "-nid",{CM_OMI_DATA_INT, 4, {NULL}}},
    {{"dir", CM_OMI_FIELD_SNAPSHOT_DIR}, "-dir",{CM_OMI_DATA_STRING, CM_NAME_LEN_DIR, {"((([a-z0-9A-Z_-]+)[./]{0,1})+)[a-z0-9A-Z_-]$"}}},
    {{"used", CM_OMI_FIELD_SNAPSHOT_USED},"-used", {CM_OMI_DATA_STRING, 32,{"[0-9]{1,10}[.]{0,1}[0-9]{0,3}[KMTGkmtg]{1}"}}},
    {{"create", CM_OMI_FIELD_SNAPSHOT_DATE},"-cr", {CM_OMI_DATA_TIME, 4, {NULL}}},
    {{"type", CM_OMI_FIELD_SNAPSHOT_TYPE},"-tp", {CM_OMI_DATA_ENUM, 4, {&CmOmiMapEnumSnapshotType}}},
};    

const cm_omi_map_object_field_t* CmOmiMapSnapshotCmdParamsGetBatch[]=
{
    &CmOmiMapFieldsSnapshot[CM_OMI_FIELD_SNAPSHOT_NID], /*node id*/
    &CmOmiMapFieldsSnapshot[CM_OMI_FIELD_SNAPSHOT_DIR], /*dir*/
    &CmOmiMapCommFields[1], /*offset*/
    &CmOmiMapCommFields[2], /*total*/
    &CmOmiMapCommFields[0], /*field*/
};

const cm_omi_map_object_field_t* CmOmiMapSnapshotCmdParamsGet[]=
{
    &CmOmiMapFieldsSnapshot[CM_OMI_FIELD_SNAPSHOT_NID], /*node id*/
    &CmOmiMapFieldsSnapshot[CM_OMI_FIELD_SNAPSHOT_DIR], /*dir*/
    &CmOmiMapFieldsSnapshot[CM_OMI_FIELD_SNAPSHOT_NAME], /*name*/
};


const cm_omi_map_object_cmd_t CmOmiMapSnapshotCmds[] =
{
    /* getbatch */
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET_BATCH],
        0,
        NULL,
        5,
        CmOmiMapSnapshotCmdParamsGetBatch,
    },
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET],
        3,
        CmOmiMapSnapshotCmdParamsGet,
        0,
        NULL,
    },
    /* count */
    {
        &CmOmiMapCmds[CM_OMI_CMD_COUNT],        
        0,
        NULL,
        3,
        CmOmiMapSnapshotCmdParamsGet,
    },
    /* add */
    {
        &CmOmiMapCmds[CM_OMI_CMD_ADD],        
        3,
        CmOmiMapSnapshotCmdParamsGet,
        0,
        NULL,
    },
    /* modify */
    {
        &CmOmiMapCmds[CM_OMI_CMD_MODIFY],        
        3,
        CmOmiMapSnapshotCmdParamsGet,
        0,
        NULL,
    },

    /* delete */
    {
        &CmOmiMapCmds[CM_OMI_CMD_DELETE],        
        3,
        CmOmiMapSnapshotCmdParamsGet,
        0,
        NULL
    },
};


const cm_omi_map_object_t CmCnmSnapshotCfg =
{
    {"snapshot", CM_OMI_OBJECT_SNAPSHOT},
    sizeof(CmOmiMapFieldsSnapshot)/sizeof(cm_omi_map_object_field_t),
    CmOmiMapFieldsSnapshot,
    sizeof(CmOmiMapSnapshotCmds)/sizeof(cm_omi_map_object_cmd_t),
    CmOmiMapSnapshotCmds
}; 

const cm_omi_map_cfg_t CmOmiMapSnapshotScheType[] =
{
    {"once",0},
    {"always",1},
};

const cm_omi_map_enum_t CmOmiMapEnumSnapshotScheType =
{
    sizeof(CmOmiMapSnapshotScheType)/sizeof(cm_omi_map_cfg_t),
    CmOmiMapSnapshotScheType
};

const cm_omi_map_cfg_t CmOmiMapSnapshotScheDType[] =
{
    {"everyday",0},
    {"monthday",1},
    {"weekday",2},
};

const cm_omi_map_enum_t CmOmiMapEnumSnapshotScheDType =
{
    sizeof(CmOmiMapSnapshotScheDType)/sizeof(cm_omi_map_cfg_t),
    CmOmiMapSnapshotScheDType
};


const cm_omi_map_object_field_t CmOmiMapFieldsSnapshotSche[] =
{
    {{"id", CM_OMI_FIELD_SNAPSHOT_SCHE_ID}, "-n",{CM_OMI_DATA_INT, 8, {NULL}}},
    {{"nid", CM_OMI_FIELD_SNAPSHOT_SCHE_NID}, "-nid",{CM_OMI_DATA_INT, 4, {NULL}}},
    {{"name", CM_OMI_FIELD_SNAPSHOT_SCHE_NAME}, "-n",{CM_OMI_DATA_STRING, CM_NAME_LEN_SNAPSHOT, {"((([a-z0-9A-Z_-]+)[.]{0,1})+)[a-z0-9A-Z_-]$"}}},
    {{"dir", CM_OMI_FIELD_SNAPSHOT_SCHE_DIR}, "-dir",{CM_OMI_DATA_STRING, CM_NAME_LEN_DIR, {"((([a-z0-9A-Z_-]+)[./]{0,1})+)[a-z0-9A-Z_-]$"}}},
    {{"expired", CM_OMI_FIELD_SNAPSHOT_SCHE_EXPIRED},"-expired", {CM_OMI_DATA_TIME, 4, {"^[1-9][0-9]{3,13}"}}},
    {{"type", CM_OMI_FIELD_SNAPSHOT_SCHE_TYPE},"-type", {CM_OMI_DATA_ENUM, 4, {&CmOmiMapEnumSnapshotScheType}}},
    {{"daytype", CM_OMI_FIELD_SNAPSHOT_SCHE_DAYTYPE},"-dtype", {CM_OMI_DATA_ENUM, 4, {&CmOmiMapEnumSnapshotScheDType}}},
    {{"minute", CM_OMI_FIELD_SNAPSHOT_SCHE_MINUTE}, "-minute",{CM_OMI_DATA_INT, 4, {NULL}}},
    {{"hours", CM_OMI_FIELD_SNAPSHOT_SCHE_HOURS}, "-hours",{CM_OMI_DATA_STRING, 64,{"[0-9,]{1,}"}}},
    {{"days", CM_OMI_FIELD_SNAPSHOT_SCHE_DAYS}, "-days",{CM_OMI_DATA_STRING, 64,{"[0-9,]{1,}"}}},
};

const cm_omi_map_object_field_t* CmOmiMapSnapshotScheCmdParamsGetBatch[]=
{
    &CmOmiMapFieldsSnapshotSche[CM_OMI_FIELD_SNAPSHOT_SCHE_DIR], /*dir*/
    &CmOmiMapCommFields[1], /*offset*/
    &CmOmiMapCommFields[2], /*total*/
    &CmOmiMapCommFields[0], /*field*/
};

const cm_omi_map_object_field_t* CmOmiMapSnapshotScheCmdParamsGet[]=
{
    &CmOmiMapFieldsSnapshotSche[CM_OMI_FIELD_SNAPSHOT_SCHE_ID], /*node id*/
};

const cm_omi_map_object_field_t* CmOmiMapSnapshotScheCmdParamsAdd[]=
{
    &CmOmiMapFieldsSnapshotSche[CM_OMI_FIELD_SNAPSHOT_SCHE_DIR], /*dir*/
    &CmOmiMapFieldsSnapshotSche[CM_OMI_FIELD_SNAPSHOT_SCHE_NAME], 
    &CmOmiMapFieldsSnapshotSche[CM_OMI_FIELD_SNAPSHOT_SCHE_EXPIRED], 
};

const cm_omi_map_object_field_t* CmOmiMapSnapshotScheCmdParamsAddOpt[]=
{
    &CmOmiMapFieldsSnapshotSche[CM_OMI_FIELD_SNAPSHOT_SCHE_TYPE], /*dir*/
    &CmOmiMapFieldsSnapshotSche[CM_OMI_FIELD_SNAPSHOT_SCHE_DAYTYPE], /*dir*/
    &CmOmiMapFieldsSnapshotSche[CM_OMI_FIELD_SNAPSHOT_SCHE_MINUTE], /*dir*/
    &CmOmiMapFieldsSnapshotSche[CM_OMI_FIELD_SNAPSHOT_SCHE_HOURS], /*dir*/
    &CmOmiMapFieldsSnapshotSche[CM_OMI_FIELD_SNAPSHOT_SCHE_DAYS], /*dir*/
    &CmOmiMapFieldsSnapshotSche[CM_OMI_FIELD_SNAPSHOT_SCHE_EXPIRED], 
    &CmOmiMapFieldsSnapshotSche[CM_OMI_FIELD_SNAPSHOT_SCHE_NAME],
};


const cm_omi_map_object_cmd_t CmOmiMapSnapshotScheCmds[] =
{
    /* getbatch */
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET_BATCH],
        0,
        NULL,
        4,
        CmOmiMapSnapshotScheCmdParamsGetBatch,
    },
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET],
        1,
        CmOmiMapSnapshotScheCmdParamsGet,
        0,
        NULL,
    },
    /* count */
    {
        &CmOmiMapCmds[CM_OMI_CMD_COUNT],        
        0,
        NULL,
        1,
        CmOmiMapSnapshotScheCmdParamsGetBatch,
    },
    /* add */
    {
        &CmOmiMapCmds[CM_OMI_CMD_ADD],        
        3,
        CmOmiMapSnapshotScheCmdParamsAdd,
        5,
        CmOmiMapSnapshotScheCmdParamsAddOpt,
    },
    /* modify */
    {
        &CmOmiMapCmds[CM_OMI_CMD_MODIFY],        
        1,
        CmOmiMapSnapshotScheCmdParamsGet,
        6,
        CmOmiMapSnapshotScheCmdParamsAddOpt,
    },

    /* delete */
    {
        &CmOmiMapCmds[CM_OMI_CMD_DELETE],        
        1,
        CmOmiMapSnapshotScheCmdParamsGet,
        0,
        NULL
    },
};

const cm_omi_map_object_t CmCnmSnapshotScheCfg =
{
    {"snapshot_sche", CM_OMI_OBJECT_SNAPSHOT_SCHE},
    sizeof(CmOmiMapFieldsSnapshotSche)/sizeof(cm_omi_map_object_field_t),
    CmOmiMapFieldsSnapshotSche,
    sizeof(CmOmiMapSnapshotScheCmds)/sizeof(cm_omi_map_object_cmd_t),
    CmOmiMapSnapshotScheCmds
};



