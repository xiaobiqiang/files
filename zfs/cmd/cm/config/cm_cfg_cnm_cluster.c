/******************************************************************************
 *          Copyright (c) 2017 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cfg_cnm_cluster.c
 * author     : wbn
 * create date: 2017Äê11ÔÂ13ÈÕ
 * description: TODO:
 *
 *****************************************************************************/
#include "cm_omi_types.h"
#include "cm_cfg_cnm.h"

extern const cm_omi_map_enum_t CmOmiMapEnumSwitchType; 

const cm_omi_map_cfg_t CmOmiMapFailOver[] = 
{
    {"roundrobin", 0},
    {"loadbalance", 1},
};

const cm_omi_map_enum_t CmOmiMapEnumFailOverType = 
{
    sizeof(CmOmiMapFailOver)/sizeof(cm_omi_map_cfg_t),
    &CmOmiMapFailOver
};

const cm_omi_map_object_field_t CmOmiMapClusterFields[] =
{
    {{"name", CM_OMI_FIELD_CLUSTER_NAME}, "-n",{CM_OMI_DATA_STRING, CM_NAME_LEN, {"[0-9a-zA-Z_]{1,63}"}}},
    {{"version", CM_OMI_FIELD_CLUSTER_VERSION}, "-v",{CM_OMI_DATA_STRING, CM_VERSION_LEN, {"[0-9a-zA-Z_]{1,63}"}}},
    {{"ipaddr", CM_OMI_FIELD_CLUSTER_IPADDR}, "-p",{CM_OMI_DATA_STRING, CM_IP_LEN, {IP_FORMAT}}},
    {{"gateway", CM_OMI_FIELD_CLUSTER_GATEWAY}, "-g",{CM_OMI_DATA_STRING, CM_IP_LEN, {IP_FORMAT}}},
    {{"netmask", CM_OMI_FIELD_CLUSTER_NETMASK}, "-m",{CM_OMI_DATA_STRING, CM_IP_LEN, {IP_FORMAT}}},
    {{"config_time", CM_OMI_FIELD_CLUSTER_TIMESTAMP}, "-t",{CM_OMI_DATA_TIME, 8, {NULL}}},
    {{"interface", CM_OMI_FIELD_CLUSTER_INTERFACE}, "-nic",{CM_OMI_DATA_STRING, CM_NAME_LEN, {NULL}}},
    {{"priority", CM_OMI_FIELD_CLUSTER_PRIORITY}, "-prio",{CM_OMI_DATA_STRING, CM_NAME_LEN, {NULL}}},
    {{"enipmi", CM_OMI_FIELD_CLUSTER_ENABLE_IPMI}, "-ep",{CM_OMI_DATA_ENUM, 4, {&CmOmiMapEnumSwitchType}}},
    {{"failover", CM_OMI_FIELD_CLUSTER_FAILOVER}, "-f",{CM_OMI_DATA_ENUM, 4, {&CmOmiMapEnumFailOverType}}},
    {{"product", CM_OMI_FIELD_CLUSTER_PRODUCT}, "-product",{CM_OMI_DATA_STRING, CM_NAME_LEN, {"[0-9a-zA-Z_]{1,63}"}}},
};

const cm_omi_map_object_field_t* CmOmiMapClusterCmdParamsModify[]=
{
    &CmOmiMapClusterFields[0], /*name*/
    &CmOmiMapClusterFields[2], /*ip*/
    &CmOmiMapClusterFields[3], /*gateway*/
    &CmOmiMapClusterFields[4], /*netmask*/
    &CmOmiMapClusterFields[6], /*interface*/
    &CmOmiMapClusterFields[CM_OMI_FIELD_CLUSTER_PRODUCT], /*product*/
};

const cm_omi_map_object_field_t* CmOmiMapClusterCmdOptParamsModify[]=
{
    &CmOmiMapClusterFields[7], /*priority*/
    &CmOmiMapClusterFields[8], /*enipmi*/
    &CmOmiMapClusterFields[9], /*failover*/
};


