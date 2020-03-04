/******************************************************************************
 *          Copyright (c) 2018 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cfg_cnm_pmm_pool.h
 * author     : xar
 * create date: 2018.10.17
 * description: TODO:
 *
 *****************************************************************************/
#include "cm_omi_types.h"
#include "cm_cfg_cnm.h"


const cm_omi_map_object_field_t CmOmiMapFieldsPmm_pool[] =
{
    {{"id", CM_OMI_FIELD_PMM_POOL_ID}, "-i",{CM_OMI_DATA_STRING, CM_ID_LEN, {"[0-9]{1,20}"}}},
    {{"datetime", CM_OMI_FIELD_PMM_POOL_DATETIME},"-t", {CM_OMI_DATA_TIME, 4, {NULL}}},
    {{"riops", CM_OMI_FIELD_PMM_POOL_RIOPS},"-riops", {CM_OMI_DATA_DOUBLE, 8, {NULL}}},
    {{"wiops", CM_OMI_FIELD_PMM_POOL_WIOPS},"-wiops", {CM_OMI_DATA_DOUBLE, 8, {NULL}}}, 
    {{"rbitys", CM_OMI_FIELD_PMM_POOL_RBITYS},"-rbitys", {CM_OMI_DATA_DOUBLE, 8, {NULL}}},
    {{"wbitys", CM_OMI_FIELD_PMM_POOL_WBIRYS},"-wbitys", {CM_OMI_DATA_DOUBLE, 8, {NULL}}},
};


const cm_omi_map_object_field_t* CmOmiMapPmmPoolCmdParamsGet[]=
{
    &CmOmiMapCommFields[4], /*parent_id*/
    &CmOmiMapCommFields[7]
};



const cm_omi_map_object_cmd_t CmOmiMapPmm_poolCmds[] =
{
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET],
        2,
        CmOmiMapPmmPoolCmdParamsGet,
        0,
        NULL,
    },
};


const cm_omi_map_object_t CmCnmPmm_poolCfg =
{
    {"pmm_pool", CM_OMI_OBJECT_PMM_POOL},
    sizeof(CmOmiMapFieldsPmm_pool) / sizeof(cm_omi_map_object_field_t),
    CmOmiMapFieldsPmm_pool,
    sizeof(CmOmiMapPmm_poolCmds) / sizeof(cm_omi_map_object_cmd_t),
    CmOmiMapPmm_poolCmds
};




