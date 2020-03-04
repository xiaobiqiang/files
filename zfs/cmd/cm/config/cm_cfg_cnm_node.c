/******************************************************************************
 *          Copyright (c) 2017 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cfg_cnm_node.c
 * author     : wbn
 * create date: 2017Äê11ÔÂ13ÈÕ
 * description: TODO:
 *
 *****************************************************************************/
#include "cm_omi_types.h"
#include "cm_cfg_cnm.h"


const cm_omi_map_cfg_t CmOmiMapDevType[] =
{
    {"SBB", 0},
    {"AIC", 1},
    {"ENC", 2},
    {"unknown", 3},
};

const cm_omi_map_enum_t CmOmiMapEnumDevType =
{
    sizeof(CmOmiMapDevType) / sizeof(cm_omi_map_cfg_t),
    CmOmiMapDevType
};

const cm_omi_map_cfg_t CmOmiMapNodeState[] =
{
    {"unknown", CM_NODE_STATE_UNKNOW},
    {"start", CM_NODE_STATE_START},
    {"normal", CM_NODE_STATE_NORMAL},
    {"degrade", CM_NODE_STATE_DEGRAGE},
    {"shutdown", CM_NODE_STATE_SHUTDOWN},
    {"upgrade", CM_NODE_STATE_UPGRADE},
    {"offline", CM_NODE_STATE_OFFLINE},
};

const cm_omi_map_enum_t CmOmiMapEnumNodeState =
{
    sizeof(CmOmiMapNodeState) / sizeof(cm_omi_map_cfg_t),
    CmOmiMapNodeState
};

const cm_omi_map_object_field_t CmOmiMapNodeFields[] =
{
    {{"id", CM_OMI_FIELD_NODE_ID}, "-i", {CM_OMI_DATA_STRING, 32, {"[0-9]{1,20}"}}},
    {{"name", CM_OMI_FIELD_NODE_NAME}, "-n", {CM_OMI_DATA_STRING, 64, {"[0-9a-zA-Z_]{1,63}"}}},
    {{"ver", CM_OMI_FIELD_NODE_VERSION}, "-ver", {CM_OMI_DATA_STRING, CM_VERSION_LEN, {NULL}}},
    {{"sn", CM_OMI_FIELD_NODE_SN}, "-sn", {CM_OMI_DATA_STRING, CM_SN_LEN, {NULL}}},
    {{"ip", CM_OMI_FIELD_NODE_IP}, "-ip", {CM_OMI_DATA_STRING, CM_IP_LEN, {IP_FORMAT}}},
    {{"frame", CM_OMI_FIELD_NODE_FRAME}, "-fr", {CM_OMI_DATA_STRING, CM_NAME_LEN, {NULL}}},
    {{"dev_type", CM_OMI_FIELD_NODE_DEV_TYPE}, "-dev", {CM_OMI_DATA_ENUM, 4, {&CmOmiMapEnumDevType}}},
    {{"slot", CM_OMI_FIELD_NODE_SLOT}, "-sl", {CM_OMI_DATA_INT, 4, {NULL}}},
    {{"ram", CM_OMI_FIELD_NODE_RAM}, "-ram", {CM_OMI_DATA_INT, 4, {NULL}}},
    {{"state", CM_OMI_FIELD_NODE_STATE}, "-state", {CM_OMI_DATA_ENUM, 4, {&CmOmiMapEnumNodeState}}},
    {{"sbbid", CM_OMI_FIELD_NODE_SBBID}, "-sid", {CM_OMI_DATA_INT, 4, {NULL}}},
    {{"username", CM_OMI_FIELD_NODE_IPMI_USER}, "-u", {CM_OMI_DATA_STRING, CM_NAME_LEN, {NULL}}},
    {{"password", CM_OMI_FIELD_NODE_IPMI_PASSWD}, "-p", {CM_OMI_DATA_STRING, CM_NAME_LEN, {NULL}}},
};

const cm_omi_map_object_field_t* CmOmiMapNodeCmdParamsGetBatch[] =
{
    &CmOmiMapCommFields[0], /*fields*/
    &CmOmiMapCommFields[1], /*from*/
    &CmOmiMapCommFields[2], /*count*/
};
const cm_omi_map_object_field_t* CmOmiMapNodeCmdParamsGet[] =
{
    &CmOmiMapNodeFields[0], /*id*/
};

