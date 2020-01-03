/******************************************************************************
 *          Copyright (c) 2018 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cfg_cnm_pool.c
 * author     : wbn
 * create date: 2018.05.07
 * description: TODO:
 *
 *****************************************************************************/
#include "cm_omi_types.h"
#include "cm_cfg_cnm.h"

const cm_omi_map_cfg_t CmOmiMapPoolStatusType[] =
{
    {"UNKNOWN",0},
    {"ONLINE",1},
    {"OFFLINE",2},
    {"AVAIL",3},
    {"INUSE",4},
    {"UNAVAIL",5},
    {"DEGRADED",6},
    {"FAULTED",7},
    {"SPLIT",8},
    {"REMOVED",9},
};

const cm_omi_map_enum_t CmOmiMapEnumPoolStatusType =
{
    sizeof(CmOmiMapPoolStatusType)/sizeof(cm_omi_map_cfg_t),
    CmOmiMapPoolStatusType
};

const cm_omi_map_cfg_t CmOmiMapPoolRaidType[] =
{
    {"raid0",CM_RAID0},
    {"raid1",CM_RAID1},
    {"raid5",CM_RAID5},
    {"raid6",CM_RAID6},
    {"raid7",CM_RAID7},
    {"raid10",CM_RAID10},
    {"raid50",CM_RAID50},
    {"raid60",CM_RAID60},
    {"raid70",CM_RAID70},
};

const cm_omi_map_enum_t CmOmiMapEnumPoolRaidType =
{
    sizeof(CmOmiMapPoolRaidType)/sizeof(cm_omi_map_cfg_t),
    CmOmiMapPoolRaidType
};

const cm_omi_map_cfg_t CmOmiMapPoolOperationType[] =
{
    {"import",CM_POOL_IMPORT},
    {"export",CM_POOL_EXPORT},
    {"scrub",CM_POOL_SCRUB},
    {"clear",CM_POOL_CLEAR},
    {"release",CM_POOL_SWITCH},
    {"destroy",CM_POOL_DESTROY},
};

const cm_omi_map_enum_t CmOmiMapEnumPoolOperationType =
{
    sizeof(CmOmiMapPoolOperationType)/sizeof(cm_omi_map_cfg_t),
    CmOmiMapPoolOperationType
};

const cm_omi_map_cfg_t CmOmiMapZpoolStatusType[] =
{
    {"CANCELED",CM_ZSTATUS_CANCELED},
    {"SCRUBING",CM_ZSTATUS_SCRUBING},
    {"SCRUBED",CM_ZSTATUS_SCRUBED},
};


const cm_omi_map_enum_t CmOmiMapEnumZpoolStatusType =
{
    sizeof(CmOmiMapZpoolStatusType)/sizeof(cm_omi_map_cfg_t),
    CmOmiMapZpoolStatusType
};

