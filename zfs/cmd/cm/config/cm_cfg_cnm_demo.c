/******************************************************************************
 *          Copyright (c) 2017 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cfg_cnm_demo.c
 * author     : wbn
 * create date: 2017Äê10ÔÂ26ÈÕ
 * description: TODO:
 *
 *****************************************************************************/
#include "cm_omi_types.h"
#include "cm_cfg_cnm.h"

const cm_omi_map_object_t CmCnmDemoCfg =
{
    {"demo", CM_OMI_OBJECT_DEMO},
    0,
    NULL,
    0,
    NULL
};


const cm_omi_map_object_field_t CmOmiMapSessionFields[] =
{
    {{"id", CM_OMI_FIELD_SESSION_ID}, "-i",{CM_OMI_DATA_STRING, CM_ID_LEN, {"[0-9a-zA-Z]{1,63}"}}},
    {{"user", CM_OMI_FIELD_SESSION_USER}, "-n",{CM_OMI_DATA_STRING, CM_NAME_LEN, {"[0-9a-zA-Z_]{1,63}"}}},
    {{"pwd", CM_OMI_FIELD_SESSION_PWD}, "-p",{CM_OMI_DATA_STRING, CM_PASSWORD_LEN, {"[0-9a-zA-Z_]{1,63}"}}},
    {{"ip", CM_OMI_FIELD_SESSION_IP},"-ip", {CM_OMI_DATA_STRING, CM_IP_LEN, {"[0-9.]{1,32}"}}},
    {{"level", CM_OMI_FIELD_SESSION_LEVEL},"-l", {CM_OMI_DATA_INT, 4, {NULL}}},
};

const cm_omi_map_object_field_t* CmOmiMapSessionCmdParamsGet[]=
{
    &CmOmiMapSessionFields[CM_OMI_FIELD_SESSION_USER],
    &CmOmiMapSessionFields[CM_OMI_FIELD_SESSION_PWD], 
    &CmOmiMapSessionFields[CM_OMI_FIELD_SESSION_IP],
};

const cm_omi_map_object_field_t* CmOmiMapSessionCmdParamsDelete[]=
{
    &CmOmiMapSessionFields[CM_OMI_FIELD_SESSION_ID],
};


const cm_omi_map_object_cmd_t CmOmiMapSessionCmds[] =
{
    /* get */
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET],
        sizeof(CmOmiMapSessionCmdParamsGet)/sizeof(cm_omi_map_object_field_t*),
        CmOmiMapSessionCmdParamsGet,
        0,
        NULL
    },
    /* delete */
    {
        &CmOmiMapCmds[CM_OMI_CMD_DELETE],
        sizeof(CmOmiMapSessionCmdParamsDelete)/sizeof(cm_omi_map_object_field_t*),
        CmOmiMapSessionCmdParamsDelete,
        0,
        NULL
    }
};

const cm_omi_map_object_t CmCnmSessionCfg =
{
    {"session", CM_OMI_OBJECT_SESSION},
    sizeof(CmOmiMapSessionFields)/sizeof(cm_omi_map_object_field_t),
    CmOmiMapSessionFields,
    sizeof(CmOmiMapSessionCmds)/sizeof(cm_omi_map_object_cmd_t),
    CmOmiMapSessionCmds
};

const cm_omi_map_cfg_t CmOmiMapRemoteType[] =
{
    {"pool",0},
    {"lun",1},
    {"nas",2},
    {"snapshot",3},    
};

const cm_omi_map_enum_t CmOmiMapRemoteTypeCfg =
{
    sizeof(CmOmiMapRemoteType)/sizeof(cm_omi_map_cfg_t),
    CmOmiMapRemoteType
};

const cm_omi_map_object_field_t CmOmiMapRemoteFields[] =
{
    {{"ip", CM_OMI_FIELD_REMOTE_IPADDR}, "-ip",{CM_OMI_DATA_STRING, CM_IP_LEN,{"[0-9.]{1,31}"}}},
    {{"type", CM_OMI_FIELD_REMOTE_TYPE}, "-type",{CM_OMI_DATA_ENUM, 1, {&CmOmiMapRemoteTypeCfg}}},
    {{"name", CM_OMI_FIELD_REMOTE_NAME}, "-name",{CM_OMI_DATA_STRING, 64, {NULL}}},
    {{"avail", CM_OMI_FIELD_REMOTE_AVAIL},"-avail", {CM_OMI_DATA_STRING, 64, {NULL}}},
    {{"rdcip", CM_OMI_FIELD_REMOTE_RDCIP},"-rdcip", {CM_OMI_DATA_STRING, 64, {NULL}}},
    {{"quota", CM_OMI_FIELD_REMOTE_QUOTA},"-quota", {CM_OMI_DATA_STRING, 32, {NULL}}},
};

const cm_omi_map_object_field_t* CmOmiMapRemoteCmdParams[]=
{
    &CmOmiMapRemoteFields[CM_OMI_FIELD_REMOTE_IPADDR],
    &CmOmiMapRemoteFields[CM_OMI_FIELD_REMOTE_TYPE], 
};

const cm_omi_map_object_field_t* CmOmiMapRemoteCmdParamsOpt[]=
{
    &CmOmiMapCommFields[1], /*offset*/
    &CmOmiMapCommFields[2], /*total*/
    &CmOmiMapCommFields[0], /*field*/
};


const cm_omi_map_object_cmd_t CmOmiMapRemoteCmds[] =
{
    /* getbatch */
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET_BATCH],
        sizeof(CmOmiMapRemoteCmdParams)/sizeof(cm_omi_map_object_field_t*),
        CmOmiMapRemoteCmdParams,
        sizeof(CmOmiMapRemoteCmdParamsOpt)/sizeof(cm_omi_map_object_field_t*),
        CmOmiMapRemoteCmdParamsOpt,
    },
    /* count */
    {
        &CmOmiMapCmds[CM_OMI_CMD_COUNT],
        sizeof(CmOmiMapRemoteCmdParams)/sizeof(cm_omi_map_object_field_t*),
        CmOmiMapRemoteCmdParams,
        0,
        NULL
    }
};


const cm_omi_map_object_t CmCnmRemoteCfg =
{
    {"remote", CM_OMI_OBJECT_REMOTE},
    sizeof(CmOmiMapRemoteFields)/sizeof(cm_omi_map_object_field_t),
    CmOmiMapRemoteFields,
    sizeof(CmOmiMapRemoteCmds)/sizeof(cm_omi_map_object_cmd_t),
    CmOmiMapRemoteCmds
};



