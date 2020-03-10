#include "cm_omi_types.h"
#include "cm_cfg_cnm.h"
#include "cm_cfg_public.h"
#include "cm_cfg_omi.h"     

const cm_omi_map_object_field_t CmOmiMapFieldsSnapshotBackup[] =
{
    {{"src_nid", CM_OMI_FIELD_SNAPSHOT_BACKUP_SRC_NID}, "-n", {CM_OMI_DATA_INT, 4, {NULL}}},
    {{"src_path", CM_OMI_FIELD_SNAPSHOT_BACKUP_SRC_PRE_PATH}, "-p", {CM_OMI_DATA_STRING, CM_NAME_LEN_DIR, {NULL}}},
    {{"ip", CM_OMI_FIELD_SNAPSHOT_BACKUP_DST_IPADDR}, "-ip", {CM_OMI_DATA_STRING, CM_IP_LEN, {NULL}}}, 
    {{"dst_dir", CM_OMI_FIELD_SNAPSHOT_BACKUP_DST_DIR}, "-d", {CM_OMI_DATA_STRING, CM_NAME_LEN_DIR, {NULL}}},
    {{"sec_path", CM_OMI_FIELD_SNAPSHOT_BACKUP_SRC_AFT_PATH}, "-sp", {CM_OMI_DATA_STRING, CM_NAME_LEN_DIR, {NULL}}},
};

const cm_omi_map_object_field_t CmOmiMapFieldsPing[] =
{
    {{"ip", CM_OMI_FIELD_PING_IPADDR}, "-p", {CM_OMI_DATA_STRING, CM_IP_LEN, {NULL}}},
};


const cm_omi_map_object_field_t* CmOmiMapSnapshotBackupCmdNeedParamsAdd[] =
{
    &CmOmiMapFieldsSnapshotBackup[CM_OMI_FIELD_SNAPSHOT_BACKUP_SRC_NID],
    &CmOmiMapFieldsSnapshotBackup[CM_OMI_FIELD_SNAPSHOT_BACKUP_SRC_PRE_PATH],
    &CmOmiMapFieldsSnapshotBackup[CM_OMI_FIELD_SNAPSHOT_BACKUP_DST_IPADDR],
    &CmOmiMapFieldsSnapshotBackup[CM_OMI_FIELD_SNAPSHOT_BACKUP_DST_DIR],
};

const cm_omi_map_object_field_t* CmOmiMapSnapshotBackupCmdOptParamsAdd[] =
{
    &CmOmiMapFieldsSnapshotBackup[CM_OMI_FIELD_SNAPSHOT_BACKUP_SRC_AFT_PATH],
};

const cm_omi_map_object_field_t* CmOmiMapPingCmdNeedParamsAdd[] =  //check ip
{
    &CmOmiMapFieldsPing[CM_OMI_FIELD_PING_IPADDR],
};


const cm_omi_map_object_cmd_t CmOmiMapSnapshotBackupCmds[] =
{
    {
        &CmOmiMapCmds[CM_OMI_CMD_ADD],
        sizeof(CmOmiMapSnapshotBackupCmdNeedParamsAdd) / sizeof(cm_omi_map_object_field_t *),
        CmOmiMapSnapshotBackupCmdNeedParamsAdd,
        sizeof(CmOmiMapSnapshotBackupCmdOptParamsAdd) / sizeof(cm_omi_map_object_field_t *),
        CmOmiMapSnapshotBackupCmdOptParamsAdd,
    },
};

const cm_omi_map_object_cmd_t CmOmiMapPingCmds[] =
{
    {
        &CmOmiMapCmds[CM_OMI_CMD_ADD],
        sizeof(CmOmiMapPingCmdNeedParamsAdd) / sizeof(cm_omi_map_object_field_t *),
        CmOmiMapPingCmdNeedParamsAdd,
        0, NULL
    },
};


const cm_omi_map_object_t CmCnmSnapshotBackupCfg =
{
    {"snapshot_backup", CM_OMI_OBJECT_SNAPSHOT_BACKUP},
    sizeof(CmOmiMapFieldsSnapshotBackup) / sizeof(cm_omi_map_object_field_t),
    CmOmiMapFieldsSnapshotBackup,
    sizeof(CmOmiMapSnapshotBackupCmds) / sizeof(cm_omi_map_object_cmd_t),
    CmOmiMapSnapshotBackupCmds,
};

const cm_omi_map_object_t CmCnmPingCfg =
{
    {"ping", CM_OMI_OBJECT_PING},
    sizeof(CmOmiMapFieldsPing) / sizeof(cm_omi_map_object_field_t),
    CmOmiMapFieldsPing,
    sizeof(CmOmiMapPingCmds) / sizeof(cm_omi_map_object_cmd_t),
    CmOmiMapPingCmds,
};
