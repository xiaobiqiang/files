/******************************************************************************
 *          Copyright (c) 2018 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cfg_cnm_nas.c
 * author     : wbn
 * create date: 2018.07.07
 * description: TODO:
 *
 *****************************************************************************/
#include "cm_omi_types.h"
#include "cm_cfg_cnm.h"

extern const cm_omi_map_enum_t CmOmiMapLunSyncTypeCfg;

const cm_omi_map_object_field_t CmOmiMapFieldsNas[] =
{
    {{"name", CM_OMI_FIELD_NAS_NAME}, "-n",{CM_OMI_DATA_STRING, CM_NAME_LEN_NAS, {"[0-9a-zA-Z_]{1,63}"}}},
    {{"nid", CM_OMI_FIELD_NAS_NID}, "-nid",{CM_OMI_DATA_INT, 4, {NULL}}},
    {{"pool", CM_OMI_FIELD_NAS_POOL}, "-pool",{CM_OMI_DATA_STRING, CM_NAME_LEN_POOL, {"[0-9a-zA-Z_]{1,63}"}}},
    {{"blocksize", CM_OMI_FIELD_NAS_BLOCKSIZE}, "-bs",{CM_OMI_DATA_INT, 4, {NULL}}},
    {{"access", CM_OMI_FIELD_NAS_ACCESS}, "-access",{CM_OMI_DATA_INT, 4, {NULL}}},
    {{"w_policy", CM_OMI_FIELD_NAS_WRITE_POLICY}, "-wp",{CM_OMI_DATA_ENUM, 4, {&CmOmiMapLunSyncTypeCfg}}},
    {{"is_comp", CM_OMI_FIELD_NAS_IS_COMP}, "-comp",{CM_OMI_DATA_ENUM, 1, {&CmOmiMapEnumBoolType}}},
    {{"qos", CM_OMI_FIELD_NAS_QOS}, "-qos",{CM_OMI_DATA_STRING,32,{"[0-9]{1,10}[KMTGPkmtgp]{0,1}"}}},
    
    {{"avail", CM_OMI_FIELD_NAS_SPACE_AVAIL},"-avail", {CM_OMI_DATA_STRING, 32,{"[0-9]{1,10}[.]{0,1}[0-9]{0,3}[KMTGPkmtgp]{1}"}}},
    {{"used", CM_OMI_FIELD_NAS_SPACE_USED},"-used", {CM_OMI_DATA_STRING, 32, {NULL}}},
    {{"quota", CM_OMI_FIELD_NAS_QUOTA},"-quota", {CM_OMI_DATA_STRING, 32,{"[0-9]{1,10}[.]{0,1}[0-9]{0,3}[KMTGPkmtgp]{1}"}}},
    {{"dedup", CM_OMI_FIELD_NAS_DEDUP}, "-dedup",{CM_OMI_DATA_ENUM, 1, {&CmOmiMapEnumBoolType}}},
    {{"avsbw", CM_OMI_FIELD_NAS_QOS_AVS}, "-avsbw",{CM_OMI_DATA_STRING,32,{"[0-9]{1,10}[KMTGPkmtgp]{0,1}"}}},
    {{"smb", CM_OMI_FIELD_NAS_SMB}, "-smb",{CM_OMI_DATA_ENUM, 1, {&CmOmiMapEnumBoolType}}},
    {{"abe", CM_OMI_FIELD_NAS_ABE}, "-abe",{CM_OMI_DATA_ENUM, 1, {&CmOmiMapEnumBoolType}}},
    {{"aclinherit", CM_OMI_FIELD_NAS_ACLINHERIT}, "-acli",{CM_OMI_DATA_ENUM, 1, {&CmOmiMapAclInheritType}}},
};

const cm_omi_map_object_field_t* CmOmiMapNasCmdParamsGetBatch[]=
{
    &CmOmiMapFieldsNas[CM_OMI_FIELD_NAS_NID], /*node id*/
    &CmOmiMapCommFields[1], /*offset*/
    &CmOmiMapCommFields[2], /*total*/
    &CmOmiMapCommFields[0], /*field*/
};

const cm_omi_map_object_field_t* CmOmiMapNasCmdParamsAdd[]=
{
    &CmOmiMapFieldsNas[CM_OMI_FIELD_NAS_NID], /*node id*/
    &CmOmiMapFieldsNas[CM_OMI_FIELD_NAS_POOL], /*pool*/
    &CmOmiMapFieldsNas[CM_OMI_FIELD_NAS_NAME], /*name*/
};

const cm_omi_map_object_field_t* CmOmiMapNasCmdParamsAddOpt[]=
{
    &CmOmiMapFieldsNas[CM_OMI_FIELD_NAS_BLOCKSIZE], 
    &CmOmiMapFieldsNas[CM_OMI_FIELD_NAS_ACCESS],
    &CmOmiMapFieldsNas[CM_OMI_FIELD_NAS_WRITE_POLICY],
    &CmOmiMapFieldsNas[CM_OMI_FIELD_NAS_IS_COMP],
    &CmOmiMapFieldsNas[CM_OMI_FIELD_NAS_QOS],
    &CmOmiMapFieldsNas[CM_OMI_FIELD_NAS_QUOTA],
    &CmOmiMapFieldsNas[CM_OMI_FIELD_NAS_DEDUP],
    &CmOmiMapFieldsNas[CM_OMI_FIELD_NAS_QOS_AVS],
    &CmOmiMapFieldsNas[CM_OMI_FIELD_NAS_SMB],
    &CmOmiMapFieldsNas[CM_OMI_FIELD_NAS_ABE],
    &CmOmiMapFieldsNas[CM_OMI_FIELD_NAS_ACLINHERIT],
};

