/******************************************************************************
 *          Copyright (c) 2017 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cnm_cluster.c
 * author     : wbn
 * create date: 2017年11月13日
 * description: TODO:
 *
 *****************************************************************************/
#include <string.h>
#include "cm_cnm_cluster.h"
#include "cm_db.h"
#include "cm_ini.h"
#include "cm_log.h"
#include "cm_omi.h"
#include "cm_cnm_nas.h"
#include "cm_cnm_pool.h"
#include "cm_cnm_san.h"
#include "cm_common.h"
#include "cm_cnm_common.h"
#include "cm_sync.h"
#include "cm_node.h"

#define CM_CNM_CLUSTER_CHECK_BIND_IP_INTR 5
#define CM_CNM_CLUSTER_CFG_SECTION "CLUSTER"
#define CM_CNM_CLUSTER_CFG_NAME "name"
#define CM_CNM_CLUSTER_CFG_VER "version"
#define CM_CNM_CLUSTER_CFG_IPADDR "ip"
#define CM_CNM_CLUSTER_CFG_GATEWAY "gateway"
#define CM_CNM_CLUSTER_CFG_NETMASK "netmask"
#define CM_CNM_CLUSTER_CFG_TIME "time"
#define CM_CNM_CLUSTER_CFG_INTERFACE "nic"
#define CM_CNM_CLUSTER_CFG_PRIORITY "priority"
#define CM_CNM_CLUSTER_CFG_IPMI "ipmi"
#define CM_CNM_CLUSTER_CFG_FAILOVER "failover"
#define CM_CNM_CLUSTER_CFG_PRODUCT "product"


extern const cm_omi_map_enum_t CmOmiMapEnumSwitchType;
extern const cm_omi_map_enum_t CmOmiMapEnumFailOverType;
static cm_cluster_info_t g_cm_cluster_info;
static cm_mutex_t g_cm_cnm_cluster_lock;

extern const sint8* cm_node_getversion(void);

sint32 cm_cnm_cluster_init(void)
{
    sint32 iRet;
    uint32 len=0;
    cm_ini_handle_t handle;
    cm_cluster_info_t *ptempptr = NULL;
    handle = cm_ini_open(CM_CLUSTER_INI);

    if(NULL == handle)
    {
        iRet = cm_system("touch "CM_CLUSTER_INI);

        if(CM_OK != iRet)
        {
            CM_LOG_ERR(CM_MOD_CNM, "touch cluster_ini fail[%d]", iRet);
            return CM_FAIL;
        }
    }
    else
    {
        cm_ini_free(handle);
    }
    CM_MEM_ZERO(&g_cm_cluster_info,sizeof(cm_cluster_info_t));
    
    cm_cnm_cluster_get(NULL,(void**)&ptempptr,&len);
    if(NULL != ptempptr)
    {
        CM_MEM_CPY(&g_cm_cluster_info,sizeof(cm_cluster_info_t),ptempptr,sizeof(cm_cluster_info_t));
    }
    return CM_OK;
}

sint32 cm_cnm_cluster_decode
(const cm_omi_obj_t ObjParam, void **ppDecodeParam)
{
    //sint32 iRet = CM_FAIL;
    cm_cluster_info_t *pinfo = CM_MALLOC(sizeof(cm_cluster_info_t));

    if(NULL == pinfo)
    {
        CM_LOG_ERR(CM_MOD_CNM, "malloc fail");
        *ppDecodeParam = NULL;
        return CM_FAIL;
    }

    CM_MEM_ZERO(pinfo, sizeof(cm_cluster_info_t));

    (void)cm_omi_obj_key_get_str_ex(ObjParam, CM_OMI_FIELD_CLUSTER_NAME,
                                    pinfo->name, CM_NAME_LEN);
    (void)cm_omi_obj_key_get_str_ex(ObjParam, CM_OMI_FIELD_CLUSTER_IPADDR,
                                    pinfo->ipaddr, CM_IP_LEN);
    (void)cm_omi_obj_key_get_str_ex(ObjParam, CM_OMI_FIELD_CLUSTER_GATEWAY,
                                    pinfo->gateway, CM_IP_LEN);
    (void)cm_omi_obj_key_get_str_ex(ObjParam, CM_OMI_FIELD_CLUSTER_NETMASK,
                                    pinfo->netmask, CM_IP_LEN);
    (void)cm_omi_obj_key_get_str_ex(ObjParam, CM_OMI_FIELD_CLUSTER_INTERFACE,
                                    pinfo->interface, sizeof(pinfo->interface));
    (void)cm_omi_obj_key_get_str_ex(ObjParam, CM_OMI_FIELD_CLUSTER_PRIORITY,
                                    pinfo->priority, sizeof(pinfo->priority));
    (void)cm_omi_obj_key_get_s32_ex(ObjParam, CM_OMI_FIELD_CLUSTER_ENABLE_IPMI,
                                    &pinfo->enipmi);
    (void)cm_omi_obj_key_get_s32_ex(ObjParam, CM_OMI_FIELD_CLUSTER_FAILOVER,
                                    &pinfo->failover);
    (void)cm_omi_obj_key_get_str_ex(ObjParam, CM_OMI_FIELD_CLUSTER_PRODUCT,
                                    pinfo->product, CM_NAME_LEN);
    *ppDecodeParam = pinfo;
    return CM_OK;
}

