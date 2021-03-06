/******************************************************************************
 *          Copyright (c) 2017 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cfg_omi.h
 * author     : wbn
 * create date: 2017年10月26日
 * description: TODO:
 *
 *****************************************************************************/
#ifndef BASE_CONFIG_CM_CFG_OMI_H_
#define BASE_CONFIG_CM_CFG_OMI_H_

typedef enum
{
    CM_OMI_OBJECT_DEMO = 0,

    CM_OMI_OBJECT_CLUSTER,
    CM_OMI_OBJECT_NODE,
    CM_OMI_OBJECT_PMM_CLUSTER,
    CM_OMI_OBJECT_PMM_NODE,
    CM_OMI_OBJECT_ALARM_CURRENT = 5,

    CM_OMI_OBJECT_ALARM_HISTORY,
    CM_OMI_OBJECT_ALARM_CONFIG,
    CM_OMI_OBJECT_SYS_TIME,
    CM_OMI_OBJECT_PHYS,
    CM_OMI_OBJECT_PMM_CONFIG = 10,

    CM_OMI_OBJECT_ADMIN,
    CM_OMI_OBJECT_SESSION,
    CM_OMI_OBJECT_DISK,
    CM_OMI_OBJECT_POOL,
    CM_OMI_OBJECT_POOLDISK = 15,

    CM_OMI_OBJECT_LUN,
    CM_OMI_OBJECT_SNAPSHOT,
    CM_OMI_OBJECT_SNAPSHOT_SCHE,
    CM_OMI_OBJECT_HOSTGROUP,
    CM_OMI_OBJECT_TARGET = 20,

    CM_OMI_OBJECT_LUNMAP,
    CM_OMI_OBJECT_DISK_SPARE,
    CM_OMI_OBJECT_NAS,
    CM_OMI_OBJECT_CIFS,
    CM_OMI_OBJECT_CAPACITY = 25,

    CM_OMI_OBJECT_USER,
    CM_OMI_OBJECT_GROUP,
    CM_OMI_OBJECT_SHARE_NFS,
    CM_OMI_OBJECT_SNAPSHOT_BACKUP,
    CM_OMI_OBJECT_TASK_MANAGER = 30,

    CM_OMI_OBJECT_REMOTE,
    CM_OMI_OBJECT_PING,
    CM_OMI_OBJECT_QUOTA,
    CM_OMI_OBJECT_IPSHFIT,
    CM_OMI_OBJECT_SNAPSHOT_CLONE = 35,

    CM_OMI_OBJECT_ROUTE,
    CM_OMI_OBJECT_PHYS_IP,
    CM_OMI_OBJECT_CLUSTER_NAS,
    CM_OMI_OBJECT_DUALLIVE_NAS,
    CM_OMI_OBJECT_BACKUP_NAS = 40,
    
    CM_OMI_OBJECT_LUN_MIRROR,
    CM_OMI_OBJECT_LUN_BACKUP,
    CM_OMI_OBJECT_CLUSTERSAN_TARGET,
    CM_OMI_OBJECT_MIGRATE,
    CM_OMI_OBJECT_POOL_RECONSTRUCT = 45,

    CM_OMI_OBJECT_NODE_SERVCE,
    CM_OMI_OBJECT_AGGR,
    CM_OMI_OBJECT_DNSM,
    CM_OMI_OBJECT_PERMISSION,
    CM_OMI_OBJECT_PMM_POOL = 50,

    CM_OMI_OBJECT_PMM_NAS,
    CM_OMI_OBJECT_PMM_SAS,
    CM_OMI_OBJECT_PMM_PROTO,
    CM_OMI_OBJECT_PMM_CACHE,
    CM_OMI_OBJECT_CNM_SAS = 55,

    CM_OMI_OBJECT_CNM_PROTO,
    CM_OMI_OBJECT_PMM_LUN,
    CM_OMI_OBJECT_PMM_NIC,
    CM_OMI_OBJECT_PMM_DISK,
    CM_OMI_OBJECT_CNM_NAS_SHADOW = 60,

    CM_OMI_OBJECT_FCINFO,
    CM_OMI_OBJECT_EXPLORER,
    CM_OMI_OBJECT_IPF,
    CM_OMI_OBJECT_DIRQUOTA,
    CM_OMI_OBJECT_TOPO_POWER = 65,
    
    CM_OMI_OBJECT_TOPO_FAN,
	CM_OMI_OBJECT_TOPO_TABLE,
	CM_OMI_OBJECT_NTP,
    CM_OMI_OBJECT_THRESHOLD,
    CM_OMI_OBJECT_LOWDATA = 70,
    
    CM_OMI_OBJECT_DIRLOWDATA,
    CM_OMI_OBJECT_DOMAIN_USER,
    CM_OMI_OBJECT_DNS,
    CM_OMI_OBJECT_DOMAIN_AD,
    CM_OMI_OBJECT_LOWDATA_SCHE = 75,

    CM_OMI_OBJECT_NAS_CLIENT,
    CM_OMI_OBJECT_POOLEXT,
    CM_OMI_OBJECT_UPGRADE,
    CM_OMI_OBJECT_LOWDATA_VOLUME,
    CM_OMI_OBJECT_MAILSEND=80,
    
    CM_OMI_OBJECT_MAILRECV,
    CM_OMI_OBJECT_NASCOPY,
    CM_OMI_OBJECT_UCACHE,
    CM_OMI_OBJECT_REMOTE_CLUSTER,
    CM_OMI_OBJECT_LOCAL_TASK=85,
    
    CM_OMI_OBJECT_BUTT
} cm_omi_object_e;

typedef enum
{
    CM_OMI_CMD_GET_BATCH = 0,
    CM_OMI_CMD_GET,
    CM_OMI_CMD_COUNT,
    CM_OMI_CMD_ADD,
    CM_OMI_CMD_MODIFY,
    CM_OMI_CMD_DELETE,
    CM_OMI_CMD_ON,
    CM_OMI_CMD_OFF,
    CM_OMI_CMD_REBOOT,
    CM_OMI_CMD_SCAN,
    CM_OMI_CMD_TEST,
    CM_OMI_CMD_BUTT
} cm_omi_cmd_e;

typedef enum
{
    CM_OMI_FIELD_FIELDS = 256, /* 0 */
    CM_OMI_FIELD_FROM,         /* 1 */
    CM_OMI_FIELD_TOTAL = 258,  /* 2 */
    CM_OMI_FIELD_COUNT,        /* 3 */
    CM_OMI_FIELD_PARENT_ID = 260, /* 4 */
    CM_OMI_FIELD_TIME_START,      /* 5 */
    CM_OMI_FIELD_TIME_END = 262,  /* 6 */
    CM_OMI_FIELD_PARAM,           /* 7 */
    CM_OMI_FIELD_PMM_GRAIN = 264, /* 8 */
    CM_OMI_FIELD_ERROR_MSG = 265,
    CM_OMI_FIELD_TASK_ID = 266,
} cm_omi_field_comm_e;

