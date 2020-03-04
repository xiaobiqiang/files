/******************************************************************************
 *          Copyright (c) 2018 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cfg_cnm_san.c
 * author     : wbn
 * create date: 2018.05.17
 * description: TODO:
 *
 *****************************************************************************/
#include "cm_omi_types.h"
#include "cm_cfg_cnm.h"
const sint8* CmOmiCheckSizeRegex = "[0-9]{1,10}[.]{0,1}[0-9]{0,3}[KMTGkmtg]{1}";

const cm_omi_map_cfg_t CmOmiMapLunSyncType[] =
{
    {"unkown",0},
    {"disk",1},
    {"poweroff",2},
    {"mirror",3},
    {"standard",4},
    {"always",5},
};

const cm_omi_map_enum_t CmOmiMapLunSyncTypeCfg =
{
    sizeof(CmOmiMapLunSyncType)/sizeof(cm_omi_map_cfg_t),
    CmOmiMapLunSyncType
};

const cm_omi_map_cfg_t CmOmiMapLunStateType[] =
{
    {"UNKNOWN",0},
    {"Active",1},
    {"Standby",2},
    {"Active->Standby",3},
    {"Standby->Active",4},
};

const cm_omi_map_enum_t CmOmiMapLunStateTypeCfg =
{
    sizeof(CmOmiMapLunStateType)/sizeof(cm_omi_map_cfg_t),
    CmOmiMapLunStateType
};

const cm_omi_map_cfg_t CmOmiMapLunQOSType[] =
{
    {"OFF",0},
    {"IOPS",1},
    {"BW",2},
};

const cm_omi_map_enum_t CmOmiMapLunQOSTypeCfg =
{
    sizeof(CmOmiMapLunQOSType)/sizeof(cm_omi_map_cfg_t),
    CmOmiMapLunQOSType
};

extern const cm_omi_map_enum_t CmOmiMapEnumPoolStatusType;

const cm_omi_map_object_field_t CmOmiMapFieldsLun[] =
{
    {{"name", CM_OMI_FIELD_LUN_NAME}, "-n",{CM_OMI_DATA_STRING, CM_NAME_LEN_LUN, {"[0-9a-zA-Z_]{1,63}"}}},
    {{"nid", CM_OMI_FIELD_LUN_NID}, "-nid",{CM_OMI_DATA_INT, 4, {NULL}}},
    {{"pool", CM_OMI_FIELD_LUN_POOL}, "-pool",{CM_OMI_DATA_STRING, CM_NAME_LEN_POOL, {"[0-9a-zA-Z_]{1,63}"}}},
    {{"space", CM_OMI_FIELD_LUN_TOTAL},"-space", {CM_OMI_DATA_STRING, 32,{"[0-9]{1,10}[.]{0,1}[0-9]{0,3}[KMTGPkmtgp]{1}"}}},
    {{"used", CM_OMI_FIELD_LUN_USED},"-used", {CM_OMI_DATA_STRING, 32, {NULL}}},
    
    {{"avail", CM_OMI_FIELD_LUN_AVAIL},"-avail", {CM_OMI_DATA_STRING, 32,{"[0-9]{1,10}[.]{0,1}[0-9]{0,3}[KMTGPkmtgp]{1}"}}},
    {{"blocksize", CM_OMI_FIELD_LUN_BLOCKSIZE}, "-bs",{CM_OMI_DATA_INT, 4, {NULL}}},
    
    {{"w_policy", CM_OMI_FIELD_LUN_WRITE_POLICY}, "-wp",{CM_OMI_DATA_ENUM, 4, {&CmOmiMapLunSyncTypeCfg}}},
    {{"access", CM_OMI_FIELD_LUN_AC_STATE}, "-as",{CM_OMI_DATA_ENUM, 4, {&CmOmiMapLunStateTypeCfg}}},
    {{"state", CM_OMI_FIELD_LUN_STATE}, "-s",{CM_OMI_DATA_ENUM, 4, {&CmOmiMapEnumPoolStatusType}}},

    {{"is_double", CM_OMI_FIELD_LUN_IS_DOUBLE}, "-double",{CM_OMI_DATA_ENUM, 1, {&CmOmiMapEnumBoolType}}},
    {{"is_compress", CM_OMI_FIELD_LUN_IS_COMPRESS}, "-comp",{CM_OMI_DATA_ENUM, 1, {&CmOmiMapEnumBoolType}}},
    {{"is_hot", CM_OMI_FIELD_LUN_IS_HOT}, "-hot",{CM_OMI_DATA_ENUM, 1, {&CmOmiMapEnumBoolType}}},
    {{"is_single", CM_OMI_FIELD_LUN_IS_SINGLE}, "-single",{CM_OMI_DATA_ENUM, 1, {&CmOmiMapEnumBoolType}}},
    {{"threshold", CM_OMI_FIELD_LUN_ALARM_THRESHOLD}, "-threshold",{CM_OMI_DATA_INT,1,{"[0-9]{1,2}"}}},
    
    {{"qos", CM_OMI_FIELD_LUN_QOS}, "-qos",{CM_OMI_DATA_ENUM, 1, {&CmOmiMapLunQOSTypeCfg}}},
    {{"qos_value", CM_OMI_FIELD_LUN_QOS_VAL}, "-qval",{CM_OMI_DATA_STRING,32,{"[0-9]{1,10}[KMTGPkmtgp]{0,1}"}}},
    {{"dedup", CM_OMI_FIELD_LUN_DEDUP}, "-dedup",{CM_OMI_DATA_ENUM, 1, {&CmOmiMapEnumBoolType}}},
};

