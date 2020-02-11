/******************************************************************************
 *          Copyright (c) 2017 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cfg_cnm.c
 * author     : wbn
 * create date: 2017年10月26日
 * description: TODO:
 *
 *****************************************************************************/
#include "cm_omi_types.h"
#include "cm_cfg_cnm.h"
const cm_omi_map_cfg_t CmOmiMapBool[] =
{
    {"FALSE", 0},
    {"TRUE", 1},
};

const cm_omi_map_enum_t CmOmiMapEnumBoolType =
{
    sizeof(CmOmiMapBool) / sizeof(cm_omi_map_cfg_t),
    CmOmiMapBool
};

const cm_omi_map_cfg_t CmOmiClusterStatusBool[] =
{
    {"down", 0},
    {"up", 1},
};

const cm_omi_map_enum_t CmOmiMapClusterStatusEnumBoolType =
{
    sizeof(CmOmiClusterStatusBool) / sizeof(cm_omi_map_cfg_t),
    CmOmiClusterStatusBool
};

const cm_omi_map_cfg_t CmOmiNasStatusMapBool[] =
{
    {"offline", 0},
    {"online", 1},
};

const cm_omi_map_enum_t CmOmiMapNasStatusEnumBoolType =
{
    sizeof(CmOmiNasStatusMapBool) / sizeof(cm_omi_map_cfg_t),
    CmOmiNasStatusMapBool
};
const cm_omi_map_cfg_t CmOmiMapNasRoleType[] =
{
    {"slave", 0},
    {"master", 1},
    {"master2", 2},
    {"master3", 3},
    {"master4", 4},
};
const cm_omi_map_enum_t CmOmiMapNasRoleTypeCfg =
{
    sizeof(CmOmiMapNasRoleType) / sizeof(cm_omi_map_cfg_t),
    CmOmiMapNasRoleType
};
const cm_omi_map_cfg_t CmOmiMapSwitch[] =
{
    {"OFF", 0},
    {"ON", 1},
};

const cm_omi_map_enum_t CmOmiMapEnumSwitchType =
{
    sizeof(CmOmiMapSwitch) / sizeof(cm_omi_map_cfg_t),
    CmOmiMapSwitch
};

const cm_omi_map_cfg_t CmOmiMapDomain[] =
{
    {"everyone", CM_DOMAIN_EVERYONE},
    {"LOCAL", CM_DOMAIN_LOCAL},
    {"AD", CM_DOMAIN_AD},
};

const cm_omi_map_enum_t CmOmiMapEnumDomainType =
{
    sizeof(CmOmiMapDomain) / sizeof(cm_omi_map_cfg_t),
    CmOmiMapDomain
};

const cm_omi_map_cfg_t CmOmiMapCmds[CM_OMI_CMD_BUTT] =
{
    {"getbatch", CM_OMI_CMD_GET_BATCH},
    {"get", CM_OMI_CMD_GET},
    {"count", CM_OMI_CMD_COUNT},
    {"insert", CM_OMI_CMD_ADD},
    {"update", CM_OMI_CMD_MODIFY},
    {"delete", CM_OMI_CMD_DELETE},
    {"poweron", CM_OMI_CMD_ON},
    {"poweroff", CM_OMI_CMD_OFF},
    {"reboot", CM_OMI_CMD_REBOOT},
    {"scan",CM_OMI_CMD_SCAN},
    {"test",CM_OMI_CMD_TEST},
};

const cm_omi_map_enum_t CmOmiMapCmdsType = 
{
    sizeof(CmOmiMapCmds) / sizeof(cm_omi_map_cfg_t),
    CmOmiMapCmds
};

