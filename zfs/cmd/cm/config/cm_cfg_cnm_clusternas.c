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
     
extern const cm_omi_map_enum_t CmOmiMapNasRoleTypeCfg;
extern const cm_omi_map_enum_t CmOmiMapClusterStatusEnumBoolType;
extern const cm_omi_map_enum_t CmOmiMapNasStatusEnumBoolType;
const cm_omi_map_object_field_t CmOmiMapFieldsClusterNas[] =
{
    {{"group_name", CM_OMI_FIELD_CLUSTER_NAS_GROUP_NAME}, "-n", {CM_OMI_DATA_STRING, CM_NAME_LEN_NAS, {"[0-9a-zA-Z_]{1,63}"}}},
    {{"nid", CM_OMI_FIELD_CLUSTER_NAS_NID}, "-nid",{CM_OMI_DATA_INT, 4, {NULL}}},
    {{"clu_status", CM_OMI_FIELD_CLUSTER_NAS_STATUS}, "-cs", {CM_OMI_DATA_ENUM, 2, {&CmOmiMapClusterStatusEnumBoolType}}},
    {{"nas", CM_OMI_FIELD_CLUSTER_NAS_ZFS_NAME}, "-nas",{CM_OMI_DATA_STRING, CM_NAME_LEN_NAS, {"((([a-z0-9A-Z_-]+)[./]{0,1})+)[a-z0-9A-Z_-]$"}}},
    {{"role", CM_OMI_FIELD_CLUSTER_NAS_ZFS_ROLE}, "-role",{CM_OMI_DATA_ENUM, 4, {&CmOmiMapNasRoleTypeCfg}}},
    {{"status", CM_OMI_FIELD_CLUSTER_NAS_ZFS_STATUS}, "-status", {CM_OMI_DATA_ENUM, 2, {&CmOmiMapNasStatusEnumBoolType}}},
    {{"avail", CM_OMI_FIELD_CLUSTER_NAS_ZFS_AVAIL},"-avail", {CM_OMI_DATA_STRING, 32,{"[0-9]{1,10}[.]{0,1}[0-9]{0,3}[KMTGkmtg]{1}"}}},
    {{"used", CM_OMI_FIELD_CLUSTER_NAS_ZFS_USED},"-used", {CM_OMI_DATA_STRING, 32, {"[0-9]{1,10}[.]{0,1}[0-9]{0,3}[KMTGkmtg]{1}"}}},
    {{"nas_num", CM_OMI_FIELD_CLUSTER_NAS_NUM}, "-nn",{CM_OMI_DATA_INT, 4, {NULL}}},
};

const cm_omi_map_object_field_t* CmOmiMapClusterNasCmdParamsGetBatch[]=
{
    &CmOmiMapFieldsClusterNas[CM_OMI_FIELD_CLUSTER_NAS_NID],/*nid*/
    &CmOmiMapFieldsClusterNas[CM_OMI_FIELD_CLUSTER_NAS_GROUP_NAME],/*groupname*/
    &CmOmiMapCommFields[1], /*offset*/
    &CmOmiMapCommFields[2], /*total*/
};

const cm_omi_map_object_field_t* CmOmiMapClusterNasCmdParamsAdd[]=
{
    
    &CmOmiMapFieldsClusterNas[CM_OMI_FIELD_CLUSTER_NAS_GROUP_NAME], /*groupname*/
    &CmOmiMapFieldsClusterNas[CM_OMI_FIELD_CLUSTER_NAS_NID], /*nid*/
    &CmOmiMapFieldsClusterNas[CM_OMI_FIELD_CLUSTER_NAS_ZFS_NAME], /*nasname*/
};
const cm_omi_map_object_field_t* CmOmiMapClusterNasCmdParamsCount[]=
{
    &CmOmiMapFieldsClusterNas[CM_OMI_FIELD_CLUSTER_NAS_NID], /*nid*/
    &CmOmiMapFieldsClusterNas[CM_OMI_FIELD_CLUSTER_NAS_GROUP_NAME], /*groupname*/
    &CmOmiMapFieldsClusterNas[CM_OMI_FIELD_CLUSTER_NAS_ZFS_ROLE], /*groupname*/
};
const cm_omi_map_object_field_t* CmOmiMapClusterNasCmdParamsSet[]=
{
    &CmOmiMapFieldsClusterNas[CM_OMI_FIELD_CLUSTER_NAS_ZFS_ROLE], /*role*/
};

const cm_omi_map_object_cmd_t CmOmiMapClusterNasCmds[] =
{
    /* getbatch */
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET_BATCH],
        0,
        NULL,
        4,
        CmOmiMapClusterNasCmdParamsGetBatch,
    },
    
    /* add */
    {
        &CmOmiMapCmds[CM_OMI_CMD_ADD],        
        3,
        CmOmiMapClusterNasCmdParamsAdd,
        0,
        NULL,
    },
    /* modify */
    {
        &CmOmiMapCmds[CM_OMI_CMD_MODIFY],        
        3,
        CmOmiMapClusterNasCmdParamsAdd,
        1,
        CmOmiMapClusterNasCmdParamsSet,
    },
    /* count */
    {
        &CmOmiMapCmds[CM_OMI_CMD_COUNT],        
        0,
        NULL,
        3,
        CmOmiMapClusterNasCmdParamsCount,
    },
    {
        &CmOmiMapCmds[CM_OMI_CMD_DELETE],        
        3,
        CmOmiMapClusterNasCmdParamsAdd,
        0,
        NULL,
    },
    
};

const cm_omi_map_object_t CmCnmClusterNasCfg =
{
    {"nas_cluster", CM_OMI_OBJECT_CLUSTER_NAS},
    sizeof(CmOmiMapFieldsClusterNas)/sizeof(cm_omi_map_object_field_t),
    CmOmiMapFieldsClusterNas,
    sizeof(CmOmiMapClusterNasCmds)/sizeof(cm_omi_map_object_cmd_t),
    CmOmiMapClusterNasCmds
}; 

