/******************************************************************************
 *          Copyright (c) 2018 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cfg_cnm_disk.c
 * author     : wbn
 * create date: 2018.05.04
 * description: TODO:
 *
 *****************************************************************************/
#include "cm_omi_types.h"
#include "cm_cfg_cnm.h"

extern const cm_omi_map_enum_t CmOmiMapEnumBoolType;

const cm_omi_map_cfg_t CmOmiMapDiskStatusType[] =
{
    {"unkown",0},
    {"free",1},
    {"busy",2},
};

const cm_omi_map_enum_t CmOmiMapEnumDiskStatusType =
{
    sizeof(CmOmiMapDiskStatusType)/sizeof(cm_omi_map_cfg_t),
    CmOmiMapDiskStatusType
};

const cm_omi_map_cfg_t CmOmiMapDiskLedType[] = 
{
    {"normal", 0},
    {"fault", 1},
    {"locate", 2},
    {"unknown", 3},
};

const cm_omi_map_enum_t CmOmiMapEnumDiskLedType =
{
    sizeof(CmOmiMapDiskLedType)/sizeof(cm_omi_map_cfg_t),
    CmOmiMapDiskLedType
};

extern const cm_omi_map_enum_t CmCnmPoolMapDiskTypeUserCfg;
const cm_omi_map_object_field_t CmOmiMapDiskFields[] =
{
    {{"id", CM_OMI_FIELD_DISK_ID}, "-i",{CM_OMI_DATA_STRING, 64, {"[0-9a-zA-Z_]{1,63}"}}},
    {{"nid", CM_OMI_FIELD_DISK_NID}, "-nid",{CM_OMI_DATA_INT, 4, {NULL}}},
    {{"sn", CM_OMI_FIELD_DISK_SN}, "-sn",{CM_OMI_DATA_STRING, CM_SN_LEN, {"[0-9a-zA-Z_]{1,63}"}}},
    {{"vendor", CM_OMI_FIELD_DISK_VENDOR}, "-ve",{CM_OMI_DATA_STRING, CM_SN_LEN, {"[0-9a-zA-Z_]{1,63}"}}},
    {{"size", CM_OMI_FIELD_DISK_SIZE},"-si", {CM_OMI_DATA_STRING, 64, {NULL}}},
    {{"rpm", CM_OMI_FIELD_DISK_RPM},"-rpm", {CM_OMI_DATA_INT, 4, {NULL}}},
    {{"enid", CM_OMI_FIELD_DISK_ENID},"-enid", {CM_OMI_DATA_INT, 4, {NULL}}},
    {{"slotid", CM_OMI_FIELD_DISK_SLOT},"-slot", {CM_OMI_DATA_INT, 4, {NULL}}},
    {{"status", CM_OMI_FIELD_DISK_STATUS},"-status", {CM_OMI_DATA_ENUM, 4, {&CmOmiMapEnumDiskStatusType}}},
    {{"pool", CM_OMI_FIELD_DISK_POOL},"-pool", {CM_OMI_DATA_STRING, 64, {"[0-9a-zA-Z_]{1,63}"}}},
    {{"led", CM_OMI_FIELD_DISK_LED},"-led", {CM_OMI_DATA_ENUM, 4, {&CmOmiMapEnumDiskLedType}}},
    {{"islocal", CM_OMI_FIELD_DISK_ISLOCAL},"-islocal", {CM_OMI_DATA_ENUM, 4, {&CmOmiMapEnumBoolType}}},
    {{"type", CM_OMI_FIELD_DISK_TYPE},"-type", {CM_OMI_DATA_ENUM, 4, {&CmCnmPoolMapDiskTypeUserCfg}}},
};

const cm_omi_map_object_field_t* CmOmiMapDiskCmdParamsGet[]=
{
    &CmOmiMapDiskFields[CM_OMI_FIELD_DISK_NID], /*node id*/
    &CmOmiMapDiskFields[CM_OMI_FIELD_DISK_ID],
};

const cm_omi_map_object_field_t* CmOmiMapDiskCmdParamsGetBatch[]=
{
    &CmOmiMapDiskFields[CM_OMI_FIELD_DISK_NID], /*node id*/
    &CmOmiMapDiskFields[CM_OMI_FIELD_DISK_RPM],
    &CmOmiMapDiskFields[CM_OMI_FIELD_DISK_ENID],
    &CmOmiMapDiskFields[CM_OMI_FIELD_DISK_STATUS],
    &CmOmiMapDiskFields[CM_OMI_FIELD_DISK_ISLOCAL],
    &CmOmiMapCommFields[0], /*fields*/
    &CmOmiMapCommFields[1], /*offset*/
    &CmOmiMapCommFields[2], /*total*/
};


