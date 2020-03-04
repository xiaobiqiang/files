/******************************************************************************
 *          Copyright (c) 2018 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cfg_cnm_admin.c
 * author     : wbn
 * create date: 2018.07.10
 * description: TODO:
 *
 *****************************************************************************/
#include "cm_omi_types.h"
#include "cm_cfg_cnm.h"

extern const cm_omi_map_enum_t CmOmiMapCmdsType;

const cm_omi_map_cfg_t CmOmiMapAdminLevelType[] =
{
    {"SuperAdmin",0},
    {"Level1",1},
    {"Level2",2},
    {"Level3",3},
    {"Level4",4},
    {"Level5",5},
    {"Level6",6},
    {"Level7",7},
    {"Level8",8},
    {"Level9",9},
    {"Level10",10},
};

const cm_omi_map_enum_t CmOmiMapEnumAdminLevelType =
{
    sizeof(CmOmiMapAdminLevelType)/sizeof(cm_omi_map_cfg_t),
    CmOmiMapAdminLevelType
};    

const cm_omi_map_object_field_t CmOmiMapFieldsAdmin[] =
{
    {{"id", CM_OMI_FIELD_ADMIN_ID}, "-id",{CM_OMI_DATA_INT, 4, {NULL}}},
    {{"nid", CM_OMI_FIELD_ADMIN_NID}, "-nid",{CM_OMI_DATA_INT, 4, {NULL}}},
    {{"level", CM_OMI_FIELD_ADMIN_LEVEL}, "-level",{CM_OMI_DATA_ENUM, 4, {&CmOmiMapEnumAdminLevelType}}},
    {{"name", CM_OMI_FIELD_ADMIN_NAME}, "-name",{CM_OMI_DATA_STRING, CM_NAME_LEN, {"[0-9a-zA-Z_]{3,16}"}}},
    {{"pwd", CM_OMI_FIELD_ADMIN_PWD}, "-pwd",{CM_OMI_DATA_PWD, CM_NAME_LEN, {"[0-9a-zA-Z_]{1,63}"}}},
};

const cm_omi_map_object_field_t* CmOmiMapAdminCmdParamsGetBatch[]=
{
    &CmOmiMapCommFields[1], /*offset*/
    &CmOmiMapCommFields[2], /*total*/
};

const cm_omi_map_object_field_t* CmOmiMapAdminCmdParamsAdd[]=
{    
    &CmOmiMapFieldsAdmin[CM_OMI_FIELD_ADMIN_NAME], /*name*/
    &CmOmiMapFieldsAdmin[CM_OMI_FIELD_ADMIN_LEVEL], /*level*/
};

const cm_omi_map_object_field_t* CmOmiMapAdminCmdParamsAddOpt[]=
{
    &CmOmiMapFieldsAdmin[CM_OMI_FIELD_ADMIN_PWD], /*pwd*/
};

const cm_omi_map_object_field_t* CmOmiMapAdminCmdParamsGet[]=
{
    &CmOmiMapFieldsAdmin[CM_OMI_FIELD_ADMIN_ID], /*level*/
};

const cm_omi_map_object_field_t* CmOmiMapAdminCmdParamsCount[]=
{
    &CmOmiMapFieldsAdmin[CM_OMI_FIELD_ADMIN_NAME], /*name*/
    &CmOmiMapFieldsAdmin[CM_OMI_FIELD_ADMIN_PWD], /*pwd*/
};

const cm_omi_map_object_field_t* CmOmiMapAdminCmdParamsModify[]=
{
    &CmOmiMapFieldsAdmin[CM_OMI_FIELD_ADMIN_LEVEL], /*level*/
    &CmOmiMapFieldsAdmin[CM_OMI_FIELD_ADMIN_PWD], /*pwd*/
};