const cm_omi_map_object_field_t CmOmiMapFieldsPool[] =
{
    {{"name", CM_OMI_FIELD_POOL_NAME}, "-n",{CM_OMI_DATA_STRING, 64, {"[0-9a-zA-Z_]{1,63}"}}},
    {{"nid", CM_OMI_FIELD_POOL_NID}, "-nid",{CM_OMI_DATA_INT, 4, {NULL}}},
    {{"src_nid", CM_OMI_FIELD_POOL_NID_SRC}, "-snid",{CM_OMI_DATA_INT, 4, {NULL}}},
    {{"status", CM_OMI_FIELD_POOL_STATUS}, "-s",{CM_OMI_DATA_ENUM, 4, {&CmOmiMapEnumPoolStatusType}}},
    {{"avail", CM_OMI_FIELD_POOL_AVAIL},"-av", {CM_OMI_DATA_STRING, 32, {NULL}}},
    {{"used", CM_OMI_FIELD_POOL_USED},"-us", {CM_OMI_DATA_STRING, 32, {NULL}}},
    {{"raid", CM_OMI_FIELD_POOL_RAID},"-raid", {CM_OMI_DATA_ENUM, 4, {&CmOmiMapEnumPoolRaidType}}},
    {{"group", CM_OMI_FIELD_POOL_GROUP}, "-group",{CM_OMI_DATA_INT, 4, {NULL}}},
    {{"rdcip", CM_OMI_FIELD_POOL_RDCIP},"-rdcip", {CM_OMI_DATA_STRING, 32, {NULL}}},
    {{"operation", CM_OMI_FIELD_POOL_OPS}, "-op", {CM_OMI_DATA_ENUM, 4,{&CmOmiMapEnumPoolOperationType}}},   
    {{"var", CM_OMI_FIELD_POOL_VAR}, "-var", {CM_OMI_DATA_STRING, CM_STRING_128,{NULL}}},

    {{"prog", CM_OMI_FIELD_POOL_PROG},"-prog", {CM_OMI_DATA_DOUBLE, 8, {NULL}}},   
    {{"speed", CM_OMI_FIELD_POOL_SPEED}, "-sp", {CM_OMI_DATA_DOUBLE, 8,{NULL}}},
    {{"repaired", CM_OMI_FIELD_POOL_REPAIRED}, "-rpd", {CM_OMI_DATA_INT, 4,{NULL}}}, 
    {{"errors",CM_OMI_FIELD_POOL_ERRORS},"-err",{CM_OMI_DATA_INT, 4,{NULL}}},
    {{"time", CM_OMI_FIELD_POOL_TIME}, "-t", {CM_OMI_DATA_STRING,CM_STRING_32,{NULL}}},
    {{"zstatus", CM_OMI_FIELD_POOL_ZSTATUS}, "-zs", {CM_OMI_DATA_ENUM,4,{&CmOmiMapEnumZpoolStatusType}}},
};

const cm_omi_map_object_field_t* CmOmiMapPoolCmdParamsGetBatch[]=
{
    &CmOmiMapFieldsPool[CM_OMI_FIELD_POOL_NID], /*node id*/
    &CmOmiMapCommFields[1], /*offset*/
    &CmOmiMapCommFields[2], /*total*/
};

const cm_omi_map_object_field_t* CmOmiMapPoolCmdParamsAdd[]=
{
    &CmOmiMapFieldsPool[CM_OMI_FIELD_POOL_NID], /*node id*/
    &CmOmiMapFieldsPool[CM_OMI_FIELD_POOL_RAID], /*raid*/
    &CmOmiMapFieldsPool[CM_OMI_FIELD_POOL_NAME], /*name*/
    &CmOmiMapCommFields[7], /*param*/
};

const cm_omi_map_object_field_t* CmOmiMapPoolCmdParamsAddOpt[]=
{
    &CmOmiMapFieldsPool[CM_OMI_FIELD_POOL_GROUP], /*group*/
};


const cm_omi_map_object_field_t* CmOmiMapPoolCmdParamsDelete[]=
{
    &CmOmiMapFieldsPool[CM_OMI_FIELD_POOL_NID], /*node id*/
    &CmOmiMapFieldsPool[CM_OMI_FIELD_POOL_NAME], /*name*/
    &CmOmiMapFieldsPool[CM_OMI_FIELD_POOL_OPS], /*ops*/
};

const cm_omi_map_object_field_t* CmOmiMapPoolOpsCmdParamsArg[]=
{
    &CmOmiMapFieldsPool[CM_OMI_FIELD_POOL_VAR], /*var*/
};