const cm_omi_map_object_field_t* CmOmiMapLunCmdParamsGetBatch[]=
{
    &CmOmiMapFieldsLun[CM_OMI_FIELD_LUN_NID], /*node id*/
    &CmOmiMapCommFields[1], /*offset*/
    &CmOmiMapCommFields[2], /*total*/
    &CmOmiMapCommFields[0], /*field*/
};

const cm_omi_map_object_field_t* CmOmiMapLunCmdParamsAddOpt[]=
{
    &CmOmiMapFieldsLun[CM_OMI_FIELD_LUN_BLOCKSIZE], /*node id*/
    &CmOmiMapFieldsLun[CM_OMI_FIELD_LUN_WRITE_POLICY], /*node id*/
    &CmOmiMapFieldsLun[CM_OMI_FIELD_LUN_IS_DOUBLE], /*offset*/
    &CmOmiMapFieldsLun[CM_OMI_FIELD_LUN_IS_COMPRESS], /*total*/
    &CmOmiMapFieldsLun[CM_OMI_FIELD_LUN_IS_HOT],
    &CmOmiMapFieldsLun[CM_OMI_FIELD_LUN_IS_SINGLE],
    &CmOmiMapFieldsLun[CM_OMI_FIELD_LUN_ALARM_THRESHOLD],
    &CmOmiMapFieldsLun[CM_OMI_FIELD_LUN_QOS],
    &CmOmiMapFieldsLun[CM_OMI_FIELD_LUN_QOS_VAL],
    &CmOmiMapFieldsLun[CM_OMI_FIELD_LUN_DEDUP],
};

const cm_omi_map_object_field_t* CmOmiMapLunCmdParamsAdd[]=
{
    &CmOmiMapFieldsLun[CM_OMI_FIELD_LUN_NID], /*node id*/
    &CmOmiMapFieldsLun[CM_OMI_FIELD_LUN_POOL], /*pool*/
    &CmOmiMapFieldsLun[CM_OMI_FIELD_LUN_NAME], /*name*/
    &CmOmiMapFieldsLun[CM_OMI_FIELD_LUN_TOTAL], /*space*/
};

const cm_omi_map_object_field_t* CmOmiMapLunCmdParamsModifyOpt[]=
{
    &CmOmiMapFieldsLun[CM_OMI_FIELD_LUN_TOTAL], /*space*/
    &CmOmiMapFieldsLun[CM_OMI_FIELD_LUN_WRITE_POLICY], /*node id*/
    &CmOmiMapFieldsLun[CM_OMI_FIELD_LUN_IS_DOUBLE], 
    &CmOmiMapFieldsLun[CM_OMI_FIELD_LUN_IS_COMPRESS], /*total*/
    &CmOmiMapFieldsLun[CM_OMI_FIELD_LUN_IS_HOT],
    /*&CmOmiMapFieldsLun[CM_OMI_FIELD_LUN_IS_SINGLE], */
    &CmOmiMapFieldsLun[CM_OMI_FIELD_LUN_ALARM_THRESHOLD],
    &CmOmiMapFieldsLun[CM_OMI_FIELD_LUN_QOS],
    &CmOmiMapFieldsLun[CM_OMI_FIELD_LUN_QOS_VAL],
    &CmOmiMapFieldsLun[CM_OMI_FIELD_LUN_DEDUP],
};


const cm_omi_map_object_cmd_t CmOmiMapLunCmds[] =
{
    /* getbatch */
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET_BATCH],
        0,
        NULL,
        4,
        CmOmiMapLunCmdParamsGetBatch,
    },
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET],
        3,
        CmOmiMapLunCmdParamsAdd,
        0,
        NULL,
    },
    /* count */
    {
        &CmOmiMapCmds[CM_OMI_CMD_COUNT],        
        0,
        NULL,
        1,
        CmOmiMapLunCmdParamsGetBatch,
    },
    /* add */
    {
        &CmOmiMapCmds[CM_OMI_CMD_ADD],        
        4,
        CmOmiMapLunCmdParamsAdd,
        sizeof(CmOmiMapLunCmdParamsAddOpt)/sizeof(cm_omi_map_object_field_t*),
        CmOmiMapLunCmdParamsAddOpt,
    },
    /* modify */
    {
        &CmOmiMapCmds[CM_OMI_CMD_MODIFY],        
        3,
        CmOmiMapLunCmdParamsAdd,
        sizeof(CmOmiMapLunCmdParamsModifyOpt)/sizeof(cm_omi_map_object_field_t*),
        CmOmiMapLunCmdParamsModifyOpt,
    },

    /* delete */
    {
        &CmOmiMapCmds[CM_OMI_CMD_DELETE],        
        3,
        CmOmiMapLunCmdParamsAdd,
        0,
        NULL
    },
};