typedef enum
{
    CM_OMI_PMM_GRAIN_SEC = 0,
    CM_OMI_PMM_GRAIN_HOUR = 1,
}cm_omi_pmm_grain_e;
#define CM_OMI_FIELD_BUTT 9
#define CM_OMI_FIELDS_LEN 128

typedef enum
{
    CM_OMI_FIELD_CLUSTER_NAME = 0,
    CM_OMI_FIELD_CLUSTER_VERSION,
    CM_OMI_FIELD_CLUSTER_IPADDR,
    CM_OMI_FIELD_CLUSTER_GATEWAY,
    CM_OMI_FIELD_CLUSTER_NETMASK,
    
    CM_OMI_FIELD_CLUSTER_TIMESTAMP = 5,
    CM_OMI_FIELD_CLUSTER_INTERFACE,
    CM_OMI_FIELD_CLUSTER_PRIORITY,
    CM_OMI_FIELD_CLUSTER_ENABLE_IPMI,
    CM_OMI_FIELD_CLUSTER_FAILOVER,
    CM_OMI_FIELD_CLUSTER_PRODUCT
} cm_omi_field_cluster_e;

typedef enum
{
    CM_OMI_FIELD_CLUSTER_DISK_USED = 0,
    CM_OMI_FIELD_CLUSTER_DISK_AVAIL,
    CM_OMI_FIELD_CLUSTER_POOL_USED,
    CM_OMI_FIELD_CLUSTER_POOL_AVAIL,
    CM_OMI_FIELD_CLUSTER_LUN_USED,
    CM_OMI_FIELD_CLUSTER_LUN_AVAIL,
    CM_OMI_FIELD_CLUSTER_NAS_USED,
    CM_OMI_FIELD_CLUSTER_NAS_AVAIL,

    CM_OMI_FIELD_CLUSTER_H_USED,
    CM_OMI_FIELD_CLUSTER_H_TOTAL,

    CM_OMI_FIELD_CLUSTER_L_USED,
    CM_OMI_FIELD_CLUSTER_L_TOTAL,
    
} cm_omi_filed_cluster_stat;

typedef enum
{
    CM_OMI_FIELD_NODE_ID = 0,
    CM_OMI_FIELD_NODE_NAME,
    CM_OMI_FIELD_NODE_VERSION,
    CM_OMI_FIELD_NODE_SN,
    CM_OMI_FIELD_NODE_IP,
    CM_OMI_FIELD_NODE_FRAME,
    CM_OMI_FIELD_NODE_DEV_TYPE,
    CM_OMI_FIELD_NODE_SLOT,
    CM_OMI_FIELD_NODE_RAM,
    CM_OMI_FIELD_NODE_STATE,
    CM_OMI_FIELD_NODE_SBBID,
    CM_OMI_FIELD_NODE_IPMI_USER,
    CM_OMI_FIELD_NODE_IPMI_PASSWD,
} cm_omi_field_node_e;

typedef enum
{
    CM_OMI_FIELD_PMM_CLUSTER_ID = 0,
    CM_OMI_FIELD_PMM_CLUSTER_TIMESTAMP,
    CM_OMI_FIELD_PMM_CLUSTER_CPU_MAX,
    CM_OMI_FIELD_PMM_CLUSTER_CPU_AVG,
    CM_OMI_FIELD_PMM_CLUSTER_MEM_MAX,
    CM_OMI_FIELD_PMM_CLUSTER_MEM_AVG,
    CM_OMI_FIELD_PMM_CLUSTER_BW_TOTAL,
    CM_OMI_FIELD_PMM_CLUSTER_BW_READ,
    CM_OMI_FIELD_PMM_CLUSTER_BW_WRITE,

    CM_OMI_FIELD_PMM_CLUSTER_IOPS_TOTAL,
    CM_OMI_FIELD_PMM_CLUSTER_IOPS_READ,
    CM_OMI_FIELD_PMM_CLUSTER_IOPS_WRITE,
} cm_omi_field_pmm_cluster_e;

typedef enum
{
    CM_OMI_FIELD_PMM_NODE_ID = 0,
    CM_OMI_FIELD_PMM_NODE_TIMESTAMP,
    CM_OMI_FIELD_PMM_NODE_CPU,
    CM_OMI_FIELD_PMM_NODE_MEM,
    CM_OMI_FIELD_PMM_NODE_BW_TOTAL,
    CM_OMI_FIELD_PMM_NODE_BW_READ,
    CM_OMI_FIELD_PMM_NODE_BW_WRITE,

    CM_OMI_FIELD_PMM_NODE_IOPS_TOTAL,
    CM_OMI_FIELD_PMM_NODE_IOPS_READ,
    CM_OMI_FIELD_PMM_NODE_IOPS_WRITE,
} cm_omi_field_pmm_node_e;

typedef enum
{
    CM_OMI_FIELD_ALARM_ID = 0,
    CM_OMI_FIELD_ALARM_PARENT_ID,
    CM_OMI_FIELD_ALARM_TYPE,
    CM_OMI_FIELD_ALARM_LVL,
    CM_OMI_FIELD_ALARM_TIME_REPORT,
    CM_OMI_FIELD_ALARM_TIME_RECOVER,
    CM_OMI_FIELD_ALARM_PARAM,
} cm_omi_field_alarm_e;

typedef enum
{
    CM_OMI_FIELD_ALARM_CFG_ID = 0,
    CM_OMI_FIELD_ALARM_CFG_MATCH,
    CM_OMI_FIELD_ALARM_CFG_PARAM_NUM,
    CM_OMI_FIELD_ALARM_CFG_TYPE,
    CM_OMI_FIELD_ALARM_CFG_LVL,
    CM_OMI_FIELD_ALARM_CFG_IS_DISABLE,
} cm_omi_field_alarm_cfg_e;

typedef enum
{
    CM_OMI_FIELD_SYSTIME_TZ = 0,
    CM_OMI_FIELD_SYSTIME_DT,
} cm_omi_field_sys_time_e;

typedef enum
{
    CM_OMI_FIELD_PHYS_NAME = 0,
    CM_OMI_FIELD_PHYS_NID,
    CM_OMI_FIELD_PHYS_STATE,
    CM_OMI_FIELD_PHYS_SPEED,
    CM_OMI_FIELD_PHYS_DUPLEX,
    CM_OMI_FIELD_PHYS_MTU,
    CM_OMI_FIELD_PHYS_MAC
} cm_omi_field_phys_e;