const cm_omi_map_object_cmd_t CmOmiMapNasCmds[] =
{
    /* getbatch */
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET_BATCH],
        0,
        NULL,
        4,
        CmOmiMapNasCmdParamsGetBatch,
    },
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET],
        3,
        CmOmiMapNasCmdParamsAdd,
        0,
        NULL,
    },
    /* count */
    {
        &CmOmiMapCmds[CM_OMI_CMD_COUNT],        
        0,
        NULL,
        1,
        CmOmiMapNasCmdParamsGetBatch,
    },
    /* add */
    {
        &CmOmiMapCmds[CM_OMI_CMD_ADD],        
        3,
        CmOmiMapNasCmdParamsAdd,
        sizeof(CmOmiMapNasCmdParamsAddOpt)/sizeof(cm_omi_map_object_field_t*),
        CmOmiMapNasCmdParamsAddOpt,
    },
    /* modify */
    {
        &CmOmiMapCmds[CM_OMI_CMD_MODIFY],        
        3,
        CmOmiMapNasCmdParamsAdd,
        sizeof(CmOmiMapNasCmdParamsAddOpt)/sizeof(cm_omi_map_object_field_t*),
        CmOmiMapNasCmdParamsAddOpt,
    },

    /* delete */
    {
        &CmOmiMapCmds[CM_OMI_CMD_DELETE],        
        3,
        CmOmiMapNasCmdParamsAdd,
        0,
        NULL
    },
};

const cm_omi_map_object_t CmCnmNasCfg =
{
    {"nas", CM_OMI_OBJECT_NAS},
    sizeof(CmOmiMapFieldsNas)/sizeof(cm_omi_map_object_field_t),
    CmOmiMapFieldsNas,
    sizeof(CmOmiMapNasCmds)/sizeof(cm_omi_map_object_cmd_t),
    CmOmiMapNasCmds
}; 

/*============================================================================*/

const cm_omi_map_cfg_t CmOmiMapCifsNameType[] =
{
    {"user",CM_NAME_USER},
    {"ugroup",CM_NAME_GROUP},
};

const cm_omi_map_enum_t CmOmiMapCifsNameTypeCfg =
{
    sizeof(CmOmiMapCifsNameType)/sizeof(cm_omi_map_cfg_t),
    CmOmiMapCifsNameType
};

const cm_omi_map_cfg_t CmOmiMapCifsPermissionType[] =
{
    {"RW",CM_NAS_PERISSION_RW},
    {"RO",CM_NAS_PERISSION_RO},
    {"RD",CM_NAS_PERISSION_RD},
};

const cm_omi_map_enum_t CmOmiMapCifsPermissionTypeCfg =
{
    sizeof(CmOmiMapCifsPermissionType)/sizeof(cm_omi_map_cfg_t),
    CmOmiMapCifsPermissionType
};

const cm_omi_map_object_field_t CmOmiMapFieldsCifs[] =
{
    {{"nas", CM_OMI_FIELD_CIFS_DIR}, "-nas",{CM_OMI_DATA_STRING, CM_NAME_LEN_DIR, {"[/]{0,1}((([a-z0-9A-Z_-]+)[./]{0,1})+)[a-z0-9A-Z_-]$"}}},
    {{"nid", CM_OMI_FIELD_CIFS_NID}, "-nid",{CM_OMI_DATA_INT, 4, {NULL}}},
    {{"name", CM_OMI_FIELD_CIFS_NAME}, "-name",{CM_OMI_DATA_STRING, CM_NAME_LEN_POOL, {"[0-9a-zA-Z_@]{1,63}"}}},
    {{"nametype", CM_OMI_FIELD_CIFS_NAME_TYPE}, "-ntype",{CM_OMI_DATA_ENUM, 4, {&CmOmiMapCifsNameTypeCfg}}},    
    {{"permission", CM_OMI_FIELD_CIFS_PERMISSION}, "-perm",{CM_OMI_DATA_ENUM, 4, {&CmOmiMapCifsPermissionTypeCfg}}},
    /*{{"domain", CM_OMI_FIELD_CIFS_DOMAIN}, "-dm",{CM_OMI_DATA_ENUM, 4, {NULL}}},*/
};

const cm_omi_map_object_field_t* CmOmiMapCifsCmdParamsGetBatch[]=
{
    &CmOmiMapFieldsCifs[CM_OMI_FIELD_CIFS_NID], /*node id*/
    &CmOmiMapFieldsCifs[CM_OMI_FIELD_CIFS_DIR], /*nas*/
    &CmOmiMapCommFields[1], /*offset*/
    &CmOmiMapCommFields[2], /*total*/
};