static void cm_cnm_cluster_encode_each
(cm_omi_obj_t item, void *eachdata, void *arg)
{
    cm_cluster_info_t *pinfo = eachdata;

    if(NULL == pinfo)
    {
        CM_LOG_ERR(CM_MOD_CNM, "data is null");
        return;
    }

    (void)cm_omi_obj_key_set_str_ex(item, CM_OMI_FIELD_CLUSTER_NAME, pinfo->name);
    (void)cm_omi_obj_key_set_str_ex(item, CM_OMI_FIELD_CLUSTER_VERSION, cm_node_getversion());
    (void)cm_omi_obj_key_set_str_ex(item, CM_OMI_FIELD_CLUSTER_IPADDR, pinfo->ipaddr);
    (void)cm_omi_obj_key_set_str_ex(item, CM_OMI_FIELD_CLUSTER_GATEWAY, pinfo->gateway);
    (void)cm_omi_obj_key_set_str_ex(item, CM_OMI_FIELD_CLUSTER_NETMASK, pinfo->netmask);
    (void)cm_omi_obj_key_set_u64_ex(item, CM_OMI_FIELD_CLUSTER_TIMESTAMP, pinfo->timestamp);
    (void)cm_omi_obj_key_set_str_ex(item, CM_OMI_FIELD_CLUSTER_PRODUCT, pinfo->product);
}


cm_omi_obj_t cm_cnm_cluster_encode
(const void *pDecodeParam, void *pAckData, uint32 AckLen)
{
    return cm_cnm_encode_comm(pAckData, AckLen, sizeof(cm_cluster_info_t),
                              cm_cnm_cluster_encode_each, NULL);
}

static sint32 cm_cnm_cluster_get_inter(cm_cluster_info_t *pdata)
{
    sint32 iRet = CM_OK;
    sint8 strtime[CM_STRING_32] = {0};

    iRet = cm_ini_get_ext(CM_CLUSTER_INI, CM_CNM_CLUSTER_CFG_SECTION,
                                 CM_CNM_CLUSTER_CFG_TIME, strtime, sizeof(strtime));

    if(CM_OK != iRet)
    {
        CM_LOG_INFO(CM_MOD_CNM, "not config cluster yet");
        return CM_ERR_NOT_EXISTS;
    }
    pdata->timestamp = cm_mktime(strtime);
    
    (void)cm_ini_get_ext(CM_CLUSTER_INI, CM_CNM_CLUSTER_CFG_SECTION,
                         CM_CNM_CLUSTER_CFG_NAME, pdata->name, sizeof(pdata->name));
    (void)cm_ini_get_ext(CM_CLUSTER_INI, CM_CNM_CLUSTER_CFG_SECTION,
                         CM_CNM_CLUSTER_CFG_VER, pdata->version, sizeof(pdata->version));
    (void)cm_ini_get_ext(CM_CLUSTER_INI, CM_CNM_CLUSTER_CFG_SECTION,
                         CM_CNM_CLUSTER_CFG_IPADDR, pdata->ipaddr, sizeof(pdata->ipaddr));
    (void)cm_ini_get_ext(CM_CLUSTER_INI, CM_CNM_CLUSTER_CFG_SECTION,
                         CM_CNM_CLUSTER_CFG_GATEWAY, pdata->gateway, sizeof(pdata->gateway));
    (void)cm_ini_get_ext(CM_CLUSTER_INI, CM_CNM_CLUSTER_CFG_SECTION,
                         CM_CNM_CLUSTER_CFG_NETMASK, pdata->netmask, sizeof(pdata->netmask));
    (void)cm_ini_get_ext(CM_CLUSTER_INI, CM_CNM_CLUSTER_CFG_SECTION,
                         CM_CNM_CLUSTER_CFG_PRODUCT, pdata->product, sizeof(pdata->product));
    return CM_OK;                     
}

//没有设置时返回CM_ERR_NOT_EXISTS
sint32 cm_cnm_cluster_get(const void *pDecodeParam, void **ppAckData, uint32 *pAckLen)
{    
    sint32 iRet = CM_OK;
    cm_cluster_info_t *pdata = NULL;    
    pdata = CM_MALLOC(sizeof(cm_cluster_info_t));

    if(NULL == pdata)
    {
        CM_LOG_ERR(CM_MOD_CNM, "malloc fail");
        return CM_FAIL;
    }

    CM_MEM_ZERO(pdata, sizeof(pdata));

    iRet = cm_cnm_cluster_get_inter(pdata);
    if(CM_OK != iRet)
    {
        CM_FREE(pdata);
        return iRet;
    }
    *ppAckData = pdata;
    *pAckLen = sizeof(cm_cluster_info_t);
    return CM_OK;
}