const cm_omi_map_cfg_t CmOmiIpfStatusBool[] =
{
    {"disable", 0},
    {"enable", 1},
};
const cm_omi_map_enum_t CmOmiMapIpfStatusEnumBoolType =
{
    sizeof(CmOmiIpfStatusBool) / sizeof(cm_omi_map_cfg_t),
    CmOmiIpfStatusBool
};
const cm_omi_map_cfg_t CmOmiIpfopBool[] =
{
    {"deny", 0},
    {"allow", 1},
};
const cm_omi_map_enum_t CmOmiMapIpfopEnumBoolType =
{
    sizeof(CmOmiIpfopBool) / sizeof(cm_omi_map_cfg_t),
    CmOmiIpfopBool
};
const cm_omi_map_cfg_t CmOmiIpfoptionBool[] =
{
    {"block", 0},
    {"pass", 1},
};
const cm_omi_map_enum_t CmOmiMapIpfoptionEnumBoolType =
{
    sizeof(CmOmiIpfoptionBool) / sizeof(cm_omi_map_cfg_t),
    CmOmiIpfoptionBool
};
const cm_omi_map_cfg_t CmOmiMapMemStatusBool[] =
{
    {"busy", 0},
    {"free", 1},
};
const cm_omi_map_enum_t CmOmiMapMemStatusEnumBoolType =
{
    sizeof(CmOmiMapMemStatusBool) / sizeof(cm_omi_map_cfg_t),
    CmOmiMapMemStatusBool
};

const cm_omi_map_cfg_t CmOmiMapPmmGrain[] =
{
    {"second", CM_OMI_PMM_GRAIN_SEC},
    {"hours", CM_OMI_PMM_GRAIN_HOUR},
};
const cm_omi_map_enum_t CmOmiMapPmmGrainType =
{
    sizeof(CmOmiMapPmmGrain) / sizeof(cm_omi_map_cfg_t),
    CmOmiMapPmmGrain
};

/* discard | noallow | restricted | passthrough | passthrough-x */
const cm_omi_map_cfg_t CmOmiMapAclInherit[] =
{
    {"discard", CM_OMI_ACL_DISCARD},
    {"noallow", CM_OMI_ACL_NOALLOW},
    {"restricted", CM_OMI_ACL_RESTRICTED},
    {"passthrough", CM_OMI_ACL_PASSTHROUGH},
    {"passthrough-x", CM_OMI_ACL_PASSTHROUGH_X},
};
const cm_omi_map_enum_t CmOmiMapAclInheritType =
{
    sizeof(CmOmiMapAclInherit) / sizeof(cm_omi_map_cfg_t),
    CmOmiMapAclInherit
};

const cm_omi_map_object_field_t CmOmiMapCommFields[CM_OMI_FIELD_BUTT] =
{
    {{"fields", CM_OMI_FIELD_FIELDS}, "-d", {CM_OMI_DATA_STRING, CM_OMI_FIELDS_LEN, {"[0-9a-zA-Z_,]{1,128}"}}},
    {{"offset", CM_OMI_FIELD_FROM}, "-f", {CM_OMI_DATA_INT, 8, {NULL}}},
    {{"total", CM_OMI_FIELD_TOTAL}, "-t", {CM_OMI_DATA_INT, 8, {NULL}}},
    {{"count", CM_OMI_FIELD_COUNT}, "-c", {CM_OMI_DATA_INT, 8, {NULL}}},
    {{"parent_id", CM_OMI_FIELD_PARENT_ID}, "-p", {CM_OMI_DATA_STRING, CM_ID_LEN, {"[0-9a-zA-Z]{1,64}"}}},
    {{"time_start", CM_OMI_FIELD_TIME_START}, "-ts", {CM_OMI_DATA_TIME, 4, {"^[1-9][0-9]{3,13}"}}},
    {{"time_end", CM_OMI_FIELD_TIME_END}, "-te", {CM_OMI_DATA_TIME, 4, {"^[1-9][0-9]{3,13}"}}},
    {{"param", CM_OMI_FIELD_PARAM}, "-param", {CM_OMI_DATA_STRING, CM_STRING_1K, {NULL}}},
    {{"grain", CM_OMI_FIELD_PMM_GRAIN}, "-grain", {CM_OMI_DATA_ENUM, 4, {&CmOmiMapPmmGrainType}}},
};