const cm_omi_map_object_field_t* CmOmiMapCifsCmdParamsAdd[]=
{
    &CmOmiMapFieldsCifs[CM_OMI_FIELD_CIFS_NID], /*node id*/
    &CmOmiMapFieldsCifs[CM_OMI_FIELD_CIFS_DIR], /*nas*/
    &CmOmiMapFieldsCifs[CM_OMI_FIELD_CIFS_NAME_TYPE], /*nametype*/
    &CmOmiMapFieldsCifs[CM_OMI_FIELD_CIFS_NAME], /*name*/    
    &CmOmiMapFieldsCifs[CM_OMI_FIELD_CIFS_PERMISSION],
};
const cm_omi_map_object_field_t* CmOmiMapCifsCmdParamsUpdate[]=
{
    &CmOmiMapFieldsCifs[CM_OMI_FIELD_CIFS_PERMISSION],
};    

const cm_omi_map_object_cmd_t CmOmiMapCifsCmds[] =
{
    /* getbatch */
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET_BATCH],
        2,
        CmOmiMapCifsCmdParamsGetBatch,
        2,
        &CmOmiMapCifsCmdParamsGetBatch[2],
    },
    
    /* count */
    {
        &CmOmiMapCmds[CM_OMI_CMD_COUNT],        
        2,
        CmOmiMapCifsCmdParamsGetBatch,
        0,
        NULL,
    },
    /* add */
    {
        &CmOmiMapCmds[CM_OMI_CMD_ADD],
        5,
        CmOmiMapCifsCmdParamsAdd,
        0,
        NULL,
    },
    /* modify */
    {
        &CmOmiMapCmds[CM_OMI_CMD_MODIFY],        
        5,
        CmOmiMapCifsCmdParamsAdd,
        0,
        NULL,
    },

    /* delete */
    {
        &CmOmiMapCmds[CM_OMI_CMD_DELETE],        
        4,
        CmOmiMapCifsCmdParamsAdd,
        0,
        NULL
    },
    /* get */
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET],        
        4,
        CmOmiMapCifsCmdParamsAdd,
        0,
        NULL
    },
};


const cm_omi_map_object_t CmCnmCifsCfg =
{
    {"cifs", CM_OMI_OBJECT_CIFS},
    sizeof(CmOmiMapFieldsCifs)/sizeof(cm_omi_map_object_field_t),
    CmOmiMapFieldsCifs,
    sizeof(CmOmiMapCifsCmds)/sizeof(cm_omi_map_object_cmd_t),
    CmOmiMapCifsCmds
}; 

/**************************************************************
                        nas_shadow
***************************************************************/

const cm_omi_map_cfg_t CmOmiMapshadowNameType[] =
{
    {"stop",CM_OPERATION_STOP},
    {"restart",CM_OPERATION_START},
};

const cm_omi_map_enum_t CmOmiMapShadowOperationTypeCfg =
{
    sizeof(CmOmiMapshadowNameType)/sizeof(cm_omi_map_cfg_t),
    CmOmiMapshadowNameType
};


const cm_omi_map_object_field_t CmOmiMapFieldsNas_shadow[] =
{
    {{"status", CM_OMI_FIELD_NAS_SHADOW_STATUS}, "-s",{CM_OMI_DATA_ENUM, 4, {&CmOmiMapShadowOperationTypeCfg}}},
    {{"nid", CM_OMI_FIELD_NAS_SHADOW_NID}, "-nid",{CM_OMI_DATA_INT, 4, {NULL}}},
    {{"shadow_status", CM_OMI_FIELD_NAS_SHADOW_STATE}, "-ss",{CM_OMI_DATA_STRING,CM_STRING_128, {NULL}}},
    {{"mirror_status", CM_OMI_FIELD_NAS_SHADOW_MIRROR}, "-ms",{CM_OMI_DATA_STRING,CM_STRING_128, {NULL}}},
    {{"dst", CM_OMI_FIELD_NAS_SHADOW_DST}, "-dst",{CM_OMI_DATA_STRING, CM_NAME_LEN_DIR, {"[/]{0,1}((([a-z0-9A-Z_-]+)[./]{0,1})+)[a-z0-9A-Z_-]$"}}},
    {{"src", CM_OMI_FIELD_NAS_SHADOW_SRC}, "-src",{CM_OMI_DATA_STRING, CM_NAME_LEN_DIR, {"[/]{0,1}((([a-z0-9A-Z_-]+)[./]{0,1})+)[a-z0-9A-Z_-]$"}}},
    {{"mount", CM_OMI_FIELD_NAS_SHADOW_MOUNT}, "-mount",{CM_OMI_DATA_STRING,CM_STRING_128, {NULL}}},
};

const cm_omi_map_object_field_t* CmOmiMapNas_shadowCmdParamsAdd[]=
{
    &CmOmiMapFieldsNas_shadow[CM_OMI_FIELD_NAS_SHADOW_NID],
    &CmOmiMapFieldsNas_shadow[CM_OMI_FIELD_NAS_SHADOW_DST],
    &CmOmiMapFieldsNas_shadow[CM_OMI_FIELD_NAS_SHADOW_SRC],
};

const cm_omi_map_object_field_t* CmOmiMapNas_shadowCmdParamsUpdate[]=
{
    &CmOmiMapFieldsNas_shadow[CM_OMI_FIELD_NAS_SHADOW_NID],
    &CmOmiMapFieldsNas_shadow[CM_OMI_FIELD_NAS_SHADOW_STATUS],
};

const cm_omi_map_object_field_t* CmOmiMapNas_shadowCmdParamsAdd_ext[]=
{
    &CmOmiMapFieldsNas_shadow[CM_OMI_FIELD_NAS_SHADOW_MOUNT],
};