static void cm_cnm_cluster_modify_save(cm_cluster_info_t *pinfo)
{
    const sint8 *enipmi = NULL;
    const sint8 *failover = NULL;
    sint8 strtime[CM_STRING_32] = {0};

    cm_utc_to_str(pinfo->timestamp,strtime,sizeof(strtime));
    enipmi = cm_cnm_get_enum_str(&CmOmiMapEnumSwitchType, pinfo->enipmi);
    failover = cm_cnm_get_enum_str(&CmOmiMapEnumFailOverType, pinfo->failover);

    (void)cm_ini_set_ext(CM_CLUSTER_INI, CM_CNM_CLUSTER_CFG_SECTION,
                         CM_CNM_CLUSTER_CFG_NAME, pinfo->name);
    (void)cm_ini_set_ext(CM_CLUSTER_INI, CM_CNM_CLUSTER_CFG_SECTION,
                         CM_CNM_CLUSTER_CFG_VER, cm_node_getversion());
    (void)cm_ini_set_ext(CM_CLUSTER_INI, CM_CNM_CLUSTER_CFG_SECTION,
                         CM_CNM_CLUSTER_CFG_IPADDR, pinfo->ipaddr);
    (void)cm_ini_set_ext(CM_CLUSTER_INI, CM_CNM_CLUSTER_CFG_SECTION,
                         CM_CNM_CLUSTER_CFG_GATEWAY, pinfo->gateway);
    (void)cm_ini_set_ext(CM_CLUSTER_INI, CM_CNM_CLUSTER_CFG_SECTION,
                         CM_CNM_CLUSTER_CFG_NETMASK, pinfo->netmask);
    (void)cm_ini_set_ext(CM_CLUSTER_INI, CM_CNM_CLUSTER_CFG_SECTION,
                         CM_CNM_CLUSTER_CFG_TIME, strtime);
    (void)cm_ini_set_ext(CM_CLUSTER_INI, CM_CNM_CLUSTER_CFG_SECTION,
                         CM_CNM_CLUSTER_CFG_INTERFACE, pinfo->interface);
    (void)cm_ini_set_ext(CM_CLUSTER_INI, CM_CNM_CLUSTER_CFG_SECTION,
                         CM_CNM_CLUSTER_CFG_PRIORITY, pinfo->priority);
    (void)cm_ini_set_ext(CM_CLUSTER_INI, CM_CNM_CLUSTER_CFG_SECTION,
                         CM_CNM_CLUSTER_CFG_IPMI, enipmi);
    (void)cm_ini_set_ext(CM_CLUSTER_INI, CM_CNM_CLUSTER_CFG_SECTION,
                         CM_CNM_CLUSTER_CFG_FAILOVER, failover);
    (void)cm_ini_set_ext(CM_CLUSTER_INI, CM_CNM_CLUSTER_CFG_SECTION,
                         CM_CNM_CLUSTER_CFG_PRODUCT, pinfo->product);
    CM_MEM_CPY(&g_cm_cluster_info,sizeof(cm_cluster_info_t),pinfo,sizeof(cm_cluster_info_t));
}

sint32 cm_cnm_cluster_modify(const void *pDecodeParam, void **ppAckData, uint32 *pAckLen)
{
    sint32 iRet;
    cm_cluster_info_t *pinfo = pDecodeParam;
    sint8 strtime[CM_STRING_32] = {0};
    
    iRet = cm_ini_get_ext(CM_CLUSTER_INI, CM_CNM_CLUSTER_CFG_SECTION,
                          CM_CNM_CLUSTER_CFG_TIME, strtime,sizeof(strtime));

    if(CM_OK == iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM, "already config");
        return CM_FAIL;
    }

    iRet = cm_cnm_exec_ping(pinfo->ipaddr);

    if(CM_OK == iRet)
    {
        // 能ping通，这个IP不适合
        CM_LOG_ERR(CM_MOD_CNM, "can link");
        return CM_FAIL;
    }

    // 设置config time
    pinfo->timestamp = cm_get_time();

    /* 暂时不加锁了，非频繁的操作类 */
    cm_cnm_cluster_modify_save(pinfo);
    return CM_OK;
}

//集群下电和重启整合到一块
//CM_OMI_CMD_OFF:集群下电
//CM_OMI_CMD_REBOOT:集群重启
static sint32 cm_cnm_cluster_trav_for_power(cm_node_info_t *pNode, void *pArg)
{
    sint32 iRet;
    uint32 cmd = *(uint32 *)pArg;
    uint32 myid = cm_node_get_id();
    //本节点最后一个关闭
    if(pNode->id == myid)
    {
        return CM_OK;
    }
    //考虑到cm_node_power_off 15秒的超时时间过长，这里先检测网络是否通畅。
    //通畅的话关机不会花费很长时间，这样就不会导致下面的设备因为超时而无法关机
    iRet = cm_system("ping %s 1 2>/dev/null", pNode->ip_addr);
    if(iRet != CM_OK)
    {
        CM_LOG_WARNING(CM_MOD_CNM, "ping node[%u] fail[%d]", pNode->id, iRet);
        return CM_FAIL;
    }
    
    if(cmd == CM_OMI_CMD_OFF)
    {
        return cm_node_power_off(pNode->id);
    }
    else
    {
        return cm_node_reboot(pNode->id);
    }
}