const uint32 g_CmOmiObjCmdNoCheck[] = 
{
    /* Objectid, CmdNum, Cmd1, Cmd2, */
    CM_OMI_OBJECT_PERMISSION,2,CM_OMI_CMD_GET_BATCH,CM_OMI_CMD_COUNT,
    CM_OMI_OBJECT_SYS_TIME,1,CM_OMI_CMD_GET,
    CM_OMI_OBJECT_SESSION,2,CM_OMI_CMD_GET,CM_OMI_CMD_DELETE,
    CM_OMI_OBJECT_NODE,3,CM_OMI_CMD_GET_BATCH,CM_OMI_CMD_COUNT,CM_OMI_CMD_GET,
    CM_OMI_OBJECT_CLUSTER,1,CM_OMI_CMD_GET,
    CM_OMI_OBJECT_ALARM_CURRENT,3,CM_OMI_CMD_GET_BATCH,CM_OMI_CMD_COUNT,CM_OMI_CMD_GET,
    CM_OMI_OBJECT_CAPACITY,1,CM_OMI_CMD_GET,
    CM_OMI_OBJECT_PMM_CLUSTER,2,CM_OMI_CMD_GET_BATCH,CM_OMI_CMD_GET,
    CM_OMI_OBJECT_PMM_NODE,2,CM_OMI_CMD_GET_BATCH,CM_OMI_CMD_GET,
    
    /* 结束定义 */
    CM_OMI_OBJECT_DEMO,0
};

const uint32 *g_CmOmiObjCmdNoCheckPtr = g_CmOmiObjCmdNoCheck;