const cm_omi_map_object_cmd_t CmOmiMapNas_shadowCmds[] =
{
    
    /* get */
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET],
        1,
        CmOmiMapNas_shadowCmdParamsAdd,
        0,
        NULL,
    },
    /* getbatch */
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET_BATCH],
        0,
        NULL,
        1,
        CmOmiMapNas_shadowCmdParamsAdd,
    },
    /* count */
    {
        &CmOmiMapCmds[CM_OMI_CMD_COUNT],
        0,
        NULL,
        1,
        CmOmiMapNas_shadowCmdParamsAdd,
    },
    /* insert */
    {
        &CmOmiMapCmds[CM_OMI_CMD_ADD],
        3,
        CmOmiMapNas_shadowCmdParamsAdd,
        1,
        CmOmiMapNas_shadowCmdParamsAdd_ext,
    },
    /* update */
    {
        &CmOmiMapCmds[CM_OMI_CMD_MODIFY],
        2,
        CmOmiMapNas_shadowCmdParamsUpdate,
        0,
        NULL,
    },
    /* delete */
    {
        &CmOmiMapCmds[CM_OMI_CMD_DELETE],
        1,
        CmOmiMapNas_shadowCmdParamsUpdate,
        0,
        NULL,
    }
};

const cm_omi_map_object_t CmCnmNas_shadowCfg =
{
    {"nas_shadow", CM_OMI_OBJECT_CNM_NAS_SHADOW},
    sizeof(CmOmiMapFieldsNas_shadow)/sizeof(cm_omi_map_object_field_t),
    CmOmiMapFieldsNas_shadow,
    sizeof(CmOmiMapNas_shadowCmds)/sizeof(cm_omi_map_object_cmd_t),
    CmOmiMapNas_shadowCmds
}; 

/*******************lowdata********************/

typedef enum
{
    CM_CNM_LOWDATA_STATUS_OFF = 0,
    CM_CNM_LOWDATA_STATUS_MIGRATE,
    CM_CNM_LOWDATA_STATUS_DELETE,
    CM_CNM_LOWDATA_STATUS_DEFAULT
}cm_cnm_lowdata_status_e;

typedef enum
{
    CM_CNM_LOWDATA_TIMEUNIT_SECOND=0,
    CM_CNM_LOWDATA_TIMEUNIT_MINUTE,
    CM_CNM_LOWDATA_TIMEUNIT_HOUR,
    CM_CNM_LOWDATA_TIMEUNIT_DAY
}cm_cnm_lowdata_timeunit_e;

typedef enum
{
    CM_CNM_LOWDATA_CER_ATIME=0,
    CM_CNM_LOWDATA_CER_CTIME
}cm_cnm_lowdata_cri_e;

typedef enum
{
    CM_CNM_LOWDATA_SWITCH_START=0,
    CM_CNM_LOWDATA_SWITCH_STOP,
    CM_CNM_LOWDATA_SWITCH_ALL
}cm_cnm_lowdata_switch_e;

const cm_omi_map_cfg_t CmOmiMapLowdataStatusType[] =
{
    {"off",CM_CNM_LOWDATA_STATUS_OFF},
    {"migrate",CM_CNM_LOWDATA_STATUS_MIGRATE},
    {"delete",CM_CNM_LOWDATA_STATUS_DELETE},
    {"default",CM_CNM_LOWDATA_STATUS_DEFAULT},
};

const cm_omi_map_enum_t CmOmiMapLowdataStatusTypeCfg =
{
    sizeof(CmOmiMapLowdataStatusType)/sizeof(cm_omi_map_cfg_t),
    CmOmiMapLowdataStatusType
};

const cm_omi_map_cfg_t CmOmiMapLowdataUnitType[] =
{
    {"second",CM_CNM_LOWDATA_TIMEUNIT_SECOND},
    {"minute",CM_CNM_LOWDATA_TIMEUNIT_MINUTE},
    {"hour",CM_CNM_LOWDATA_TIMEUNIT_HOUR},
    {"day",CM_CNM_LOWDATA_TIMEUNIT_DAY},
};

const cm_omi_map_enum_t CmOmiMapLowdataUnitTypeCfg =
{
    sizeof(CmOmiMapLowdataUnitType)/sizeof(cm_omi_map_cfg_t),
    CmOmiMapLowdataUnitType
};

const cm_omi_map_cfg_t CmOmiMapLowdataCriType[] =
{
    {"atime",CM_CNM_LOWDATA_CER_ATIME},
    {"ctime",CM_CNM_LOWDATA_CER_CTIME},
};

const cm_omi_map_enum_t CmOmiMapLowdataCriTypeCfg =
{
    sizeof(CmOmiMapLowdataCriType)/sizeof(cm_omi_map_cfg_t),
    CmOmiMapLowdataCriType
};

const cm_omi_map_cfg_t CmOmiMapLowdataSwitchType[] =
{
    {"start",CM_CNM_LOWDATA_SWITCH_START},
    {"stop",CM_CNM_LOWDATA_SWITCH_STOP},
    {"all",CM_CNM_LOWDATA_SWITCH_ALL},
};

const cm_omi_map_enum_t CmOmiMapLowdataSwitchTypeCfg =
{
    sizeof(CmOmiMapLowdataSwitchType)/sizeof(cm_omi_map_cfg_t),
    CmOmiMapLowdataSwitchType
};