const cm_omi_map_object_t CmCnmLunCfg =
{
    {"lun", CM_OMI_OBJECT_LUN},
    sizeof(CmOmiMapFieldsLun)/sizeof(cm_omi_map_object_field_t),
    CmOmiMapFieldsLun,
    sizeof(CmOmiMapLunCmds)/sizeof(cm_omi_map_object_cmd_t),
    CmOmiMapLunCmds
}; 


const cm_omi_map_cfg_t CmOmiMapHgAct[] =
{
    {"add",0},
    {"remove",1},
};

const cm_omi_map_enum_t CmOmiMapEnumHgActType =
{
    sizeof(CmOmiMapHgAct)/sizeof(cm_omi_map_cfg_t),
    CmOmiMapHgAct
};

const cm_omi_map_cfg_t CmOmiMapHgType[] =
{
    {"host",0},
    {"target",1},
};

const cm_omi_map_enum_t CmOmiMapEnumHgType =
{
    sizeof(CmOmiMapHgType)/sizeof(cm_omi_map_cfg_t),
    CmOmiMapHgType
};

const cm_omi_map_object_field_t CmOmiMapFieldsHostGroup[] =
{
    {{"name", CM_OMI_FIELD_HG_NAME}, "-n",{CM_OMI_DATA_STRING, CM_NAME_LEN_HOSTGROUP, {"[0-9a-zA-Z_]{1,63}"}}},
    {{"nid", CM_OMI_FIELD_HG_NID}, "-nid",{CM_OMI_DATA_INT, 4, {NULL}}},
    {{"members", CM_OMI_FIELD_HG_MEMBER}, "-mbs",{CM_OMI_DATA_STRING, 1024, {"[^;|]{1,}"}}},
    {{"action", CM_OMI_FIELD_HG_ACT},"-act", {CM_OMI_DATA_ENUM,4,{&CmOmiMapEnumHgActType}}},
    {{"type", CM_OMI_FIELD_HG_TYPE},"-type", {CM_OMI_DATA_ENUM,4,{&CmOmiMapEnumHgType}}},
};

const cm_omi_map_object_field_t* CmOmiMapHostGroupCmdParamsGetBatch[]=
{
    &CmOmiMapFieldsHostGroup[CM_OMI_FIELD_HG_NAME],
    &CmOmiMapCommFields[1], /*offset*/
    &CmOmiMapCommFields[2], /*total*/
    &CmOmiMapCommFields[0], /*field*/
};

const cm_omi_map_object_field_t* CmOmiMapHostGroupCmdParamsUpdate[]=
{
    &CmOmiMapFieldsHostGroup[CM_OMI_FIELD_HG_TYPE],
    &CmOmiMapFieldsHostGroup[CM_OMI_FIELD_HG_NAME],
    &CmOmiMapFieldsHostGroup[CM_OMI_FIELD_HG_ACT],
    &CmOmiMapFieldsHostGroup[CM_OMI_FIELD_HG_MEMBER],
};


const cm_omi_map_object_cmd_t CmOmiMapHostGroupCmds[] =
{
    /* getbatch */
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET_BATCH],
        1,
        CmOmiMapHostGroupCmdParamsUpdate,
        4,
        CmOmiMapHostGroupCmdParamsGetBatch,
    },
    /* count */
    {
        &CmOmiMapCmds[CM_OMI_CMD_COUNT],        
        1,
        CmOmiMapHostGroupCmdParamsUpdate,
        1,
        CmOmiMapHostGroupCmdParamsGetBatch,
    },
    /* add */
    {
        &CmOmiMapCmds[CM_OMI_CMD_ADD],        
        2,
        CmOmiMapHostGroupCmdParamsUpdate,
        0,
        NULL,
        
    },
    /* modify */
    {
        &CmOmiMapCmds[CM_OMI_CMD_MODIFY],        
        4,
        CmOmiMapHostGroupCmdParamsUpdate,
        0,
        NULL,
    },

    /* delete */
    {
        &CmOmiMapCmds[CM_OMI_CMD_DELETE],        
        2,
        CmOmiMapHostGroupCmdParamsUpdate,
        0,
        NULL
    },
};