extern const cm_omi_map_object_t CmCnmDemoCfg;
extern const cm_omi_map_object_t CmCnmClusterCfg;
extern const cm_omi_map_object_t CmCnmNodeCfg;
extern const cm_omi_map_object_t CmCnmPmmClusterCfg;
extern const cm_omi_map_object_t CmCnmPmmNodeCfg;
extern const cm_omi_map_object_t CmCnmAlarmCfg;
extern const cm_omi_map_object_t CmCnmAlarmHisCfg;
extern const cm_omi_map_object_t CmCnmAlarmConfigCfg;
extern const cm_omi_map_object_t CmCnmSysTimeCfg;
extern const cm_omi_map_object_t CmCnmPhysCfg;
extern const cm_omi_map_object_t CmCnmPmmConfigCfg;
extern const cm_omi_map_object_t CmCnmAdminCfg;
extern const cm_omi_map_object_t CmCnmSessionCfg;
extern const cm_omi_map_object_t CmCnmDiskCfg;
extern const cm_omi_map_object_t CmCnmPoolCfg;
extern const cm_omi_map_object_t CmCnmPoolDiskCfg;
extern const cm_omi_map_object_t CmCnmLunCfg;
extern const cm_omi_map_object_t CmCnmSnapshotCfg;
extern const cm_omi_map_object_t CmCnmSnapshotScheCfg;
extern const cm_omi_map_object_t CmCnmHostGroupCfg;
extern const cm_omi_map_object_t CmCnmTargetCfg;
extern const cm_omi_map_object_t CmCnmLunmapCfg;
extern const cm_omi_map_object_t CmCnmDiskSpareCfg;
extern const cm_omi_map_object_t CmCnmNasCfg;
extern const cm_omi_map_object_t CmCnmCifsCfg;
extern const cm_omi_map_object_t CmCnmClusterStatCfg;
extern const cm_omi_map_object_t CmUserCfg;
extern const cm_omi_map_object_t CmGroupCfg;
extern const cm_omi_map_object_t CmCnmNfsCfg;
extern const cm_omi_map_object_t CmCnmSnapshotBackupCfg;
extern const cm_omi_map_object_t CmCnmTaskManagerCfg;
extern const cm_omi_map_object_t CmCnmRemoteCfg;
extern const cm_omi_map_object_t CmCnmPingCfg;
extern const cm_omi_map_object_t CmCnmQuotaCfg;
extern const cm_omi_map_object_t CmCnmIpshiftCfg;
extern const cm_omi_map_object_t CmCnmSnapshotCloneCfg;
extern const cm_omi_map_object_t CmCnmRouteCfg;
extern const cm_omi_map_object_t CmCnmPhys_ipCfg;
extern const cm_omi_map_object_t CmCnmClusterNasCfg;
extern const cm_omi_map_object_t CmCnmDualliveNasCfg;
extern const cm_omi_map_object_t CmCnmBackupNasCfg;
extern const cm_omi_map_object_t CmCnmLunMirrorCfg;
extern const cm_omi_map_object_t CmCnmLunBackupCfg;
extern const cm_omi_map_object_t CmCnmDualliveNetifCfg;
extern const cm_omi_map_object_t CmCnmMigrateCfg;
extern const cm_omi_map_object_t CmCnmPoolReconstructCfg;
extern const cm_omi_map_object_t CmCnmNodeServiceCfg;
extern const cm_omi_map_object_t CmCnmAggrCfg;
extern const cm_omi_map_object_t CmCnmDnsmCfg;
extern const cm_omi_map_object_t CmCnmPmm_poolCfg;
extern const cm_omi_map_object_t CmCnmPermissionCfg;
extern const cm_omi_map_object_t CmCnmPmmNasCfg;
extern const cm_omi_map_object_t CmCnmPmmSasCfg;
extern const cm_omi_map_object_t CmCnmPmmProtocolCfg;
extern const cm_omi_map_object_t CmCnmPmmCacheCfg;
extern const cm_omi_map_object_t CmCnmSasCfg;
extern const cm_omi_map_object_t CmCnmProtoCfg;
extern const cm_omi_map_object_t CmCnmPmmLunCfg;
extern const cm_omi_map_object_t CmCnmPmmNicCfg;
extern const cm_omi_map_object_t CmCnmPmmDiskCfg;
extern const cm_omi_map_object_t CmCnmNas_shadowCfg;
extern const cm_omi_map_object_t CmCnmFcinfoCfg;
extern const cm_omi_map_object_t CmExplorerCfg;
extern const cm_omi_map_object_t CmCnmIpfCfg;
extern const cm_omi_map_object_t CmCnmDirQuotaCfg;
extern const cm_omi_map_object_t CmCnmTopoPowerCfg;
extern const cm_omi_map_object_t CmCnmTopoFanCfg;
extern const cm_omi_map_object_t CmCnmTopoTableCfg;
extern const cm_omi_map_object_t CmCnmNtpCfg;
extern const cm_omi_map_object_t CmCnmThresholdCfg;
extern const cm_omi_map_object_t CmCnmLowdataCfg;
extern const cm_omi_map_object_t CmCnmDirLowdataCfg;
extern const cm_omi_map_object_t CmDomainUserCfg;
extern const cm_omi_map_object_t CmCnmDnsCfg;
extern const cm_omi_map_object_t CmCnmDomainAdCfg;
extern const cm_omi_map_object_t CmCnmLowdataScheCfg;
extern const cm_omi_map_object_t CmCnmNasClientCfg;
extern const cm_omi_map_object_t CmCnmPoolExtCfg;
extern const cm_omi_map_object_t CmCnmUpgradeCfg;
extern const cm_omi_map_object_t CmCnmLowdataVolumeCfg;
extern const cm_omi_map_object_t CmCnmMailsendCfg;
extern const cm_omi_map_object_t CmCnmMailrecvCfg;
extern const cm_omi_map_object_t CmCnmNascopyCfg;
extern const cm_omi_map_object_t CmDomainUserCacheCfg;

