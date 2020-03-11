/******************************************************************************
 *          Copyright (c) 2018 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cfg_cnm_alarm.c
 * author     : wbn
 * create date: 2018Äê1ÔÂ23ÈÕ
 * description: TODO:
 *
 *****************************************************************************/
#include "cm_omi_types.h"
#include "cm_cfg_cnm.h"

const cm_omi_map_cfg_t CmOmiMapAlarmType[] =
{
    {"FAULT",0},
    {"EVENT",1},
    {"LOG",2},
};

const cm_omi_map_enum_t CmOmiMapEnumAlarmType =
{
    sizeof(CmOmiMapAlarmType)/sizeof(cm_omi_map_cfg_t),
    CmOmiMapAlarmType
};    
        
const cm_omi_map_cfg_t CmOmiMapAlarmLvl[] =
{
    {"CRITICAL",0},
    {"MAJOR",1},
    {"MINOR",2},
    {"TRIVIAL",3},
};

const cm_omi_map_enum_t CmOmiMapEnumAlarmLvl =
{
    sizeof(CmOmiMapAlarmLvl)/sizeof(cm_omi_map_cfg_t),
    CmOmiMapAlarmLvl
}; 

const cm_omi_map_cfg_t CmOmiLanguage[] =
{
    {"ch", 0},
    {"en", 1},   
};
const cm_omi_map_enum_t CmOmiMapEnumLanguage =
{
    sizeof(CmOmiLanguage) / sizeof(cm_omi_map_cfg_t),
    CmOmiLanguage
};

const cm_omi_map_object_field_t CmOmiMapAlarmFields[] =
{
    {{"id", CM_OMI_FIELD_ALARM_ID}, "-i",{CM_OMI_DATA_STRING, CM_ID_LEN, {"[0-9]{1,20}"}}},
    {{"almid", CM_OMI_FIELD_ALARM_PARENT_ID}, "-aid",{CM_OMI_DATA_STRING, CM_ID_LEN, {"[0-9]{1,20}"}}},
    {{"type", CM_OMI_FIELD_ALARM_TYPE},"-type", {CM_OMI_DATA_ENUM, 4, {&CmOmiMapEnumAlarmType}}},
    {{"lvl", CM_OMI_FIELD_ALARM_LVL},"-lvl", {CM_OMI_DATA_ENUM, 4, {&CmOmiMapEnumAlarmLvl}}},
    {{"report", CM_OMI_FIELD_ALARM_TIME_REPORT},"-rep", {CM_OMI_DATA_TIME, 4, {NULL}}},
    {{"recovery", CM_OMI_FIELD_ALARM_TIME_RECOVER},"-rec", {CM_OMI_DATA_TIME, 4, {NULL}}},
    {{"param", CM_OMI_FIELD_ALARM_PARAM},"-p", {CM_OMI_DATA_STRING, 256, {NULL}}},
};

const cm_omi_map_object_field_t* CmOmiMapAlarmCmdParamsGet[]=
{
    &CmOmiMapAlarmFields[CM_OMI_FIELD_ALARM_ID], /*id*/
};
const cm_omi_map_object_field_t* CmOmiMapAlarmCmdParamsGetOpt[]=
{
    &CmOmiMapCommFields[0], /*field*/
};
const cm_omi_map_object_field_t* CmOmiMapAlarmCmdParamsGetBatchOpt[]=
{
    &CmOmiMapCommFields[0], /*field*/
    &CmOmiMapCommFields[1], /*offset*/
    &CmOmiMapCommFields[2], /*total*/
    &CmOmiMapCommFields[5], /*time_start*/
    &CmOmiMapCommFields[6], /*time_end*/
    &CmOmiMapAlarmFields[CM_OMI_FIELD_ALARM_LVL],
};

const cm_omi_map_object_field_t* CmOmiMapAlarmCmdParamsCountOpt[]=
{
    &CmOmiMapAlarmFields[CM_OMI_FIELD_ALARM_LVL],
};


const cm_omi_map_object_cmd_t CmOmiMapAlarmCmds[] =
{
    /* getbatch */
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET_BATCH],
        0,
        NULL,
        sizeof(CmOmiMapAlarmCmdParamsGetBatchOpt)/sizeof(cm_omi_map_object_field_t*),
        CmOmiMapAlarmCmdParamsGetBatchOpt
    },
    /* get */
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET],        
        1,
        CmOmiMapAlarmCmdParamsGet,
        1,
        CmOmiMapAlarmCmdParamsGetOpt
    },
    /* count */
    {
        &CmOmiMapCmds[CM_OMI_CMD_COUNT],        
        0,
        NULL,
        sizeof(CmOmiMapAlarmCmdParamsCountOpt)/sizeof(cm_omi_map_object_field_t*),
        CmOmiMapAlarmCmdParamsCountOpt,
    }
};

