/******************************************************************************
 *          Copyright (c) 2018 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cfg_cnm_aggr.c
 * author     : xar
 * create date: 2018.09.18
 * description: TODO:
 *
 *****************************************************************************/


#include "cm_omi_types.h"
#include "cm_cfg_cnm.h"

typedef enum
{
    LACPACTIVITY_ACTIVE=0,
    LACPACTIVITY_OFF,
    LACPACTIVITY_PASSIVE,
}cm_cnm_aggr_lacpactivity_e;

const cm_omi_map_cfg_t CmOmiMapPortLacpactivityType[] =
{
    {"active",LACPACTIVITY_ACTIVE},
    {"off",LACPACTIVITY_OFF},
    {"passive",LACPACTIVITY_PASSIVE},
};

const cm_omi_map_enum_t CmOmiMapEnumLacpactivityType =
{
    sizeof(CmOmiMapPortLacpactivityType)/sizeof(cm_omi_map_cfg_t),
    CmOmiMapPortLacpactivityType
};

typedef enum
{
    LACPTIMER_SHORT=0,
    LACPTIMER_LONG,
}cm_cnm_aggr_lacptimer_e;

const cm_omi_map_cfg_t CmOmiMapPortLacptimerType[] =
{
    {"short",LACPTIMER_SHORT},
    {"long",LACPTIMER_LONG},
};

const cm_omi_map_enum_t CmOmiMapEnumLacptimerType[] =
{
    sizeof(CmOmiMapPortLacptimerType)/sizeof(cm_omi_map_cfg_t),
    CmOmiMapPortLacptimerType
};

typedef enum
{
    POLICY_L2=0,
    POLICY_L3,
    POLICY_L4,
}cm_cnm_aggr_policy_e;

const cm_omi_map_cfg_t CmOmiMapPortPolicyType[] =
{
    {"L2",POLICY_L2},
    {"L3",POLICY_L3},
    {"L4",POLICY_L4},
};

const cm_omi_map_enum_t CmOmiMapEnumPolicyType =
{
    sizeof(CmOmiMapPortPolicyType)/sizeof(cm_omi_map_cfg_t),
    CmOmiMapPortPolicyType
};



const cm_omi_map_object_field_t CmOmiMapFieldsAggr[] =
{
    {{"name", CM_OMI_FIELD_AGGR_NAME}, "-name",{CM_OMI_DATA_STRING, CM_STRING_64, {"[0-9a-zA-Z_]{1,64}"}}},
    {{"nid", CM_OMI_FIELD_AGGR_NID}, "-nid", {CM_OMI_DATA_INT, 4, {NULL}}},
    {{"state", CM_OMI_FIELD_AGGR_STATE}, "-state",{CM_OMI_DATA_STRING, CM_STRING_64, {"[0-9a-zA-Z_]{1,64}"}}},
    {{"ip", CM_OMI_FIELD_AGGR_IP}, "-ip", {CM_OMI_DATA_STRING, CM_STRING_64, {"([0-9]{1,3}\.){3}[0-9]{1,3}"}}},
    {{"netmask", CM_OMI_FIELD_AGGR_NETMASK}, "-netmask", {CM_OMI_DATA_STRING, CM_STRING_64, {"([0-9]{1,3}\.){3}[0-9]{1,3}"}}},
    {{"mtu", CM_OMI_FIELD_AGGR_MTU}, "-mtu",{CM_OMI_DATA_STRING, CM_STRING_64, {NULL}}},
    {{"mac", CM_OMI_FIELD_AGGR_MAC},"-mac", {CM_OMI_DATA_STRING, CM_STRING_64, {NULL}}},
    {{"adapter", CM_OMI_FIELD_AGGR_ADAPTER},"-adapter", {CM_OMI_DATA_STRING, CM_STRING_64, {NULL}}},
    {{"policy", CM_OMI_FIELD_AGGR_POLICY}, "-policy",{CM_OMI_DATA_ENUM, 4, {&CmOmiMapEnumPolicyType}}},
    {{"lacpactivity", CM_OMI_FIELD_AGGR_LACPACTIVITY}, "-lacpactivity",{CM_OMI_DATA_ENUM, 4, {&CmOmiMapEnumLacpactivityType}}},
    {{"lacptimer", CM_OMI_FIELD_AGGR_LACPTIMER}, "-lacptimer",{CM_OMI_DATA_ENUM, 4, {&CmOmiMapEnumLacptimerType}}},
};

const cm_omi_map_object_field_t* CmOmiMapAggrCmdParamsGetBatch[] =
{
    &CmOmiMapCommFields[1], /*offset*/
    &CmOmiMapCommFields[2], /*total*/
    &CmOmiMapFieldsAggr[CM_OMI_FIELD_AGGR_NID],
};

const cm_omi_map_object_field_t* CmOmiMapAggrCmdParamsAdd[] =
{
    &CmOmiMapFieldsAggr[CM_OMI_FIELD_AGGR_NID],
    &CmOmiMapFieldsAggr[CM_OMI_FIELD_AGGR_ADAPTER],
    &CmOmiMapFieldsAggr[CM_OMI_FIELD_AGGR_IP],
};

const cm_omi_map_object_field_t* CmOmiMapAggrCmdParamsUpdate[] =
{
    &CmOmiMapFieldsAggr[CM_OMI_FIELD_AGGR_POLICY],   
    &CmOmiMapFieldsAggr[CM_OMI_FIELD_AGGR_LACPACTIVITY],
    &CmOmiMapFieldsAggr[CM_OMI_FIELD_AGGR_LACPTIMER],
    &CmOmiMapFieldsAggr[CM_OMI_FIELD_AGGR_NETMASK],
    &CmOmiMapFieldsAggr[CM_OMI_FIELD_AGGR_IP],
};

const cm_omi_map_object_field_t* CmOmiMapAggrCmdParamsdelete[] =
{
    &CmOmiMapFieldsAggr[CM_OMI_FIELD_AGGR_NID],
    &CmOmiMapFieldsAggr[CM_OMI_FIELD_AGGR_NAME],
};

const cm_omi_map_object_cmd_t CmOmiMapAggrCmds[] =
{
    /*getbath*/
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET_BATCH],
        0,
        NULL,
        3,
        CmOmiMapAggrCmdParamsGetBatch
    },
    /* count */
    {
        &CmOmiMapCmds[CM_OMI_CMD_COUNT],
        1,
        CmOmiMapAggrCmdParamsAdd,
        0,
        NULL
    },
    /*insert*/
    {
        &CmOmiMapCmds[CM_OMI_CMD_ADD],
        2,
        CmOmiMapAggrCmdParamsAdd,
        5,
        CmOmiMapAggrCmdParamsUpdate
    },
    /*update*/
    {
        &CmOmiMapCmds[CM_OMI_CMD_MODIFY],
        2,
        CmOmiMapAggrCmdParamsdelete,
        5,
        CmOmiMapAggrCmdParamsUpdate
    },
     /*delete*/
    {
        &CmOmiMapCmds[CM_OMI_CMD_DELETE],
        2,
        CmOmiMapAggrCmdParamsdelete,
        0,
        NULL
    },
};




const cm_omi_map_object_t CmCnmAggrCfg =
{
    {"aggr", CM_OMI_OBJECT_AGGR},
    sizeof(CmOmiMapFieldsAggr) / sizeof(cm_omi_map_object_field_t),
    CmOmiMapFieldsAggr,
    sizeof(CmOmiMapAggrCmds) / sizeof(cm_omi_map_object_cmd_t),
    CmOmiMapAggrCmds
};


