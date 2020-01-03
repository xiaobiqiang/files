/******************************************************************************
 *          Copyright (c) 2017 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cfg_cnm_pmm_lun.c
 * author     : wbn
 * create date: 2017Äê11ÔÂ13ÈÕ
 * description: TODO:
 *
 *****************************************************************************/
#include "cm_omi_types.h"
#include "cm_cfg_cnm.h"
   
const cm_omi_map_object_field_t CmOmiMapFieldsPmmLun[] =
{
    {{"id", CM_OMI_FIELD_PMM_LUN_ID}, "-id", {CM_OMI_DATA_INT, 4, {NULL}}},
    {{"datetime", CM_OMI_FIELD_PMM_LUN_TIME}, "-tm", {CM_OMI_DATA_TIME, 8, {NULL}}},
    {{"reads", CM_OMI_FIELD_PMM_LUN_READS}, "reads", {CM_OMI_DATA_DOUBLE, 8, {NULL}}},
    {{"writes", CM_OMI_FIELD_PMM_LUN_WRITES}, "writes", {CM_OMI_DATA_DOUBLE, 8, {NULL}}},
    {{"nread", CM_OMI_FIELD_PMM_LUN_NREAD}, "nr", {CM_OMI_DATA_DOUBLE, 8, {NULL}}},
    {{"nread_kb", CM_OMI_FIELD_PMM_LUN_NREAD_KB}, "nrk", {CM_OMI_DATA_DOUBLE, 8, {NULL}}},
    {{"nwritten", CM_OMI_FIELD_PMM_LUN_NWRITTEN}, "nw", {CM_OMI_DATA_DOUBLE, 8, {NULL}}},
    {{"nwritten_kb", CM_OMI_FIELD_PMM_LUN_NWRITTEN_KB}, "nwk", {CM_OMI_DATA_DOUBLE, 8, {NULL}}},
};

const cm_omi_map_object_field_t* CmOmiMapPmmLunCmdParamsCount[]=
{
    &CmOmiMapCommFields[4], /*parent_id*/
    &CmOmiMapCommFields[7], /*param*/
};


const cm_omi_map_object_cmd_t CmOmiMapPmmLunCmds[] =
{
    /* get */
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET],        
        2,
        CmOmiMapPmmLunCmdParamsCount,
        0,
        NULL,
    },
    
};

const cm_omi_map_object_t CmCnmPmmLunCfg =
{
    {"pmm_lun", CM_OMI_OBJECT_PMM_LUN},
    sizeof(CmOmiMapFieldsPmmLun)/sizeof(cm_omi_map_object_field_t),
    CmOmiMapFieldsPmmLun,
    sizeof(CmOmiMapPmmLunCmds)/sizeof(cm_omi_map_object_cmd_t),
    CmOmiMapPmmLunCmds
}; 
/*********************************nic******************************************/
const cm_omi_map_object_field_t CmOmiMapFieldsPmmNic[] =
{
    {{"id", CM_OMI_FIELD_PMM_NIC_ID}, "-id", {CM_OMI_DATA_INT, 4, {NULL}}},
    {{"datetime", CM_OMI_FIELD_PMM_NIC_TIME}, "-tm", {CM_OMI_DATA_TIME, 8, {NULL}}},
    {{"collisions", CM_OMI_FIELD_PMM_NIC_COLLISIONS}, "col", {CM_OMI_DATA_DOUBLE, 8, {NULL}}},
    {{"ierrors", CM_OMI_FIELD_PMM_NIC_IERRORS}, "ierr", {CM_OMI_DATA_DOUBLE, 8, {NULL}}},
    {{"oerrors", CM_OMI_FIELD_PMM_NIC_OERRORS}, "oerr", {CM_OMI_DATA_DOUBLE, 8, {NULL}}},
    {{"rbytes", CM_OMI_FIELD_PMM_NIC_RBYTES}, "rbp", {CM_OMI_DATA_DOUBLE, 8, {NULL}}},
    {{"obytes", CM_OMI_FIELD_PMM_NIC_OBYTES}, "wbp", {CM_OMI_DATA_DOUBLE, 8, {NULL}}},
    {{"util", CM_OMI_FIELD_PMM_NIC_UTIL}, "util", {CM_OMI_DATA_DOUBLE, 8, {NULL}}},
    {{"sat", CM_OMI_FIELD_PMM_NIC_SAT}, "sat", {CM_OMI_DATA_DOUBLE, 8, {NULL}}},
};