typedef enum
{
    CM_OMI_FIELD_PMM_CFG_SAVE_HOURS = 0,
} cm_omi_field_pmm_config_e;

typedef enum
{
    CM_OMI_FIELD_ADMIN_ID = 0,
    CM_OMI_FIELD_ADMIN_NID,
    CM_OMI_FIELD_ADMIN_LEVEL,
    CM_OMI_FIELD_ADMIN_NAME,
    CM_OMI_FIELD_ADMIN_PWD,
} cm_omi_field_admin_e;

typedef enum
{
    CM_OMI_FIELD_USER_ID = 0,
    CM_OMI_FIELD_USER_GID,
    CM_OMI_FIELD_USER_NAME,
    CM_OMI_FIELD_USER_PWD,
    CM_OMI_FIELD_USER_PATH
} cm_omi_filed_user_e;

typedef enum
{
    CM_OMI_FIELD_SESSION_ID = 0,
    CM_OMI_FIELD_SESSION_USER,
    CM_OMI_FIELD_SESSION_PWD,
    CM_OMI_FIELD_SESSION_IP,
    CM_OMI_FIELD_SESSION_LEVEL
} cm_omi_field_session_e;

typedef enum
{
    CM_OMI_FIELD_DISK_ID = 0,
    CM_OMI_FIELD_DISK_NID,
    CM_OMI_FIELD_DISK_SN,
    CM_OMI_FIELD_DISK_VENDOR,
    CM_OMI_FIELD_DISK_SIZE,
    CM_OMI_FIELD_DISK_RPM,
    CM_OMI_FIELD_DISK_ENID,
    CM_OMI_FIELD_DISK_SLOT,
    CM_OMI_FIELD_DISK_STATUS,
    CM_OMI_FIELD_DISK_POOL,
    CM_OMI_FIELD_DISK_LED,
    CM_OMI_FIELD_DISK_ISLOCAL,
    CM_OMI_FIELD_DISK_TYPE,
} cm_omi_field_disk_e;

typedef enum
{
    CM_OMI_FIELD_POOL_NAME = 0,
    CM_OMI_FIELD_POOL_NID,
    CM_OMI_FIELD_POOL_NID_SRC,
    CM_OMI_FIELD_POOL_STATUS,
    CM_OMI_FIELD_POOL_AVAIL,
    CM_OMI_FIELD_POOL_USED,
    CM_OMI_FIELD_POOL_RAID,
    CM_OMI_FIELD_POOL_GROUP,
    CM_OMI_FIELD_POOL_RDCIP,
    CM_OMI_FIELD_POOL_OPS,
    CM_OMI_FIELD_POOL_VAR,
    /*zpool status*/
    CM_OMI_FIELD_POOL_PROG,
    CM_OMI_FIELD_POOL_SPEED,
    CM_OMI_FIELD_POOL_REPAIRED,
    CM_OMI_FIELD_POOL_ERRORS,
    CM_OMI_FIELD_POOL_TIME,
    CM_OMI_FIELD_POOL_ZSTATUS,   
} cm_omi_field_pool_e;

typedef enum
{
    CM_OMI_FIELD_POOLDISK_NAME = 0,
    CM_OMI_FIELD_POOLDISK_NID,
    CM_OMI_FIELD_POOLDISK_ID,
    CM_OMI_FIELD_POOLDISK_RAID,
    CM_OMI_FIELD_POOLDISK_TYPE,
    CM_OMI_FIELD_POOLDISK_ERR_READ,
    CM_OMI_FIELD_POOLDISK_ERR_WRITE,
    CM_OMI_FIELD_POOLDISK_ERR_SUM,
    CM_OMI_FIELD_POOLDISK_ENID,
    CM_OMI_FIELD_POOLDISK_SLOTID,
    CM_OMI_FIELD_POOLDISK_STATUS,
    CM_OMI_FIELD_POOLDISK_GROUP,
    CM_OMI_FIELD_POOLDISK_SIZE,
} cm_omi_field_pooldisk_e;

typedef enum
{
    CM_OMI_FIELD_LUN_NAME = 0,
    CM_OMI_FIELD_LUN_NID,
    CM_OMI_FIELD_LUN_POOL,
    CM_OMI_FIELD_LUN_TOTAL,
    CM_OMI_FIELD_LUN_USED,

    CM_OMI_FIELD_LUN_AVAIL = 5,
    CM_OMI_FIELD_LUN_BLOCKSIZE,
    CM_OMI_FIELD_LUN_WRITE_POLICY,
    CM_OMI_FIELD_LUN_AC_STATE,
    CM_OMI_FIELD_LUN_STATE,

    CM_OMI_FIELD_LUN_IS_DOUBLE = 10,
    CM_OMI_FIELD_LUN_IS_COMPRESS,
    CM_OMI_FIELD_LUN_IS_HOT,
    CM_OMI_FIELD_LUN_IS_SINGLE,
    CM_OMI_FIELD_LUN_ALARM_THRESHOLD,

    CM_OMI_FIELD_LUN_QOS = 15,
    CM_OMI_FIELD_LUN_QOS_VAL,
    CM_OMI_FIELD_LUN_DEDUP,
} cm_omi_field_lun_e;

typedef enum
{
    CM_OMI_FIELD_SNAPSHOT_NAME = 0,
    CM_OMI_FIELD_SNAPSHOT_NID,
    CM_OMI_FIELD_SNAPSHOT_DIR,
    CM_OMI_FIELD_SNAPSHOT_USED,
    CM_OMI_FIELD_SNAPSHOT_DATE,
    CM_OMI_FIELD_SNAPSHOT_TYPE,
} cm_omi_field_snapshot_e;

typedef enum
{
    CM_OMI_FIELD_SNAPSHOT_SCHE_ID = 0,
    CM_OMI_FIELD_SNAPSHOT_SCHE_NID, /* 预留 */
    CM_OMI_FIELD_SNAPSHOT_SCHE_NAME,
    CM_OMI_FIELD_SNAPSHOT_SCHE_DIR,
    CM_OMI_FIELD_SNAPSHOT_SCHE_EXPIRED,
    CM_OMI_FIELD_SNAPSHOT_SCHE_TYPE,
    CM_OMI_FIELD_SNAPSHOT_SCHE_DAYTYPE,
    CM_OMI_FIELD_SNAPSHOT_SCHE_MINUTE,
    CM_OMI_FIELD_SNAPSHOT_SCHE_HOURS,
    CM_OMI_FIELD_SNAPSHOT_SCHE_DAYS,
} cm_omi_field_snapshot_sche_e;

