/******************************************************************************
 *          Copyright (c) 2017 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cfg_cnm_pmm.c
 * author     : wbn
 * create date: 2017Äê12ÔÂ21ÈÕ
 * description: TODO:
 *
 *****************************************************************************/
#include "cm_omi_types.h"
#include "cm_cfg_cnm.h"

const cm_omi_map_object_field_t CmOmiMapPmmClusterFields[] =
{
    {{"id", CM_OMI_FIELD_PMM_CLUSTER_ID}, "-i",{CM_OMI_DATA_STRING, CM_ID_LEN, {"[0-9]{1,20}"}}},
    {{"datetime", CM_OMI_FIELD_PMM_CLUSTER_TIMESTAMP}, "-t",{CM_OMI_DATA_TIME, 4, {NULL}}},
    {{"cpu_max", CM_OMI_FIELD_PMM_CLUSTER_CPU_MAX},"-cm", {CM_OMI_DATA_DOUBLE, 8, {NULL}}},
    {{"cpu_avg", CM_OMI_FIELD_PMM_CLUSTER_CPU_AVG},"-ca", {CM_OMI_DATA_DOUBLE, 8, {NULL}}},
    {{"mem_max", CM_OMI_FIELD_PMM_CLUSTER_MEM_MAX},"-mm", {CM_OMI_DATA_DOUBLE, 8, {NULL}}},
    {{"mem_avg", CM_OMI_FIELD_PMM_CLUSTER_MEM_AVG},"-ma", {CM_OMI_DATA_DOUBLE, 8, {NULL}}},
    {{"bw_total", CM_OMI_FIELD_PMM_CLUSTER_BW_TOTAL},"-bt", {CM_OMI_DATA_DOUBLE, 8, {NULL}}},
    {{"bw_read", CM_OMI_FIELD_PMM_CLUSTER_BW_READ},"-br", {CM_OMI_DATA_DOUBLE, 8, {NULL}}},
    {{"bw_write", CM_OMI_FIELD_PMM_CLUSTER_BW_WRITE},"-bw", {CM_OMI_DATA_DOUBLE, 8, {NULL}}},
    {{"iops_total", CM_OMI_FIELD_PMM_CLUSTER_IOPS_TOTAL},"-it", {CM_OMI_DATA_DOUBLE, 8, {NULL}}},
    {{"iops_read", CM_OMI_FIELD_PMM_CLUSTER_IOPS_READ},"-ir", {CM_OMI_DATA_DOUBLE, 8, {NULL}}},
    {{"iops_write", CM_OMI_FIELD_PMM_CLUSTER_IOPS_WRITE},"-iw", {CM_OMI_DATA_DOUBLE, 8, {NULL}}},
};

const cm_omi_map_object_field_t* CmOmiMapPmmClusterCmdParamsGetBatch[]=
{
    &CmOmiMapCommFields[0], /*fields*/
    &CmOmiMapCommFields[2], /*total */
    &CmOmiMapCommFields[5], /*time_start */
    &CmOmiMapCommFields[6], /*time_end */
    &CmOmiMapCommFields[8], /*grain */
};


const cm_omi_map_object_cmd_t CmOmiMapPmmClusterCmds[] =
{
    /* getbatch */
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET_BATCH],
        0,
        NULL,
        sizeof(CmOmiMapPmmClusterCmdParamsGetBatch)/sizeof(cm_omi_map_object_field_t*),
        CmOmiMapPmmClusterCmdParamsGetBatch
    },
    /* get */
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET],        
        0,
        NULL,
        0,
        NULL,
    }
};

const cm_omi_map_object_t CmCnmPmmClusterCfg =
{
    {"pmm_cluster", CM_OMI_OBJECT_PMM_CLUSTER},
    sizeof(CmOmiMapPmmClusterFields)/sizeof(cm_omi_map_object_field_t),
    CmOmiMapPmmClusterFields,
    sizeof(CmOmiMapPmmClusterCmds)/sizeof(cm_omi_map_object_cmd_t),
    CmOmiMapPmmClusterCmds
};