const cm_omi_map_object_field_t* CmOmiMapNodeCmdParamsAdd[] =
{
    &CmOmiMapNodeFields[CM_OMI_FIELD_NODE_IP], /*ip*/
};
const cm_omi_map_object_field_t* CmOmiMapNodeCmdParamsAddOpt[] =
{
    &CmOmiMapNodeFields[CM_OMI_FIELD_NODE_SBBID], /*sbbid*/
};

const cm_omi_map_object_field_t* CmOmiMapNodeCmdParamsPowerOn[] =
{
    &CmOmiMapNodeFields[CM_OMI_FIELD_NODE_ID], /*nid*/
    &CmOmiMapNodeFields[CM_OMI_FIELD_NODE_IPMI_USER], /*user*/
};

const cm_omi_map_object_field_t* CmOmiMapNodeCmdParamsPowerOnOpt[] =
{
    &CmOmiMapNodeFields[CM_OMI_FIELD_NODE_IPMI_PASSWD], /*passwd*/
};


const cm_omi_map_object_cmd_t CmOmiMapNodeCmds[] =
{
    /* getbatch */
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET_BATCH],
        0,
        NULL,
        sizeof(CmOmiMapNodeCmdParamsGetBatch) / sizeof(cm_omi_map_object_field_t*),
        CmOmiMapNodeCmdParamsGetBatch
    },
    /* get */
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET],
        sizeof(CmOmiMapNodeCmdParamsGet) / sizeof(cm_omi_map_object_field_t*),
        CmOmiMapNodeCmdParamsGet,
        0,
        NULL
    },
    /* count */
    {
        &CmOmiMapCmds[CM_OMI_CMD_COUNT],
        0,
        NULL,
        0,
        NULL
    },
    /* add */
    {
        &CmOmiMapCmds[CM_OMI_CMD_ADD],
        1,
        CmOmiMapNodeCmdParamsAdd,
        1,
        CmOmiMapNodeCmdParamsAddOpt
    },
    {
        &CmOmiMapCmds[CM_OMI_CMD_DELETE],
        sizeof(CmOmiMapNodeCmdParamsGet) / sizeof(cm_omi_map_object_field_t*),
        CmOmiMapNodeCmdParamsGet,
        0,
        NULL
    },
    /* power on */
    {
        &CmOmiMapCmds[CM_OMI_CMD_ON],
        sizeof(CmOmiMapNodeCmdParamsPowerOn) / sizeof(cm_omi_map_object_field_t*),
        CmOmiMapNodeCmdParamsPowerOn,
        1,
        CmOmiMapNodeCmdParamsPowerOnOpt
    },
    /* power off */
    {
        &CmOmiMapCmds[CM_OMI_CMD_OFF],
        sizeof(CmOmiMapNodeCmdParamsGet) / sizeof(cm_omi_map_object_field_t*),
        CmOmiMapNodeCmdParamsGet,
        0,
        NULL
    },
    /* reboot */
    {
        &CmOmiMapCmds[CM_OMI_CMD_REBOOT],
        sizeof(CmOmiMapNodeCmdParamsGet) / sizeof(cm_omi_map_object_field_t*),
        CmOmiMapNodeCmdParamsGet,
        0,
        NULL
    },
};

const cm_omi_map_object_t CmCnmNodeCfg =
{
    {"node", CM_OMI_OBJECT_NODE},
    sizeof(CmOmiMapNodeFields) / sizeof(cm_omi_map_object_field_t),
    CmOmiMapNodeFields,
    sizeof(CmOmiMapNodeCmds) / sizeof(cm_omi_map_object_cmd_t),
    CmOmiMapNodeCmds
};


/**********************************************/
const cm_omi_map_cfg_t CmOmiServiceStatusType[] =
{
    {"off", 0},
    {"online",1},
    {"offline",2},
    {"partOnline",3},
    {"maintenance",4},
    {"trasition",5}
};

const cm_omi_map_enum_t CmOmiEnumServiceStatusType =
{
    sizeof(CmOmiServiceStatusType) / sizeof(cm_omi_map_cfg_t),
    CmOmiServiceStatusType
};

