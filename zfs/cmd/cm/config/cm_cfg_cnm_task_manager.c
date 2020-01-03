#include "cm_omi_types.h"
#include "cm_cfg_cnm.h"

#define CM_TASK_CMD_LEN CM_STRING_256
#define CM_TASK_DESC_LEN CM_STRING_256

const cm_omi_map_cfg_t CmTaskManagerMapTypeName[] =
{
    {"SNAPSHOT_BACKUP", 0},
    {"LUN_BACKUP", 1},
    {"ZPOOL_EXPORT",2},
    {"ZPOOL_IMPORT",3},
    {"LUN_MIGRATE",4},
};

const cm_omi_map_enum_t CmTaskManagerMapEnumTypeName =
{
    sizeof(CmTaskManagerMapTypeName) / sizeof(cm_omi_map_cfg_t),
    CmTaskManagerMapTypeName
};


const cm_omi_map_cfg_t CmTaskManagerMapStateName[] =
{
    {"READY", 0},
    {"RUNNING", 1},
    {"FINISHED", 2},
    {"ERROR", 3},
};

const cm_omi_map_enum_t CmTaskManagerMapEnumStateName =
{
    sizeof(CmTaskManagerMapStateName) / sizeof(cm_omi_map_cfg_t),
    CmTaskManagerMapStateName
};


const cm_omi_map_object_field_t CmOmiMapFieldsTaskManager[] =
{
    {{"tid", CM_OMI_FIELD_TASK_MANAGER_TID}, "-ti", {CM_OMI_DATA_INT, 4, {NULL}}},
    {{"pid", CM_OMI_FIELD_TASK_MANAGER_PID}, "-pi", {CM_OMI_DATA_INT, 4, {NULL}}},
    {{"nid", CM_OMI_FIELD_TASK_MANAGER_NID}, "-ni", {CM_OMI_DATA_INT, 4, {NULL}}},
    {{"type", CM_OMI_FIELD_TASK_MANAGER_TYPE}, "-tp", {CM_OMI_DATA_ENUM, 4, {&CmTaskManagerMapEnumTypeName}}},
    {{"state", CM_OMI_FIELD_TASK_MANAGER_STATE}, "-s", {CM_OMI_DATA_ENUM, 4, {&CmTaskManagerMapEnumStateName}}},
    {{"prog", CM_OMI_FIELD_TASK_MANAGER_PROG}, "-p", {CM_OMI_DATA_INT, 4, {NULL}}},
    {{"start", CM_OMI_FIELD_TASK_MANAGER_STTM}, "-st", {CM_OMI_DATA_TIME, 8, {NULL}}},
    {{"finished", CM_OMI_FIELD_TASK_MANAGER_ENDTM}, "-ft", {CM_OMI_DATA_TIME, 8, {NULL}}},
    {{"description", CM_OMI_FIELD_TASK_MANAGER_DESC}, "-d", {CM_OMI_DATA_STRING, CM_TASK_DESC_LEN, {NULL}}},
};


const cm_omi_map_object_field_t *CmOmiMapTaskManagerCmdNeedParamsGet[] =
{
    &CmOmiMapFieldsTaskManager[CM_OMI_FIELD_TASK_MANAGER_TID],
};

const cm_omi_map_object_field_t *CmOmiMapTaskManagerCmdOptParamsGetBatch[] =
{
//  &CmOmiMapFieldsTaskManager[CM_OMI_FIELD_TASK_MANAGER_TID],
    &CmOmiMapFieldsTaskManager[CM_OMI_FIELD_TASK_MANAGER_NID],
    &CmOmiMapFieldsTaskManager[CM_OMI_FIELD_TASK_MANAGER_TYPE],
    &CmOmiMapFieldsTaskManager[CM_OMI_FIELD_TASK_MANAGER_STATE],
//  &CmOmiMapFieldsTaskManager[CM_OMI_FIELD_TASK_MANAGER_PROG],
//  &CmOmiMapFieldsTaskManager[CM_OMI_FIELD_TASK_MANAGER_STTM],
//  &CmOmiMapFieldsTaskManager[CM_OMI_FIELD_TASK_MANAGER_ENDTM],
    &CmOmiMapCommFields[1],
    &CmOmiMapCommFields[2],
//  &CmOmiMapCommFields[0],
};

const cm_omi_map_object_field_t *CmOmiMapTaskManagerCmdNeedParamsDelete[] =
{
    &CmOmiMapFieldsTaskManager[CM_OMI_FIELD_TASK_MANAGER_TID],
};

const cm_omi_map_object_field_t *CmOmiMapTaskManagerCmdOptParamsCount[] =
{
//  &CmOmiMapFieldsTaskManager[CM_OMI_FIELD_TASK_MANAGER_TID],
    &CmOmiMapFieldsTaskManager[CM_OMI_FIELD_TASK_MANAGER_NID],
    &CmOmiMapFieldsTaskManager[CM_OMI_FIELD_TASK_MANAGER_TYPE],
    &CmOmiMapFieldsTaskManager[CM_OMI_FIELD_TASK_MANAGER_STATE],
//  &CmOmiMapFieldsTaskManager[CM_OMI_FIELD_TASK_MANAGER_PROG],
//  &CmOmiMapFieldsTaskManager[CM_OMI_FIELD_TASK_MANAGER_STTM],
//  &CmOmiMapFieldsTaskManager[CM_OMI_FIELD_TASK_MANAGER_ENDTM],
};

const cm_omi_map_object_cmd_t CmOmiMapTaskManagerCmds[] =
{
    /********************get cmd**************************************/
    {
        &CmOmiMapCmds[1],
        sizeof(CmOmiMapTaskManagerCmdNeedParamsGet) / sizeof(cm_omi_map_object_field_t *), CmOmiMapTaskManagerCmdNeedParamsGet,
        0, NULL
    },

    /********************getbatch cmd*********************************/
    {
        &CmOmiMapCmds[0],
        0, NULL,
        sizeof(CmOmiMapTaskManagerCmdOptParamsGetBatch) / sizeof(cm_omi_map_object_field_t *), CmOmiMapTaskManagerCmdOptParamsGetBatch
    },

    /********************delete cmd************************************/
    {
        &CmOmiMapCmds[5],
        sizeof(CmOmiMapTaskManagerCmdNeedParamsDelete) / sizeof(cm_omi_map_object_field_t *), CmOmiMapTaskManagerCmdNeedParamsDelete,
        0, NULL
    },

    /********************count cmd*************************************/
    {
        &CmOmiMapCmds[2],
        0, NULL,
        sizeof(CmOmiMapTaskManagerCmdOptParamsCount) / sizeof(cm_omi_map_object_field_t *), CmOmiMapTaskManagerCmdOptParamsCount
    },
};

const cm_omi_map_object_t CmCnmTaskManagerCfg =
{
    {"task", CM_OMI_OBJECT_TASK_MANAGER},
    sizeof(CmOmiMapFieldsTaskManager) / sizeof(cm_omi_map_object_field_t),
    CmOmiMapFieldsTaskManager,
    sizeof(CmOmiMapTaskManagerCmds) / sizeof(cm_omi_map_object_cmd_t),
    CmOmiMapTaskManagerCmds
};
