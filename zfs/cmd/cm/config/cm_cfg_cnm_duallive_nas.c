/******************************************************************************
 *          Copyright (c) 2018 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cfg_cnm_duallive_nas.c
 * author     : zjd
 * create date: 2018.07.07
 * description: TODO:
 *
 *****************************************************************************/
#include "cm_omi_types.h"
#include "cm_cfg_cnm.h"

const cm_omi_map_cfg_t CmOmiBackupSyncType[] =
{
    {"off", 0},
    {"async",1},
    {"sync",2},
};

const cm_omi_map_enum_t CmOmiEnumBackupSyncType =
{
    sizeof(CmOmiBackupSyncType) / sizeof(cm_omi_map_cfg_t),
    CmOmiBackupSyncType
};


const cm_omi_map_object_field_t CmOmiMapFieldsDualliveNas[] =
{
    {{"master_nas", CM_OMI_FIELD_DUALLIVE_NAS_MNAS}, "-mnas",{CM_OMI_DATA_STRING, CM_NAS_PATH_LEN, {NULL}}},
    {{"master_nid", CM_OMI_FIELD_DUALLIVE_NAS_MNID}, "-mnid",{CM_OMI_DATA_INT, 4, {NULL}}},
    {{"slave_nas", CM_OMI_FIELD_DUALLIVE_NAS_SNAS}, "-snas",{CM_OMI_DATA_STRING, CM_NAS_PATH_LEN, {NULL}}},
    {{"slave_nid", CM_OMI_FIELD_DUALLIVE_NAS_SNID}, "-snid",{CM_OMI_DATA_INT, 4, {NULL}}},
    {{"synctype", CM_OMI_FIELD_DUALLIVE_NAS_SYNC},"-sync",{CM_OMI_DATA_ENUM,4,{&CmOmiEnumBackupSyncType}}},
    {{"workload_if", CM_OMI_FIELD_DUALLIVE_NAS_WKLOAD_IF}, "-wkif",{CM_OMI_DATA_STRING,CM_STRING_32, {NULL}}},
    {{"workload_ip", CM_OMI_FIELD_DUALLIVE_NAS_WKLOAD_IP}, "-wkip",{CM_OMI_DATA_STRING, CM_IP_LEN, {NULL}}},
    {{"netmask", CM_OMI_FIELD_DUALLIVE_NAS_NETMASK}, "-netmask",{CM_OMI_DATA_STRING,CM_IP_LEN, {NULL}}},
    {{"status", CM_OMI_FIELD_DUALLIVE_NAS_STATUAS}, "-status",{CM_OMI_DATA_STRING, CM_STRING_32, {NULL}}},
};

const cm_omi_map_object_field_t* CmOmiMapDualliveNasCmdParamsGetBatch[]=
{
    &CmOmiMapCommFields[1], /*offset*/
    &CmOmiMapCommFields[2], /*total*/
};

const cm_omi_map_object_field_t* CmOmiMapDualliveNasCmdParamsAdd[]=
{
    &CmOmiMapFieldsDualliveNas[CM_OMI_FIELD_DUALLIVE_NAS_MNID], 
    &CmOmiMapFieldsDualliveNas[CM_OMI_FIELD_DUALLIVE_NAS_MNAS], /*node id*/
    &CmOmiMapFieldsDualliveNas[CM_OMI_FIELD_DUALLIVE_NAS_SNID],
    &CmOmiMapFieldsDualliveNas[CM_OMI_FIELD_DUALLIVE_NAS_SNAS], 
    &CmOmiMapFieldsDualliveNas[CM_OMI_FIELD_DUALLIVE_NAS_SYNC],
    &CmOmiMapFieldsDualliveNas[CM_OMI_FIELD_DUALLIVE_NAS_WKLOAD_IF],
    &CmOmiMapFieldsDualliveNas[CM_OMI_FIELD_DUALLIVE_NAS_WKLOAD_IP],
    &CmOmiMapFieldsDualliveNas[CM_OMI_FIELD_DUALLIVE_NAS_NETMASK],
};


const cm_omi_map_object_cmd_t CmOmiMapDualliveNasCmds[] =
{
    /* getbatch */
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET_BATCH],
        0,
        NULL,
        2,
        CmOmiMapDualliveNasCmdParamsGetBatch,
    },
    /* count */
    {
        &CmOmiMapCmds[CM_OMI_CMD_COUNT],        
        0,
        NULL,
        0,
        NULL,
    },
    /* creat */
    {
        &CmOmiMapCmds[CM_OMI_CMD_ADD],        
        sizeof(CmOmiMapDualliveNasCmdParamsAdd)/sizeof(CmOmiMapDualliveNasCmdParamsAdd[0]),
        CmOmiMapDualliveNasCmdParamsAdd,
        0,
        NULL,
    },

    /* delete */
    {
        &CmOmiMapCmds[CM_OMI_CMD_DELETE],        
        2,
        CmOmiMapDualliveNasCmdParamsAdd,
        0,
        NULL,
    },
};

const cm_omi_map_object_t CmCnmDualliveNasCfg =
{
    {"nas_duallive", CM_OMI_OBJECT_DUALLIVE_NAS},
    sizeof(CmOmiMapFieldsDualliveNas)/sizeof(cm_omi_map_object_field_t),
    CmOmiMapFieldsDualliveNas,
    sizeof(CmOmiMapDualliveNasCmds)/sizeof(cm_omi_map_object_cmd_t),
    CmOmiMapDualliveNasCmds
}; 