const cm_omi_map_object_cmd_t CmOmiMapClusterCmds[] =
{
    /* get */
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET],
        0,
        NULL,
        0,
        NULL
    },
    /* modify */
    {
        &CmOmiMapCmds[CM_OMI_CMD_MODIFY],
        sizeof(CmOmiMapClusterCmdParamsModify)/sizeof(cm_omi_map_object_field_t*),
        CmOmiMapClusterCmdParamsModify,
        sizeof(CmOmiMapClusterCmdOptParamsModify)/sizeof(cm_omi_map_object_field_t*),
        CmOmiMapClusterCmdOptParamsModify,
    },
    /* poweroff */
    {
        &CmOmiMapCmds[CM_OMI_CMD_OFF],
        0,
        NULL,
        0,
        NULL,
    },
    /* reboot */
    {
        &CmOmiMapCmds[CM_OMI_CMD_REBOOT],
        0,
        NULL,
        0,
        NULL,
    }
};

const cm_omi_map_object_t CmCnmClusterCfg =
{
    {"cluster", CM_OMI_OBJECT_CLUSTER},
    sizeof(CmOmiMapClusterFields)/sizeof(cm_omi_map_object_field_t),
    CmOmiMapClusterFields,
    sizeof(CmOmiMapClusterCmds)/sizeof(cm_omi_map_object_cmd_t),
    CmOmiMapClusterCmds
};


const cm_omi_map_object_field_t CmOmiMapRemoteClusterFields[] =
{
    {{"ip", CM_OMI_FIELD_REMOTECLUSTER_IP}, "-ip",{CM_OMI_DATA_STRING, CM_IP_LEN, {IP_FORMAT}}},
    {{"nid", CM_OMI_FIELD_REMOTECLUSTER_NID}, "-nid",{CM_OMI_DATA_INT, 4, {NULL}}},
    {{"name", CM_OMI_FIELD_REMOTECLUSTER_NAME}, "-name",{CM_OMI_DATA_STRING, 256, {NULL}}},
};


const cm_omi_map_object_field_t* CmOmiMapRemoteClusterCmdParamsGetBatch[]=
{
    &CmOmiMapCommFields[1], /*offset*/
    &CmOmiMapCommFields[2], /*total*/
};

const cm_omi_map_object_field_t* CmOmiMapRemoteClusterCmdParamsAdd[]=
{
    &CmOmiMapRemoteClusterFields[CM_OMI_FIELD_REMOTECLUSTER_IP], /*IP*/
    &CmOmiMapRemoteClusterFields[CM_OMI_FIELD_REMOTECLUSTER_NAME], /*IP*/
};


const cm_omi_map_object_cmd_t CmOmiMapRemoteClusterCmds[] =
{
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET_BATCH],
        0,
        NULL,
        2,
        CmOmiMapRemoteClusterCmdParamsGetBatch,
    },
    {
        &CmOmiMapCmds[CM_OMI_CMD_COUNT],
        0,
        NULL,
        0,
        NULL,
    },  
    {
        &CmOmiMapCmds[CM_OMI_CMD_ADD],
        2,
        CmOmiMapRemoteClusterCmdParamsAdd,
        0,
        NULL,
    },
    {
        &CmOmiMapCmds[CM_OMI_CMD_DELETE],
        1,
        CmOmiMapRemoteClusterCmdParamsAdd,
        0,
        NULL,
    },  
    {
        &CmOmiMapCmds[CM_OMI_CMD_MODIFY],
        2,
        CmOmiMapRemoteClusterCmdParamsAdd,
        0,
        NULL,
    },  
};


const cm_omi_map_object_t CmCnmRemoteClusterCfg =
{
    {"remote_cluster", CM_OMI_OBJECT_REMOTE_CLUSTER},
    sizeof(CmOmiMapRemoteClusterFields)/sizeof(cm_omi_map_object_field_t),
    CmOmiMapRemoteClusterFields,
    sizeof(CmOmiMapRemoteClusterCmds)/sizeof(cm_omi_map_object_cmd_t),
    CmOmiMapRemoteClusterCmds
};


