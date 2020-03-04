/******************************************************************************
 *          Copyright (c) 2018 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cfg_cnm_nfs.c
 * author     : zjd
 * create date: 2018.07.07
 * description: TODO:
 *
 *****************************************************************************/
#include "cm_omi_types.h"
#include "cm_cfg_cnm.h"

const cm_omi_map_cfg_t CmOmiMapNfsPermissionType[] =
{
    {"rw",0},
    {"ro",1},
};

const cm_omi_map_enum_t CmOmiMapEnumNfsPermissionType =
{
    sizeof(CmOmiMapNfsPermissionType)/sizeof(cm_omi_map_cfg_t),
    CmOmiMapNfsPermissionType
};


const cm_omi_map_object_field_t CmOmiMapFieldsNfs[] =
{
    {{"path", CM_OMI_NFS_CFG_DIR_PATH}, "-p",{CM_OMI_DATA_STRING, CM_NAS_PATH_LEN, {NULL}}},
    {{"nid", CM_OMI_NFS_CFG_NID}, "-nid",{CM_OMI_DATA_INT, 4, {NULL}}},
    {{"permission", CM_OMI_NFS_CFG_LIMIT}, "-per",{CM_OMI_DATA_ENUM, 4, {&CmOmiMapEnumNfsPermissionType}}},
    {{"ipaddress", CM_OMI_NFS_CFG_NFS_IP}, "-ip",{CM_OMI_DATA_STRING, 32, {NULL}}},
};

const cm_omi_map_object_field_t* CmOmiMapNfsCmdParamsGetBatch[]=
{
    &CmOmiMapFieldsNfs[CM_OMI_NFS_CFG_NID], /*node id*/
    &CmOmiMapFieldsNfs[CM_OMI_NFS_CFG_DIR_PATH], 
};

const cm_omi_map_object_field_t* CmOmiMapNfsCmdParamsGetBatchOpt[]=
{
    &CmOmiMapCommFields[1], /*offset*/
    &CmOmiMapCommFields[2], /*total*/
};

const cm_omi_map_object_field_t* CmOmiMapNfsCmdParamsAdd[]=
{
    &CmOmiMapFieldsNfs[CM_OMI_NFS_CFG_NID], /*node id*/
    &CmOmiMapFieldsNfs[CM_OMI_NFS_CFG_DIR_PATH], 
    &CmOmiMapFieldsNfs[CM_OMI_NFS_CFG_NFS_IP], 
    &CmOmiMapFieldsNfs[CM_OMI_NFS_CFG_LIMIT],    
};

const cm_omi_map_object_field_t* CmOmiMapNfsCmdParamsCount[]=
{
    &CmOmiMapFieldsNfs[CM_OMI_NFS_CFG_NID], /*node id*/
    &CmOmiMapFieldsNfs[CM_OMI_NFS_CFG_DIR_PATH],     
};

const cm_omi_map_object_cmd_t CmOmiMapNfsCmds[] =
{
    /* getbatch */
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET_BATCH],
        2,
        CmOmiMapNfsCmdParamsGetBatch,
        2,
        CmOmiMapNfsCmdParamsGetBatchOpt,
    },
    /* count */
    {
        &CmOmiMapCmds[CM_OMI_CMD_COUNT],        
        2,
        CmOmiMapNfsCmdParamsCount,
        0,
        NULL,
    },
    /* add */
    {
        &CmOmiMapCmds[CM_OMI_CMD_ADD],        
        4,
        CmOmiMapNfsCmdParamsAdd,
        0,
        NULL,
    },

    /* delete */
    {
        &CmOmiMapCmds[CM_OMI_CMD_DELETE],        
        3,
        CmOmiMapNfsCmdParamsAdd,
        0,
        NULL,
    },
};

const cm_omi_map_object_t CmCnmNfsCfg =
{
    {"nfs", CM_OMI_OBJECT_SHARE_NFS},
    sizeof(CmOmiMapFieldsNfs)/sizeof(cm_omi_map_object_field_t),
    CmOmiMapFieldsNfs,
    sizeof(CmOmiMapNfsCmds)/sizeof(cm_omi_map_object_cmd_t),
    CmOmiMapNfsCmds
}; 

