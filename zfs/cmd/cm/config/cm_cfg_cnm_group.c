/******************************************************************************
 *          Copyright (c) 2018 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cfg_cnm_gruo.c
 * author     : xar
 * create date: 2018.08.13
 * description: TODO:
 *
 *****************************************************************************/

#include "cm_omi_types.h"
#include "cm_cfg_cnm.h"

const cm_omi_map_object_field_t CmOmiMapFieldsGroup[] = 
{
    {{"id",CM_OMI_FIELD_GROUP_ID},"-id",{CM_OMI_DATA_INT, 4, {NULL}}},
    {{"name", CM_OMI_FIELD_GROUP_NAME}, "-name",{CM_OMI_DATA_STRING, CM_NAME_LEN, {"[0-9a-zA-Z_]{3,16}"}}}
};


const cm_omi_map_object_field_t* CmOmiMapGroupCmdParamsGetBatch[]=
{
    &CmOmiMapCommFields[1], /*offset*/
    &CmOmiMapCommFields[2], /*total*/
};

const cm_omi_map_object_field_t* CmOmiMapGroupCmdParamsGet[]=
{
    &CmOmiMapFieldsGroup[CM_OMI_FIELD_GROUP_NAME],/*name*/
};

const cm_omi_map_object_field_t* CmOmiMapGroupCmdParamsAdd[]=
{
    &CmOmiMapFieldsGroup[CM_OMI_FIELD_GROUP_ID],/*id*/
};



const cm_omi_map_object_cmd_t CmOmiMapGroupCmds[] =
{
    /* getbatch */
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET_BATCH],
        0,
        NULL,
        2,
        CmOmiMapGroupCmdParamsGetBatch,
    },
    /* get */
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET],
        1,
        CmOmiMapGroupCmdParamsGet,
        0,
        NULL,
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
        1,
        CmOmiMapGroupCmdParamsGet,
        1,
        CmOmiMapGroupCmdParamsAdd,
    },
    /* delete */
    {
        &CmOmiMapCmds[CM_OMI_CMD_DELETE],
        1,
        CmOmiMapGroupCmdParamsGet,
        0,
        NULL,
    }
};

 
const cm_omi_map_object_t CmGroupCfg =
{
    {"usergp",CM_OMI_OBJECT_GROUP},
    sizeof(CmOmiMapFieldsGroup)/sizeof(cm_omi_map_object_field_t),
    CmOmiMapFieldsGroup,
    sizeof(CmOmiMapGroupCmds)/sizeof(cm_omi_map_object_cmd_t),
    CmOmiMapGroupCmds
};