const cm_omi_map_object_field_t CmOmiMapFieldsLowdata[] =
{
    {{"name", CM_OMI_FIELD_LOWDATA_NAME}, "-name",{CM_OMI_DATA_STRING, CM_STRING_128, {NULL}}},
    {{"nid", CM_OMI_FIELD_LOWDATA_NID}, "-nid",{CM_OMI_DATA_INT, 4, {NULL}}},
    {{"status", CM_OMI_FIELD_LOWDATA_STATUS}, "-s",{CM_OMI_DATA_ENUM, 4, {&CmOmiMapLowdataStatusTypeCfg}}},
    {{"unit", CM_OMI_FIELD_LOWDATA_UNIT}, "-u",{CM_OMI_DATA_ENUM,4, {&CmOmiMapLowdataUnitTypeCfg}}},
    {{"cri", CM_OMI_FIELD_LOWDATA_CRI}, "-c",{CM_OMI_DATA_ENUM,4, {&CmOmiMapLowdataCriTypeCfg}}},
    {{"timeout", CM_OMI_FIELD_LOWDATA_TIMEOUT}, "-tm",{CM_OMI_DATA_INT, 4, {NULL}}},
    {{"delete", CM_OMI_FIELD_LOWDATA_DELETE}, "-dt",{CM_OMI_DATA_INT, 4, {NULL}}},
    {{"switch", CM_OMI_FIELD_LOWDATA_SWITCH}, "-sw",{CM_OMI_DATA_ENUM, 4, {&CmOmiMapLowdataSwitchTypeCfg}}},
    {{"process", CM_OMI_FIELD_LOWDATA_PROCESS}, "-p",{CM_OMI_DATA_INT, 4, {NULL}}},
};

const cm_omi_map_object_field_t* CmOmiMapLowdataCmdParamsGetbatch[]=
{
    &CmOmiMapFieldsLowdata[CM_OMI_FIELD_LOWDATA_NID],
};

const cm_omi_map_object_field_t* CmOmiMapLowdataCmdParamsUpdate[]=
{
    &CmOmiMapFieldsLowdata[CM_OMI_FIELD_LOWDATA_NID],
    &CmOmiMapFieldsLowdata[CM_OMI_FIELD_LOWDATA_NAME],
};

const cm_omi_map_object_field_t* CmOmiMapLowdataCmdParamsUpdate_ext[]=
{
    &CmOmiMapFieldsLowdata[CM_OMI_FIELD_LOWDATA_STATUS],
    &CmOmiMapFieldsLowdata[CM_OMI_FIELD_LOWDATA_UNIT],
    &CmOmiMapFieldsLowdata[CM_OMI_FIELD_LOWDATA_CRI],
    &CmOmiMapFieldsLowdata[CM_OMI_FIELD_LOWDATA_TIMEOUT],
    &CmOmiMapFieldsLowdata[CM_OMI_FIELD_LOWDATA_DELETE],
    &CmOmiMapFieldsLowdata[CM_OMI_FIELD_LOWDATA_SWITCH],
};


const cm_omi_map_object_field_t* CmOmiMapLowdataCmdParamsGetbatch_ext[]=
{
    &CmOmiMapCommFields[1], /*offset*/
    &CmOmiMapCommFields[2], /*total*/
};

const cm_omi_map_object_cmd_t CmOmiMapLowdataCmds[] =
{
    /* getbatch */
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET_BATCH],
        1,
        CmOmiMapLowdataCmdParamsGetbatch,
        2,
        CmOmiMapLowdataCmdParamsGetbatch_ext,
    },
    /* count */
    {
        &CmOmiMapCmds[CM_OMI_CMD_COUNT],
        1,
        CmOmiMapLowdataCmdParamsGetbatch,
        0,
        NULL,
    },
    /* update */
    {
        &CmOmiMapCmds[CM_OMI_CMD_MODIFY],
        2,
        CmOmiMapLowdataCmdParamsUpdate,
        6,
        CmOmiMapLowdataCmdParamsUpdate_ext,
    },
    /* get */
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET],
        2,
        CmOmiMapLowdataCmdParamsUpdate,
        0,
        NULL,
    },
};

const cm_omi_map_object_t CmCnmLowdataCfg =
{
    {"lowdata", CM_OMI_OBJECT_LOWDATA},
    sizeof(CmOmiMapFieldsLowdata)/sizeof(cm_omi_map_object_field_t),
    CmOmiMapFieldsLowdata,
    sizeof(CmOmiMapLowdataCmds)/sizeof(cm_omi_map_object_cmd_t),
    CmOmiMapLowdataCmds
}; 

/*******************dirlowdata**********************/

