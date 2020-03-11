/******************************************************************************
 *          Copyright (c) 2018 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cfg_usr.c
 * author     : xar
 * create date: 2018.07.31
 * description: TODO:
 *
 *****************************************************************************/

#include "cm_omi_types.h"
#include "cm_cfg_cnm.h"

const cm_omi_map_object_field_t CmOmiMapFieldsUser[] =
{
    {{"id",CM_OMI_FIELD_USER_ID},"-id",{CM_OMI_DATA_INT, 4, {NULL}}},
    {{"gid", CM_OMI_FIELD_USER_GID}, "-gid",{CM_OMI_DATA_INT, 4, {NULL}}},
    {{"name", CM_OMI_FIELD_USER_NAME}, "-name",{CM_OMI_DATA_STRING, CM_NAME_LEN, 
    {"[0-9a-zA-Z_-]{3,63}"}}},
    {{"pwd", CM_OMI_FIELD_USER_PWD}, "-pwd",{CM_OMI_DATA_STRING, CM_NAME_LEN, 
    {"[0-9a-zA-Z_-]{1,63}"}}},
    {{"path",CM_OMI_FIELD_USER_PATH},"-path",{CM_OMI_DATA_STRING, CM_NAME_LEN, {"[/]{0,1}((([a-z0-9A-Z_-]+)[./]{0,1})+)[a-z0-9A-Z_-]$"}}}
};

const cm_omi_map_object_field_t* CmOmiMapUserCmdParamsGetBatch[]=
{
    &CmOmiMapCommFields[1], /*offset*/
    &CmOmiMapCommFields[2], /*total*/
    &CmOmiMapFieldsUser[CM_OMI_FIELD_USER_GID],/*gid*/
};

const cm_omi_map_object_field_t* CmOmiMapUserCmdParamsAdd[]=
{
    &CmOmiMapFieldsUser[CM_OMI_FIELD_USER_NAME],/*name*/
    &CmOmiMapFieldsUser[CM_OMI_FIELD_USER_PWD],/*Password*/
};

const cm_omi_map_object_field_t* CmOmiMapUserCmdParamsGet[]=
{
    &CmOmiMapFieldsUser[CM_OMI_FIELD_USER_ID],/*ID*/
};

const cm_omi_map_object_field_t* CmOmiMapUserCmdParamsCount[]=
{
    &CmOmiMapFieldsUser[CM_OMI_FIELD_USER_ID],
    &CmOmiMapFieldsUser[CM_OMI_FIELD_USER_GID],/*gid*/
    &CmOmiMapFieldsUser[CM_OMI_FIELD_USER_PATH],
    &CmOmiMapFieldsUser[CM_OMI_FIELD_USER_PWD],/*pwd*/
};

const cm_omi_map_object_field_t* CmOmiMapUserCmdParamsModify[]=
{   
    &CmOmiMapFieldsUser[CM_OMI_FIELD_USER_NAME],/*name*/
};

const cm_omi_map_object_field_t* CmOmiMapUserCmdParamsDelete[]=
{
    &CmOmiMapFieldsUser[CM_OMI_FIELD_USER_NAME],/*name*/
};


const cm_omi_map_object_cmd_t CmOmiMapUserCmds[] =
{
    /* getbatch */
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET_BATCH],
        0,
        NULL,
        3,
        CmOmiMapUserCmdParamsGetBatch,
    },
    /* get */
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET],
        1,
        CmOmiMapUserCmdParamsDelete,
        0,
        NULL,
    },
    /* count */
    {
        &CmOmiMapCmds[CM_OMI_CMD_COUNT],
        0,
        NULL,
        1,
        CmOmiMapUserCmdParamsCount,
    },
    /* add */
    {
        &CmOmiMapCmds[CM_OMI_CMD_ADD],
        2,
        CmOmiMapUserCmdParamsAdd,
        3,
        CmOmiMapUserCmdParamsCount,
    },
    /* modify */
    {
        &CmOmiMapCmds[CM_OMI_CMD_MODIFY],
        1,
        CmOmiMapUserCmdParamsModify,
        4,
        CmOmiMapUserCmdParamsCount,
    },
    /* delete */
    {
        &CmOmiMapCmds[CM_OMI_CMD_DELETE],
        1,
        CmOmiMapUserCmdParamsDelete,
        0,
        NULL,
    }
};