const cm_omi_map_object_t CmCnmHostGroupCfg =
{
    {"group", CM_OMI_OBJECT_HOSTGROUP},
    sizeof(CmOmiMapFieldsHostGroup)/sizeof(cm_omi_map_object_field_t),
    CmOmiMapFieldsHostGroup,
    sizeof(CmOmiMapHostGroupCmds)/sizeof(cm_omi_map_object_cmd_t),
    CmOmiMapHostGroupCmds
}; 


const cm_omi_map_cfg_t CmOmiMapProtocolType[] =
{
    {"UNKNOWN",0},
    {"ISCSI",1},
    {"FC",2},
    {"IB",3},
};

const cm_omi_map_enum_t CmOmiMapEnumProtocolType =
{
    sizeof(CmOmiMapProtocolType)/sizeof(cm_omi_map_cfg_t),
    CmOmiMapProtocolType
};

const cm_omi_map_cfg_t CmOmiMapProtocolInType[] =
{
    {"UNKNOWN",0},
    {"iSCSI",1},
    {"Fibre Channel",2},
    {"IB",3},
};

const cm_omi_map_enum_t CmOmiMapEnumProtocolInType =
{
    sizeof(CmOmiMapProtocolInType)/sizeof(cm_omi_map_cfg_t),
    CmOmiMapProtocolInType
};

const cm_omi_map_object_field_t CmOmiMapFieldsTarget[] =
{
    {{"name", CM_OMI_FIELD_TGT_NAME}, "-n",{CM_OMI_DATA_STRING, CM_NAME_LEN_TARGET, {"[^;|]{1,}"}}},
    {{"nid", CM_OMI_FIELD_TGT_NID}, "-nid",{CM_OMI_DATA_INT, 4, {NULL}}},
    {{"provider", CM_OMI_FIELD_TGT_PROVIDER}, "-pro",{CM_OMI_DATA_STRING, 64, {"[^;|]{1,}"}}},
    {{"alias", CM_OMI_FIELD_TGT_ALIAS}, "-al",{CM_OMI_DATA_STRING, 64, {"[^;|]{1,}"}}},
    {{"protocol", CM_OMI_FIELD_TGT_PROTOCOL}, "-p",{CM_OMI_DATA_ENUM, 4, {&CmOmiMapEnumProtocolType}}},
    {{"status", CM_OMI_FIELD_TGT_STATUS}, "-st",{CM_OMI_DATA_ENUM, 4, {&CmOmiMapEnumPoolStatusType}}},
    {{"sessions", CM_OMI_FIELD_TGT_SESSIONS},"-act", {CM_OMI_DATA_INT,4,{NULL}}},
    {{"initiator", CM_OMI_FIELD_TGT_INITIATOR},"-initiator", {CM_OMI_DATA_STRING,1024,{"[^;|]{1,}"}}},
};

const cm_omi_map_object_field_t* CmOmiMapTargetCmdParamsGetBatch[]=
{
    &CmOmiMapFieldsTarget[CM_OMI_FIELD_TGT_NID],
    &CmOmiMapFieldsTarget[CM_OMI_FIELD_TGT_NAME],
    &CmOmiMapCommFields[1], /*offset*/
    &CmOmiMapCommFields[2], /*total*/
    &CmOmiMapCommFields[0], /*field*/
};
const cm_omi_map_object_field_t* CmOmiMapTargetCmdParamsAdd[]=
{
    &CmOmiMapFieldsTarget[CM_OMI_FIELD_TGT_PROTOCOL],
};

const cm_omi_map_object_cmd_t CmOmiMapTargetCmds[] =
{
    /* getbatch */
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET_BATCH],
        0,
        NULL,
        5,
        CmOmiMapTargetCmdParamsGetBatch,
    },
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET],
        1,
        CmOmiMapTargetCmdParamsGetBatch,
        0,
        NULL,
    },
    /* count */
    {
        &CmOmiMapCmds[CM_OMI_CMD_COUNT],        
        0,
        NULL,
        2,
        CmOmiMapTargetCmdParamsGetBatch,
    },
    /* add */
    {
        &CmOmiMapCmds[CM_OMI_CMD_ADD],        
        1,
        CmOmiMapTargetCmdParamsAdd,
        0,
        NULL,
        
    },
    /* delete */
    {
        &CmOmiMapCmds[CM_OMI_CMD_DELETE],        
        2,
        CmOmiMapTargetCmdParamsGetBatch,
        0,
        NULL,
        
    },
};

const cm_omi_map_object_t CmCnmTargetCfg =
{
    {"port", CM_OMI_OBJECT_TARGET},
    sizeof(CmOmiMapFieldsTarget)/sizeof(cm_omi_map_object_field_t),
    CmOmiMapFieldsTarget,
    sizeof(CmOmiMapTargetCmds)/sizeof(cm_omi_map_object_cmd_t),
    CmOmiMapTargetCmds
}; 