const cm_omi_map_object_field_t CmOmiMapFieldsDirLowdata[] =
{
    {{"name", CM_OMI_FIELD_DIRLOWDATA_NAME}, "-name",{CM_OMI_DATA_STRING, CM_STRING_128, {NULL}}},
    {{"nid", CM_OMI_FIELD_DIRLOWDATA_NID}, "-nid",{CM_OMI_DATA_INT, 4, {NULL}}},
    {{"dir", CM_OMI_FIELD_DIRLOWDATA_DIR}, "-dir",{CM_OMI_DATA_STRING, CM_STRING_128, {NULL}}},
    {{"status", CM_OMI_FIELD_DIRLOWDATA_STATUS}, "-s",{CM_OMI_DATA_ENUM, 4, {&CmOmiMapLowdataStatusTypeCfg}}},
    {{"unit", CM_OMI_FIELD_DIRLOWDATA_UNIT}, "-u",{CM_OMI_DATA_ENUM,4, {&CmOmiMapLowdataUnitTypeCfg}}},
    {{"cri", CM_OMI_FIELD_DIRLOWDATA_CRI}, "-c",{CM_OMI_DATA_ENUM,4, {&CmOmiMapLowdataCriTypeCfg}}},
    {{"timeout", CM_OMI_FIELD_DIRLOWDATA_TIMEOUT}, "-tm",{CM_OMI_DATA_INT, 4, {NULL}}},
    {{"delete", CM_OMI_FIELD_DIRLOWDATA_DELETE}, "-dt",{CM_OMI_DATA_INT, 4, {NULL}}},
    {{"switch", CM_OMI_FIELD_DIRLOWDATA_SWITCH}, "-sw",{CM_OMI_DATA_ENUM, 4, {&CmOmiMapLowdataSwitchTypeCfg}}},
    {{"cluster", CM_OMI_FIELD_DIRLOWDATA_CLUSTER}, "-cluster",{CM_OMI_DATA_STRING, CM_STRING_128, {NULL}}},
};

const cm_omi_map_object_field_t* CmOmiMapDirLowdataCmdParamsGetbatch[]=
{
    &CmOmiMapFieldsDirLowdata[CM_OMI_FIELD_DIRLOWDATA_NID],
    &CmOmiMapFieldsDirLowdata[CM_OMI_FIELD_DIRLOWDATA_NAME],
};

const cm_omi_map_object_field_t* CmOmiMapDirLowdataCmdParamsGetbatch_ext[]=
{
    &CmOmiMapCommFields[1], /*offset*/
    &CmOmiMapCommFields[2], /*total*/
    &CmOmiMapFieldsDirLowdata[CM_OMI_FIELD_DIRLOWDATA_CLUSTER],
    &CmOmiMapFieldsDirLowdata[CM_OMI_FIELD_DIRLOWDATA_STATUS],
};


const cm_omi_map_object_field_t* CmOmiMapDirLowdataCmdParamsUpdate[]=
{
    &CmOmiMapFieldsDirLowdata[CM_OMI_FIELD_DIRLOWDATA_NID],
    &CmOmiMapFieldsDirLowdata[CM_OMI_FIELD_DIRLOWDATA_NAME],
    &CmOmiMapFieldsDirLowdata[CM_OMI_FIELD_DIRLOWDATA_DIR],
};

const cm_omi_map_object_field_t* CmOmiMapDirLowdataCmdParamsUpdate_ext[]=
{
    &CmOmiMapFieldsDirLowdata[CM_OMI_FIELD_DIRLOWDATA_STATUS],
    &CmOmiMapFieldsDirLowdata[CM_OMI_FIELD_DIRLOWDATA_UNIT],
    &CmOmiMapFieldsDirLowdata[CM_OMI_FIELD_DIRLOWDATA_CRI],
    &CmOmiMapFieldsDirLowdata[CM_OMI_FIELD_DIRLOWDATA_TIMEOUT],
    &CmOmiMapFieldsDirLowdata[CM_OMI_FIELD_DIRLOWDATA_DELETE],
    &CmOmiMapFieldsDirLowdata[CM_OMI_FIELD_DIRLOWDATA_SWITCH],
};

const cm_omi_map_object_field_t* CmOmiMapDirLowdataCmdParamsCount[]=
{
    &CmOmiMapFieldsDirLowdata[CM_OMI_FIELD_DIRLOWDATA_CLUSTER],
};


const cm_omi_map_object_cmd_t CmOmiMapDirLowdataCmds[] =
{
    /* getbatch */
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET_BATCH],
        2,
        CmOmiMapDirLowdataCmdParamsGetbatch,
        4,
        CmOmiMapDirLowdataCmdParamsGetbatch_ext,
    },
    /* count */
    {
        &CmOmiMapCmds[CM_OMI_CMD_COUNT],
        2,
        CmOmiMapDirLowdataCmdParamsGetbatch,
        1,
        CmOmiMapDirLowdataCmdParamsCount,
    },
    /* update */
    {
        &CmOmiMapCmds[CM_OMI_CMD_MODIFY],
        3,
        CmOmiMapDirLowdataCmdParamsUpdate,
        6,
        CmOmiMapDirLowdataCmdParamsUpdate_ext,
    },
    /* get */
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET],
        3,
        CmOmiMapDirLowdataCmdParamsUpdate,
        0,
        NULL,
    },
};

const cm_omi_map_object_t CmCnmDirLowdataCfg =
{
    {"dirlowdata", CM_OMI_OBJECT_DIRLOWDATA},
    sizeof(CmOmiMapFieldsDirLowdata)/sizeof(cm_omi_map_object_field_t),
    CmOmiMapFieldsDirLowdata,
    sizeof(CmOmiMapDirLowdataCmds)/sizeof(cm_omi_map_object_cmd_t),
    CmOmiMapDirLowdataCmds
}; 


/***********************lowdata sche************************/


const cm_omi_map_cfg_t CmOmiMapLowdataScheType[] =
{
    {"once",0},
    {"always",1},
};

const cm_omi_map_enum_t CmOmiMapEnumLowdataScheType =
{
    sizeof(CmOmiMapLowdataScheType)/sizeof(cm_omi_map_cfg_t),
    CmOmiMapLowdataScheType
};