typedef enum
{
    CM_OMI_FIELD_HG_NAME = 0,
    CM_OMI_FIELD_HG_NID,
    CM_OMI_FIELD_HG_MEMBER,
    CM_OMI_FIELD_HG_ACT, /* 下发参数, 0:add, 1:delete */
    CM_OMI_FIELD_HG_TYPE,
} cm_omi_field_hostgroup_e;

typedef enum
{
    CM_OMI_FIELD_TGT_NAME = 0,
    CM_OMI_FIELD_TGT_NID,
    CM_OMI_FIELD_TGT_PROVIDER,
    CM_OMI_FIELD_TGT_ALIAS,
    CM_OMI_FIELD_TGT_PROTOCOL,
    CM_OMI_FIELD_TGT_STATUS,
    CM_OMI_FIELD_TGT_SESSIONS,
    CM_OMI_FIELD_TGT_INITIATOR,
} cm_omi_field_target_e;

typedef enum
{
    CM_OMI_FIELD_LUNMAP_LUN = 0,
    CM_OMI_FIELD_LUNMAP_NID,
    CM_OMI_FIELD_LUNMAP_HG,
    CM_OMI_FIELD_LUNMAP_TG,
    CM_OMI_FIELD_LUNMAP_LID,
} cm_omi_field_lunmap_e;

typedef enum
{
    CM_OMI_FIELD_DISKSPARE_DISK = 0,
    CM_OMI_FIELD_DISKSPARE_NID,
    CM_OMI_FIELD_DISKSPARE_ENID,
    CM_OMI_FIELD_DISKSPARE_SLOT,
    CM_OMI_FIELD_DISKSPARE_STATUS,
    CM_OMI_FIELD_DISKSPARE_POOL = 5,
    CM_OMI_FIELD_DISKSPARE_USED,
} cm_omi_diskspare_field_e;

typedef enum
{
    CM_OMI_FIELD_NAS_NAME = 0,
    CM_OMI_FIELD_NAS_NID,
    CM_OMI_FIELD_NAS_POOL,
    CM_OMI_FIELD_NAS_BLOCKSIZE,
    CM_OMI_FIELD_NAS_ACCESS,
    CM_OMI_FIELD_NAS_WRITE_POLICY = 5,
    CM_OMI_FIELD_NAS_IS_COMP,
    CM_OMI_FIELD_NAS_QOS,
    CM_OMI_FIELD_NAS_SPACE_AVAIL,
    CM_OMI_FIELD_NAS_SPACE_USED,
    CM_OMI_FIELD_NAS_QUOTA,
    CM_OMI_FIELD_NAS_DEDUP,
    CM_OMI_FIELD_NAS_QOS_AVS,
    CM_OMI_FIELD_NAS_SMB,
    CM_OMI_FIELD_NAS_ABE,
    CM_OMI_FIELD_NAS_ACLINHERIT,
} cm_omi_field_nas_e;

typedef enum
{
    CM_OMI_FIELD_CIFS_DIR = 0,
    CM_OMI_FIELD_CIFS_NID,
    CM_OMI_FIELD_CIFS_NAME,
    CM_OMI_FIELD_CIFS_NAME_TYPE,
    CM_OMI_FIELD_CIFS_PERMISSION,
    CM_OMI_FIELD_CIFS_DOMAIN,
} cm_omi_field_cifs_e;

typedef enum
{
    CM_OMI_NFS_CFG_DIR_PATH,
    CM_OMI_NFS_CFG_NID,
    CM_OMI_NFS_CFG_LIMIT,
    CM_OMI_NFS_CFG_NFS_IP,
} cm_omi_share_nfs_cfg_e;

typedef enum
{
    CM_OMI_FIELD_GROUP_ID = 0,
    CM_OMI_FIELD_GROUP_NAME,
} cm_omi_field_group_e;

typedef enum
{
    CM_OMI_FIELD_SNAPSHOT_BACKUP_SRC_NID = 0,
    CM_OMI_FIELD_SNAPSHOT_BACKUP_SRC_PRE_PATH,
    CM_OMI_FIELD_SNAPSHOT_BACKUP_DST_IPADDR,
    CM_OMI_FIELD_SNAPSHOT_BACKUP_DST_DIR,
    CM_OMI_FIELD_SNAPSHOT_BACKUP_SRC_AFT_PATH,
} cm_omi_field_snapshot_backup_e;

typedef enum
{
    CM_OMI_FIELD_TASK_MANAGER_TID = 0,
    CM_OMI_FIELD_TASK_MANAGER_PID,
    CM_OMI_FIELD_TASK_MANAGER_NID,
    CM_OMI_FIELD_TASK_MANAGER_TYPE,
    CM_OMI_FIELD_TASK_MANAGER_STATE,
    CM_OMI_FIELD_TASK_MANAGER_PROG = 5, //not supported now.
    CM_OMI_FIELD_TASK_MANAGER_STTM, //start time
    CM_OMI_FIELD_TASK_MANAGER_ENDTM,
    CM_OMI_FIELD_TASK_MANAGER_DESC,
} cm_omi_field_task_manager_e;

typedef enum
{
    CM_OMI_FIELD_REMOTE_IPADDR = 0,
    CM_OMI_FIELD_REMOTE_TYPE,
    CM_OMI_FIELD_REMOTE_NAME,
    CM_OMI_FIELD_REMOTE_AVAIL,
    CM_OMI_FIELD_REMOTE_RDCIP,
    CM_OMI_FIELD_REMOTE_QUOTA,
} cm_omi_field_remote_t;

typedef enum
{
    CM_OMI_FIELD_PING_IPADDR,
} cm_omi_field_ping_e;

typedef enum
{
    CM_OMI_FIELD_QUOTA_USERTYPE = 0,
    CM_OMI_FIELD_QUOTA_NID,
    CM_OMI_FIELD_QUOTA_NAME,
    CM_OMI_FIELD_QUOTA_FILESYSTEM,
    CM_OMI_FIELD_QUOTA_HARDSPACE,
    CM_OMI_FIELD_QUOTA_SOFTSPACE,
    CM_OMI_FIELD_QUOTA_USED,
    CM_OMI_FIELD_QUOTA_DOMAIN,
} cm_omi_field_quota_e;

typedef enum
{
    CM_OMI_FIELD_IPSHIFT_FILESYSTEM = 0,
    CM_OMI_FIELD_IPSHIFT_NID,
    CM_OMI_FIELD_IPSHIFT_ADAPTER,
    CM_OMI_FIELD_IPSHIFT_IP,
    CM_OMI_FIELD_IPSHIFT_NETMASK,
} cm_omi_field_ipshift_e;