const cm_omi_map_object_field_t CmOmiMapNodeServiceFields[] =
{
    {{"service", CM_OMI_FIELD_NODE_SERVCE_SERVCE}, "-s", {CM_OMI_DATA_STRING,CM_STRING_32, {NULL}}},
    {{"nid", CM_OMI_FIELD_NODE_SERVCE_NID}, "-nid", {CM_OMI_DATA_INT, 4, {"[0-9a-zA-Z_]{1,63}"}}},
    {{"nfs", CM_OMI_FIELD_NODE_SERVCE_NFS}, "-nfs", {CM_OMI_DATA_ENUM,4, {&CmOmiEnumServiceStatusType}}},
    {{"stmf", CM_OMI_FIELD_NODE_SERVCE_STMF}, "-stmf", {CM_OMI_DATA_ENUM, 4, {&CmOmiEnumServiceStatusType}}},   
    {{"ssh", CM_OMI_FIELD_NODE_SERVCE_SSH}, "-ssh", {CM_OMI_DATA_ENUM, 4, {&CmOmiEnumServiceStatusType}}},
    {{"ftp", CM_OMI_FIELD_NODE_SERVCE_FTP}, "-ftp", {CM_OMI_DATA_ENUM, 4, {&CmOmiEnumServiceStatusType}}},
    {{"smb", CM_OMI_FIELD_NODE_SERVCE_SMB}, "-smb", {CM_OMI_DATA_ENUM, 4, {&CmOmiEnumServiceStatusType}}},
    {{"guiview", CM_OMI_FIELD_NODE_SERVCE_GUIVIEW}, "-gui", {CM_OMI_DATA_ENUM, 4, {&CmOmiEnumServiceStatusType}}},
    {{"fmd", CM_OMI_FIELD_NODE_SERVCE_FMD}, "-fmd", {CM_OMI_DATA_ENUM, 4, {&CmOmiEnumServiceStatusType}}},
    {{"iscsi", CM_OMI_FIELD_NODE_SERVCE_ISCSI}, "-iscsi", {CM_OMI_DATA_ENUM, 4, {&CmOmiEnumServiceStatusType}}},
};

const cm_omi_map_object_field_t* CmOmiMapNodeServiceCmdParamsGetBatchOpt[] =
{
    &CmOmiMapCommFields[1], /*offset*/
    &CmOmiMapCommFields[2], /*total*/
    &CmOmiMapNodeServiceFields[CM_OMI_FIELD_NODE_SERVCE_NID],
};

const cm_omi_map_object_field_t* CmOmiMapNodeServiceCmdParamsUpdata[] =
{
    &CmOmiMapNodeServiceFields[CM_OMI_FIELD_NODE_SERVCE_NID],
};


const cm_omi_map_object_field_t* CmOmiMapNodeServiceCmdParamsUpdataOpt[] =
{ 
    &CmOmiMapNodeServiceFields[CM_OMI_FIELD_NODE_SERVCE_NFS], 
    &CmOmiMapNodeServiceFields[CM_OMI_FIELD_NODE_SERVCE_STMF],
    &CmOmiMapNodeServiceFields[CM_OMI_FIELD_NODE_SERVCE_SSH],
    &CmOmiMapNodeServiceFields[CM_OMI_FIELD_NODE_SERVCE_FTP],
    &CmOmiMapNodeServiceFields[CM_OMI_FIELD_NODE_SERVCE_SMB],
    &CmOmiMapNodeServiceFields[CM_OMI_FIELD_NODE_SERVCE_GUIVIEW],
    &CmOmiMapNodeServiceFields[CM_OMI_FIELD_NODE_SERVCE_FMD],
    &CmOmiMapNodeServiceFields[CM_OMI_FIELD_NODE_SERVCE_ISCSI],
};


const cm_omi_map_object_cmd_t CmOmiMapNodeServiceCmds[] =
{
    /* getbatch */
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET_BATCH],
        0,
        NULL,
        sizeof(CmOmiMapNodeServiceCmdParamsGetBatchOpt) / sizeof(CmOmiMapNodeServiceCmdParamsGetBatchOpt[0]),
        CmOmiMapNodeServiceCmdParamsGetBatchOpt
    },
    /* update */
    {
        &CmOmiMapCmds[CM_OMI_CMD_MODIFY],
         sizeof(CmOmiMapNodeServiceCmdParamsUpdata) /sizeof(CmOmiMapNodeServiceCmdParamsUpdata[0]),
        CmOmiMapNodeServiceCmdParamsUpdata,
        8,
        CmOmiMapNodeServiceCmdParamsUpdataOpt
    },
   
};