const cm_omi_map_cfg_t CmOmiMapLowdataScheDType[] =
{
    {"everyday",0},
    {"monthday",1},
    {"weekday",2},
};

const cm_omi_map_enum_t CmOmiMapEnumLowdataScheDType =
{
    sizeof(CmOmiMapLowdataScheDType)/sizeof(cm_omi_map_cfg_t),
    CmOmiMapLowdataScheDType
};


const cm_omi_map_object_field_t CmOmiMapFieldsLowdataSche[] =
{
    {{"id", CM_OMI_FIELD_LOWDATA_SCHE_ID}, "-n",{CM_OMI_DATA_INT, 8, {NULL}}},
    {{"nid", CM_OMI_FIELD_LOWDATA_SCHE_NID}, "-nid",{CM_OMI_DATA_INT, 4, {NULL}}},
    {{"name", CM_OMI_FIELD_LOWDATA_SCHE_NAME}, "-n",{CM_OMI_DATA_STRING, CM_NAME_LEN_SNAPSHOT, {NULL}}},
    {{"expired", CM_OMI_FIELD_LOWDATA_SCHE_EXPIRED},"-expired", {CM_OMI_DATA_TIME, 4, {"^[1-9][0-9]{3,13}"}}},
    {{"type", CM_OMI_FIELD_LOWDATA_SCHE_TYPE},"-type", {CM_OMI_DATA_ENUM, 4, {&CmOmiMapEnumLowdataScheType}}},
    {{"daytype", CM_OMI_FIELD_LOWDATA_SCHE_DAYTYPE},"-dtype", {CM_OMI_DATA_ENUM, 4, {&CmOmiMapEnumLowdataScheDType}}},
    {{"minute", CM_OMI_FIELD_LOWDATA_SCHE_MINUTE}, "-minute",{CM_OMI_DATA_INT, 4, {NULL}}},
    {{"hours", CM_OMI_FIELD_LOWDATA_SCHE_HOURS}, "-hours",{CM_OMI_DATA_STRING, 64,{"[0-9,]{1,}"}}},
    {{"days", CM_OMI_FIELD_LOWDATA_SCHE_DAYS}, "-days",{CM_OMI_DATA_STRING, 64,{"[0-9,]{1,}"}}},
};

const cm_omi_map_object_field_t* CmOmiMapLowdataScheCmdParamsGetBatch[]=
{
    &CmOmiMapCommFields[1], /*offset*/
    &CmOmiMapCommFields[2], /*total*/
    &CmOmiMapCommFields[0], /*field*/
    &CmOmiMapFieldsLowdataSche[CM_OMI_FIELD_LOWDATA_SCHE_NAME],
};

const cm_omi_map_object_field_t* CmOmiMapLowdataScheCmdParamsAdd[]=
{
    &CmOmiMapFieldsLowdataSche[CM_OMI_FIELD_LOWDATA_SCHE_NAME], 
    &CmOmiMapFieldsLowdataSche[CM_OMI_FIELD_LOWDATA_SCHE_EXPIRED], 
};

const cm_omi_map_object_field_t* CmOmiMapLowdataScheCmdParamsAddOpt[]=
{
    &CmOmiMapFieldsLowdataSche[CM_OMI_FIELD_LOWDATA_SCHE_TYPE], 
    &CmOmiMapFieldsLowdataSche[CM_OMI_FIELD_LOWDATA_SCHE_DAYTYPE],
    &CmOmiMapFieldsLowdataSche[CM_OMI_FIELD_LOWDATA_SCHE_MINUTE], 
    &CmOmiMapFieldsLowdataSche[CM_OMI_FIELD_LOWDATA_SCHE_HOURS], 
    &CmOmiMapFieldsLowdataSche[CM_OMI_FIELD_LOWDATA_SCHE_DAYS], 
    &CmOmiMapFieldsLowdataSche[CM_OMI_FIELD_LOWDATA_SCHE_EXPIRED], 
};


const cm_omi_map_object_cmd_t CmOmiMapLowdataScheCmds[] =
{
    /* getbatch */
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET_BATCH],
        0,
        NULL,
        4,
        CmOmiMapLowdataScheCmdParamsGetBatch,
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
        CmOmiMapLowdataScheCmdParamsAdd,
        5,
        CmOmiMapLowdataScheCmdParamsAddOpt,
    },
    /* delete */
    {
        &CmOmiMapCmds[CM_OMI_CMD_DELETE],        
        1,
        CmOmiMapLowdataScheCmdParamsAdd,
        0,
        NULL
    },
    /* modify */
    {
        &CmOmiMapCmds[CM_OMI_CMD_MODIFY],        
        1,
        CmOmiMapLowdataScheCmdParamsAdd,
        6,
        CmOmiMapLowdataScheCmdParamsAddOpt,
    },
};

const cm_omi_map_object_t CmCnmLowdataScheCfg =
{
    {"lowdata_sche", CM_OMI_OBJECT_LOWDATA_SCHE},
    sizeof(CmOmiMapFieldsLowdataSche)/sizeof(cm_omi_map_object_field_t),
    CmOmiMapFieldsLowdataSche,
    sizeof(CmOmiMapLowdataScheCmds)/sizeof(cm_omi_map_object_cmd_t),
    CmOmiMapLowdataScheCmds
};