const cm_omi_map_object_t CmCnmAlarmCfg =
{
    {"alarm", CM_OMI_OBJECT_ALARM_CURRENT},
    sizeof(CmOmiMapAlarmFields)/sizeof(cm_omi_map_object_field_t),
    CmOmiMapAlarmFields,
    sizeof(CmOmiMapAlarmCmds)/sizeof(cm_omi_map_object_cmd_t),
    CmOmiMapAlarmCmds
};

const cm_omi_map_object_field_t* CmOmiMapAlarmHisCmdParamsGetBatchOpt[]=
{
    &CmOmiMapCommFields[0], /*field*/
    &CmOmiMapCommFields[1], /*offset*/
    &CmOmiMapCommFields[2], /*total*/
    &CmOmiMapCommFields[5], /*time_start*/
    &CmOmiMapCommFields[6], /*time_end*/
    &CmOmiMapAlarmFields[CM_OMI_FIELD_ALARM_TYPE],
    &CmOmiMapAlarmFields[CM_OMI_FIELD_ALARM_LVL],
};

const cm_omi_map_object_field_t* CmOmiMapAlarmHisCmdParamsCountOpt[]=
{
    &CmOmiMapAlarmFields[CM_OMI_FIELD_ALARM_TYPE],
    &CmOmiMapAlarmFields[CM_OMI_FIELD_ALARM_LVL],
};

const cm_omi_map_object_cmd_t CmOmiMapAlarmHisCmds[] =
{
    /* getbatch */
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET_BATCH],
        0,
        NULL,
        sizeof(CmOmiMapAlarmHisCmdParamsGetBatchOpt)/sizeof(cm_omi_map_object_field_t*),
        CmOmiMapAlarmHisCmdParamsGetBatchOpt
    },
    /* get */
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET],        
        1,
        CmOmiMapAlarmCmdParamsGet,
        1,
        CmOmiMapAlarmCmdParamsGetOpt
    },
    /* count */
    {
        &CmOmiMapCmds[CM_OMI_CMD_COUNT],        
        0,
        NULL,
        sizeof(CmOmiMapAlarmHisCmdParamsCountOpt)/sizeof(cm_omi_map_object_field_t*),
        CmOmiMapAlarmHisCmdParamsCountOpt,
    },
    /* delete */
    {
        &CmOmiMapCmds[CM_OMI_CMD_DELETE],        
        1,
        CmOmiMapAlarmCmdParamsGet,
        0,
        NULL
    },
};

const cm_omi_map_object_t CmCnmAlarmHisCfg =
{
    {"alarm_history", CM_OMI_OBJECT_ALARM_HISTORY},
    sizeof(CmOmiMapAlarmFields)/sizeof(cm_omi_map_object_field_t),
    CmOmiMapAlarmFields,
    sizeof(CmOmiMapAlarmHisCmds)/sizeof(cm_omi_map_object_cmd_t),
    CmOmiMapAlarmHisCmds
};


const cm_omi_map_object_field_t CmOmiMapAlarmConfigFields[] =
{
    {{"almid", CM_OMI_FIELD_ALARM_CFG_ID}, "-i",{CM_OMI_DATA_STRING, CM_ID_LEN, {"[0-9]{1,20}"}}},
    {{"match_bits", CM_OMI_FIELD_ALARM_CFG_MATCH}, "-m",{CM_OMI_DATA_BITS, 4, {NULL}}},
    {{"param_num", CM_OMI_FIELD_ALARM_CFG_PARAM_NUM}, "-pn",{CM_OMI_DATA_INT, 4, {NULL}}},
    {{"type", CM_OMI_FIELD_ALARM_CFG_TYPE},"-type", {CM_OMI_DATA_ENUM, 4, {&CmOmiMapEnumAlarmType}}},
    {{"lvl", CM_OMI_FIELD_ALARM_CFG_LVL},"-lvl", {CM_OMI_DATA_ENUM, 4, {&CmOmiMapEnumAlarmLvl}}},
    {{"is_disable", CM_OMI_FIELD_ALARM_CFG_IS_DISABLE},"-disable", {CM_OMI_DATA_ENUM, 4, {&CmOmiMapEnumBoolType}}},
};

const cm_omi_map_object_field_t* CmOmiMapAlarmConfigCmdParamsGetBatchOpt[]=
{
    &CmOmiMapCommFields[0], /*field*/
    &CmOmiMapCommFields[1], /*offset*/
    &CmOmiMapCommFields[2], /*total*/
    &CmOmiMapAlarmConfigFields[CM_OMI_FIELD_ALARM_CFG_TYPE],
    &CmOmiMapAlarmConfigFields[CM_OMI_FIELD_ALARM_CFG_LVL],
    &CmOmiMapAlarmConfigFields[CM_OMI_FIELD_ALARM_CFG_IS_DISABLE],
};

