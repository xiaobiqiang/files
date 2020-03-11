#include "cm_omi_types.h"
#include "cm_cfg_cnm.h"
#include "cm_cfg_public.h"
#include "cm_cfg_omi.h"

const cm_omi_map_object_field_t CmOmiMapFieldsSnapshotClone[] =
{
    {{"nid", CM_OMI_FIELD_SNAPSHOT_CLONE_NID}, "-n", {CM_OMI_DATA_INT, 4, {NULL}}},
    {{"path", CM_OMI_FIELD_SNAPSHOT_CLONE_PATH}, "-p", {CM_OMI_DATA_STRING, 64, {NULL}}},
    {{"dst", CM_OMI_FIELD_SNAPSHOT_CLONE_DST}, "-d", {CM_OMI_DATA_STRING, 64, {NULL}}},
};

const cm_omi_map_object_field_t* CmOmiMapSnapshotCloneCmdNeedParamsAdd[] =
{
    &CmOmiMapFieldsSnapshotClone[CM_OMI_FIELD_SNAPSHOT_CLONE_NID],
    &CmOmiMapFieldsSnapshotClone[CM_OMI_FIELD_SNAPSHOT_CLONE_PATH],
    &CmOmiMapFieldsSnapshotClone[CM_OMI_FIELD_SNAPSHOT_CLONE_DST],
};

const cm_omi_map_object_cmd_t CmOmiMapSnapshotCloneCmds[] =
{
    {
        &CmOmiMapCmds[CM_OMI_CMD_ADD],
        sizeof(CmOmiMapSnapshotCloneCmdNeedParamsAdd) / sizeof(cm_omi_map_object_field_t *),
        CmOmiMapSnapshotCloneCmdNeedParamsAdd,
        0, NULL
    },
};

const cm_omi_map_object_t CmCnmSnapshotCloneCfg =
{
    {"snapshot_clone", CM_OMI_OBJECT_SNAPSHOT_CLONE},
    sizeof(CmOmiMapFieldsSnapshotClone) / sizeof(cm_omi_map_object_field_t),
    CmOmiMapFieldsSnapshotClone,
    sizeof(CmOmiMapSnapshotCloneCmds) / sizeof(cm_omi_map_object_cmd_t),
    CmOmiMapSnapshotCloneCmds,
};