const cm_omi_map_object_t CmUserCfg =
{
    {"user",CM_OMI_OBJECT_USER},
    sizeof(CmOmiMapFieldsUser)/sizeof(cm_omi_map_object_field_t),
    CmOmiMapFieldsUser,
    sizeof(CmOmiMapUserCmds)/sizeof(cm_omi_map_object_cmd_t),
    CmOmiMapUserCmds
};

/********************************************************
                  explorer(¡Á¨º?¡ä1¨¹¨¤¨ª?¡Â)
*********************************************************/

typedef enum
{
    CM_CNM_EXPLORER_TYPE_FILE=0,
    CM_CNM_EXPLORER_TYPE_DIRECTORY,
    CM_CNM_EXPLORER_TYPE_BLOCK,
    CM_CNM_EXPLORER_TYPE_CHARACTER,
    CM_CNM_EXPLORER_TYPE_FIFO,
    CM_CNM_EXPLORER_TYPE_SOCKET,
    CM_CNM_EXPLORER_TYPE_SYMBOLIEK,
}cm_cnm_explorer_type_e;

const cm_omi_map_cfg_t CmOmiMapExplorerType[] =
{
    {"file",CM_CNM_EXPLORER_TYPE_FILE},  
    {"directory",CM_CNM_EXPLORER_TYPE_DIRECTORY},
    {"block",CM_CNM_EXPLORER_TYPE_BLOCK},
    {"character",CM_CNM_EXPLORER_TYPE_CHARACTER},
    {"fifo",CM_CNM_EXPLORER_TYPE_FIFO},
    {"socket",CM_CNM_EXPLORER_TYPE_SOCKET},
    {"symboliek",CM_CNM_EXPLORER_TYPE_SYMBOLIEK},
};

const cm_omi_map_enum_t CmOmiMapEnumExplorerType =
{
    sizeof(CmOmiMapExplorerType)/sizeof(cm_omi_map_cfg_t),
    CmOmiMapExplorerType
};


const cm_omi_map_object_field_t CmOmiMapFieldsExplorer[]=
{
    {{"name", CM_OMI_FIELD_EXPLORER_NAME}, "-n",{CM_OMI_DATA_STRING, CM_NAME_LEN, {NULL}}},
    {{"nid", CM_OMI_FIELD_EXPLORER_NID}, "-nid",{CM_OMI_DATA_INT, 4, {NULL}}},
    {{"dir", CM_OMI_FIELD_EXPLORER_DIR}, "-dir",{CM_OMI_DATA_STRING, CM_STRING_64, {NULL}}},
    {{"permission", CM_OMI_FIELD_EXPLORER_PERMISSION}, "-permission",{CM_OMI_DATA_INT, 4, {NULL}}},
    {{"type", CM_OMI_FIELD_EXPLORER_TYPE}, "-type",{CM_OMI_DATA_ENUM, 4, {&CmOmiMapEnumExplorerType}}},
    {{"size", CM_OMI_FIELD_EXPLORER_SIZE}, "-size",{CM_OMI_DATA_INT, 4, {NULL}}},
    {{"user", CM_OMI_FIELD_EXPLORER_USER}, "-user",{CM_OMI_DATA_STRING, CM_STRING_64, {NULL}}},
    {{"group", CM_OMI_FIELD_EXPLORER_GROUP}, "-group",{CM_OMI_DATA_STRING, CM_STRING_64, {NULL}}},
    {{"atime", CM_OMI_FIELD_EXPLORER_ATIME}, "-at",{CM_OMI_DATA_TIME, 4, {NULL}}},
    {{"mtime", CM_OMI_FIELD_EXPLORER_MTIME}, "-mt",{CM_OMI_DATA_TIME, 4, {NULL}}},
    {{"ctime", CM_OMI_FIELD_EXPLORER_CTIME}, "-ct",{CM_OMI_DATA_TIME, 4, {NULL}}},
    {{"find", CM_OMI_FIELD_EXPLORER_FIND}, "-find",{CM_OMI_DATA_STRING, CM_STRING_64, {NULL}}},
    {{"flag", CM_OMI_FIELD_EXPLORER_FLAG}, "-flag",{CM_OMI_DATA_STRING, CM_STRING_64, {NULL}}},
};


