/******************************************************************************
 *          Copyright (c) 2018 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cfg_cnm_systime.c
 * author     : wbn
 * create date: 2018Äê3ÔÂ5ÈÕ
 * description: TODO:
 *
 *****************************************************************************/
#include "cm_omi_types.h"
#include "cm_cfg_cnm.h"
     
const cm_omi_map_object_field_t CmOmiMapSysTimeFields[] =
{
    {{"timezone", CM_OMI_FIELD_SYSTIME_TZ},"-tz", {CM_OMI_DATA_INT, 4, {NULL}}},
    {{"datetime", CM_OMI_FIELD_SYSTIME_DT},"-dt", {CM_OMI_DATA_TIME, 4, {NULL}}},
};

const cm_omi_map_object_field_t* CmOmiMapSysTimeCmdParamsGet[]=
{
    &CmOmiMapCommFields[0], /*fields*/
};

const cm_omi_map_object_field_t* CmOmiMapSysTimeCmdParamsSet[]=
{
    /*&CmOmiMapSysTimeFields[CM_OMI_FIELD_SYSTIME_TZ],*/
    &CmOmiMapSysTimeFields[CM_OMI_FIELD_SYSTIME_DT],
};


const cm_omi_map_object_cmd_t CmOmiMapSysTimeCmds[] =
{
    /* get */
    {
      &CmOmiMapCmds[CM_OMI_CMD_GET],      
      0,
      NULL,
      sizeof(CmOmiMapSysTimeCmdParamsGet)/sizeof(cm_omi_map_object_field_t*),
      CmOmiMapSysTimeCmdParamsGet,
    },
    /* MODIFY */
    {
      &CmOmiMapCmds[CM_OMI_CMD_MODIFY],      
      sizeof(CmOmiMapSysTimeCmdParamsSet)/sizeof(cm_omi_map_object_field_t*),
      CmOmiMapSysTimeCmdParamsSet,
      0,
      NULL,
    },
};

const cm_omi_map_object_t CmCnmSysTimeCfg =
{
    {"systime", CM_OMI_OBJECT_SYS_TIME},
    sizeof(CmOmiMapSysTimeFields)/sizeof(cm_omi_map_object_field_t),
    CmOmiMapSysTimeFields,
    sizeof(CmOmiMapSysTimeCmds)/sizeof(cm_omi_map_object_cmd_t),
    CmOmiMapSysTimeCmds
};

/************************   ntp   ***************************/

const cm_omi_map_object_field_t CmOmiMapNtpFields[] =
{
    {{"ntp server", CM_OMI_FIELD_NTP_IP},"-i", {CM_OMI_DATA_STRING, CM_STRING_128, {IP_FORMAT}}},
};

const cm_omi_map_object_field_t* CmOmiMapNtpCmdParamsInsert[]=
{
    &CmOmiMapNtpFields[0], /*ip*/
};



const cm_omi_map_object_cmd_t CmOmiMapNtpCmds[] =
{
    /* insert */
    {
        &CmOmiMapCmds[CM_OMI_CMD_ADD],      
        sizeof(CmOmiMapNtpCmdParamsInsert)/sizeof(cm_omi_map_object_field_t*),
        CmOmiMapNtpCmdParamsInsert,
        0,
        NULL,
    },
    /* delete */
    {
        &CmOmiMapCmds[CM_OMI_CMD_DELETE],      
        0,
        NULL,
        0,
        NULL,
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


const cm_omi_map_object_t CmCnmNtpCfg =
{
    {"ntp", CM_OMI_OBJECT_NTP},
    sizeof(CmOmiMapNtpFields)/sizeof(cm_omi_map_object_field_t),
    CmOmiMapNtpFields,
    sizeof(CmOmiMapNtpCmds)/sizeof(cm_omi_map_object_cmd_t),
    CmOmiMapNtpCmds
};