//集群节点电源控制函数
//command:CM_OMI_CMD_OFF | CM_OMI_CMD_REBOOT
static sint32 cm_cnm_cluster_power_ctrl(const uint32 command)
{
    sint32 iRet;
    uint32 myid = 0;
    
    iRet = cm_node_traversal_all(
               cm_cnm_cluster_trav_for_power, &command, CM_FALSE);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM, "trav all nodes fail");
    }

    myid = cm_node_get_id();
    //最后控制自己节点的电源
    if(command == CM_OMI_CMD_OFF)
    {  
        return cm_node_power_off(myid);
    }
    else
    {
        return cm_node_reboot(myid);
    }
}

extern sint32 cm_cnm_node_request(uint32 cmd, uint32 nid);

sint32 cm_cnm_cluster_power_off
(const void *pDecodeParam, void **ppAckData, uint32 *pAckLen)
{
    return cm_cnm_node_request(CM_OMI_CMD_OFF,CM_NODE_ID_NONE);
}

sint32 cm_cnm_cluster_reboot
(const void *pDecodeParam, void **ppAckData, uint32 *pAckLen)
{
    return cm_cnm_node_request(CM_OMI_CMD_REBOOT,CM_NODE_ID_NONE);
}

static sint32 cm_cluster_untie_cluster_net(const sint8 *ip)
{
    return cm_system(CM_SHELL_EXEC" cm_masterip_release");
}

static sint32 cm_cluster_bind_cluster_net(cm_cluster_info_t *pinfo)
{
    sint32 iRet = CM_OK;

    if('\0' == pinfo->ipaddr[0])
    {
        iRet = cm_cnm_cluster_get_inter(pinfo);
        if(CM_OK != iRet)
        {
            return CM_OK;
        }
    }
    
    if((0 == pinfo->timestamp) 
        || ('\0' == pinfo->ipaddr[0]) 
        || (CM_OK == cm_cnm_exec_ping(pinfo->ipaddr)))
    {
        return CM_OK;
    }

    return cm_system(CM_SHELL_EXEC" cm_masterip_bind");
}

//每5秒执行一次,用于cm_main，间隔检测是否绑定了集群IP
void cm_cluster_check_bind_cluster_ip(void)
{
    sint32 iRet;
    static cm_time_t prev = 0;
    cm_time_t now = cm_get_time();

    uint32 infolen = 0;
    cm_cluster_info_t *pinfo = &g_cm_cluster_info;

    uint32 myid = cm_node_get_id();
    uint32 masterid = cm_node_get_master();

    if(now - prev < CM_CNM_CLUSTER_CHECK_BIND_IP_INTR)
    {
        return;
    }

    prev = now;

    if(myid != masterid)
    {
        //解绑
        iRet = cm_cluster_untie_cluster_net(pinfo->ipaddr);

        if(CM_OK != iRet)
        {
            CM_LOG_ERR(CM_MOD_CNM, "untie fail");
        }

        return ;
    }

    // 没有绑定，直接绑定
    CM_MUTEX_LOCK(&g_cm_cnm_cluster_lock);
    iRet = cm_cluster_bind_cluster_net(pinfo);
    CM_MUTEX_UNLOCK(&g_cm_cnm_cluster_lock);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM, "bind cluster net fail");
    }

    return ;
}

//set master时实时绑定和解绑。
void cm_cluster_bind_master_ip(uint32 old_id, uint32 new_id)
{
    sint32 iRet;
    cm_cluster_info_t *pinfo = &g_cm_cluster_info;
    uint32 myid = cm_node_get_id();

    if(old_id == new_id)
    {
        CM_LOG_WARNING(CM_MOD_CNM, "master is not changed");
        return ;
    }

    if(myid != new_id)
    {
        //不是主节点, 解绑
        (void)cm_cluster_untie_cluster_net(pinfo->ipaddr);
        return ;
    }

    CM_MUTEX_LOCK(&g_cm_cnm_cluster_lock);
    iRet = cm_cluster_bind_cluster_net(pinfo);
    CM_MUTEX_UNLOCK(&g_cm_cnm_cluster_lock);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM, "bind cluster net fail");
    }

    return ;
}