const cm_omi_map_object_field_t CmOmiMapFieldsNasClient[] =
{
    {{"ip", CM_OMI_FIELD_NAS_CLIENT_IP}, "-ip",{CM_OMI_DATA_STRING, CM_IP_LEN, {NULL}}},
    {{"nid", CM_OMI_FIELD_NAS_CLIENT_NID}, "-nid",{CM_OMI_DATA_INT, 4, {NULL}}},
    {{"proto", CM_OMI_FIELD_NAS_CLIENT_PROTO}, "-proto",{CM_OMI_DATA_STRING, 32, {NULL}}},
};


const cm_omi_map_object_cmd_t CmOmiMapNasClientCmds[] =
{
    /* getbatch */
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET_BATCH],
        0,
        NULL,
        2,
        CmOmiMapLowdataCmdParamsGetbatch_ext,
    },
    /* count */
    {
        &CmOmiMapCmds[CM_OMI_CMD_COUNT],        
        0,
        NULL,
        0,
        NULL,
    },
};    

const cm_omi_map_object_t CmCnmNasClientCfg =
{
    {"nas_client", CM_OMI_OBJECT_NAS_CLIENT},
    sizeof(CmOmiMapFieldsNasClient)/sizeof(cm_omi_map_object_field_t),
    CmOmiMapFieldsNasClient,
    sizeof(CmOmiMapNasClientCmds)/sizeof(cm_omi_map_object_cmd_t),
    CmOmiMapNasClientCmds
};

/*********************** lowdata_volume ****************************/


const cm_omi_map_object_field_t CmOmiMapFieldsLowdataVolume[] =
{
    {{"start", CM_OMI_FIELD_LOWDATA_VOLUME_PERCENT}, "-p",{CM_OMI_DATA_INT, 4, {NULL}}},
    {{"stop", CM_OMI_FIELD_LOWDATA_VOLUME_STOP}, "-s",{CM_OMI_DATA_INT, 4, {NULL}}},
};

const cm_omi_map_object_field_t* CmOmiMapLowdataVolumeCmdParamsAdd[]=
{
    &CmOmiMapFieldsLowdataVolume[CM_OMI_FIELD_LOWDATA_VOLUME_PERCENT], 
    &CmOmiMapFieldsLowdataVolume[CM_OMI_FIELD_LOWDATA_VOLUME_STOP], 
};

const cm_omi_map_object_cmd_t CmOmiMapLowdataVolumeCmds[] =
{
    /* add */
    {
        &CmOmiMapCmds[CM_OMI_CMD_ADD],
        0,
        NULL,
        2,
        CmOmiMapLowdataVolumeCmdParamsAdd,
    },
    /* get */
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET],
        0,
        NULL,
        0,
        NULL,
    },
};    



const cm_omi_map_object_t CmCnmLowdataVolumeCfg =
{
    {"lowdata_volume", CM_OMI_OBJECT_LOWDATA_VOLUME},
    sizeof(CmOmiMapFieldsLowdataVolume)/sizeof(cm_omi_map_object_field_t),
    CmOmiMapFieldsLowdataVolume,
    sizeof(CmOmiMapLowdataVolumeCmds)/sizeof(cm_omi_map_object_cmd_t),
    CmOmiMapLowdataVolumeCmds
}; 

/*********************** nas copy ****************************/

const cm_omi_map_object_field_t CmOmiMapFieldsNascopy[] =
{
    {{"nas", CM_OMI_FIELD_NASCOPY_NAS}, "-n",{CM_OMI_DATA_STRING, CM_NAME_LEN_NAS, {NULL}}},
    {{"nid", CM_OMI_FIELD_NASCOPY_NID}, "-nid",{CM_OMI_DATA_INT, 4, {NULL}}},
    {{"copynum", CM_OMI_FIELD_NASCOPY_NUM}, "-num",{CM_OMI_DATA_INT, 4, {NULL}}},
};

const cm_omi_map_object_field_t* CmOmiMapNascopyCmdParamsAdd[]=
{
    &CmOmiMapFieldsNascopy[CM_OMI_FIELD_NASCOPY_NAS],
    &CmOmiMapFieldsNascopy[CM_OMI_FIELD_NASCOPY_NID],
    &CmOmiMapFieldsNascopy[CM_OMI_FIELD_NASCOPY_NUM],
};

const cm_omi_map_object_field_t* CmOmiMapNascopyCmdParamsGet[]=
{
    &CmOmiMapFieldsNascopy[CM_OMI_FIELD_NASCOPY_NAS],
};

const cm_omi_map_object_cmd_t CmOmiMapNascopyCmds[] =
{   
    {
        &CmOmiMapCmds[CM_OMI_CMD_COUNT],
        1,
        CmOmiMapNascopyCmdParamsGet,
        0,
        NULL,
    },
    {
        &CmOmiMapCmds[CM_OMI_CMD_MODIFY],
        3,
        CmOmiMapNascopyCmdParamsAdd,
        0,
        NULL,
    },
};

const cm_omi_map_object_t CmCnmNascopyCfg =
{
    {"nas_copy", CM_OMI_OBJECT_NASCOPY},
    sizeof(CmOmiMapFieldsNascopy)/sizeof(cm_omi_map_object_field_t),
    CmOmiMapFieldsNascopy,
    sizeof(CmOmiMapNascopyCmds)/sizeof(cm_omi_map_object_cmd_t),
    CmOmiMapNascopyCmds
}; 