const cm_omi_map_object_cmd_t CmOmiMapPoolCmds[] =
{
    /* getbatch */
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET_BATCH],
        0,
        NULL,
        3,
        CmOmiMapPoolCmdParamsGetBatch,
    },
    /* count */
    {
        &CmOmiMapCmds[CM_OMI_CMD_COUNT],        
        0,
        NULL,
        1,
        CmOmiMapPoolCmdParamsGetBatch,
    },
    /* add */
    {
        &CmOmiMapCmds[CM_OMI_CMD_ADD],        
        sizeof(CmOmiMapPoolCmdParamsAdd)/sizeof(cm_omi_map_object_field_t*),
        CmOmiMapPoolCmdParamsAdd,
        1,
        CmOmiMapPoolCmdParamsAddOpt,
    },
    /* delete */
    {
        &CmOmiMapCmds[CM_OMI_CMD_DELETE],        
        2,
        CmOmiMapPoolCmdParamsDelete,
        0,
        NULL,
    },
    {
        &CmOmiMapCmds[CM_OMI_CMD_MODIFY],        
        3,
        CmOmiMapPoolCmdParamsDelete,
        1,
        CmOmiMapPoolOpsCmdParamsArg,
    },
    /*scan*/
    {
        &CmOmiMapCmds[CM_OMI_CMD_SCAN],
        2,
        CmOmiMapPoolCmdParamsDelete,
        0,
        NULL,
    },
};

const cm_omi_map_object_t CmCnmPoolCfg =
{
    {"pool", CM_OMI_OBJECT_POOL},
    sizeof(CmOmiMapFieldsPool)/sizeof(cm_omi_map_object_field_t),
    CmOmiMapFieldsPool,
    sizeof(CmOmiMapPoolCmds)/sizeof(cm_omi_map_object_cmd_t),
    CmOmiMapPoolCmds
}; 

const cm_omi_map_cfg_t CmCnmPoolMapErr[] = 
{
   {"exists and is not empty",CM_ERR_POOL_ALREADY_EXISTS},
   {"pool already exists",CM_ERR_POOL_ALREADY_EXISTS},
   {"no such device",CM_ERR_POOL_DISK_NOT_EXISTS},
   {"is not ceresdata company",CM_ERR_POOL_DISK_NOT_COMP},
   {"requires at least 2 devices",CM_ERR_POOL_DISK_NUM_2},
   {"is part of",CM_ERR_POOL_DISK_USED},
   {"is busy or one or more vdevs refer to the same device",CM_ERR_POOL_DISK_USED},
   {"is busy", CM_ERR_BUSY},
   {"spare is too small", CM_ERR_POOL_DISK_TOO_SMALL},
   {"no such pool",CM_ERR_POOL_NOT_EXISTS},
};
   
const cm_omi_map_enum_t CmCnmPoolMapErrCfg =
{
   sizeof(CmCnmPoolMapErr)/sizeof(cm_omi_map_cfg_t),
   CmCnmPoolMapErr
};

const cm_omi_map_cfg_t CmCnmPoolMapDiskType[] = 
{
   {"data",CM_POOL_DISK_DATA},
   {"metas",CM_POOL_DISK_META},
   {"spares",CM_POOL_DISK_SPARE},
   {"metaspares",CM_POOL_DISK_METASPARE},
   {"logs",CM_POOL_DISK_LOG},
   {"logdatas",CM_POOL_DISK_LOGDATA},
   {"logdataspares",CM_POOL_DISK_LOGDATASPARE},
   {"l2caches",CM_POOL_DISK_CACHE},
   {"lows",CM_POOL_DISK_LOW},
   {"lowspares",CM_POOL_DISK_LOWSPARE},
   {"mirrorspares",CM_POOL_DISK_MIRRORSPARE},
};
   
const cm_omi_map_enum_t CmCnmPoolMapDiskTypeCfg =
{
   sizeof(CmCnmPoolMapDiskType)/sizeof(cm_omi_map_cfg_t),
   CmCnmPoolMapDiskType
};