const cm_omi_map_object_field_t CmOmiMapFieldsLunmap[] =
{
    {{"lun", CM_OMI_FIELD_LUNMAP_LUN}, "-lun",{CM_OMI_DATA_STRING, 256, {"[^;|]{1,}"}}},
    {{"nid", CM_OMI_FIELD_LUNMAP_NID}, "-nid",{CM_OMI_DATA_INT, 4, {NULL}}},
    {{"hostgroup", CM_OMI_FIELD_LUNMAP_HG}, "-hg",{CM_OMI_DATA_STRING, 64, {"[^;|]{0,}"}}},
    {{"targetgroup", CM_OMI_FIELD_LUNMAP_TG}, "-tg",{CM_OMI_DATA_STRING, 64,{"[^;|]{0,}"}}},
    {{"lunid", CM_OMI_FIELD_LUNMAP_LID}, "-lid",{CM_OMI_DATA_INT, 4, {NULL}}},
};

const cm_omi_map_object_field_t* CmOmiMapLunmapCmdParamsGetBatch[]=
{
    &CmOmiMapFieldsLunmap[CM_OMI_FIELD_LUNMAP_LUN],
    &CmOmiMapFieldsLunmap[CM_OMI_FIELD_LUNMAP_HG],
    &CmOmiMapFieldsLunmap[CM_OMI_FIELD_LUNMAP_TG],
    &CmOmiMapCommFields[1], /*offset*/
    &CmOmiMapCommFields[2], /*total*/
    &CmOmiMapCommFields[0], /*field*/
};

const cm_omi_map_object_cmd_t CmOmiMapLunmapCmds[] =
{
    /* getbatch */
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET_BATCH],
        0,
        NULL,
        6,
        CmOmiMapLunmapCmdParamsGetBatch,
    },
    /* count */
    {
        &CmOmiMapCmds[CM_OMI_CMD_COUNT],        
        0,
        NULL,
        3,
        CmOmiMapLunmapCmdParamsGetBatch,
    },
    /* add */
    {
        &CmOmiMapCmds[CM_OMI_CMD_ADD],        
        1,
        CmOmiMapLunmapCmdParamsGetBatch,
        2,
        &CmOmiMapLunmapCmdParamsGetBatch[1],
    },
    /* update */
    {
        &CmOmiMapCmds[CM_OMI_CMD_MODIFY],
        0,
        NULL,
        0,
        NULL,
    },
    /* delete */
    {
        &CmOmiMapCmds[CM_OMI_CMD_DELETE],        
        2,
        CmOmiMapLunmapCmdParamsGetBatch,
        0,
        NULL
    },
};

const cm_omi_map_object_t CmCnmLunmapCfg =
{
    {"lunmap", CM_OMI_OBJECT_LUNMAP},
    sizeof(CmOmiMapFieldsLunmap)/sizeof(cm_omi_map_object_field_t),
    CmOmiMapFieldsLunmap,
    sizeof(CmOmiMapLunmapCmds)/sizeof(cm_omi_map_object_cmd_t),
    CmOmiMapLunmapCmds
}; 




// =========================================================================================
const cm_omi_map_cfg_t CmOmiMapInsActions[] = 
{
    {"SYNC", 0},
    {"ASYNC", 1},
};

const cm_omi_map_enum_t CmOmiMapEnumLunInsActions = 
{
    sizeof(CmOmiMapInsActions)/sizeof(cm_omi_map_cfg_t),
    CmOmiMapInsActions
};

const cm_omi_map_cfg_t CmOmiMapUpdActions[] = 
{
    {"SET_TIME", 0},
    {"FULL_SYNC", 1},
    {"UPDATE_SYNC", 2},
    {"REV_FULL_SYNC", 3},   // reverse_full_sync
    {"REV_UPDATE_SYNC", 4}  // reverse_update_sync
};

const cm_omi_map_enum_t CmOmiMapEnumLunUpdActions = 
{
    sizeof(CmOmiMapUpdActions)/sizeof(cm_omi_map_cfg_t),
    CmOmiMapUpdActions
}; 

extern const cm_omi_map_enum_t CmOmiMapEnumSwitchType;