typedef enum
{
    CM_OMI_FIELD_SNAPSHOT_CLONE_NID = 0,
    CM_OMI_FIELD_SNAPSHOT_CLONE_PATH,
    CM_OMI_FIELD_SNAPSHOT_CLONE_DST,
} cm_omi_field_snapshot_clone_e;

typedef enum
{
    CM_OMI_FIELD_ROUTE_DESTINATION = 0,
    CM_OMI_FIELD_ROUTE_NID,
    CM_OMI_FIELD_ROUTE_NETMASK,
    CM_OMI_FIELD_ROUTE_GETEWAY,
} cm_omi_field_route_e;

typedef enum
{
    CM_OMI_FIELD_PHYS_IP_ADAPTER = 0,
    CM_OMI_FIELD_PHYS_IP_NID,
    CM_OMI_FIELD_PHYS_IP_IP,
    CM_OMI_FIELD_PHYS_IP_NETMASK,
} cm_omi_field_phys_ip_e;

typedef enum
{
    CM_OMI_FIELD_CLUSTER_NAS_GROUP_NAME = 0,
    CM_OMI_FIELD_CLUSTER_NAS_NID,
    CM_OMI_FIELD_CLUSTER_NAS_STATUS,
    CM_OMI_FIELD_CLUSTER_NAS_ZFS_NAME,
    CM_OMI_FIELD_CLUSTER_NAS_ZFS_ROLE,
    CM_OMI_FIELD_CLUSTER_NAS_ZFS_STATUS,
    CM_OMI_FIELD_CLUSTER_NAS_ZFS_AVAIL,
    CM_OMI_FIELD_CLUSTER_NAS_ZFS_USED,
    CM_OMI_FIELD_CLUSTER_NAS_NUM,
} cm_omi_filed_cluster_nas_e;

typedef enum
{
    CM_OMI_FIELD_DUALLIVE_NAS_MNAS,
    CM_OMI_FIELD_DUALLIVE_NAS_MNID,
    CM_OMI_FIELD_DUALLIVE_NAS_SNAS,
    CM_OMI_FIELD_DUALLIVE_NAS_SNID,
    CM_OMI_FIELD_DUALLIVE_NAS_SYNC,
    CM_OMI_FIELD_DUALLIVE_NAS_WKLOAD_IF,
    CM_OMI_FIELD_DUALLIVE_NAS_WKLOAD_IP,
    CM_OMI_FIELD_DUALLIVE_NAS_NETMASK,
    CM_OMI_FIELD_DUALLIVE_NAS_STATUAS,
} cm_omi_filed_duallive_nas_e;

typedef enum
{
    CM_OMI_FIELD_BACKUP_NAS_MNAS,
    CM_OMI_FIELD_BACKUP_NAS_MNID,
    CM_OMI_FIELD_BACKUP_NAS_SNAS,
    CM_OMI_FIELD_BACKUP_NAS_SNID,
	CM_OMI_FIELD_BACKUP_NAS_SYNC,
    CM_OMI_FIELD_BACKUP_NAS_STATUAS,
} cm_omi_filed_backup_nas_e;

typedef enum
{
    CM_OMI_FIELD_LUNBACKUP_INSERT_ACTION = 0,
    CM_OMI_FIELD_LUNBACKUP_NID_MASTER,
    CM_OMI_FIELD_LUNBACKUP_UPDATE_ACTION,
    CM_OMI_FIELD_LUNBACKUP_ASYNC_TIME,
    CM_OMI_FIELD_LUNBACKUP_PATH,
    CM_OMI_FIELD_LUNBACKUP_IP_SLAVE,
    CM_OMI_FIELD_LUNBACKUP_NIC,
    CM_OMI_FIELD_LUNBACKUP_RDC_IP_MASTER,
    CM_OMI_FIELD_LUNBACKUP_RDC_IP_SLAVE,
    CM_OMI_FIELD_LUNBACKUP_PATH_SLAVE,
    CM_OMI_FIELD_LUNBACKUP_SYNC_ONCE,
    CM_OMI_FIELD_LUNBACKUP_NETMASK_MASTER,
    CM_OMI_FIELD_LUNBACKUP_NETMASK_SLAVE,
} cm_omi_field_lunbackup_e;

typedef enum
{
    CM_OMI_FIELD_LUNMIRROR_INSERT_ACTION = 0,
    CM_OMI_FIELD_LUNMIRROR_NID_MASTER,
    CM_OMI_FIELD_LUNMIRROR_PATH,
    CM_OMI_FIELD_LUNMIRROR_NID_SLAVE,
    CM_OMI_FIELD_LUNMIRROR_NIC,
    CM_OMI_FIELD_LUNMIRROR_RDC_IP_MASTER,
    CM_OMI_FIELD_LUNMIRROR_RDC_IP_SLAVE,
    CM_OMI_FIELD_LUNMIRROR_SYNC_ONCE,
    CM_OMI_FIELD_LUNMIRROR_NETMASK_MASTER,
    CM_OMI_FIELD_LUNMIRROR_NETMASK_SLAVE,
} cm_omi_field_lunmirror_e;

typedef enum
{
    CM_OMI_FIELD_DUALLIVE_NETIF_TARGET,
    CM_OMI_FIELD_DUALLIVE_NETIF_NID,
} cm_omi_filed_duallive_netif_e;

typedef enum
{
    CM_OMI_FIELD_MIGRATE_TYPE = 0,
    CM_OMI_FIELD_MIGRATE_NID,
    CM_OMI_FIELD_MIGRATE_OPERATION,
    CM_OMI_FIELD_MIGRATE_PATH,
    CM_OMI_FIELD_MIGRATE_POOL,
    CM_OMI_FIELD_MIGRATE_LUNID,
    CM_OMI_FIELD_MIGRATE_PROGRESS,
} cm_omi_field_migrate_e;

typedef enum
{
    CM_OMI_FIELD_POOL_RECONSTRUCT_NAME = 0,
    CM_OMI_FIELD_POOL_RECONSTRUCT_NID,
    CM_OMI_FIELD_POOL_RECONSTRUCT_STATUS,
    CM_OMI_FIELD_POOL_RECONSTRUCT_SPEED,
    CM_OMI_FIELD_POOL_RECONSTRUCT_TIME_COST,
    CM_OMI_FIELD_POOL_RECONSTRUCT_PROGRESS,
} cm_omi_filed_pool_reconstruct_e;

typedef enum
{
    CM_OMI_FIELD_NODE_SERVCE_SERVCE,
    CM_OMI_FIELD_NODE_SERVCE_NID,
    CM_OMI_FIELD_NODE_SERVCE_NFS,
    CM_OMI_FIELD_NODE_SERVCE_STMF,
    CM_OMI_FIELD_NODE_SERVCE_SSH,
    CM_OMI_FIELD_NODE_SERVCE_FTP,
    CM_OMI_FIELD_NODE_SERVCE_SMB,
    CM_OMI_FIELD_NODE_SERVCE_GUIVIEW,
    CM_OMI_FIELD_NODE_SERVCE_FMD,
    CM_OMI_FIELD_NODE_SERVCE_ISCSI,
} cm_omi_filed_node_service_e;