void cm_cnm_cluster_oplog_modify(const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK ? CM_ALARM_LOG_CLUSTER_UPDATE_OK : CM_ALARM_LOG_CLUSTER_UPDATE_FAIL);
    const uint32 cnt = 8;
    cm_omi_field_flag_t set;
    
    CM_OMI_FIELDS_FLAG_CLR_ALL(&set);
    if(NULL == pDecodeParam)
    {   
        cm_cnm_oplog_report(sessionid,alarmid,NULL,cnt,NULL);
    }    
    else    
    {       
        const cm_cluster_info_t *info = (const cm_cluster_info_t*)pDecodeParam;
        cm_cnm_oplog_param_t params[8] = {
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_CLUSTER_NAME,strlen(info->name),info->name},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_CLUSTER_IPADDR,strlen(info->ipaddr),info->ipaddr},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_CLUSTER_GATEWAY,strlen(info->gateway),info->gateway},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_CLUSTER_NETMASK,strlen(info->netmask),info->netmask},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_CLUSTER_INTERFACE,strlen(info->interface),info->interface},
            
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_CLUSTER_PRIORITY,strlen(info->priority),info->priority},
            {CM_OMI_DATA_ENUM,CM_OMI_FIELD_CLUSTER_ENABLE_IPMI,sizeof(info->enipmi),&info->enipmi},
            {CM_OMI_DATA_ENUM,CM_OMI_FIELD_CLUSTER_FAILOVER,sizeof(info->failover),&info->failover},
        };

        CM_OMI_FIELDS_FLAG_SET(&set, CM_OMI_FIELD_CLUSTER_NAME);
        CM_OMI_FIELDS_FLAG_SET(&set, CM_OMI_FIELD_CLUSTER_IPADDR);
        CM_OMI_FIELDS_FLAG_SET(&set, CM_OMI_FIELD_CLUSTER_GATEWAY);
        CM_OMI_FIELDS_FLAG_SET(&set, CM_OMI_FIELD_CLUSTER_NETMASK);
        CM_OMI_FIELDS_FLAG_SET(&set, CM_OMI_FIELD_CLUSTER_INTERFACE);
        if(strlen(info->priority) != 0)
        {
            CM_OMI_FIELDS_FLAG_SET(&set, CM_OMI_FIELD_CLUSTER_PRIORITY);
        }
        CM_OMI_FIELDS_FLAG_SET(&set, CM_OMI_FIELD_CLUSTER_ENABLE_IPMI);
        CM_OMI_FIELDS_FLAG_SET(&set, CM_OMI_FIELD_CLUSTER_FAILOVER);
        
        cm_cnm_oplog_report(sessionid,alarmid,params,cnt,&set);
    } 
    return;
}

void cm_cnm_cluster_oplog_power_off(const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK ? CM_ALARM_LOG_CLUSTER_OFF_OK : CM_ALARM_LOG_CLUSTER_OFF_FAIL);
    const char *opmsg = "cluster power off";
    const uint32 cnt = 1;
    cm_omi_field_flag_t set;
    cm_cnm_oplog_param_t params[1] = {
        {CM_OMI_DATA_STRING,CM_OMI_FIELD_CLUSTER_NAME,strlen(opmsg),opmsg},
    };
        
    CM_OMI_FIELDS_FLAG_CLR_ALL(&set);
    CM_OMI_FIELDS_FLAG_SET(&set, CM_OMI_FIELD_CLUSTER_NAME);
    cm_cnm_oplog_report(sessionid,alarmid,params,cnt,&set);
    return;
}

void cm_cnm_cluster_oplog_reboot(const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK ? CM_ALARM_LOG_CLUSTER_REBOOT_OK : CM_ALARM_LOG_CLUSTER_REBOOT_FAIL);
    const char *opmsg = "cluster reboot";
    const uint32 cnt = 1;
    cm_omi_field_flag_t set;
    cm_cnm_oplog_param_t params[1] = {
        {CM_OMI_DATA_STRING,CM_OMI_FIELD_CLUSTER_NAME,strlen(opmsg),opmsg},
    };
        
    CM_OMI_FIELDS_FLAG_CLR_ALL(&set);
    CM_OMI_FIELDS_FLAG_SET(&set, CM_OMI_FIELD_CLUSTER_NAME);
    cm_cnm_oplog_report(sessionid,alarmid,params,cnt,&set);
    return;
}


/*============================================================================*/

typedef struct
{
    uint64 disk_used;
    uint64 disk_avail;

    uint64 pool_used;
    uint64 pool_avail;

    uint64 lun_used;  /*实际代表lun total*/
    uint64 lun_avail;

    uint64 nas_used;  /*实际代表nas total*/
    uint64 nas_avail;

    uint64 high_used;
    uint64 high_total;
    
    uint64 low_used;
    uint64 low_total;  
    
} cm_cnm_cluster_stat_t;

#define CM_CNM_CLUSTER_STAT_TABLE_COL_NUM 3
static cm_cnm_cluster_stat_t  g_cm_cnm_cluster_stat;

sint8 cm_cnm_get_last_char(sint8* str)
{
    sint8 c;
    int len = strlen(str);

    if(str[0] == '\0')
    {
        return 0;
    }
    else
    {
        if('b' == str[len - 1] || 'B' == str[len - 1])
        {
            c = str[len - 2];
        }
        else
        {
            c = str[len - 1];
        }
    }

    return c;
}