/******************************************************************************************

******************************************************************************************/

const cm_omi_map_object_field_t CmOmiMapFieldsBackUpNas[] =
{
    {{"master_nas", CM_OMI_FIELD_BACKUP_NAS_MNAS}, "-mnas",{CM_OMI_DATA_STRING, CM_NAS_PATH_LEN, {NULL}}},
    {{"master_nid", CM_OMI_FIELD_BACKUP_NAS_MNID}, "-mnid",{CM_OMI_DATA_INT, 4, {NULL}}},
    {{"slave_nas", CM_OMI_FIELD_BACKUP_NAS_SNAS}, "-snas",{CM_OMI_DATA_STRING, CM_NAS_PATH_LEN, {NULL}}},
    {{"slave_nid", CM_OMI_FIELD_BACKUP_NAS_SNID}, "-snid",{CM_OMI_DATA_INT, 4, {NULL}}},
    {{"synctype", CM_OMI_FIELD_BACKUP_NAS_SYNC},"-sync",{CM_OMI_DATA_ENUM,4,{&CmOmiEnumBackupSyncType}}},
    {{"status", CM_OMI_FIELD_BACKUP_NAS_STATUAS}, "-status",{CM_OMI_DATA_STRING, CM_STRING_32, {NULL}}},
};

const cm_omi_map_object_field_t* CmOmiMapBackUpNasCmdParamsGetBatch[]=
{
    &CmOmiMapCommFields[1], /*offset*/
    &CmOmiMapCommFields[2], /*total*/
};

const cm_omi_map_object_field_t* CmOmiMapBackupNasCmdParamsAdd[]=
{
    &CmOmiMapFieldsBackUpNas[CM_OMI_FIELD_BACKUP_NAS_MNID], 
    &CmOmiMapFieldsBackUpNas[CM_OMI_FIELD_BACKUP_NAS_MNAS], /*node id*/
    &CmOmiMapFieldsBackUpNas[CM_OMI_FIELD_BACKUP_NAS_SNID],
    &CmOmiMapFieldsBackUpNas[CM_OMI_FIELD_BACKUP_NAS_SNAS],
    &CmOmiMapFieldsBackUpNas[CM_OMI_FIELD_BACKUP_NAS_SYNC],
};


const cm_omi_map_object_cmd_t CmOmiMapBackupNasCmds[] =
{
    /* getbatch */
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET_BATCH],
        0,
        NULL,
        2,
        CmOmiMapBackUpNasCmdParamsGetBatch,
    },
    /* count */
    {
        &CmOmiMapCmds[CM_OMI_CMD_COUNT],        
        0,
        NULL,
        0,
        NULL,
    },
    /* creat */
    {
        &CmOmiMapCmds[CM_OMI_CMD_ADD],        
        sizeof(CmOmiMapBackupNasCmdParamsAdd)/sizeof(CmOmiMapBackupNasCmdParamsAdd[0]),
        CmOmiMapBackupNasCmdParamsAdd,
        0,
        NULL,
    },

    /* delete */
    {
        &CmOmiMapCmds[CM_OMI_CMD_DELETE],        
        2,
        CmOmiMapBackupNasCmdParamsAdd,
        0,
        NULL,
    },
};

const cm_omi_map_object_t CmCnmBackupNasCfg =
{
    {"nas_backup", CM_OMI_OBJECT_BACKUP_NAS},
    sizeof(CmOmiMapFieldsBackUpNas)/sizeof(cm_omi_map_object_field_t),
    CmOmiMapFieldsBackUpNas,
    sizeof(CmOmiMapBackupNasCmds)/sizeof(cm_omi_map_object_cmd_t),
    CmOmiMapBackupNasCmds,
}; 



/******************************************************************************************

******************************************************************************************/



const cm_omi_map_object_field_t CmOmiMapFieldsDualliveNetif[] =
{
    {{"list_target", CM_OMI_FIELD_DUALLIVE_NETIF_TARGET}, "-lt",{CM_OMI_DATA_STRING, CM_STRING_32, {NULL}}},
    {{"nid", CM_OMI_FIELD_DUALLIVE_NETIF_NID}, "-nid",{CM_OMI_DATA_INT, 4, {NULL}}},
};

const cm_omi_map_object_field_t* CmOmiMapDualliveNetifCmdParamsGetbatch[]=
{
    &CmOmiMapFieldsDualliveNetif[CM_OMI_FIELD_DUALLIVE_NETIF_NID], 
};

const cm_omi_map_object_cmd_t CmOmiMapDualliveNetifCmds[] =
{
    /* getbatch */
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET_BATCH],
        1,
        CmOmiMapDualliveNetifCmdParamsGetbatch,
        0,
        NULL,
    },
};

const cm_omi_map_object_t CmCnmDualliveNetifCfg =
{
    {"cluster_target", CM_OMI_OBJECT_CLUSTERSAN_TARGET},
    sizeof(CmOmiMapFieldsDualliveNetif)/sizeof(cm_omi_map_object_field_t),
    CmOmiMapFieldsDualliveNetif,
    sizeof(CmOmiMapDualliveNetifCmds)/sizeof(cm_omi_map_object_cmd_t),
    CmOmiMapDualliveNetifCmds
};