typedef enum
{
    CM_OMI_FIELD_AGGR_NAME = 0,
    CM_OMI_FIELD_AGGR_NID,
    CM_OMI_FIELD_AGGR_STATE,
    CM_OMI_FIELD_AGGR_IP,
    CM_OMI_FIELD_AGGR_NETMASK,
    CM_OMI_FIELD_AGGR_MTU,
    CM_OMI_FIELD_AGGR_MAC,
    CM_OMI_FIELD_AGGR_ADAPTER,
    CM_OMI_FIELD_AGGR_POLICY,
    CM_OMI_FIELD_AGGR_LACPACTIVITY,
    CM_OMI_FIELD_AGGR_LACPTIMER,
} cm_omi_field_aggr_e;

typedef enum
{
  CM_OMI_FIELD_DNSM_IP=0,
  CM_OMI_FIELD_DNSM_NID,
  CM_OMI_FIELD_DNSM_DOMAIN,
}cm_omi_filed_dnsm_e;

typedef enum
{
    CM_OMI_FIELD_PERMISSION_OBJ = 0,
    CM_OMI_FIELD_PERMISSION_CMD,
    CM_OMI_FIELD_PERMISSION_MASK,
}cm_omi_field_permission_e;
typedef enum
{
    CM_OMI_FIELD_PMM_POOL_ID=0,
    CM_OMI_FIELD_PMM_POOL_DATETIME,
    CM_OMI_FIELD_PMM_POOL_RIOPS,
    CM_OMI_FIELD_PMM_POOL_WIOPS,
    CM_OMI_FIELD_PMM_POOL_RBITYS,
    CM_OMI_FIELD_PMM_POOL_WBIRYS,
}cm_omi_field_pmm_pool_e;

typedef enum 
{
    CM_OMI_FIELD_PMM_NAS_ID,
    CM_OMI_FIELD_PMM_NAS_TIMESTAMP,
    CM_OMI_FIELD_PMM_NAS_NEWFILE,
    CM_OMI_FIELD_PMM_NAS_RMFILE,
    CM_OMI_FIELD_PMM_NAS_ERDDIR,
    CM_OMI_FIELD_PMM_NAS_IOPS,
    CM_OMI_FIELD_PMM_NAS_READ,
    CM_OMI_FIELD_PMM_NAS_WRITTEN,
    CM_OMI_FIELD_PMM_NAS_DELAY,
}cm_omi_field_pmm_nas_e;

typedef enum 
{
    CM_OMI_FIELD_PMM_SAS_ID,
    CM_OMI_FIELD_PMM_SAS_TIMESTAMP,
    CM_OMI_FIELD_PMM_SAS_READS,
    CM_OMI_FIELD_PMM_SAS_WRITTES,
    CM_OMI_FIELD_PMM_SAS_RIOPS,
    CM_OMI_FIELD_PMM_SAS_WIOPS,
}cm_omi_field_pmm_sas_e;

typedef enum 
{
    CM_OMI_FIELD_PMM_PROTO_ID,
    CM_OMI_FIELD_PMM_PROTO_TIMESTAMP,
    CM_OMI_FIELD_PMM_PROTO_IOPS,
}cm_omi_field_pmm_protocol_e;

typedef enum 
{
    CM_OMI_FIELD_PMM_CACHE_ID,
    CM_OMI_FIELD_PMM_CACHE_TIMESTAMP,
    CM_OMI_FIELD_PMM_CACHE_L1_HIT,
    CM_OMI_FIELD_PMM_CACHE_L1_MISS,
    CM_OMI_FIELD_PMM_CACHE_L1_SIZE,
    CM_OMI_FIELD_PMM_CACHE_L2_HIT,
    CM_OMI_FIELD_PMM_CACHE_L2_MISS,
    CM_OMI_FIELD_PMM_CACHE_L2_SIZE,
}cm_omi_field_pmm_cache_e;

typedef enum
{
    CM_OMI_FIELD_PMM_NAME,
    CM_OMI_FIELD_PMM_NID,
}cm_omi_field_cnm_pmm_e;

typedef enum
{
  CM_OMI_FIELD_PMM_LUN_ID=0,
  CM_OMI_FIELD_PMM_LUN_TIME,
  CM_OMI_FIELD_PMM_LUN_READS,
  CM_OMI_FIELD_PMM_LUN_WRITES,
  CM_OMI_FIELD_PMM_LUN_NREAD,
  CM_OMI_FIELD_PMM_LUN_NREAD_KB,
  CM_OMI_FIELD_PMM_LUN_NWRITTEN,
  CM_OMI_FIELD_PMM_LUN_NWRITTEN_KB,
}cm_omi_filed_pmm_lun_e;

typedef enum
{
  CM_OMI_FIELD_PMM_NIC_ID=0,
  CM_OMI_FIELD_PMM_NIC_TIME,
  CM_OMI_FIELD_PMM_NIC_COLLISIONS,
  CM_OMI_FIELD_PMM_NIC_IERRORS,
  CM_OMI_FIELD_PMM_NIC_OERRORS,
  CM_OMI_FIELD_PMM_NIC_RBYTES,
  CM_OMI_FIELD_PMM_NIC_OBYTES,
  CM_OMI_FIELD_PMM_NIC_UTIL,
  CM_OMI_FIELD_PMM_NIC_SAT,
}cm_omi_filed_pmm_nic_e;

typedef enum
{
  CM_OMI_FIELD_PMM_DISK_ID=0,
  CM_OMI_FIELD_PMM_DISK_TIME,
  CM_OMI_FIELD_PMM_DISK_READS,
  CM_OMI_FIELD_PMM_DISK_WRITES,
  CM_OMI_FIELD_PMM_DISK_NREAD,
  CM_OMI_FIELD_PMM_DISK_NREAD_KB,
  CM_OMI_FIELD_PMM_DISK_NWRITTEN,
  CM_OMI_FIELD_PMM_DISK_NWRITEN_KB,
  CM_OMI_FIELD_PMM_DISK_RLENTIME,
  CM_OMI_FIELD_PMM_DISK_WLENTIME,
  CM_OMI_FIELD_PMM_DISK_WTIME,
  CM_OMI_FIELD_PMM_DISK_RTIME,
  CM_OMI_FIELD_PMM_DISK_SNAPTIME,
}cm_omi_filed_pmm_disk_e;