const cm_omi_map_object_field_t* CmOmiMapAlarmConfigCmdParamsGet[]=
{
    &CmOmiMapAlarmConfigFields[CM_OMI_FIELD_ALARM_CFG_ID],
};

const cm_omi_map_object_cmd_t CmOmiMapAlarmConfigCmds[] =
{
    /* getbatch */
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET_BATCH],
        0,
        NULL,
        sizeof(CmOmiMapAlarmConfigCmdParamsGetBatchOpt)/sizeof(cm_omi_map_object_field_t*),
        CmOmiMapAlarmConfigCmdParamsGetBatchOpt
    },
    /* get */
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET],        
        1,
        CmOmiMapAlarmConfigCmdParamsGet,
        1,
        CmOmiMapAlarmConfigCmdParamsGetBatchOpt
    },
    /* count */
    {
        &CmOmiMapCmds[CM_OMI_CMD_COUNT],        
        0,
        NULL,
        3,
        CmOmiMapAlarmConfigCmdParamsGetBatchOpt+3,
    },
    /* modify */
    {
        &CmOmiMapCmds[CM_OMI_CMD_MODIFY],        
        1,
        CmOmiMapAlarmConfigCmdParamsGet,
        2,
        CmOmiMapAlarmConfigCmdParamsGetBatchOpt+4,
    }
};

const cm_omi_map_object_t CmCnmAlarmConfigCfg =
{
    {"alarm_config", CM_OMI_OBJECT_ALARM_CONFIG},
    sizeof(CmOmiMapAlarmConfigFields)/sizeof(cm_omi_map_object_field_t),
    CmOmiMapAlarmConfigFields,
    sizeof(CmOmiMapAlarmConfigCmds)/sizeof(cm_omi_map_object_cmd_t),
    CmOmiMapAlarmConfigCmds
};

/****************************mail***************************************/
extern const cm_omi_map_enum_t CmOmiMapEnumSwitchType;

const cm_omi_map_object_field_t CmOmiMapFieldsMailsend[] =
{
    {{"switch", CM_OMI_FIELD_MAILSEND_STATE}, "-sw", {CM_OMI_DATA_ENUM, 4, {&CmOmiMapEnumSwitchType}}},
    {{"sender", CM_OMI_FIELD_MAILSEND_SENDER}, "-send", {CM_OMI_DATA_STRING, CM_STRING_64, {NULL}}},
    {{"server", CM_OMI_FIELD_MAILSEND_SERVER}, "-serv", {CM_OMI_DATA_STRING, CM_STRING_64, {IP_FORMAT}}},
    {{"port", CM_OMI_FIELD_MAILSEND_PORT}, "-port", {CM_OMI_DATA_INT, 4, {NULL}}},
    {{"language", CM_OMI_FIELD_MAILSEND_LANGUAGE}, "-lan", {CM_OMI_DATA_ENUM, 4, {&CmOmiMapEnumLanguage}}},
    {{"switch_user", CM_OMI_FIELD_MAILSEND_USERON}, "-swu", {CM_OMI_DATA_ENUM, 4, {&CmOmiMapEnumSwitchType}}},
    {{"user", CM_OMI_FIELD_MAILSEND_USER}, "-user", {CM_OMI_DATA_STRING, CM_STRING_64, {NULL}}},
    {{"passwd", CM_OMI_FIELD_MAILSEND_PASSWD}, "-pass", {CM_OMI_DATA_STRING, CM_STRING_64, {NULL}}},   
};

const cm_omi_map_object_field_t* CmOmiMapMailsendCmdParamsUpdate[]=
{
    &CmOmiMapFieldsMailsend[CM_OMI_FIELD_MAILSEND_STATE],
    &CmOmiMapFieldsMailsend[CM_OMI_FIELD_MAILSEND_SENDER],
    &CmOmiMapFieldsMailsend[CM_OMI_FIELD_MAILSEND_SERVER],
    &CmOmiMapFieldsMailsend[CM_OMI_FIELD_MAILSEND_PORT],
    &CmOmiMapFieldsMailsend[CM_OMI_FIELD_MAILSEND_LANGUAGE],
    &CmOmiMapFieldsMailsend[CM_OMI_FIELD_MAILSEND_USERON],
    &CmOmiMapFieldsMailsend[CM_OMI_FIELD_MAILSEND_USER],
    &CmOmiMapFieldsMailsend[CM_OMI_FIELD_MAILSEND_PASSWD],
};

const cm_omi_map_object_field_t* CmOmiMapMailsendCmdParamsTest[]=
{
    &CmOmiMapFieldsMailsend[CM_OMI_FIELD_MAILSEND_SENDER],
    &CmOmiMapFieldsMailsend[CM_OMI_FIELD_MAILSEND_SERVER],
    &CmOmiMapFieldsMailsend[CM_OMI_FIELD_MAILSEND_PORT],
};