const cm_omi_map_object_cmd_t CmOmiMapDiskCmds[] =
{
    /* getbatch */
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET_BATCH],
        0,
        NULL,
        8,
        CmOmiMapDiskCmdParamsGetBatch,
    },
    /* get */
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET],        
        2,
        CmOmiMapDiskCmdParamsGet,
        0,
        NULL,
    },
    /* count */
    {
        &CmOmiMapCmds[CM_OMI_CMD_COUNT],        
        0,
        NULL,
        5,
        CmOmiMapDiskCmdParamsGetBatch,
    },
    /* modify */
    {
        &CmOmiMapCmds[CM_OMI_CMD_MODIFY],        
        2,
        CmOmiMapDiskCmdParamsGet,
        0,
        NULL,
    },
};

const cm_omi_map_object_t CmCnmDiskCfg =
{
    {"disk", CM_OMI_OBJECT_DISK},
    sizeof(CmOmiMapDiskFields)/sizeof(cm_omi_map_object_field_t),
    CmOmiMapDiskFields,
    sizeof(CmOmiMapDiskCmds)/sizeof(cm_omi_map_object_cmd_t),
    CmOmiMapDiskCmds
}; 
extern const cm_omi_map_enum_t CmOmiMapEnumPoolStatusType;
const cm_omi_map_object_field_t CmOmiMapDiskSpareFields[] =
{
    {{"disk", CM_OMI_FIELD_DISKSPARE_DISK}, "-disk",{CM_OMI_DATA_STRING, 64, {"[0-9a-zA-Z_]{1,63}"}}},
    {{"nid", CM_OMI_FIELD_DISKSPARE_NID}, "-nid",{CM_OMI_DATA_INT, 4, {NULL}}},
    {{"enid", CM_OMI_FIELD_DISKSPARE_ENID},"-enid", {CM_OMI_DATA_INT, 4, {NULL}}},
    {{"slotid", CM_OMI_FIELD_DISKSPARE_SLOT},"-slot", {CM_OMI_DATA_INT, 4, {NULL}}},
    {{"status", CM_OMI_FIELD_DISKSPARE_STATUS},"-status", {CM_OMI_DATA_ENUM, 4, {&CmOmiMapEnumPoolStatusType}}},
    {{"pool", CM_OMI_FIELD_DISKSPARE_POOL},"-pool", {CM_OMI_DATA_STRING, 64, {"[0-9a-zA-Z_]{1,63}"}}},
    {{"used", CM_OMI_FIELD_DISKSPARE_USED},"-used", {CM_OMI_DATA_ENUM, 1, {&CmOmiMapEnumBoolType}}},
};

const cm_omi_map_object_field_t* CmOmiMapDiskSpareCmdParamsGetBatch[]=
{
    &CmOmiMapDiskSpareFields[CM_OMI_FIELD_DISKSPARE_DISK], /*node id*/
    &CmOmiMapCommFields[1], /*offset*/
    &CmOmiMapCommFields[2], /*total*/
};

const cm_omi_map_object_cmd_t CmOmiMapDiskSpareCmds[] =
{
    /* getbatch */
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET_BATCH],
        0,
        NULL,
        3,
        CmOmiMapDiskSpareCmdParamsGetBatch,
    },
    /* count */
    {
        &CmOmiMapCmds[CM_OMI_CMD_COUNT],        
        0,
        NULL,
        1,
        CmOmiMapDiskSpareCmdParamsGetBatch,
    },
    /* update */
    {
        &CmOmiMapCmds[CM_OMI_CMD_MODIFY],        
        0,
        NULL,
        0,
        NULL,
    },
};

const cm_omi_map_object_t CmCnmDiskSpareCfg =
{
    {"disk_spare", CM_OMI_OBJECT_DISK_SPARE},
    sizeof(CmOmiMapDiskSpareFields)/sizeof(cm_omi_map_object_field_t),
    CmOmiMapDiskSpareFields,
    sizeof(CmOmiMapDiskSpareCmds)/sizeof(cm_omi_map_object_cmd_t),
    CmOmiMapDiskSpareCmds
}; 