const cm_omi_map_object_field_t CmOmiMapFieldsLunMirror[] =
{
    {{"ins_act", CM_OMI_FIELD_LUNMIRROR_INSERT_ACTION}, "-ia", {CM_OMI_DATA_ENUM, 4, {&CmOmiMapEnumLunInsActions}}},
    {{"master_nid", CM_OMI_FIELD_LUNMIRROR_NID_MASTER}, "-nid",{CM_OMI_DATA_INT, 4, {NULL}}},
    {{"path", CM_OMI_FIELD_LUNMIRROR_PATH}, "-path",{CM_OMI_DATA_STRING, 256, {"[^;|]{1,}"}}},
    {{"slave_nid", CM_OMI_FIELD_LUNMIRROR_NID_SLAVE}, "-snid",{CM_OMI_DATA_INT, 4, {NULL}}},
    {{"nic", CM_OMI_FIELD_LUNMIRROR_NIC}, "-nic",{CM_OMI_DATA_STRING, 64,  {"[0-9a-zA-Z]{3,63}"}}},
    {{"rdcip_master", CM_OMI_FIELD_LUNMIRROR_RDC_IP_MASTER}, "-mip",{CM_OMI_DATA_STRING, 32,{"([0-9]{1,3}\.){3}[0-9]{1,3}"}}},
    {{"rdcip_slave", CM_OMI_FIELD_LUNMIRROR_RDC_IP_SLAVE}, "-sip",{CM_OMI_DATA_STRING, 32, {"([0-9]{1,3}\.){3}[0-9]{1,3}"}}},
    {{"sync", CM_OMI_FIELD_LUNMIRROR_SYNC_ONCE}, "-sy",{CM_OMI_DATA_ENUM, 4, {&CmOmiMapEnumSwitchType}}},
    {{"master_netmask", CM_OMI_FIELD_LUNMIRROR_NETMASK_MASTER}, "-mm",{CM_OMI_DATA_STRING, 32, {NULL}}},
    {{"slave_netmask", CM_OMI_FIELD_LUNMIRROR_NETMASK_SLAVE}, "-sm",{CM_OMI_DATA_STRING, 32, {NULL}}},
};

const cm_omi_map_object_field_t* CmOmiMapLunMirrorCmdParamsGetBatch[]=
{
    &CmOmiMapFieldsLunMirror[CM_OMI_FIELD_LUNMIRROR_NID_MASTER],
    &CmOmiMapCommFields[1], /*offset*/
    &CmOmiMapCommFields[2], /*total*/
    &CmOmiMapCommFields[0], /*field*/
};

const cm_omi_map_object_field_t* CmOmiMapLunMirrorCmdParamsAdd[]=
{
    &CmOmiMapFieldsLunMirror[CM_OMI_FIELD_LUNMIRROR_INSERT_ACTION],
    &CmOmiMapFieldsLunMirror[CM_OMI_FIELD_LUNMIRROR_NID_MASTER],
    &CmOmiMapFieldsLunMirror[CM_OMI_FIELD_LUNMIRROR_PATH],    
    &CmOmiMapFieldsLunMirror[CM_OMI_FIELD_LUNMIRROR_NID_SLAVE],
    &CmOmiMapFieldsLunMirror[CM_OMI_FIELD_LUNMIRROR_NIC],
};

const cm_omi_map_object_field_t* CmOmiMapLunMirrorCmdParamsAddOpt[]=
{
    &CmOmiMapFieldsLunMirror[CM_OMI_FIELD_LUNMIRROR_RDC_IP_MASTER],
    &CmOmiMapFieldsLunMirror[CM_OMI_FIELD_LUNMIRROR_RDC_IP_SLAVE],
    &CmOmiMapFieldsLunMirror[CM_OMI_FIELD_LUNMIRROR_SYNC_ONCE],
    &CmOmiMapFieldsLunMirror[CM_OMI_FIELD_LUNMIRROR_NETMASK_MASTER],
    &CmOmiMapFieldsLunMirror[CM_OMI_FIELD_LUNMIRROR_NETMASK_SLAVE],
};

const cm_omi_map_object_field_t* CmOmiMapLunMirrorCmdParamsDelete[]=
{
    &CmOmiMapFieldsLunMirror[CM_OMI_FIELD_LUNMIRROR_NID_MASTER],
    &CmOmiMapFieldsLunMirror[CM_OMI_FIELD_LUNMIRROR_PATH],
};


const cm_omi_map_object_cmd_t CmOmiMapLunMirrorCmds[] =
{
    /* getbatch */
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET_BATCH],
        0,
        NULL,
        sizeof(CmOmiMapLunMirrorCmdParamsGetBatch)/sizeof(cm_omi_map_object_field_t*),
        CmOmiMapLunMirrorCmdParamsGetBatch,
    },
    /* count */
    {
        &CmOmiMapCmds[CM_OMI_CMD_COUNT],        
        0,
        NULL,
        1,
        CmOmiMapLunMirrorCmdParamsGetBatch,
    },
    /* add */
    {
        &CmOmiMapCmds[CM_OMI_CMD_ADD],        
        sizeof(CmOmiMapLunMirrorCmdParamsAdd)/sizeof(cm_omi_map_object_field_t*),
        CmOmiMapLunMirrorCmdParamsAdd,
        sizeof(CmOmiMapLunMirrorCmdParamsAddOpt)/sizeof(cm_omi_map_object_field_t*),
        CmOmiMapLunMirrorCmdParamsAddOpt,
    },
    /* delete */
    {
        &CmOmiMapCmds[CM_OMI_CMD_DELETE],        
        sizeof(CmOmiMapLunMirrorCmdParamsDelete)/sizeof(cm_omi_map_object_field_t*),
        CmOmiMapLunMirrorCmdParamsDelete,
        0,
        NULL
    },
};