uint64 cm_cnm_unit_conversion(float x, sint8 y)
{
    double result = 0;

    switch(y)
    {
        case 'G':
        case 'g':
        {
            result = x * 1024;
            break;
        }

        case 'M':
        case 'm':
        {
            result = x;
            break;
        }

        case 'T':
        case 't':
        {
            result = x * 1024 * 1024;
            break;
        }

        case 'P':
        case 'p':
        {
            result = x * 1024 * 1024 * 1024;
            break;
        }

        default:
            break;
    }

    return (uint64)result;
}

uint64 cm_cnm_trans_result(sint8* str)
{
    sint8 a;
    float num = 0;
    a = cm_cnm_get_last_char(str);
    num = atof(str);
    return cm_cnm_unit_conversion(num, a);
}

sint32 cm_cnm_cluster_stat_disk_each(
    void *arg, sint32 col_cnt, sint8 **col_vals, sint8 **col_names)
{
    float space = 0.0;
    sint8 dim = 0;
    cm_cnm_cluster_stat_t *stat = arg;

    if(col_cnt != CM_CNM_CLUSTER_STAT_TABLE_COL_NUM)
    {
        CM_LOG_ERR(CM_MOD_CNM, "col_cnt[%d]", col_cnt);
        return col_cnt;
    }

    space = atof(col_vals[0]);
    dim = *(col_vals[1]);

    if(0 == strcmp(col_vals[2], "free"))
    {
        stat->disk_avail += cm_cnm_unit_conversion(space, dim);
    }
    else
    {
        stat->disk_used += cm_cnm_unit_conversion(space, dim);
    }

    return CM_OK;
}

static void cm_cnm_cluster_stat_disk(cm_cnm_cluster_stat_t *stat)
{
    cm_db_handle_t handle = NULL;
    sint32 iRet = cm_db_open(CM_CNM_DISK_FILE, &handle);

    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM, "open %s fail", CM_CNM_DISK_FILE);
        return;
    }

    iRet = cm_db_exec(handle, cm_cnm_cluster_stat_disk_each, stat, "select gsize,dim,status from record_t");
    cm_db_close(handle);
}

sint32 cm_cnm_cluster_stat_nas(uint64 *aval, uint64 *used)
{
    sint32 len = sizeof(cm_cnm_decode_info_t) + sizeof(cm_cnm_nas_info_t);
    cm_cnm_decode_info_t *param = CM_MALLOC(len);
    sint32 iRet = CM_OK;
    void *AckData = NULL;
    uint32 AckLen = 0;
    uint32 iloop = 0;
    cm_cnm_nas_info_t *info = NULL;

    if(NULL == param)
    {
        CM_LOG_DEBUG(CM_MOD_CNM, "malloc fail");
        return CM_FAIL;
    }

    *aval = 0;
    *used = 0;
    CM_MEM_ZERO(param, len);/*为参数初始化内存*/
    param->offset = 0;
    param->total = 20; /*一页取20个*/

    while(1)
    {
        AckData = NULL;
        AckLen = 0;
        iRet = cm_cnm_nas_getbatch(param, &AckData, &AckLen);

        if(CM_OK != iRet)
        {
            CM_LOG_ERR(CM_MOD_CNM, "iRet[%d]", iRet);
            break;
        }

        if(NULL == AckData)
        {
            break;
        }

        AckLen = AckLen / sizeof(cm_cnm_nas_info_t);
        CM_LOG_DEBUG(CM_MOD_CNM, "nascnt=%u", AckLen);

        for(iloop = 0, info = AckData; iloop < AckLen; iloop++, info++)
        {
            /* *aval+=cm_cnm_trans_result(info->space_avail); */
            *used += cm_cnm_trans_result(info->space_used);
        }

        CM_FREE(AckData);

        if(AckLen < param->total)
        {
            CM_LOG_DEBUG(CM_MOD_CNM, "offset[%u] total[%u] get[%u]",
                        param->offset, param->total, AckLen);
            break;
        }

        /* 取下一页 */
        param->offset += param->total;
    }

    CM_FREE(param);
    CM_LOG_DEBUG(CM_MOD_CNM, "end");
    return NULL;

}

