/******************************************************************************
 *          Copyright (c) 2018 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cfg_cnm_migrate.c
 * author     : xar
 * create date: 2018.09.13
 * description: TODO:
 *
 *****************************************************************************/


#include "cm_omi_types.h"
#include "cm_cfg_cnm.h"


const cm_omi_map_cfg_t CmOmiMapMigrateNameType[] =
{
    {"disk",CM_MIGRATE_DISK},
    {"lun",CM_MIGRATE_LUN},
};

const cm_omi_map_enum_t CmOmiMapMigrateNameTypeCfg =
{
    sizeof(CmOmiMapMigrateNameType)/sizeof(cm_omi_map_cfg_t),
    CmOmiMapMigrateNameType
};

const cm_omi_map_cfg_t CmOmiMapOperationNameType[] =
{
    {"stop",CM_OPERATION_STOP},
    {"start",CM_OPERATION_START},
};

const cm_omi_map_enum_t CmOmiMapMigrateOperationTypeCfg =
{
    sizeof(CmOmiMapOperationNameType)/sizeof(cm_omi_map_cfg_t),
    CmOmiMapOperationNameType
};


const cm_omi_map_object_field_t CmOmiMapFieldsMigrate[] =
{
    {{"type", CM_OMI_FIELD_MIGRATE_TYPE}, "-type", {CM_OMI_DATA_ENUM, 4, {&CmOmiMapMigrateNameTypeCfg}}},
    {{"nid", CM_OMI_FIELD_MIGRATE_NID}, "-nid",{CM_OMI_DATA_STRING, 32, {"[0-9]{1,20}"}}},
    {{"operation", CM_OMI_FIELD_MIGRATE_OPERATION}, "-operation", {CM_OMI_DATA_ENUM, 4, {&CmOmiMapMigrateOperationTypeCfg}}},
    {{"name", CM_OMI_FIELD_MIGRATE_PATH}, "-name", {CM_OMI_DATA_STRING, CM_STRING_256, {"((([/a-z0-9A-Z_-]+)[./]{0,1})+)[a-z0-9A-Z_-]$"}}},
    {{"pool", CM_OMI_FIELD_MIGRATE_POOL}, "-pool", {CM_OMI_DATA_STRING, CM_STRING_64, {"[0-9a-zA-Z_]{1,64}"}}},
    {{"lunid", CM_OMI_FIELD_MIGRATE_LUNID}, "-lunid", {CM_OMI_DATA_STRING, CM_STRING_64, {"[0-9a-zA-Z_]{1,64}"}}},
    {{"progress", CM_OMI_FIELD_MIGRATE_PROGRESS}, "-progress", {CM_OMI_DATA_STRING, CM_STRING_64, {"[0-9a-zA-Z_]{1,64}"}}},
};

const cm_omi_map_object_field_t* CmOmiMapMigrateCmdParamsInsert[] =
{
    &CmOmiMapFieldsMigrate[CM_OMI_FIELD_MIGRATE_NID],
    &CmOmiMapFieldsMigrate[CM_OMI_FIELD_MIGRATE_TYPE],
    &CmOmiMapFieldsMigrate[CM_OMI_FIELD_MIGRATE_PATH],
    &CmOmiMapFieldsMigrate[CM_OMI_FIELD_MIGRATE_POOL],
};


const cm_omi_map_object_field_t* CmOmiMapMigrateCmdParamsInsert_ext[] =
{
    &CmOmiMapFieldsMigrate[CM_OMI_FIELD_MIGRATE_LUNID],
};

const cm_omi_map_object_field_t* CmOmiMapMigrateCmdParamsModify[] =
{
    &CmOmiMapFieldsMigrate[CM_OMI_FIELD_MIGRATE_NID],
    &CmOmiMapFieldsMigrate[CM_OMI_FIELD_MIGRATE_TYPE],
    &CmOmiMapFieldsMigrate[CM_OMI_FIELD_MIGRATE_PATH],
    &CmOmiMapFieldsMigrate[CM_OMI_FIELD_MIGRATE_OPERATION],
};



const cm_omi_map_object_cmd_t CmOmiMapMigrateCmds[] =
{
    /* get */
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET],
        3,
        CmOmiMapMigrateCmdParamsModify,
        0,
        NULL,
    },
    /* insert */
    {
        &CmOmiMapCmds[CM_OMI_CMD_ADD],
        4,
        CmOmiMapMigrateCmdParamsInsert,
        1,
        CmOmiMapMigrateCmdParamsInsert_ext,
    },
    /* update */
    {
        &CmOmiMapCmds[CM_OMI_CMD_MODIFY],
        4,
        CmOmiMapMigrateCmdParamsModify,
        0,
        NULL,
    },
    /* scan */
    {
        &CmOmiMapCmds[CM_OMI_CMD_SCAN],
        0,
        NULL,
        0,
        NULL,
    },
};



const cm_omi_map_object_t CmCnmMigrateCfg =
{
    {"lun_migrate", CM_OMI_OBJECT_MIGRATE},
    sizeof(CmOmiMapFieldsMigrate) / sizeof(cm_omi_map_object_field_t),
    CmOmiMapFieldsMigrate,
    sizeof(CmOmiMapMigrateCmds) / sizeof(cm_omi_map_object_cmd_t),
    CmOmiMapMigrateCmds
};