const cm_omi_map_object_cmd_t CmOmiMapPmmNicCmds[] =
{
    /* get */
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET],        
        2,
        CmOmiMapPmmLunCmdParamsCount,
        0,
        NULL,
    },
    
};

const cm_omi_map_object_t CmCnmPmmNicCfg =
{
    {"pmm_nic", CM_OMI_OBJECT_PMM_NIC},
    sizeof(CmOmiMapFieldsPmmNic)/sizeof(cm_omi_map_object_field_t),
    CmOmiMapFieldsPmmNic,
    sizeof(CmOmiMapPmmNicCmds)/sizeof(cm_omi_map_object_cmd_t),
    CmOmiMapPmmNicCmds
}; 
/********************************disk******************************************/

const cm_omi_map_object_field_t CmOmiMapFieldsPmmDisk[] =
{
    {{"id", CM_OMI_FIELD_PMM_DISK_ID}, "-id", {CM_OMI_DATA_INT, 4, {NULL}}},
    {{"datetime", CM_OMI_FIELD_PMM_DISK_TIME}, "-tm", {CM_OMI_DATA_TIME, 8, {NULL}}},
    {{"reads", CM_OMI_FIELD_PMM_DISK_READS}, "reads", {CM_OMI_DATA_DOUBLE, 8, {NULL}}},
    {{"writes", CM_OMI_FIELD_PMM_DISK_WRITES}, "writes", {CM_OMI_DATA_DOUBLE, 8, {NULL}}},
    {{"nread", CM_OMI_FIELD_PMM_DISK_NREAD}, "nr", {CM_OMI_DATA_DOUBLE, 8, {NULL}}},
    {{"nread_kb", CM_OMI_FIELD_PMM_DISK_NREAD_KB}, "nkb", {CM_OMI_DATA_DOUBLE, 8, {NULL}}},
    {{"nwritten", CM_OMI_FIELD_PMM_DISK_NWRITTEN}, "nw", {CM_OMI_DATA_DOUBLE, 8, {NULL}}},
    {{"nwritten_kb", CM_OMI_FIELD_PMM_DISK_NWRITEN_KB}, "nwk", {CM_OMI_DATA_DOUBLE, 8, {NULL}}},
    {{"rlentime", CM_OMI_FIELD_PMM_DISK_RLENTIME}, "rlt", {CM_OMI_DATA_DOUBLE, 8, {NULL}}},
    {{"wlentime", CM_OMI_FIELD_PMM_DISK_WLENTIME}, "wlt", {CM_OMI_DATA_DOUBLE, 8, {NULL}}},
    {{"wtime", CM_OMI_FIELD_PMM_DISK_WTIME}, "wt", {CM_OMI_DATA_DOUBLE, 8, {NULL}}},
    {{"rtime", CM_OMI_FIELD_PMM_DISK_RTIME}, "rt", {CM_OMI_DATA_DOUBLE, 8, {NULL}}},
    {{"snaptime", CM_OMI_FIELD_PMM_DISK_SNAPTIME}, "snpt", {CM_OMI_DATA_DOUBLE, 8, {NULL}}},
};

const cm_omi_map_object_cmd_t CmOmiMapPmmDiskCmds[] =
{
    /* get */
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET],        
        2,
        CmOmiMapPmmLunCmdParamsCount,
        0,
        NULL,
    },
    
};

const cm_omi_map_object_t CmCnmPmmDiskCfg =
{
    {"pmm_disk", CM_OMI_OBJECT_PMM_DISK},
    sizeof(CmOmiMapFieldsPmmDisk)/sizeof(cm_omi_map_object_field_t),
    CmOmiMapFieldsPmmDisk,
    sizeof(CmOmiMapPmmDiskCmds)/sizeof(cm_omi_map_object_cmd_t),
    CmOmiMapPmmDiskCmds
}; 