const cm_omi_map_object_cmd_t CmOmiMapAdminCmds[] =
{
    /* getbatch */
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET_BATCH],
        0,
        NULL,
        2,
        CmOmiMapAdminCmdParamsGetBatch,
    },
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET],
        1,
        CmOmiMapAdminCmdParamsGet,
        0,
        NULL,
    },
    /* count */
    {
        &CmOmiMapCmds[CM_OMI_CMD_COUNT],        
        0,
        NULL,
        2,
        CmOmiMapAdminCmdParamsCount,
    },
    /* add */
    {
        &CmOmiMapCmds[CM_OMI_CMD_ADD],        
        2,
        CmOmiMapAdminCmdParamsAdd,
        sizeof(CmOmiMapAdminCmdParamsAddOpt)/sizeof(cm_omi_map_object_field_t*),
        CmOmiMapAdminCmdParamsAddOpt,
    },
    /* modify */
    {
        &CmOmiMapCmds[CM_OMI_CMD_MODIFY],        
        1,
        CmOmiMapAdminCmdParamsGet,
        sizeof(CmOmiMapAdminCmdParamsModify)/sizeof(cm_omi_map_object_field_t*),
        CmOmiMapAdminCmdParamsModify,
    },

    /* delete */
    {
        &CmOmiMapCmds[CM_OMI_CMD_DELETE],        
        1,
        CmOmiMapAdminCmdParamsGet,
        0,
        NULL
    },
};

const cm_omi_map_object_t CmCnmAdminCfg =
{
    {"admin", CM_OMI_OBJECT_ADMIN},
    sizeof(CmOmiMapFieldsAdmin)/sizeof(cm_omi_map_object_field_t),
    CmOmiMapFieldsAdmin,
    sizeof(CmOmiMapAdminCmds)/sizeof(cm_omi_map_object_cmd_t),
    CmOmiMapAdminCmds
}; 

const cm_omi_map_object_field_t CmOmiMapFieldsPermission[] =
{
    {{"obj", CM_OMI_FIELD_PERMISSION_OBJ}, "-obj",{CM_OMI_DATA_ENUM_OBJ, 4, {NULL}}},
    {{"cmd", CM_OMI_FIELD_PERMISSION_CMD}, "-cmd",{CM_OMI_DATA_ENUM, 4, {&CmOmiMapCmdsType}}},
    {{"levels", CM_OMI_FIELD_PERMISSION_MASK}, "-levels",{CM_OMI_DATA_BITS_LEVELS, 4, {NULL}}},
};

const cm_omi_map_object_field_t* CmOmiMapPermissionCmdParams[]=
{
    &CmOmiMapFieldsPermission[CM_OMI_FIELD_PERMISSION_OBJ],
    &CmOmiMapFieldsPermission[CM_OMI_FIELD_PERMISSION_CMD],
    &CmOmiMapFieldsPermission[CM_OMI_FIELD_PERMISSION_MASK],
    &CmOmiMapCommFields[1], /*offset*/
    &CmOmiMapCommFields[2], /*total*/
    &CmOmiMapCommFields[0], /*field*/
};


const cm_omi_map_object_cmd_t CmOmiMapPermissionCmds[] =
{
    /* getbatch */
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET_BATCH],
        0,
        NULL,
        6,
        CmOmiMapPermissionCmdParams,
    },
    /* count */
    {
        &CmOmiMapCmds[CM_OMI_CMD_COUNT],        
        0,
        NULL,
        3,
        CmOmiMapPermissionCmdParams,
    },
    /* add */
    {
        &CmOmiMapCmds[CM_OMI_CMD_ADD],        
        3,
        CmOmiMapPermissionCmdParams,
        0,
        NULL,
    },
    /* modify */
    {
        &CmOmiMapCmds[CM_OMI_CMD_MODIFY],        
        3,
        CmOmiMapPermissionCmdParams,
        0,
        NULL,
    },

    /* delete */
    {
        &CmOmiMapCmds[CM_OMI_CMD_DELETE],        
        3,
        CmOmiMapPermissionCmdParams,
        0,
        NULL,
    },
};


const cm_omi_map_object_t CmCnmPermissionCfg =
{
    {"permission", CM_OMI_OBJECT_PERMISSION},
    sizeof(CmOmiMapFieldsPermission)/sizeof(cm_omi_map_object_field_t),
    CmOmiMapFieldsPermission,
    sizeof(CmOmiMapPermissionCmds)/sizeof(cm_omi_map_object_cmd_t),
    CmOmiMapPermissionCmds
}; 