const cm_omi_map_object_field_t CmOmiMapPmmNodeFields[] =
{
    {{"id", CM_OMI_FIELD_PMM_NODE_ID}, "-i",{CM_OMI_DATA_STRING, CM_ID_LEN, {"[0-9]{1,20}"}}},
    {{"datetime", CM_OMI_FIELD_PMM_NODE_TIMESTAMP}, "-t",{CM_OMI_DATA_TIME, 4, {NULL}}},
    {{"cpu", CM_OMI_FIELD_PMM_NODE_CPU},"-cm", {CM_OMI_DATA_DOUBLE, 8, {NULL}}},
    {{"mem", CM_OMI_FIELD_PMM_NODE_MEM},"-ma", {CM_OMI_DATA_DOUBLE, 8, {NULL}}},
    {{"bw_total", CM_OMI_FIELD_PMM_NODE_BW_TOTAL},"-bt", {CM_OMI_DATA_DOUBLE, 8, {NULL}}},
    {{"bw_read", CM_OMI_FIELD_PMM_NODE_BW_READ},"-br", {CM_OMI_DATA_DOUBLE, 8, {NULL}}},
    {{"bw_write", CM_OMI_FIELD_PMM_NODE_BW_WRITE},"-bw", {CM_OMI_DATA_DOUBLE, 8, {NULL}}},
    {{"iops_total", CM_OMI_FIELD_PMM_NODE_IOPS_TOTAL},"-it", {CM_OMI_DATA_DOUBLE, 8, {NULL}}},
    {{"iops_read", CM_OMI_FIELD_PMM_NODE_IOPS_READ},"-ir", {CM_OMI_DATA_DOUBLE, 8, {NULL}}},
    {{"iops_write", CM_OMI_FIELD_PMM_NODE_IOPS_WRITE},"-iw", {CM_OMI_DATA_DOUBLE, 8, {NULL}}},
};

const cm_omi_map_object_field_t* CmOmiMapPmmNodeCmdParamsGet[]=
{
    &CmOmiMapCommFields[4], /*parent_id*/
};


const cm_omi_map_object_cmd_t CmOmiMapPmmNodeCmds[] =
{
    /* getbatch */
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET_BATCH],
        1,
        CmOmiMapPmmNodeCmdParamsGet,
        sizeof(CmOmiMapPmmClusterCmdParamsGetBatch)/sizeof(cm_omi_map_object_field_t*),
        CmOmiMapPmmClusterCmdParamsGetBatch
    },
    /* get */
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET],        
        1,
        CmOmiMapPmmNodeCmdParamsGet,
        0,
        NULL,
    }
};


const cm_omi_map_object_t CmCnmPmmNodeCfg =
{
    {"pmm_node", CM_OMI_OBJECT_PMM_NODE},
    sizeof(CmOmiMapPmmNodeFields)/sizeof(cm_omi_map_object_field_t),
    CmOmiMapPmmNodeFields,
    sizeof(CmOmiMapPmmNodeCmds)/sizeof(cm_omi_map_object_cmd_t),
    CmOmiMapPmmNodeCmds
};

const cm_omi_map_object_field_t CmOmiMapPmmConfigFields[] =
{
    {{"save_days", CM_OMI_FIELD_PMM_CFG_SAVE_HOURS}, "-sd",{CM_OMI_DATA_INT, 4, {NULL}}},
};

const cm_omi_map_object_field_t* CmOmiMapPmmConfigCmdParamsModify[]=
{
    &CmOmiMapPmmConfigFields[CM_OMI_FIELD_PMM_CFG_SAVE_HOURS], /*save_hours*/
};


const cm_omi_map_object_cmd_t CmOmiMapPmmConfigCmds[] =
{
    /* get */
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET],        
        0,
        NULL,
        0,
        NULL,
    },
    /* modify */
    {
        &CmOmiMapCmds[CM_OMI_CMD_MODIFY],
        0,
        NULL,
        sizeof(CmOmiMapPmmConfigCmdParamsModify)/sizeof(cm_omi_map_object_field_t*),
        CmOmiMapPmmConfigCmdParamsModify
    },
};


const cm_omi_map_object_t CmCnmPmmConfigCfg =
{
    {"pmm_cfg", CM_OMI_OBJECT_PMM_CONFIG},
    sizeof(CmOmiMapPmmConfigFields)/sizeof(cm_omi_map_object_field_t),
    CmOmiMapPmmConfigFields,
    sizeof(CmOmiMapPmmConfigCmds)/sizeof(cm_omi_map_object_cmd_t),
    CmOmiMapPmmConfigCmds
};



const cm_omi_map_object_field_t CmOmiMapPmmCmd[] =
{
    {{"nid", CM_OMI_FIELD_PMM_NID}, "-nid",{CM_OMI_DATA_INT,4, {NULL}}},
    {{"name", CM_OMI_FIELD_PMM_NAME},"-name", {CM_OMI_DATA_STRING, 128, {NULL}}},
};

const cm_omi_map_object_field_t* CmOmiMapCnmCmdParamsGet[]=
{
    &CmOmiMapPmmCmd[0], /*nid*/
    &CmOmiMapPmmCmd[1], /*name*/
};