const cm_omi_map_object_field_t* CmOmiMapExplorerCmdParamsGetBatch_ext[]=
{
    &CmOmiMapFieldsExplorer[CM_OMI_FIELD_EXPLORER_FIND],
    &CmOmiMapCommFields[1], /*offset*/
    &CmOmiMapCommFields[2], /*total*/
};

const cm_omi_map_object_field_t* CmOmiMapExplorerCmdParamsGetBatch[]=
{
    &CmOmiMapFieldsExplorer[CM_OMI_FIELD_EXPLORER_NID],
    &CmOmiMapFieldsExplorer[CM_OMI_FIELD_EXPLORER_DIR],
    &CmOmiMapFieldsExplorer[CM_OMI_FIELD_EXPLORER_FIND],
};

const cm_omi_map_object_field_t* CmOmiMapExplorerCmdParamsCount[]=
{
    &CmOmiMapFieldsExplorer[CM_OMI_FIELD_EXPLORER_FIND],
    &CmOmiMapFieldsExplorer[CM_OMI_FIELD_EXPLORER_FLAG],
};

const cm_omi_map_object_field_t* CmOmiMapExplorerCmdParamsAddOpt[]=
{
    &CmOmiMapFieldsExplorer[CM_OMI_FIELD_EXPLORER_NAME],    
    &CmOmiMapFieldsExplorer[CM_OMI_FIELD_EXPLORER_FIND],
    &CmOmiMapFieldsExplorer[CM_OMI_FIELD_EXPLORER_TYPE],
    &CmOmiMapFieldsExplorer[CM_OMI_FIELD_EXPLORER_PERMISSION],
};


const cm_omi_map_object_cmd_t CmOmiMapExplorerCmds[] =
{
    /* getbatch */
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET_BATCH],
        2,
        CmOmiMapExplorerCmdParamsGetBatch,
        3,
        CmOmiMapExplorerCmdParamsGetBatch_ext,
    },
    /* count */
    {
        &CmOmiMapCmds[CM_OMI_CMD_COUNT],
        2,
        CmOmiMapExplorerCmdParamsGetBatch,
        2,
        CmOmiMapExplorerCmdParamsCount,
    },
    /* insert */
    {
        &CmOmiMapCmds[CM_OMI_CMD_ADD],
        2,
        CmOmiMapExplorerCmdParamsGetBatch,
        4,
        CmOmiMapExplorerCmdParamsAddOpt,
    },
    /* delete */
    {
        &CmOmiMapCmds[CM_OMI_CMD_DELETE],
        2,
        CmOmiMapExplorerCmdParamsGetBatch,
        1,
        CmOmiMapExplorerCmdParamsAddOpt,
    },
    /* update */
    {
        &CmOmiMapCmds[CM_OMI_CMD_MODIFY],
        2,
        CmOmiMapExplorerCmdParamsGetBatch,
        4,
        CmOmiMapExplorerCmdParamsAddOpt,
    },
};


const cm_omi_map_object_t CmExplorerCfg =
{
    {"explorer",CM_OMI_OBJECT_EXPLORER},
    sizeof(CmOmiMapFieldsExplorer)/sizeof(cm_omi_map_object_field_t),
    CmOmiMapFieldsExplorer,
    sizeof(CmOmiMapExplorerCmds)/sizeof(cm_omi_map_object_cmd_t),
    CmOmiMapExplorerCmds
};

/************ domain user ******************/

const cm_omi_map_object_field_t CmOmiMapFieldsDomainUser[] =
{
    {{"domain_user", CM_OMI_FIELD_DOMAIN_USER}, "-du",{CM_OMI_DATA_STRING, CM_NAME_LEN, 
    {"[0-9a-zA-Z_-]{3,63}"}}},
    {{"local_user", CM_OMI_FIELD_DOMAIN_LOCAL_USER}, "-lu",{CM_OMI_DATA_STRING, CM_NAME_LEN, 
    {"[0-9a-zA-Z_-]{1,63}"}}},
};

const cm_omi_map_object_field_t* CmOmiMapDomainUserCmdParamsGetBatch[]=
{
    &CmOmiMapCommFields[1], /*offset*/
    &CmOmiMapCommFields[2], /*total*/
};