sint32 cm_cnm_cluster_stat_lun(uint64 *aval, uint64 *used)
{
    sint32 len = sizeof(cm_cnm_lun_param_t);
    cm_cnm_lun_param_t *param = CM_MALLOC(len);
    sint32 iRet = CM_OK;
    void *AckData = NULL;
    uint32 AckLen = 0;
    uint32 iloop = 0;
    cm_cnm_lun_info_t *info = NULL;

    if(NULL == param)
    {
        CM_LOG_DEBUG(CM_MOD_CNM, "start");
        return CM_FAIL;
    }

    *aval = 0;
    *used = 0;
    CM_MEM_ZERO(param, len);/*为参数初始化内存*/
    param->offset = 0;
    param->total = 20; /*一页取20个*/

    while(1)
    {
        AckData = NULL;
        AckLen = 0;
        iRet = cm_cnm_lun_getbatch(param, &AckData, &AckLen);

        if(CM_OK != iRet)
        {
            CM_LOG_ERR(CM_MOD_CNM, "iRet[%d]", iRet);
            break;
        }

        if(NULL == AckData)
        {
            break;
        }

        AckLen = AckLen / sizeof(cm_cnm_lun_info_t);
        CM_LOG_DEBUG(CM_MOD_CNM, "luncnt=%u", AckLen);

        for(iloop = 0, info = AckData; iloop < AckLen; iloop++, info++)
        {
            *used += cm_cnm_trans_result(info->space_used);
        }

        CM_FREE(AckData);

        if(AckLen < param->total)
        {
            CM_LOG_DEBUG(CM_MOD_CNM, "offset[%u] total[%u] get[%u]",
                        param->offset, param->total, AckLen);
            break;
        }

        /* 取下一页 */
        param->offset += param->total;

    }

    CM_LOG_DEBUG(CM_MOD_CNM, "avail[%llu] used[%llu]\n", *aval, *used);
    CM_FREE(param);
    CM_LOG_INFO(CM_MOD_CNM, "end");
    return NULL;
}

sint32 cm_cnm_cluster_stat_pool(uint64 *aval, uint64 *used)
{
    sint32 len = sizeof(cm_cnm_pool_list_param_t);
    cm_cnm_pool_list_param_t *param = CM_MALLOC(len);
    sint32 iRet = CM_OK;
    void *AckData = NULL;
    uint32 AckLen = 0;
    uint32 iloop = 0;
    cm_cnm_pool_list_t *info = NULL;

    if(NULL == param)
    {
        CM_LOG_INFO(CM_MOD_CNM, "start");
        return CM_FAIL;
    }

    *aval = 0;
    *used = 0;
    CM_MEM_ZERO(param, len);/*为参数初始化内存*/
    param->offset = 0;
    param->total = 20; /*一页取20个*/

    while(1)
    {
        AckData = NULL;
        AckLen = 0;
        iRet = cm_cnm_pool_getbatch(param, &AckData, &AckLen);

        if(CM_OK != iRet)
        {
            CM_LOG_ERR(CM_MOD_CNM, "iRet[%d]", iRet);
            break;
        }

        if(NULL == AckData)
        {
            break;
        }

        AckLen = AckLen / sizeof(cm_cnm_pool_list_t);
        CM_LOG_INFO(CM_MOD_CNM, "poolcnt=%u", AckLen);

        for(iloop = 0, info = AckData; iloop < AckLen; iloop++, info++)
        {
            *aval += cm_cnm_trans_result(info->avail);
            *used += cm_cnm_trans_result(info->used);
        }

        CM_FREE(AckData);

        if(AckLen < param->total)
        {
            CM_LOG_INFO(CM_MOD_CNM, "offset[%u] total[%u] get[%u]",
                        param->offset, param->total, AckLen);
            break;
        }

        /* 取下一页 */
        param->offset += param->total;
    }

    CM_FREE(param);
    return NULL;
}

sint32 cm_cnm_cluster_stat_get(const void *pDecodeParam, void **ppAckData, uint32 *pAckLen)

{
    //cm_thread_t handle;
    cm_cnm_cluster_stat_t *pdata = CM_MALLOC(sizeof(cm_cnm_cluster_stat_t));

    if(NULL == pdata)
    {
        CM_LOG_ERR(CM_MOD_CNM, "malloc fail");
        return CM_FAIL;
    }

    CM_MEM_ZERO(pdata, sizeof(cm_cnm_cluster_stat_t));
    CM_MUTEX_LOCK(&g_cm_cnm_cluster_lock);
    CM_MEM_CPY(pdata, sizeof(cm_cnm_cluster_stat_t), &g_cm_cnm_cluster_stat, sizeof(cm_cnm_cluster_stat_t));
    CM_MUTEX_UNLOCK(&g_cm_cnm_cluster_lock);
    /***更新数据要上锁***/
    *ppAckData = pdata;
    *pAckLen = sizeof(cm_cnm_cluster_stat_t);
    return CM_OK;
}


static void cm_cnm_cluster_stat_pooldisk(cm_cnm_cluster_stat_t *data)
{
    /*const char* cmd="/var/cm/script/cm_cnm.sh pool_stat_in";*/
    sint32 iRet = CM_OK;
    sint8 buff[CM_STRING_1K] = {0};
    iRet = cm_exec(buff,sizeof(buff),  "/var/cm/script/cm_cnm.sh pool_stat_in");
    if(CM_OK != iRet)
    {        
        return;
    }
    iRet = sscanf(buff,"%llu %llu %llu %llu", &data->high_total,&data->high_used,
        &data->low_total,&data->low_used);
    if(iRet != 4)
    {
        CM_LOG_INFO(CM_MOD_CNM, "sscanf count(%d)",iRet);
    }
    return;
}