const cm_omi_map_cfg_t CmCnmPoolMapDiskTypeUser[] = 
{
   {"data",CM_POOL_DISK_DATA},
   {"meta",CM_POOL_DISK_META},
   {"spare",CM_POOL_DISK_SPARE},
   {"metaspare",CM_POOL_DISK_METASPARE},
   {"log",CM_POOL_DISK_LOG},
   {"logdata",CM_POOL_DISK_LOGDATA},
   {"logdataspare",CM_POOL_DISK_LOGDATASPARE},
   {"cache",CM_POOL_DISK_CACHE},
   {"low",CM_POOL_DISK_LOW},
   {"lowspare",CM_POOL_DISK_LOWSPARE},
   {"mirrorspare",CM_POOL_DISK_MIRRORSPARE},
};
   
const cm_omi_map_enum_t CmCnmPoolMapDiskTypeUserCfg =
{
   sizeof(CmCnmPoolMapDiskTypeUser)/sizeof(cm_omi_map_cfg_t),
   CmCnmPoolMapDiskTypeUser
};

const cm_omi_map_object_field_t CmOmiMapFieldsPoolDisk[] =
{
    {{"pool", CM_OMI_FIELD_POOLDISK_NAME}, "-pool",{CM_OMI_DATA_STRING, 64, {"[0-9a-zA-Z_]{1,63}"}}},
    {{"nid", CM_OMI_FIELD_POOLDISK_NID}, "-nid",{CM_OMI_DATA_INT, 4, {NULL}}},
    {{"disk", CM_OMI_FIELD_POOLDISK_ID}, "-disk",{CM_OMI_DATA_STRING, 64, {"[0-9a-zA-Z_]{1,63}"}}},
    {{"raid", CM_OMI_FIELD_POOLDISK_RAID}, "-raid",{CM_OMI_DATA_ENUM, 4, {&CmOmiMapEnumPoolRaidType}}},
    {{"type", CM_OMI_FIELD_POOLDISK_TYPE},"-type", {CM_OMI_DATA_ENUM, 4, {&CmCnmPoolMapDiskTypeUserCfg}}},
    {{"err_read", CM_OMI_FIELD_POOLDISK_ERR_READ},"-er", {CM_OMI_DATA_INT, 4, {NULL}}},
    {{"err_write", CM_OMI_FIELD_POOLDISK_ERR_WRITE},"-er", {CM_OMI_DATA_INT, 4, {NULL}}},
    {{"err_sum", CM_OMI_FIELD_POOLDISK_ERR_SUM},"-er", {CM_OMI_DATA_INT, 4, {NULL}}},
    {{"en_id", CM_OMI_FIELD_POOLDISK_ENID},"-er", {CM_OMI_DATA_INT, 4, {NULL}}},
    {{"slot_id", CM_OMI_FIELD_POOLDISK_SLOTID},"-er", {CM_OMI_DATA_INT, 4, {NULL}}},
    {{"status", CM_OMI_FIELD_POOLDISK_STATUS},"-status", {CM_OMI_DATA_ENUM, 4, {&CmOmiMapEnumPoolStatusType}}},
    {{"group", CM_OMI_FIELD_POOLDISK_GROUP},"-group", {CM_OMI_DATA_INT, 4, {NULL}}},
    {{"size", CM_OMI_FIELD_POOLDISK_SIZE}, "-size",{CM_OMI_DATA_STRING, 64, {"[0-9a-zA-Z_]{1,63}"}}},
};

const cm_omi_map_object_field_t* CmOmiMapPoolDiskCmdParamsAdd[]=
{
    &CmOmiMapFieldsPoolDisk[CM_OMI_FIELD_POOLDISK_NID], /*node id*/
    &CmOmiMapFieldsPoolDisk[CM_OMI_FIELD_POOLDISK_NAME], /*name*/
    &CmOmiMapCommFields[7], /*param*/
};

const cm_omi_map_object_field_t* CmOmiMapPoolDiskCmdParamsAddOpt[]=
{
    &CmOmiMapFieldsPoolDisk[CM_OMI_FIELD_POOLDISK_TYPE], /*type*/
    &CmOmiMapFieldsPoolDisk[CM_OMI_FIELD_POOLDISK_RAID], /*raid*/
    &CmOmiMapFieldsPoolDisk[CM_OMI_FIELD_POOLDISK_GROUP], /*name*/
};


