/******************************************************************************
 *          Copyright (c) 2017 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_types.h
 * author     : wbn
 * create date: 2017年10月23日
 * description: TODO:
 *
 *****************************************************************************/
#ifndef BASE_INCLUDE_CM_TYPES_H_
#define BASE_INCLUDE_CM_TYPES_H_
#include "cm_platform.h"
#include "cm_cfg_public.h"
/* module define */
typedef enum
{
    CM_MOD_NONE=0,
    CM_MOD_LOG,
    CM_MOD_QUEUE,
    CM_MOD_DB,
    CM_MOD_RPC,
    CM_MOD_CMT,
    CM_MOD_OMI,
    CM_MOD_NODE,
    CM_MOD_HTBT,
    CM_MOD_CNM,
    CM_MOD_SYNC,
    CM_MOD_PMM,
    CM_MOD_CMD,
    CM_MOD_ALARM,
    CM_MOD_USER,
    CM_MOD_SCHE,
    CM_MOD_TASK,
    CM_MOD_BUTT
}cm_moudule_e;

typedef enum
{
    CM_NODE_STATE_UNKNOW = 0,
    CM_NODE_STATE_START,
    CM_NODE_STATE_NORMAL,
    CM_NODE_STATE_DEGRAGE,
    CM_NODE_STATE_SHUTDOWN,
    CM_NODE_STATE_UPGRADE,
    CM_NODE_STATE_OFFLINE,
}cm_node_state_e;

typedef enum
{
    CM_NODE_DEV_SBB,
    CM_NODE_DEV_AIC,
    CM_NODE_DEV_ENC,
    CM_NODE_DEV_UNKNOWN,
    CM_NODE_DEV_BUTT
}cm_node_dev_e;

typedef enum
{
    CM_PORT_STATE_UNKNOW = 0,
    CM_PORT_STATE_UP,
    CM_PORT_STATE_DOWN,
}cm_port_state_e;

typedef enum
{
    CM_PORT_DUPLEX_UNKNOW = 0,
    CM_PORT_DUPLEX_FULL,
    CM_PORT_DUPLEX_HALF
}cm_port_duplex_e;

typedef struct cm_cluster_info_tt
{
    sint8 name[CM_NAME_LEN];
    sint8 product[CM_NAME_LEN];
    sint8 version[CM_VERSION_LEN];
    sint8 ipaddr[CM_IP_LEN];
    sint8 gateway[CM_IP_LEN];
    sint8 netmask[CM_IP_LEN];
    cm_time_t timestamp;
    sint8 interface[CM_NAME_LEN];
    sint8 priority[CM_NAME_LEN];
    uint32 enipmi;
    uint32 failover;
}cm_cluster_info_t;

typedef struct cm_node_info_tt
{
    uint32 id;
    uint32 subdomain_id;
    sint8 name[CM_NAME_LEN];
    sint8 sn[CM_SN_LEN];
    sint8 version[CM_VERSION_LEN];
    sint8 ip_addr[CM_IP_LEN];
    sint8 frame[CM_NAME_LEN];
    uint32 dev_type; /* cm_node_dev_e */
    uint32 slot_num;
    uint32 ram_size; /* MB */
    uint32 state; /* cm_node_state_e */
    uint32 inter_id;
    /* 针对双控设计，一对节点SBBID一样，添加时自动分配 */
    uint32 sbbid;
    /* SBB中的主备 */
    bool_t ismaster; 
}cm_node_info_t;

typedef struct
{
    uint32 id;
    uint32 submaster_id;
    uint32 node_cnt;
}cm_subdomain_info_t;

typedef sint32 (*cm_main_init_func_t)(void);

typedef void* (*cm_thread_func_t)(void* arg);

typedef uint32 (*cm_get_node_func_t)(void);


typedef enum
{
    CM_RAID0 = 0,
    CM_RAID1 = 1,
    CM_RAID5 = 5,
    CM_RAID6 = 6,
    CM_RAID7 = 7,
    CM_RAID10 = 10,
    CM_RAID50 = 50,
    CM_RAID60 = 60,
    CM_RAID70 = 70,
    CM_RAID_BUTT,
}cm_raid_type_e;

typedef enum 
{
    CM_POOL_IMPORT,
    CM_POOL_EXPORT,
    CM_POOL_SCRUB,
    CM_POOL_CLEAR,
    CM_POOL_SWITCH,
    CM_POOL_DESTROY,
}cm_operation_type_e;

enum
{
    CM_ZSTATUS_CANCELED,
    CM_ZSTATUS_SCRUBING,
    CM_ZSTATUS_SCRUBED,        
}cm_zstatus_type_e;


typedef enum
{
    CM_POOL_DISK_DATA = 0,
    CM_POOL_DISK_META,
    CM_POOL_DISK_SPARE,
    CM_POOL_DISK_METASPARE,
    CM_POOL_DISK_LOG,
    CM_POOL_DISK_LOGDATA,
    CM_POOL_DISK_LOGDATASPARE,
    CM_POOL_DISK_CACHE,
    CM_POOL_DISK_LOW,
    CM_POOL_DISK_LOWSPARE,
    CM_POOL_DISK_MIRRORSPARE,
    CM_POOL_DISK_BUTT
}cm_pool_disk_type_e;

typedef enum
{
    CM_DOMAIN_EVERYONE = 0,
    CM_DOMAIN_LOCAL,  
    CM_DOMAIN_AD,
    CM_DOMAIN_LDAP,
    CM_DOMAIN_NIS,  
    CM_DOMAIN_BUTT
}cm_domain_type_e;

typedef enum
{
    /* 完全控制 */
    CM_NAS_PERISSION_RW = 0,

    /* 只读 */
    CM_NAS_PERISSION_RO,

    /* 除 write_acl 和 write_owner 外的所有权限*/
    CM_NAS_PERISSION_RD,
    
    CM_NAS_PERISSION_BUTT
}cm_nas_permission_e;

typedef enum
{
    CM_NAME_USER = 0,
    CM_NAME_GROUP,
    CM_NAME_BUTT
}cm_name_type_e;

/*
 * task state
 *
 */
typedef enum
{
	CM_TASK_STATE_READY = 0,
	CM_TASK_STATE_RUNNING,	/* 1 */
	CM_TASK_STATE_FINISHED,	/* 2 */
	CM_TASK_STATE_WRONG,	/* 3 */
	CM_TASK_STATE_BUTT
} cm_task_state_e;

typedef enum
{
    CM_MIGRATE_DISK=0,
    CM_MIGRATE_LUN,
}cm_cnm_migrate_name_e;

typedef enum
{
    CM_OPERATION_STOP=0,
    CM_OPERATION_START,
}cm_cnm_migrate_operation_e;


/* 通用错误码定义 */
#define CM_OK 0
#define CM_FAIL 1
#define CM_PARAM_ERR   2
#define CM_ERR_TIMEOUT 3
#define CM_ERR_NOT_SUPPORT 4
#define CM_ERR_BUSY 5
#define CM_ERR_MSG_TOO_LONG 6
#define CM_ERR_NO_MASTER 7
#define CM_ERR_REDIRECT_MASTER 8
#define CM_ERR_CONN_NONE 9
#define CM_NEED_LOGIN 10

#define CM_ERR_PWD 11

#define CM_ERR_NOT_EXISTS 12
#define CM_ERR_ALREADY_EXISTS 13
#define CM_ERR_CONN_FAIL 14
#define CM_ERR_NO_PERMISSION 15
#define CM_ERR_UNKNOWN 16
#define CM_ERR_LOGIN_LOCK 17


#endif /* BASE_INCLUDE_CM_TYPES_H_ */