typedef enum
{
    CM_OMI_FIELD_NAS_SHADOW_STATUS=0,
    CM_OMI_FIELD_NAS_SHADOW_NID,
    CM_OMI_FIELD_NAS_SHADOW_STATE,
    CM_OMI_FIELD_NAS_SHADOW_MIRROR,
    CM_OMI_FIELD_NAS_SHADOW_DST,
    CM_OMI_FIELD_NAS_SHADOW_SRC,
    CM_OMI_FIELD_NAS_SHADOW_MOUNT,
}cm_omi_field_cnm_nas_shadow_e;

typedef enum
{
  CM_OMI_FIELD_FCINFO_PORT_WWN = 0,
  CM_OMI_FIELD_FCINFO_NID,
  CM_OMI_FIELD_FCINFO_PORT_MODE,
  CM_OMI_FIELD_FCINFO_DRIVER_NAME,
  CM_OMI_FIELD_FCINFO_STATE,
  CM_OMI_FIELD_FCINFO_SPEED,
  CM_OMI_FIELD_FCINFO_CURSPEED,
}cm_omi_filed_fcinfo_e;

typedef enum
{
    CM_OMI_FIELD_EXPLORER_NAME=0,
    CM_OMI_FIELD_EXPLORER_NID,
    CM_OMI_FIELD_EXPLORER_DIR,
    CM_OMI_FIELD_EXPLORER_PERMISSION,
    CM_OMI_FIELD_EXPLORER_TYPE,
    CM_OMI_FIELD_EXPLORER_SIZE,
    CM_OMI_FIELD_EXPLORER_USER,
    CM_OMI_FIELD_EXPLORER_GROUP,
    CM_OMI_FIELD_EXPLORER_ATIME,
    CM_OMI_FIELD_EXPLORER_MTIME,
    CM_OMI_FIELD_EXPLORER_CTIME,
    CM_OMI_FIELD_EXPLORER_FIND,
    CM_OMI_FIELD_EXPLORER_FLAG,
}cm_omi_field_cnm_nas_explorer_e;

typedef enum
{
  CM_OMI_FIELD_IPF_OPERATE = 0,
  CM_OMI_FIELD_IPF_NID,
  CM_OMI_FIELD_IPF_NIC,
  CM_OMI_FIELD_IPF_IP,
  CM_OMI_FIELD_IPF_PORT,
  CM_OMI_FIELD_IPF_STATE,
}cm_omi_filed_ipf_e;

typedef enum
{
    CM_OMI_FIRLD_DIRQUOTA_DIR = 0,
    CM_OMI_FIRLD_DIRQUOTA_NID,
    CM_OMI_FIRLD_DIRQUOTA_QUOTA,
    CM_OMI_FIRLD_DIRQUOTA_USED,
    CM_OMI_FIRLD_DIRQUOTA_REST,
    CM_OMI_FIRLD_DIRQUOTA_PER_USED,
    CM_OMI_FIRLD_DIRQUOTA_NAS,
}cm_omi_field_cnm_dirquota_e;

typedef enum
{
    CM_OMI_FIRLD_TOPO_POWER_ENID,
    CM_OMI_FIRLD_TOPO_POWER,
    CM_OMI_FIRLD_TOPO_POWER_VOLT,
    CM_OMI_FIRLD_TOPO_POWER_STATUS,
}cm_omi_field_cnm_topo_power_e;

typedef enum
{
    CM_OMI_FIRLD_TOPO_FAN_ENID,
    CM_OMI_FIRLD_TOPO_FAN_NID,
    CM_OMI_FIRLD_TOPO_FAN,
    CM_OMI_FIRLD_TOPO_FAN_ROTATE,
    CM_OMI_FIRLD_TOPO_FAN_STATUS,
}cm_omi_field_cnm_topo_rotate_e;

typedef enum
{
    CM_OMI_FIELD_TOPO_TABLE_NAME=0,
    CM_OMI_FIELD_TOPO_TABLE_SET,
    CM_OMI_FIELD_TOPO_TABLE_ENID,
    CM_OMI_FIELD_TOPO_TABLE_TYPE,
    CM_OMI_FIELD_TOPO_TABLE_UNUM,
    CM_OMI_FIELD_TOPO_TABLE_SLOT,
    CM_OMI_FIELD_TOPO_TABLE_SN,
}cm_omi_field_cnm_topo_table_e;

typedef enum
{
    CM_OMI_FIELD_NTP_IP=0,
}cm_omi_field_cnm_ntp_e;

typedef enum
{
    CM_OMI_FIELD_THRESHOLD_ALARM_ID=0,
    CM_OMI_FIELD_THRESHOLD_RECOVERVAL,
    CM_OMI_FIELD_THRESHOLD_VALUE,
}cm_omi_field_cnm_threshold_e;

typedef enum
{
    CM_OMI_FIELD_LOWDATA_NAME=0,
    CM_OMI_FIELD_LOWDATA_NID,
    CM_OMI_FIELD_LOWDATA_STATUS,
    CM_OMI_FIELD_LOWDATA_UNIT,
    CM_OMI_FIELD_LOWDATA_CRI,
    CM_OMI_FIELD_LOWDATA_TIMEOUT,
    CM_OMI_FIELD_LOWDATA_DELETE,
    CM_OMI_FIELD_LOWDATA_SWITCH,
    CM_OMI_FIELD_LOWDATA_PROCESS,
}cm_omi_field_cnm_lowdata_e;

typedef enum
{
    CM_OMI_FIELD_DIRLOWDATA_NAME=0,
    CM_OMI_FIELD_DIRLOWDATA_NID,
    CM_OMI_FIELD_DIRLOWDATA_DIR,
    CM_OMI_FIELD_DIRLOWDATA_STATUS,
    CM_OMI_FIELD_DIRLOWDATA_UNIT,
    CM_OMI_FIELD_DIRLOWDATA_CRI,
    CM_OMI_FIELD_DIRLOWDATA_TIMEOUT,
    CM_OMI_FIELD_DIRLOWDATA_DELETE,
    CM_OMI_FIELD_DIRLOWDATA_SWITCH,
    CM_OMI_FIELD_DIRLOWDATA_CLUSTER,
}cm_omi_field_cnm_dirlowdata_e;

typedef enum
{
    CM_OMI_FIELD_DOMAIN_USER=0,
    CM_OMI_FIELD_DOMAIN_LOCAL_USER,
}cm_omi_field_cnm_domain_user_e;

typedef enum
{
    /* discard | noallow | restricted | passthrough | passthrough-x */
    CM_OMI_ACL_DISCARD = 0,
    CM_OMI_ACL_NOALLOW,
    CM_OMI_ACL_RESTRICTED,
    CM_OMI_ACL_PASSTHROUGH,
    CM_OMI_ACL_PASSTHROUGH_X,
}cm_omi_aclinherit_e;