const cm_omi_map_object_field_t* CmOmiMapPoolDiskCmdParamsGetBatch[]=
{
    &CmOmiMapFieldsPoolDisk[CM_OMI_FIELD_POOLDISK_TYPE], /*type*/
    &CmOmiMapFieldsPoolDisk[CM_OMI_FIELD_POOLDISK_RAID], /*raid*/
    &CmOmiMapCommFields[1], /*offset*/
    &CmOmiMapCommFields[2], /*total*/
};


const cm_omi_map_object_cmd_t CmOmiMapPoolDiskCmds[] =
{
    /* getbatch */
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET_BATCH],
        2,
        CmOmiMapPoolDiskCmdParamsAdd,
        4,
        CmOmiMapPoolDiskCmdParamsGetBatch,
    },
    /* count */
    {
        &CmOmiMapCmds[CM_OMI_CMD_COUNT],        
        2,
        CmOmiMapPoolDiskCmdParamsAdd,
        2,
        CmOmiMapPoolDiskCmdParamsGetBatch,
    },
    /* add */
    {
        &CmOmiMapCmds[CM_OMI_CMD_ADD],        
        3,
        CmOmiMapPoolDiskCmdParamsAdd,
        3,
        CmOmiMapPoolDiskCmdParamsAddOpt,
    },
    /* delete */
    {
        &CmOmiMapCmds[CM_OMI_CMD_DELETE],        
        3,
        CmOmiMapPoolDiskCmdParamsAdd,
        0,
        NULL,
    },
};

const cm_omi_map_object_t CmCnmPoolDiskCfg =
{
    {"pool_disk", CM_OMI_OBJECT_POOLDISK},
    sizeof(CmOmiMapFieldsPoolDisk)/sizeof(cm_omi_map_object_field_t),
    CmOmiMapFieldsPoolDisk,
    sizeof(CmOmiMapPoolDiskCmds)/sizeof(cm_omi_map_object_cmd_t),
    CmOmiMapPoolDiskCmds
}; 

const cm_omi_map_object_field_t CmOmiMapFieldsPoolReconstruct[] =
{
    {{"name", CM_OMI_FIELD_POOL_RECONSTRUCT_NAME}, "-n",{CM_OMI_DATA_STRING, 64, {"[0-9a-zA-Z_]{1,63}"}}},
    {{"nid", CM_OMI_FIELD_POOL_RECONSTRUCT_NID}, "-nid",{CM_OMI_DATA_INT, 4, {NULL}}},
    {{"status", CM_OMI_FIELD_POOL_RECONSTRUCT_STATUS}, "-st", {CM_OMI_DATA_STRING, 32,{NULL}}},   
    {{"progress", CM_OMI_FIELD_POOL_RECONSTRUCT_PROGRESS}, "-pg", {CM_OMI_DATA_STRING, 32,{NULL}}},
    {{"speed", CM_OMI_FIELD_POOL_RECONSTRUCT_SPEED}, "-sd", {CM_OMI_DATA_STRING, 32,{NULL}}},
    {{"time_cost", CM_OMI_FIELD_POOL_RECONSTRUCT_TIME_COST},"-tc", {CM_OMI_DATA_STRING, 32, {NULL}}},
};
const cm_omi_map_object_field_t* CmOmiMapPoolReconstructCmdParamsGetBatch[]=
{
    &CmOmiMapFieldsPoolReconstruct[CM_OMI_FIELD_POOL_RECONSTRUCT_NID], /*node id*/
    &CmOmiMapCommFields[1], /*offset*/
    &CmOmiMapCommFields[2], /*total*/
};
const cm_omi_map_object_field_t* CmOmiMapPoolReconstructCmdParamsCount[]=
{
    &CmOmiMapFieldsPoolReconstruct[CM_OMI_FIELD_POOL_RECONSTRUCT_NID], /*node id*/
};
const cm_omi_map_object_cmd_t CmOmiMapPoolReconstructCmds[] =
{
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET_BATCH],
        0,
        NULL,
        3,
        CmOmiMapPoolReconstructCmdParamsGetBatch,
    },
    {
       &CmOmiMapCmds[CM_OMI_CMD_COUNT],
        0,
        NULL,
        1,
        CmOmiMapPoolReconstructCmdParamsCount,
    },
};
const cm_omi_map_object_t CmCnmPoolReconstructCfg =
{
    {"pool_reconstruct", CM_OMI_OBJECT_POOL_RECONSTRUCT},
    sizeof(CmOmiMapFieldsPoolReconstruct)/sizeof(cm_omi_map_object_field_t),
    CmOmiMapFieldsPoolReconstruct,
    sizeof(CmOmiMapPoolReconstructCmds)/sizeof(cm_omi_map_object_cmd_t),
    CmOmiMapPoolReconstructCmds
}; 