const cm_omi_map_object_field_t* CmOmiMapDomainUserCmdParamsAdd[]=
{
    &CmOmiMapFieldsDomainUser[CM_OMI_FIELD_DOMAIN_USER],/*name*/
    &CmOmiMapFieldsDomainUser[CM_OMI_FIELD_DOMAIN_LOCAL_USER],/*Password*/
};


const cm_omi_map_object_cmd_t CmOmiMapDomainUserCmds[] =
{
    /* getbatch */
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET_BATCH],
        0,
        NULL,
        2,
        CmOmiMapDomainUserCmdParamsGetBatch,
    },
    /* count */
    {
        &CmOmiMapCmds[CM_OMI_CMD_COUNT],
        0,
        NULL,
        0,
        NULL,
    },
    /* add */
    {
        &CmOmiMapCmds[CM_OMI_CMD_ADD],
        2,
        CmOmiMapDomainUserCmdParamsAdd,
        0,
        NULL,
    },
    /* delete */
    {
        &CmOmiMapCmds[CM_OMI_CMD_DELETE],
        2,
        CmOmiMapDomainUserCmdParamsAdd,
        0,
        NULL,
    }
};


const cm_omi_map_object_t CmDomainUserCfg =
{
    {"domain_user",CM_OMI_OBJECT_DOMAIN_USER},
    sizeof(CmOmiMapFieldsDomainUser)/sizeof(cm_omi_map_object_field_t),
    CmOmiMapFieldsDomainUser,
    sizeof(CmOmiMapDomainUserCmds)/sizeof(cm_omi_map_object_cmd_t),
    CmOmiMapDomainUserCmds
};

extern const cm_omi_map_enum_t CmOmiMapCifsNameTypeCfg;
const cm_omi_map_object_field_t CmOmiMapFieldsUserCache[]=
{
    {{"id", CM_OMI_FIELD_UCACHE_ID}, "-i",{CM_OMI_DATA_STRING, CM_NAME_LEN, {NULL}}},
    {{"nid", CM_OMI_FIELD_UCACHE_NID}, "-nid",{CM_OMI_DATA_INT, 4, {NULL}}},
    {{"name", CM_OMI_FIELD_UCACHE_NAME}, "-n",{CM_OMI_DATA_STRING, CM_STRING_64, {NULL}}},
    {{"type", CM_OMI_FIELD_UCACHE_TYPE}, "-type",{CM_OMI_DATA_ENUM, 4, {&CmOmiMapCifsNameTypeCfg}}},
    {{"domain", CM_OMI_FIELD_UCACHE_DOMAIN}, "-domain",{CM_OMI_DATA_ENUM, 4, {&CmOmiMapEnumDomainType}}},
};

const cm_omi_map_object_field_t* CmOmiMapDomainUserCacheCmdParams[]=
{
    &CmOmiMapFieldsUserCache[CM_OMI_FIELD_UCACHE_NID],
    &CmOmiMapFieldsUserCache[CM_OMI_FIELD_UCACHE_DOMAIN],
    &CmOmiMapFieldsUserCache[CM_OMI_FIELD_UCACHE_TYPE],
    &CmOmiMapFieldsUserCache[CM_OMI_FIELD_UCACHE_NAME],
};


const cm_omi_map_object_cmd_t CmOmiMapDomainUserCacheCmds[] =
{
    /* getbatch */
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET_BATCH],
        3,
        CmOmiMapDomainUserCacheCmdParams,
        2,
        CmOmiMapDomainUserCmdParamsGetBatch,
    },
    /* count */
    {
        &CmOmiMapCmds[CM_OMI_CMD_COUNT],
        3,
        CmOmiMapDomainUserCacheCmdParams,
        0,
        NULL,
    },
    {
        &CmOmiMapCmds[CM_OMI_CMD_TEST],
        4,
        CmOmiMapDomainUserCacheCmdParams,
        0,
        NULL,
    },
};

const cm_omi_map_object_t CmDomainUserCacheCfg =
{
    {"user_cache",CM_OMI_OBJECT_UCACHE},
    sizeof(CmOmiMapFieldsUserCache)/sizeof(cm_omi_map_object_field_t),
    CmOmiMapFieldsUserCache,
    sizeof(CmOmiMapDomainUserCacheCmds)/sizeof(cm_omi_map_object_cmd_t),
    CmOmiMapDomainUserCacheCmds
};