typedef enum
{
    CM_OMI_FIELD_DNS_IPMASTER=0,
    CM_OMI_FIELD_DNS_IPSLAVE,
}cm_omi_field_cnm_dns_e;

typedef enum
{
    CM_OMI_FIELD_DOMAINAD_DOMIAN=0,
    CM_OMI_FIELD_DOMAINAD_DOMAINCTL,
    CM_OMI_FIELD_DOMAINAD_USERNAME,
    CM_OMI_FIELD_DOMAINAD_USERPWD,
    CM_OMI_FIELD_DOMAINAD_STATE,
}cm_omi_field_cnm_domain_ad_e;

typedef enum
{
    CM_OMI_FIELD_LOWDATA_SCHE_ID = 0,
    CM_OMI_FIELD_LOWDATA_SCHE_NID, /* 预留 */
    CM_OMI_FIELD_LOWDATA_SCHE_NAME,
    CM_OMI_FIELD_LOWDATA_SCHE_EXPIRED,
    CM_OMI_FIELD_LOWDATA_SCHE_TYPE,
    CM_OMI_FIELD_LOWDATA_SCHE_DAYTYPE,
    CM_OMI_FIELD_LOWDATA_SCHE_MINUTE,
    CM_OMI_FIELD_LOWDATA_SCHE_HOURS,
    CM_OMI_FIELD_LOWDATA_SCHE_DAYS,
} cm_omi_field_lowdata_sche_e;

typedef enum
{
    CM_OMI_FIELD_NAS_CLIENT_IP = 0,
    CM_OMI_FIELD_NAS_CLIENT_NID, 
    CM_OMI_FIELD_NAS_CLIENT_PROTO,
    
}cm_omi_field_nas_client_e;

typedef enum
{
    CM_OMI_FIELD_POOLEXT_NAME = 0,
    CM_OMI_FIELD_POOLEXT_NID, 
    CM_OMI_FIELD_POOLEXT_FORCE,
    CM_OMI_FIELD_POOLEXT_POLICY,

    CM_OMI_FIELD_POOLEXT_DATA_RAID=4,
    CM_OMI_FIELD_POOLEXT_DATA_NUM,
    CM_OMI_FIELD_POOLEXT_DATA_GROUP,
    CM_OMI_FIELD_POOLEXT_DATA_SPARE,

    CM_OMI_FIELD_POOLEXT_META_RAID=8,
    CM_OMI_FIELD_POOLEXT_META_NUM,
    CM_OMI_FIELD_POOLEXT_META_GROUP,
    CM_OMI_FIELD_POOLEXT_META_SPARE,

    CM_OMI_FIELD_POOLEXT_LOW_RAID=12,
    CM_OMI_FIELD_POOLEXT_LOW_NUM,
    CM_OMI_FIELD_POOLEXT_LOW_GROUP,
    CM_OMI_FIELD_POOLEXT_LOW_SPARE,
    
}cm_omi_field_poolext_e;

typedef enum
{
    CM_OMI_FIELD_UPGRADE_NAME = 0,    
    CM_OMI_FIELD_UPGRADE_NID,
    CM_OMI_FIELD_UPGRADE_STATE,
    CM_OMI_FIELD_UPGRADE_PROCESS,
    CM_OMI_FIELD_UPGRADE_RDDIR,
    CM_OMI_FIELD_UPGRADE_MODE,
    CM_OMI_FIELD_UPGRADE_VERSION,
}cm_omi_field_upgrade_e;


typedef enum
{
    CM_OMI_FIELD_LOWDATA_VOLUME_PERCENT,
    CM_OMI_FIELD_LOWDATA_VOLUME_STOP,
}cm_omi_field_lowdata_volume_e;

typedef enum
{
    CM_OMI_FIELD_MAILSEND_STATE = 0,     
    CM_OMI_FIELD_MAILSEND_SENDER,
    CM_OMI_FIELD_MAILSEND_SERVER, 
    CM_OMI_FIELD_MAILSEND_PORT,
    CM_OMI_FIELD_MAILSEND_LANGUAGE,
    CM_OMI_FIELD_MAILSEND_USERON,  
    CM_OMI_FIELD_MAILSEND_USER,
    CM_OMI_FIELD_MAILSEND_PASSWD, 
}cm_omi_mailsend_e;

typedef enum
{
    CM_OMI_FIELD_MAILRECV_ID = 0,
    CM_OMI_FIELD_MAILRECV_RECEIVER,
    CM_OMI_FIELD_MAILRECV_LEVEL,
}cm_omi_mailrecv_e;

typedef enum
{
    CM_OMI_FIELD_NASCOPY_NAS = 0,
    CM_OMI_FIELD_NASCOPY_NID,
    CM_OMI_FIELD_NASCOPY_NUM,  
}cm_omi_field_cnm_nascopy_e;

typedef enum
{
    CM_OMI_FIELD_UCACHE_ID = 0,
    CM_OMI_FIELD_UCACHE_NID,
    CM_OMI_FIELD_UCACHE_NAME,
    CM_OMI_FIELD_UCACHE_TYPE,
    CM_OMI_FIELD_UCACHE_DOMAIN,
}cm_omi_field_cnm_usercache_e;

typedef enum
{
    CM_OMI_FIELD_REMOTECLUSTER_IP = 0,
    CM_OMI_FIELD_REMOTECLUSTER_NID,
    CM_OMI_FIELD_REMOTECLUSTER_NAME,
}cm_omi_field_remote_cluster_e;

typedef enum
{
    CM_OMI_FIELD_LOCALTASK_TID = 0,
    CM_OMI_FIELD_LOCALTASK_NID,
    CM_OMI_FIELD_LOCALTASK_PROG,
    CM_OMI_FIELD_LOCALTASK_STATUS,
    CM_OMI_FIELD_LOCALTASK_START,
    CM_OMI_FIELD_LOCALTASK_END,
    CM_OMI_FIELD_LOCALTASK_DESC,
}cm_omi_field_localtask_e;


extern const uint32 *g_CmOmiObjCmdNoCheckPtr;
extern const cm_omi_map_cfg_t CmOmiMapCmds[CM_OMI_CMD_BUTT];

extern const cm_omi_map_object_field_t CmOmiMapCommFields[CM_OMI_FIELD_BUTT];

extern const cm_omi_map_object_t* CmOmiMap[CM_OMI_OBJECT_BUTT];

extern const cm_omi_map_enum_t CmOmiMapEnumBoolType;
extern const cm_omi_map_enum_t CmOmiMapEnumSwitchType;

#endif /* BASE_CONFIG_CM_CFG_OMI_H_ */