/*
*nas
*/
const cm_omi_map_object_field_t CmOmiMapPmmNasFields[] =
{
    {{"id", CM_OMI_FIELD_PMM_NAS_ID}, "-i",{CM_OMI_DATA_STRING, CM_ID_LEN, {"[0-9]{1,20}"}}},
    {{"datetime", CM_OMI_FIELD_PMM_NAS_TIMESTAMP}, "-t",{CM_OMI_DATA_TIME, 4, {NULL}}},
    {{"newfile", CM_OMI_FIELD_PMM_NAS_NEWFILE},"-newf", {CM_OMI_DATA_DOUBLE, 8, {NULL}}},
    {{"rmfile", CM_OMI_FIELD_PMM_NAS_RMFILE},"-rmf", {CM_OMI_DATA_DOUBLE, 8, {NULL}}},
    {{"readdir", CM_OMI_FIELD_PMM_NAS_ERDDIR},"-readd", {CM_OMI_DATA_DOUBLE, 8, {NULL}}},
    {{"iops", CM_OMI_FIELD_PMM_NAS_IOPS},"-iops", {CM_OMI_DATA_DOUBLE, 8, {NULL}}},
    {{"reads", CM_OMI_FIELD_PMM_NAS_READ},"-red", {CM_OMI_DATA_DOUBLE, 8, {NULL}}},
    {{"writes", CM_OMI_FIELD_PMM_NAS_WRITTEN},"-wri", {CM_OMI_DATA_DOUBLE, 8, {NULL}}},
	{{"delay", CM_OMI_FIELD_PMM_NAS_DELAY},"-d", {CM_OMI_DATA_DOUBLE, 8, {NULL}}},
};

const cm_omi_map_object_field_t* CmOmiMapPmmNasCmdParamsGet[]=
{
    &CmOmiMapCommFields[4], /*parent_id*/
    &CmOmiMapCommFields[7], /*param*/
};


const cm_omi_map_object_cmd_t CmOmiMapPmmNasCmds[] =
{
    /* get */
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET],        
        2,
        CmOmiMapPmmNasCmdParamsGet,
        0,
        NULL,
    }
};


const cm_omi_map_object_t CmCnmPmmNasCfg =
{
    {"pmm_nas", CM_OMI_OBJECT_PMM_NAS},
    sizeof(CmOmiMapPmmNasFields)/sizeof(cm_omi_map_object_field_t),
    CmOmiMapPmmNasFields,
    sizeof(CmOmiMapPmmNasCmds)/sizeof(cm_omi_map_object_cmd_t),
    CmOmiMapPmmNasCmds
};

/*
*sas
*/

const cm_omi_map_object_field_t CmOmiMapPmmSasFields[] =
{
    {{"id", CM_OMI_FIELD_PMM_SAS_ID}, "-i",{CM_OMI_DATA_STRING, CM_ID_LEN, {"[0-9]{1,20}"}}},
    {{"datetime", CM_OMI_FIELD_PMM_SAS_TIMESTAMP}, "-t",{CM_OMI_DATA_TIME, 4, {NULL}}},     
    {{"reads", CM_OMI_FIELD_PMM_SAS_READS},"-reads", {CM_OMI_DATA_DOUBLE, 8, {NULL}}},
    {{"writes", CM_OMI_FIELD_PMM_SAS_WRITTES},"-writes", {CM_OMI_DATA_DOUBLE, 8, {NULL}}},
    {{"riops", CM_OMI_FIELD_PMM_SAS_RIOPS},"-riops", {CM_OMI_DATA_DOUBLE, 8, {NULL}}},
    {{"wiops", CM_OMI_FIELD_PMM_SAS_WIOPS},"-wiops", {CM_OMI_DATA_DOUBLE, 8, {NULL}}},
};

const cm_omi_map_object_cmd_t CmOmiMapPmmSasCmds[] =
{
    /* get */
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET],        
        2,
        CmOmiMapPmmNasCmdParamsGet,
        0,
        NULL,
    }
};


const cm_omi_map_object_t CmCnmPmmSasCfg =
{
    {"pmm_sas", CM_OMI_OBJECT_PMM_SAS},
    sizeof(CmOmiMapPmmSasFields)/sizeof(cm_omi_map_object_field_t),
    CmOmiMapPmmSasFields,
    sizeof(CmOmiMapPmmSasCmds)/sizeof(cm_omi_map_object_cmd_t),
    CmOmiMapPmmSasCmds
};

const cm_omi_map_object_cmd_t CmOmiMapCnmSasCmds[] =
{
    /* getbatch */
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET_BATCH],        
        0,
        NULL,
        0,
        NULL,
    }
};