const cm_omi_map_object_t CmCnmNodeServiceCfg =
{
    {"node_service", CM_OMI_OBJECT_NODE_SERVCE},
    sizeof(CmOmiMapNodeServiceFields) / sizeof(cm_omi_map_object_field_t),
    CmOmiMapNodeServiceFields,
    sizeof(CmOmiMapNodeServiceCmds) / sizeof(cm_omi_map_object_cmd_t),
    CmOmiMapNodeServiceCmds
};

/*   upgrade    */

const cm_omi_map_cfg_t CmOmiUpgradeStateType[] =
{
    {"init", 0},
    {"transfer",1},
    {"wait",2},
    {"unpack",3},
    {"bak",4},
    {"rdupgrade",5},
    {"reboot",6},
    {"poweron",7},
    {"error",9},
};

const cm_omi_map_enum_t CmOmiEnumUpgradeStateType =
{
    sizeof(CmOmiUpgradeStateType) / sizeof(cm_omi_map_cfg_t),
    CmOmiUpgradeStateType
};

const cm_omi_map_cfg_t CmOmiUpgradeModeType[] =
{
    {"starton", 0},
    {"startoff",1},
};

const cm_omi_map_enum_t CmOmiEnumUpgradeModeType =
{
    sizeof(CmOmiUpgradeModeType) / sizeof(cm_omi_map_cfg_t),
    CmOmiUpgradeModeType
};



const cm_omi_map_object_field_t CmOmiMapUpgradeFields[] =
{
    {{"name", CM_OMI_FIELD_UPGRADE_NAME}, "-n", {CM_OMI_DATA_STRING,CM_STRING_32, {NULL}}},
    {{"nid", CM_OMI_FIELD_UPGRADE_NID}, "-nid", {CM_OMI_DATA_INT, 4, {"[0-9a-zA-Z_]{1,63}"}}},
    {{"state", CM_OMI_FIELD_UPGRADE_STATE}, "-s", {CM_OMI_DATA_ENUM,4, {&CmOmiEnumUpgradeStateType}}},
    {{"process", CM_OMI_FIELD_UPGRADE_PROCESS}, "-p", {CM_OMI_DATA_INT, 4, {"[0-9a-zA-Z_]{1,63}"}}},
    {{"rd_dir", CM_OMI_FIELD_UPGRADE_RDDIR}, "-n", {CM_OMI_DATA_STRING,CM_STRING_128, {NULL}}},
    {{"mode", CM_OMI_FIELD_UPGRADE_MODE,}, "-m", {CM_OMI_DATA_ENUM,4, {&CmOmiEnumUpgradeModeType}}},
    {{"version", CM_OMI_FIELD_UPGRADE_VERSION}, "-v", {CM_OMI_DATA_STRING,CM_STRING_32, {NULL}}},
};

const cm_omi_map_object_field_t* CmOmiMapUpgradeCmdParamsGetBatch[] =
{
    &CmOmiMapCommFields[1], /*offset*/
    &CmOmiMapCommFields[2], /*total*/
};

const cm_omi_map_object_field_t* CmOmiMapUpgradeCmdParamsInsert[] =
{
    &CmOmiMapUpgradeFields[CM_OMI_FIELD_UPGRADE_NID],
    &CmOmiMapUpgradeFields[CM_OMI_FIELD_UPGRADE_RDDIR],
    &CmOmiMapUpgradeFields[CM_OMI_FIELD_UPGRADE_MODE], 
};

const cm_omi_map_object_cmd_t CmOmiMapUpgradeCmds[] =
{
    /* getbatch */
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET_BATCH],
        0,
        NULL,
        2,
        CmOmiMapUpgradeCmdParamsGetBatch,
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
        3,
        CmOmiMapUpgradeCmdParamsInsert,
        0,
        NULL,
    },
};


const cm_omi_map_object_t CmCnmUpgradeCfg =
{
    {"upgrade", CM_OMI_OBJECT_UPGRADE},
    sizeof(CmOmiMapUpgradeFields) / sizeof(cm_omi_map_object_field_t),
    CmOmiMapUpgradeFields,
    sizeof(CmOmiMapUpgradeCmds) / sizeof(cm_omi_map_object_cmd_t),
    CmOmiMapUpgradeCmds
};