const cm_omi_map_object_field_t* CmOmiMapMailsendCmdParamsTestOpt[]=
{
    &CmOmiMapFieldsMailsend[CM_OMI_FIELD_MAILSEND_USER],
    &CmOmiMapFieldsMailsend[CM_OMI_FIELD_MAILSEND_PASSWD],
};


const cm_omi_map_object_cmd_t CmOmiMapMailsendCmds[] =
{
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET],
        0,
        NULL,
        0,
        NULL,
    }, 
    {
        &CmOmiMapCmds[CM_OMI_CMD_MODIFY],
        0,
        NULL,
        8,
        CmOmiMapMailsendCmdParamsUpdate,
    },
    {
        &CmOmiMapCmds[CM_OMI_CMD_TEST],
        3,
        CmOmiMapMailsendCmdParamsTest,
        2,
        CmOmiMapMailsendCmdParamsTestOpt,
    },
};
const cm_omi_map_object_t CmCnmMailsendCfg =
{
    {"mailserver", CM_OMI_OBJECT_MAILSEND},
    sizeof(CmOmiMapFieldsMailsend) / sizeof(cm_omi_map_object_field_t),
    CmOmiMapFieldsMailsend,
    sizeof(CmOmiMapMailsendCmds) / sizeof(cm_omi_map_object_cmd_t),
    CmOmiMapMailsendCmds
};
const cm_omi_map_object_field_t CmOmiMapFieldsMailrecv[] =
{
    {{"id", CM_OMI_FIELD_MAILRECV_ID}, "-id", {CM_OMI_DATA_INT, 4, {NULL}}},
    {{"receiver", CM_OMI_FIELD_MAILRECV_RECEIVER}, "-recv", {CM_OMI_DATA_STRING, CM_STRING_64, {NULL}}},
    {{"level", CM_OMI_FIELD_MAILRECV_LEVEL}, "-lvl", {CM_OMI_DATA_ENUM, 4, {&CmOmiMapEnumAlarmLvl}}},
};
const cm_omi_map_object_field_t* CmOmiMapMailrecvCmdParamsGetBatch[]=
{
    &CmOmiMapCommFields[1], /*offset*/
    &CmOmiMapCommFields[2], /*total*/
};

const cm_omi_map_object_field_t* CmOmiMapMailrecvCmdParamsUpdate[]=
{
    &CmOmiMapFieldsMailrecv[CM_OMI_FIELD_MAILRECV_ID],
    &CmOmiMapFieldsMailrecv[CM_OMI_FIELD_MAILRECV_LEVEL],
};

const cm_omi_map_object_field_t* CmOmiMapMailrecvCmdParamsAdd[]=
{
    &CmOmiMapFieldsMailrecv[CM_OMI_FIELD_MAILRECV_RECEIVER],
    &CmOmiMapFieldsMailrecv[CM_OMI_FIELD_MAILRECV_LEVEL],
};


const cm_omi_map_object_cmd_t CmOmiMapMailrecvCmds[] =
{
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET_BATCH],
        0,
        NULL,
        2,
        CmOmiMapMailrecvCmdParamsGetBatch,
    },
    {
        &CmOmiMapCmds[CM_OMI_CMD_COUNT],
        0,
        NULL,
        0,
        NULL,
    },  
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET],
        1,
        CmOmiMapMailrecvCmdParamsUpdate,
        0,
        NULL,
    },
    {
        &CmOmiMapCmds[CM_OMI_CMD_DELETE],
        1,
        CmOmiMapMailrecvCmdParamsUpdate,
        0,
        NULL,
    },  
    {
        &CmOmiMapCmds[CM_OMI_CMD_MODIFY],
        2,
        CmOmiMapMailrecvCmdParamsUpdate,
        0,
        NULL,
    },  
    {
        &CmOmiMapCmds[CM_OMI_CMD_ADD],
        2,
        CmOmiMapMailrecvCmdParamsAdd,
        0,
        NULL,
    },
    {
        &CmOmiMapCmds[CM_OMI_CMD_TEST],
        1,
        CmOmiMapMailrecvCmdParamsAdd,
        0,
        NULL,
    },
};
const cm_omi_map_object_t CmCnmMailrecvCfg =
{
    {"mailrecv", CM_OMI_OBJECT_MAILRECV},
    sizeof(CmOmiMapFieldsMailrecv) / sizeof(cm_omi_map_object_field_t),
    CmOmiMapFieldsMailrecv,
    sizeof(CmOmiMapMailrecvCmds) / sizeof(cm_omi_map_object_cmd_t),
    CmOmiMapMailrecvCmds
};
