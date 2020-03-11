/******************************************************************************
 *          Copyright (c) 2018 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cfg_cnm_dirquota.c
 * author     : xar
 * create date: 2018.08.24
 * description: TODO:
 *
 *****************************************************************************/

#include "cm_omi_types.h"
#include "cm_cfg_cnm.h"

extern const cm_omi_map_enum_t CmOmiMapCifsNameTypeCfg;

const cm_omi_map_object_field_t CmOmiMapFieldsDirQuota[] =
{
    {{"dir_path", CM_OMI_FIRLD_DIRQUOTA_DIR}, "-dir", {CM_OMI_DATA_STRING, CM_STRING_256, {"[/]{0,1}((([a-z0-9A-Z_-]+)[./]{0,1})+)[a-z0-9A-Z_-]$"}}},
    {{"nid", CM_OMI_FIRLD_DIRQUOTA_NID}, "-nid", {CM_OMI_DATA_INT, 4, {NULL}}},
    {{"quota", CM_OMI_FIRLD_DIRQUOTA_QUOTA}, "-quota", {CM_OMI_DATA_STRING, CM_STRING_32, {"[0-9]{1,10}[.]{0,1}[0-9]{0,3}[KMTGPkmtgp]{1}"}}},
    {{"used", CM_OMI_FIRLD_DIRQUOTA_USED}, "-used", {CM_OMI_DATA_STRING, CM_STRING_32, {"[0-9]{1,10}[.]{0,1}[0-9]{0,3}[KMTGPkmtgp]{1}"}}},
    {{"rest", CM_OMI_FIRLD_DIRQUOTA_REST}, "-rest", {CM_OMI_DATA_STRING, CM_STRING_32, {"[0-9]{1,10}[.]{0,1}[0-9]{0,3}[KMTGPkmtgp]{1}"}}},
    {{"per_used", CM_OMI_FIRLD_DIRQUOTA_PER_USED}, "-per_used", {CM_OMI_DATA_DOUBLE, 8, {NULL}}},
    {{"nas", CM_OMI_FIRLD_DIRQUOTA_NAS}, "-nas", {CM_OMI_DATA_STRING,CM_NAME_LEN_NAS, {"[/]{0,1}((([a-z0-9A-Z_-]+)[./]{0,1})+)[a-z0-9A-Z_-]$"}}},
};

const cm_omi_map_object_field_t* CmOmiMapDirQuotaCmdParamsGetBatch[] =
{
    &CmOmiMapFieldsDirQuota[CM_OMI_FIRLD_DIRQUOTA_NID],
    &CmOmiMapFieldsDirQuota[CM_OMI_FIRLD_DIRQUOTA_NAS],
    &CmOmiMapCommFields[1], /*offset*/
    &CmOmiMapCommFields[2], /*total*/
};


const cm_omi_map_object_field_t* CmOmiMapDirQuotaCmdParamsGet[] =
{
    &CmOmiMapFieldsDirQuota[CM_OMI_FIELD_QUOTA_NID],
    &CmOmiMapFieldsDirQuota[CM_OMI_FIRLD_DIRQUOTA_DIR],   
};

const cm_omi_map_object_field_t* CmOmiMapDirQuotaCmdParamsAdd[] =
{
    &CmOmiMapFieldsDirQuota[CM_OMI_FIELD_QUOTA_NID],
    &CmOmiMapFieldsDirQuota[CM_OMI_FIRLD_DIRQUOTA_DIR],
    &CmOmiMapFieldsDirQuota[CM_OMI_FIRLD_DIRQUOTA_QUOTA],

};

const cm_omi_map_object_cmd_t CmOmiMapDirQuotaCmds[] =
{
    /* getbatch */
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET_BATCH],
        0,
        NULL,
        4,
        CmOmiMapDirQuotaCmdParamsGetBatch,
    },
    /* get */
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET],
        2,
        CmOmiMapDirQuotaCmdParamsGet,
        0,
        NULL,
    },
    /* count */
    {
        &CmOmiMapCmds[CM_OMI_CMD_COUNT],
        0,
        NULL,
        2,
        CmOmiMapDirQuotaCmdParamsGetBatch,
    },
    /* add */
    {
        &CmOmiMapCmds[CM_OMI_CMD_ADD],
        3,
        CmOmiMapDirQuotaCmdParamsAdd,
        0,
        NULL
    },
    {
        &CmOmiMapCmds[CM_OMI_CMD_DELETE],
        2,
        CmOmiMapDirQuotaCmdParamsAdd,
        0,
        NULL
    },
};

const cm_omi_map_object_t CmCnmDirQuotaCfg =
{
    {"dirquota", CM_OMI_OBJECT_DIRQUOTA},
    sizeof(CmOmiMapFieldsDirQuota) / sizeof(cm_omi_map_object_field_t),
    CmOmiMapFieldsDirQuota,
    sizeof(CmOmiMapDirQuotaCmds) / sizeof(cm_omi_map_object_cmd_t),
    CmOmiMapDirQuotaCmds
};