const cm_omi_map_enum_t CmOmiMapEnumPoolRaidTypex =
{
    5,
    CmOmiMapPoolRaidType
};

const cm_omi_map_cfg_t CmOmiMapPoolDiskPolicy[] =
{
    {"local",0},
    {"distributed",1},
};


const cm_omi_map_enum_t CmOmiMapEnumPoolDiskPolicyType =
{
    sizeof(CmOmiMapPoolDiskPolicy)/sizeof(cm_omi_map_cfg_t),
    CmOmiMapPoolDiskPolicy
};


const cm_omi_map_object_field_t CmOmiMapFieldsPoolExt[] =
{
    {{"name", CM_OMI_FIELD_POOLEXT_NAME}, "-n",{CM_OMI_DATA_STRING, 64, {"[0-9a-zA-Z_]{1,63}"}}},
    {{"nid", CM_OMI_FIELD_POOLEXT_NID}, "-nid",{CM_OMI_DATA_INT, 4, {NULL}}},
    {{"force", CM_OMI_FIELD_POOLEXT_FORCE}, "-force", {CM_OMI_DATA_ENUM, 4, {&CmOmiMapEnumBoolType}}},  
    {{"policy", CM_OMI_FIELD_POOLEXT_POLICY}, "-policy", {CM_OMI_DATA_ENUM, 4, {&CmOmiMapEnumPoolDiskPolicyType}}},
    
    {{"data_raid", CM_OMI_FIELD_POOLEXT_DATA_RAID}, "-dr", {CM_OMI_DATA_ENUM, 4, {&CmOmiMapEnumPoolRaidTypex}}},
    {{"data_num", CM_OMI_FIELD_POOLEXT_DATA_NUM}, "-dn", {CM_OMI_DATA_INT, 4,{NULL}}},
    {{"data_group", CM_OMI_FIELD_POOLEXT_DATA_GROUP}, "-dg", {CM_OMI_DATA_INT, 4,{NULL}}},
    {{"data_spare", CM_OMI_FIELD_POOLEXT_DATA_SPARE},"-ds", {CM_OMI_DATA_INT, 4, {NULL}}},

    {{"meta_raid", CM_OMI_FIELD_POOLEXT_META_RAID}, "-mr", {CM_OMI_DATA_ENUM, 4, {&CmOmiMapEnumPoolRaidTypex}}},
    {{"meta_num", CM_OMI_FIELD_POOLEXT_META_NUM}, "-mn", {CM_OMI_DATA_INT, 4,{NULL}}},
    {{"meta_group", CM_OMI_FIELD_POOLEXT_META_GROUP}, "-mg", {CM_OMI_DATA_INT, 4,{NULL}}},
    {{"meta_spare", CM_OMI_FIELD_POOLEXT_META_SPARE},"-ms", {CM_OMI_DATA_INT, 4, {NULL}}},

    {{"low_raid", CM_OMI_FIELD_POOLEXT_LOW_RAID}, "-lr", {CM_OMI_DATA_ENUM, 4, {&CmOmiMapEnumPoolRaidTypex}}},
    {{"low_num", CM_OMI_FIELD_POOLEXT_LOW_NUM}, "-ln", {CM_OMI_DATA_INT, 4,{NULL}}},
    {{"low_group", CM_OMI_FIELD_POOLEXT_LOW_GROUP}, "-lg", {CM_OMI_DATA_INT, 4,{NULL}}},
    {{"low_spare", CM_OMI_FIELD_POOLEXT_LOW_SPARE},"-ls", {CM_OMI_DATA_INT, 4, {NULL}}},
};

