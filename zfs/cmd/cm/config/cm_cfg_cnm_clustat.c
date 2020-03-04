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
     



const cm_omi_map_object_field_t CmOmiMapClusterCapacityFileds[] =
{
    {{"disk_used", CM_OMI_FIELD_CLUSTER_DISK_USED}, "-du", {CM_OMI_DATA_INT, 64, {NULL}}},
    {{"disk_total", CM_OMI_FIELD_CLUSTER_DISK_AVAIL}, "-da", {CM_OMI_DATA_INT, 64, {NULL}}},
    {{"pool_used", CM_OMI_FIELD_CLUSTER_POOL_USED}, "-pu", {CM_OMI_DATA_INT, 64, {NULL}}},
    {{"pool_total", CM_OMI_FIELD_CLUSTER_POOL_AVAIL},"-pa", {CM_OMI_DATA_INT, 64, {NULL}}},
    {{"lun_used", CM_OMI_FIELD_CLUSTER_LUN_USED},"-lu", {CM_OMI_DATA_INT, 64, {NULL}}},
    {{"lun_total", CM_OMI_FIELD_CLUSTER_LUN_AVAIL},"-la", {CM_OMI_DATA_INT, 64, {NULL}}},
    {{"nas_used", CM_OMI_FIELD_CLUSTER_NAS_USED},"-nu", {CM_OMI_DATA_INT, 64, {NULL}}},
    {{"nas_total", CM_OMI_FIELD_CLUSTER_NAS_AVAIL},"-na", {CM_OMI_DATA_INT, 64, {NULL}}},
    
    {{"high_used", CM_OMI_FIELD_CLUSTER_H_USED},"-dku", {CM_OMI_DATA_INT, 64, {NULL}}},
    {{"high_total", CM_OMI_FIELD_CLUSTER_H_TOTAL},"-dkt", {CM_OMI_DATA_INT, 64, {NULL}}},
    {{"low_used", CM_OMI_FIELD_CLUSTER_L_USED},"-lku", {CM_OMI_DATA_INT, 64, {NULL}}},
    {{"low_total", CM_OMI_FIELD_CLUSTER_L_TOTAL},"-lkt", {CM_OMI_DATA_INT, 64, {NULL}}},
};

const cm_omi_map_object_cmd_t CmOmiMapClusterStatCmds[] =
{
    /* get */
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET],
        0,
        NULL,
        0,
        NULL
    }
    
};

const cm_omi_map_object_t CmCnmClusterStatCfg =
{
    {"capacity", CM_OMI_OBJECT_CAPACITY},
    sizeof(CmOmiMapClusterCapacityFileds)/sizeof(cm_omi_map_object_field_t),
    CmOmiMapClusterCapacityFileds,
    sizeof(CmOmiMapClusterStatCmds)/sizeof(cm_omi_map_object_cmd_t),
    CmOmiMapClusterStatCmds
};


