const cm_omi_map_object_t* CmOmiMap[CM_OMI_OBJECT_BUTT] =
{
    /* Demo */
    &CmCnmDemoCfg,
    /* cluster */
    &CmCnmClusterCfg,
    /* node */
    &CmCnmNodeCfg,
    /* PmmCluster */
    &CmCnmPmmClusterCfg,
    /* PmmNode */
    &CmCnmPmmNodeCfg,
    /* Alarm*/
    &CmCnmAlarmCfg,
    &CmCnmAlarmHisCfg,
    &CmCnmAlarmConfigCfg,
    &CmCnmSysTimeCfg,
    &CmCnmPhysCfg,
    &CmCnmPmmConfigCfg,
    &CmCnmAdminCfg,
    &CmCnmSessionCfg,
    &CmCnmDiskCfg,
    &CmCnmPoolCfg,
    &CmCnmPoolDiskCfg,
    &CmCnmLunCfg,
    &CmCnmSnapshotCfg,
    &CmCnmSnapshotScheCfg,
    &CmCnmHostGroupCfg,
    &CmCnmTargetCfg,
    &CmCnmLunmapCfg,
    &CmCnmDiskSpareCfg,
    &CmCnmNasCfg,
    &CmCnmCifsCfg,
    &CmCnmClusterStatCfg,
    &CmUserCfg,
    &CmGroupCfg,
    &CmCnmNfsCfg,
    &CmCnmSnapshotBackupCfg,
    &CmCnmTaskManagerCfg,
    &CmCnmRemoteCfg,
    &CmCnmPingCfg,
    &CmCnmQuotaCfg,
    &CmCnmIpshiftCfg,
    &CmCnmSnapshotCloneCfg,
    &CmCnmRouteCfg,
    &CmCnmPhys_ipCfg,
    &CmCnmClusterNasCfg,
    &CmCnmDualliveNasCfg,
    &CmCnmBackupNasCfg,
    &CmCnmLunMirrorCfg,
    &CmCnmLunBackupCfg,
    &CmCnmDualliveNetifCfg,
    &CmCnmMigrateCfg,
    &CmCnmPoolReconstructCfg,
    &CmCnmNodeServiceCfg,
    &CmCnmAggrCfg,
	&CmCnmDnsmCfg,

	&CmCnmPermissionCfg,
	&CmCnmPmm_poolCfg,
	&CmCnmPmmNasCfg,
	&CmCnmPmmSasCfg,
	&CmCnmPmmProtocolCfg,
	&CmCnmPmmCacheCfg,
	&CmCnmSasCfg,
	&CmCnmProtoCfg,
	&CmCnmPmmLunCfg,
	&CmCnmPmmNicCfg,
	&CmCnmPmmDiskCfg,
	&CmCnmNas_shadowCfg,
	&CmCnmFcinfoCfg,
	&CmExplorerCfg,
	&CmCnmIpfCfg,
	&CmCnmDirQuotaCfg,
  	&CmCnmTopoPowerCfg,
	&CmCnmTopoFanCfg,
	&CmCnmTopoTableCfg,
	&CmCnmNtpCfg,
	&CmCnmThresholdCfg,
	&CmCnmLowdataCfg,
	&CmCnmDirLowdataCfg,
	&CmDomainUserCfg,
	&CmCnmDnsCfg,
	&CmCnmDomainAdCfg,
	&CmCnmLowdataScheCfg,
	&CmCnmNasClientCfg,
	&CmCnmPoolExtCfg,
	&CmCnmUpgradeCfg,
	&CmCnmLowdataVolumeCfg,
	&CmCnmMailsendCfg,
	&CmCnmMailrecvCfg,
	&CmCnmNascopyCfg,
	&CmDomainUserCacheCfg,
};

const cm_omi_map_cfg_t CmCnmMapCommErr[] =
{
    {"fmadm: failed to connect to fmd", CM_ERR_LUN_FMADM},
    {"does not exist", CM_ERR_NOT_EXISTS},
    {"not found", CM_ERR_NOT_EXISTS},
    {"already exists", CM_ERR_ALREADY_EXISTS},
    {"volume size must be", CM_ERR_LUN_SIZE},
    {"cannot modify while in standby mode", CM_ERR_LUN_STANDBY},
    {"more recent snapshots exist", CM_ERR_SNAPSHOT_RECENT_EXISTS},
    {"snapshot has dependent clones",CM_ERR_SNAPSHOT_DEPENDS_CLONES},
    {"group is in use by existing view entry",CM_ERR_HOSTGROUP_USING_BY_VIEWENTRY},
};

const cm_omi_map_enum_t CmCnmMapCommErrCfg =
{
    sizeof(CmCnmMapCommErr) / sizeof(cm_omi_map_cfg_t),
    CmCnmMapCommErr
};