const cm_omi_map_object_field_t* CmOmiMapPoolExtCmdParamsSet[]=
{
    &CmOmiMapFieldsPoolExt[CM_OMI_FIELD_POOLEXT_NID], /*node id*/
    &CmOmiMapFieldsPoolExt[CM_OMI_FIELD_POOLEXT_NAME], /*name*/
};

const cm_omi_map_object_field_t* CmOmiMapPoolExtCmdParamsSetOpt[]=
{
    &CmOmiMapFieldsPoolExt[CM_OMI_FIELD_POOLEXT_FORCE],
    &CmOmiMapFieldsPoolExt[CM_OMI_FIELD_POOLEXT_POLICY],
    
    &CmOmiMapFieldsPoolExt[CM_OMI_FIELD_POOLEXT_DATA_RAID],
    &CmOmiMapFieldsPoolExt[CM_OMI_FIELD_POOLEXT_DATA_NUM],
    &CmOmiMapFieldsPoolExt[CM_OMI_FIELD_POOLEXT_DATA_GROUP],
    &CmOmiMapFieldsPoolExt[CM_OMI_FIELD_POOLEXT_DATA_SPARE],

    &CmOmiMapFieldsPoolExt[CM_OMI_FIELD_POOLEXT_META_RAID],
    &CmOmiMapFieldsPoolExt[CM_OMI_FIELD_POOLEXT_META_NUM],
    &CmOmiMapFieldsPoolExt[CM_OMI_FIELD_POOLEXT_META_GROUP],
    &CmOmiMapFieldsPoolExt[CM_OMI_FIELD_POOLEXT_META_SPARE],

    &CmOmiMapFieldsPoolExt[CM_OMI_FIELD_POOLEXT_LOW_RAID],
    &CmOmiMapFieldsPoolExt[CM_OMI_FIELD_POOLEXT_LOW_NUM],
    &CmOmiMapFieldsPoolExt[CM_OMI_FIELD_POOLEXT_LOW_GROUP],
    &CmOmiMapFieldsPoolExt[CM_OMI_FIELD_POOLEXT_LOW_SPARE],
};


const cm_omi_map_object_cmd_t CmOmiMapPoolExtCmds[] =
{
    {
        &CmOmiMapCmds[CM_OMI_CMD_ADD],
        2,
        CmOmiMapPoolExtCmdParamsSet,
        sizeof(CmOmiMapPoolExtCmdParamsSetOpt)/sizeof(cm_omi_map_object_field_t*),
        CmOmiMapPoolExtCmdParamsSetOpt,
    },
    {
        &CmOmiMapCmds[CM_OMI_CMD_MODIFY],
        2,
        CmOmiMapPoolExtCmdParamsSet,
        sizeof(CmOmiMapPoolExtCmdParamsSetOpt)/sizeof(cm_omi_map_object_field_t*),
        CmOmiMapPoolExtCmdParamsSetOpt,
    },
};


const cm_omi_map_object_t CmCnmPoolExtCfg =
{
    {"poolext", CM_OMI_OBJECT_POOLEXT},
    sizeof(CmOmiMapFieldsPoolExt)/sizeof(cm_omi_map_object_field_t),
    CmOmiMapFieldsPoolExt,
    sizeof(CmOmiMapPoolExtCmds)/sizeof(cm_omi_map_object_cmd_t),
    CmOmiMapPoolExtCmds
}; 