const cm_omi_map_object_t CmCnmSasCfg =
{
    {"sas", CM_OMI_OBJECT_CNM_SAS},
    sizeof(CmOmiMapPmmCmd)/sizeof(cm_omi_map_object_field_t),
    CmOmiMapPmmCmd,
    sizeof(CmOmiMapCnmSasCmds)/sizeof(cm_omi_map_object_cmd_t),
    CmOmiMapCnmSasCmds
};


/*
*protocol
*/

const cm_omi_map_object_field_t CmOmiMapPmmProtocolFields[] =
{
    {{"id", CM_OMI_FIELD_PMM_PROTO_ID}, "-i",{CM_OMI_DATA_STRING, CM_ID_LEN, {"[0-9]{1,20}"}}},
    {{"datetime", CM_OMI_FIELD_PMM_PROTO_TIMESTAMP}, "-t",{CM_OMI_DATA_TIME, 4, {NULL}}},    
    {{"iops", CM_OMI_FIELD_PMM_PROTO_IOPS},"-iops", {CM_OMI_DATA_DOUBLE, 8, {NULL}}},
};

const cm_omi_map_object_cmd_t CmOmiMapPmmProtocolCmds[] =
{
    /* get */
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET],        
        2,
        CmOmiMapPmmNasCmdParamsGet,
        0,
        NULL,
    }
};


const cm_omi_map_object_t CmCnmPmmProtocolCfg =
{
    {"pmm_proto", CM_OMI_OBJECT_PMM_PROTO},
    sizeof(CmOmiMapPmmProtocolFields)/sizeof(cm_omi_map_object_field_t),
    CmOmiMapPmmProtocolFields,
    sizeof(CmOmiMapPmmProtocolCmds)/sizeof(cm_omi_map_object_cmd_t),
    CmOmiMapPmmProtocolCmds
};

const cm_omi_map_object_cmd_t CmOmiMapCnmProtoCmds[] =
{
    /* getbatch */
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET_BATCH],        
        0,
        NULL,
        0,
        NULL,
    }
};


const cm_omi_map_object_t CmCnmProtoCfg =
{
    {"protocol", CM_OMI_OBJECT_CNM_PROTO},
    sizeof(CmOmiMapPmmCmd)/sizeof(cm_omi_map_object_field_t),
    CmOmiMapPmmCmd,
    sizeof(CmOmiMapCnmProtoCmds)/sizeof(cm_omi_map_object_cmd_t),
    CmOmiMapCnmProtoCmds
};


/*
*cache
*/

const cm_omi_map_object_field_t CmOmiMapPmmCacheFields[] =
{
    {{"id", CM_OMI_FIELD_PMM_CACHE_ID}, "-i",{CM_OMI_DATA_STRING, CM_ID_LEN, {"[0-9]{1,20}"}}},
    {{"datetime", CM_OMI_FIELD_PMM_CACHE_TIMESTAMP}, "-t",{CM_OMI_DATA_TIME, 4, {NULL}}},   
    {{"l1_hits", CM_OMI_FIELD_PMM_CACHE_L1_HIT},"-l1_h", {CM_OMI_DATA_DOUBLE, 8, {NULL}}},
    {{"l1_misses", CM_OMI_FIELD_PMM_CACHE_L1_MISS},"-l1_m", {CM_OMI_DATA_DOUBLE, 8, {NULL}}},
    {{"l1_size", CM_OMI_FIELD_PMM_CACHE_L1_SIZE},"-l1_s", {CM_OMI_DATA_DOUBLE, 8, {NULL}}},
    {{"l2_hits", CM_OMI_FIELD_PMM_CACHE_L2_HIT},"-l2_h", {CM_OMI_DATA_DOUBLE, 8, {NULL}}},
    {{"l2_misses", CM_OMI_FIELD_PMM_CACHE_L2_MISS},"-l2_m", {CM_OMI_DATA_DOUBLE, 8, {NULL}}},
    {{"l2_size", CM_OMI_FIELD_PMM_CACHE_L2_SIZE},"-l2_s", {CM_OMI_DATA_DOUBLE, 8, {NULL}}},
};

const cm_omi_map_object_cmd_t CmOmiMapPmmCacheCmds[] =
{
    /* get */
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET],        
        1,
        CmOmiMapPmmNasCmdParamsGet,
        0,
        NULL,
    }
};


const cm_omi_map_object_t CmCnmPmmCacheCfg =
{
    {"pmm_cache", CM_OMI_OBJECT_PMM_CACHE},
    sizeof(CmOmiMapPmmCacheFields)/sizeof(cm_omi_map_object_field_t),
    CmOmiMapPmmCacheFields,
    sizeof(CmOmiMapPmmCacheCmds)/sizeof(cm_omi_map_object_cmd_t),
    CmOmiMapPmmCacheCmds
};