const cm_omi_map_object_t CmCnmLunMirrorCfg =
{
    {"lun_duallive", CM_OMI_OBJECT_LUN_MIRROR},
    sizeof(CmOmiMapFieldsLunMirror)/sizeof(cm_omi_map_object_field_t),
    CmOmiMapFieldsLunMirror,
    sizeof(CmOmiMapLunMirrorCmds)/sizeof(cm_omi_map_object_cmd_t),
    CmOmiMapLunMirrorCmds
}; 

//========================================================================================================================
/* "((25[0-5]|2[0-4]\d|((1\d{2})||([0-9]?\d)))\.){3}(25[0-5]|2[0-4]\d|((1\d{2})|([1-9]?\d)))" */
const cm_omi_map_object_field_t CmOmiMapFieldsLunBackup[] =
{
    {{"ins_act", CM_OMI_FIELD_LUNBACKUP_INSERT_ACTION}, "-ia", {CM_OMI_DATA_ENUM, 4, {&CmOmiMapEnumLunInsActions}}},
    {{"nid", CM_OMI_FIELD_LUNBACKUP_NID_MASTER}, "-nid",{CM_OMI_DATA_INT, 4, {NULL}}},
    {{"upd_act", CM_OMI_FIELD_LUNBACKUP_UPDATE_ACTION}, "-ua", {CM_OMI_DATA_ENUM, 4, {&CmOmiMapEnumLunUpdActions}}},
    {{"interval", CM_OMI_FIELD_LUNBACKUP_ASYNC_TIME}, "-at", {CM_OMI_DATA_INT, 4, {NULL}}},
    {{"path", CM_OMI_FIELD_LUNBACKUP_PATH}, "-path",{CM_OMI_DATA_STRING, 256, {"[^;|]{1,}"}}},  
    {{"dest_ip", CM_OMI_FIELD_LUNBACKUP_IP_SLAVE}, "-destip",{CM_OMI_DATA_STRING, 32, {"([0-9]{1,3}\.){3}[0-9]{1,3}"}}},
    {{"nic", CM_OMI_FIELD_LUNBACKUP_NIC}, "-nic",{CM_OMI_DATA_STRING, 64,  {"[0-9a-zA-Z]{3,63}"}}},
    {{"rdcip_master", CM_OMI_FIELD_LUNBACKUP_RDC_IP_MASTER}, "-mip",{CM_OMI_DATA_STRING, 32,{"([0-9]{1,3}\.){3}[0-9]{1,3}"}}},
    {{"rdcip_slave", CM_OMI_FIELD_LUNBACKUP_RDC_IP_SLAVE}, "-sip",{CM_OMI_DATA_STRING, 32, {"([0-9]{1,3}\.){3}[0-9]{1,3}"}}},
    {{"dest_path", CM_OMI_FIELD_LUNBACKUP_PATH_SLAVE}, "-dest",{CM_OMI_DATA_STRING, 256, {"[^;|]{1,}"}}},
    {{"sync", CM_OMI_FIELD_LUNBACKUP_SYNC_ONCE}, "-sy",{CM_OMI_DATA_ENUM, 4, {&CmOmiMapEnumSwitchType}}},
    {{"master_netmask", CM_OMI_FIELD_LUNBACKUP_NETMASK_MASTER}, "-mm",{CM_OMI_DATA_STRING, 32, {NULL}}},
    {{"slave_netmask", CM_OMI_FIELD_LUNBACKUP_NETMASK_SLAVE}, "-sm",{CM_OMI_DATA_STRING, 32, {NULL}}},
};

const cm_omi_map_object_field_t* CmOmiMapLunBackupCmdParamsGetBatch[]=
{
    &CmOmiMapFieldsLunBackup[CM_OMI_FIELD_LUNBACKUP_NID_MASTER],
    &CmOmiMapCommFields[1], /*offset*/
    &CmOmiMapCommFields[2], /*total*/
    &CmOmiMapCommFields[0], /*field*/
};

const cm_omi_map_object_field_t* CmOmiMapLunBackupCmdParamsAdd[]=
{
    &CmOmiMapFieldsLunBackup[CM_OMI_FIELD_LUNBACKUP_INSERT_ACTION],
    &CmOmiMapFieldsLunBackup[CM_OMI_FIELD_LUNBACKUP_NID_MASTER],
    &CmOmiMapFieldsLunBackup[CM_OMI_FIELD_LUNBACKUP_PATH],    
    &CmOmiMapFieldsLunBackup[CM_OMI_FIELD_LUNBACKUP_IP_SLAVE],
    &CmOmiMapFieldsLunBackup[CM_OMI_FIELD_LUNBACKUP_PATH_SLAVE],
    &CmOmiMapFieldsLunBackup[CM_OMI_FIELD_LUNBACKUP_NIC],
};