void cm_cnm_cluster_stat_thread(void)
{
    cm_cnm_cluster_stat_t data;
    uint32 master = cm_node_get_master();
    uint32 myid = cm_node_get_id();

    if((master == myid) || (master == CM_NODE_ID_NONE))
    {
        CM_MEM_ZERO(&data, sizeof(data));
        cm_cnm_cluster_stat_disk(&data);
        cm_cnm_cluster_stat_pool(&data.pool_avail, &data.pool_used);
        cm_cnm_cluster_stat_lun(&data.lun_avail, &data.lun_used);
        cm_cnm_cluster_stat_nas(&data.nas_avail, &data.nas_used);
        data.lun_avail = data.lun_used + data.pool_avail;
        data.nas_avail = data.nas_used + data.pool_avail; /*avail记录的是总容量total*/
        data.disk_avail += data.disk_used;
        data.pool_avail += data.pool_used;
        cm_cnm_cluster_stat_pooldisk(&data);
#ifdef CM_OMI_LOCAL
        cm_sync_request(CM_SYNC_OBJ_CLU_STAT,0,&data,sizeof(data));
#else        
        CM_MUTEX_LOCK(&g_cm_cnm_cluster_lock);
        CM_MEM_CPY(&g_cm_cnm_cluster_stat, sizeof(cm_cnm_cluster_stat_t), &data, sizeof(cm_cnm_cluster_stat_t));
        CM_MUTEX_UNLOCK(&g_cm_cnm_cluster_lock);
#endif        
    }

    return;
}

sint32 cm_cnm_cluster_stat_init(void)
{
    CM_MEM_ZERO(&g_cm_cnm_cluster_stat, sizeof(cm_cnm_cluster_stat_t));
    CM_MUTEX_INIT(&g_cm_cnm_cluster_lock);
    return CM_OK;
}

cm_omi_obj_t cm_cnm_cluster_stat_encode(const void *pDecodeParam, void *pAckData, uint32 AckLen)
{
    cm_omi_obj_t items = NULL;
    cm_omi_obj_t item  = NULL;
    cm_cnm_cluster_stat_t *pData = (cm_cnm_cluster_stat_t*)pAckData;
    items = cm_omi_obj_new_array();

    if(NULL == items)
    {
        CM_LOG_ERR(CM_MOD_CNM, "new items fail");
        return NULL;
    }

    item = cm_omi_obj_new();

    if(NULL == item)
    {
        CM_LOG_ERR(CM_MOD_CNM, "new item fail");
        return items;
    }

    cm_omi_obj_key_set_u64_ex(item, CM_OMI_FIELD_CLUSTER_DISK_USED, pData->disk_used);
    cm_omi_obj_key_set_u64_ex(item, CM_OMI_FIELD_CLUSTER_DISK_AVAIL, pData->disk_avail);
    cm_omi_obj_key_set_u64_ex(item, CM_OMI_FIELD_CLUSTER_POOL_USED, pData->pool_used);
    cm_omi_obj_key_set_u64_ex(item, CM_OMI_FIELD_CLUSTER_POOL_AVAIL, pData->pool_avail);
    cm_omi_obj_key_set_u64_ex(item, CM_OMI_FIELD_CLUSTER_LUN_USED, pData->lun_used);
    cm_omi_obj_key_set_u64_ex(item, CM_OMI_FIELD_CLUSTER_LUN_AVAIL, pData->lun_avail);
    cm_omi_obj_key_set_u64_ex(item, CM_OMI_FIELD_CLUSTER_NAS_USED, pData->nas_used);
    cm_omi_obj_key_set_u64_ex(item, CM_OMI_FIELD_CLUSTER_NAS_AVAIL, pData->nas_avail);

    cm_omi_obj_key_set_u64_ex(item, CM_OMI_FIELD_CLUSTER_H_USED, pData->high_used);
    cm_omi_obj_key_set_u64_ex(item, CM_OMI_FIELD_CLUSTER_H_TOTAL, pData->high_total);

    cm_omi_obj_key_set_u64_ex(item, CM_OMI_FIELD_CLUSTER_L_USED, pData->low_used);
    cm_omi_obj_key_set_u64_ex(item, CM_OMI_FIELD_CLUSTER_L_TOTAL, pData->low_total);

    if(CM_OK != cm_omi_obj_array_add(items, item))
    {
        cm_omi_obj_delete(item);
    }

    return items;
}

sint32 cm_cnm_cluster_stat_sync_request(uint64 data_id, void *pdata, uint32 len)
{
    if(len != sizeof(cm_cnm_cluster_stat_t))
    {
        return CM_FAIL;
    }
    CM_MUTEX_LOCK(&g_cm_cnm_cluster_lock);
    CM_MEM_CPY(&g_cm_cnm_cluster_stat, sizeof(cm_cnm_cluster_stat_t), pdata, sizeof(cm_cnm_cluster_stat_t));
    CM_MUTEX_UNLOCK(&g_cm_cnm_cluster_lock);
    return CM_OK;
}