const cm_omi_map_object_field_t* CmOmiMapLunBackupCmdParamsAddOpt[]=
{
    &CmOmiMapFieldsLunBackup[CM_OMI_FIELD_LUNBACKUP_RDC_IP_MASTER],
    &CmOmiMapFieldsLunBackup[CM_OMI_FIELD_LUNBACKUP_RDC_IP_SLAVE],
    &CmOmiMapFieldsLunBackup[CM_OMI_FIELD_LUNBACKUP_SYNC_ONCE],
    &CmOmiMapFieldsLunBackup[CM_OMI_FIELD_LUNBACKUP_NETMASK_MASTER],
    &CmOmiMapFieldsLunBackup[CM_OMI_FIELD_LUNBACKUP_NETMASK_SLAVE],
};

const cm_omi_map_object_field_t* CmOmiMapLunBackupCmdParamsModify[] =
{
    &CmOmiMapFieldsLunBackup[CM_OMI_FIELD_LUNBACKUP_UPDATE_ACTION],
    &CmOmiMapFieldsLunBackup[CM_OMI_FIELD_LUNBACKUP_NID_MASTER],
    &CmOmiMapFieldsLunBackup[CM_OMI_FIELD_LUNBACKUP_PATH],
    &CmOmiMapFieldsLunBackup[CM_OMI_FIELD_LUNBACKUP_IP_SLAVE],
    &CmOmiMapFieldsLunBackup[CM_OMI_FIELD_LUNBACKUP_PATH_SLAVE],
};

const cm_omi_map_object_field_t* CmOmiMapLunBackupCmdParamsModifyOpt[] =
{
    &CmOmiMapFieldsLunBackup[CM_OMI_FIELD_LUNBACKUP_ASYNC_TIME],
};

const cm_omi_map_object_field_t* CmOmiMapLunBackupCmdParamsDelete[] =
{
    &CmOmiMapFieldsLunBackup[CM_OMI_FIELD_LUNBACKUP_NID_MASTER],
    &CmOmiMapFieldsLunBackup[CM_OMI_FIELD_LUNBACKUP_PATH],    
    &CmOmiMapFieldsLunBackup[CM_OMI_FIELD_LUNBACKUP_IP_SLAVE],
    &CmOmiMapFieldsLunBackup[CM_OMI_FIELD_LUNBACKUP_PATH_SLAVE],
};


const cm_omi_map_object_cmd_t CmOmiMapLunBackupCmds[] =
{
    /* getbatch */
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET_BATCH],
        0,
        NULL,
        sizeof(CmOmiMapLunBackupCmdParamsGetBatch)/sizeof(cm_omi_map_object_field_t*),
        CmOmiMapLunBackupCmdParamsGetBatch,
    },
    /* count */
    {
        &CmOmiMapCmds[CM_OMI_CMD_COUNT],        
        0,
        NULL,
        sizeof(CmOmiMapLunBackupCmdParamsGetBatch)/sizeof(cm_omi_map_object_field_t*),
        CmOmiMapLunBackupCmdParamsGetBatch,
    },
    /* add */
    {
        &CmOmiMapCmds[CM_OMI_CMD_ADD],        
        sizeof(CmOmiMapLunBackupCmdParamsAdd)/sizeof(cm_omi_map_object_field_t*),
        CmOmiMapLunBackupCmdParamsAdd,
        sizeof(CmOmiMapLunBackupCmdParamsAddOpt)/sizeof(cm_omi_map_object_field_t*),
        CmOmiMapLunBackupCmdParamsAddOpt,
    },
    /* modify */
    {
        &CmOmiMapCmds[CM_OMI_CMD_MODIFY],
        sizeof(CmOmiMapLunBackupCmdParamsModify)/sizeof(cm_omi_map_object_field_t*),
        CmOmiMapLunBackupCmdParamsModify,
        sizeof(CmOmiMapLunBackupCmdParamsModifyOpt)/sizeof(cm_omi_map_object_field_t*),
        CmOmiMapLunBackupCmdParamsModifyOpt
    },
    /* delete */
    {
        &CmOmiMapCmds[CM_OMI_CMD_DELETE],        
        sizeof(CmOmiMapLunBackupCmdParamsDelete)/sizeof(cm_omi_map_object_field_t*),
        CmOmiMapLunBackupCmdParamsDelete,
        0,
        NULL
    },
};


const cm_omi_map_object_t CmCnmLunBackupCfg =
{
    {"lun_backup", CM_OMI_OBJECT_LUN_BACKUP},
    sizeof(CmOmiMapFieldsLunBackup)/sizeof(cm_omi_map_object_field_t),
    CmOmiMapFieldsLunBackup,
    sizeof(CmOmiMapLunBackupCmds)/sizeof(cm_omi_map_object_cmd_t),
    CmOmiMapLunBackupCmds
}; 
