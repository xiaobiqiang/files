/******************************************************************************
 *          Copyright (c) 2017 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_main.c
 * author     : wbn
 * create date: 2017Äê10ÔÂ25ÈÕ
 * description: TODO:
 *
 *****************************************************************************/
#include "cm_log.h"
#include "cm_rpc.h"
#include "cm_cmt.h"
#include "cm_omi.h"
#include "cm_node.h"
#include "cm_htbt.h"
#include "cm_sync.h"
#include "cm_pmm.h"
#include "cm_cmd_server.h"
#include "cm_alarm.h"
#include "cm_sche.h"
#include "cm_task.h"
#include "cm_cnm_cluster.h"
#include "cm_cnm_expansion_cabinet.h"
#include "cm_cnm_systime.h"
#include "cm_cnm_alarm.h"
#include "cm_base.h"
#include "cm_cfg_omi.h"
#include "cm_cnm_nas.h"

extern const cm_cmt_config_t g_CmCmtCbks[CM_CMT_MSG_BUTT];
extern const uint32 *g_CmOmiObjCmdNoCheckPtr;
extern const cm_omi_object_cfg_t g_CmOmiObjectCfg[CM_OMI_OBJECT_BUTT];
typedef void (*cm_main_period_func_t)(void);

extern void cm_cnm_cluster_stat_thread(void);
extern void cm_cnm_cluster_nas_thread(void);
extern void cm_cnm_lunmap_update_period(void);
extern void cm_cnm_ntp_sync_thread(void);
extern void cm_cnm_alarm_common_thread(void);

static void cm_period_20s(uint32 sec);
static void cm_period_5min(uint32 minute);

const cm_main_init_func_t g_cm_init_list[] = 
{
    cm_node_init,
    cm_sync_init,
    cm_htbt_init,
    cm_pmm_init,
    cm_cmd_init,
    cm_alarm_init,
    cm_sche_init,
    cm_task_init,
    NULL,
};

const cm_main_period_func_t g_cm_period_list_second[]=
{
    cm_cluster_check_bind_cluster_ip,
    /*cm_cnm_cabinet_thread,*/
    NULL
};

const cm_main_period_func_t g_cm_period_list_munite[]=
{
    cm_cnm_cluster_stat_thread,
    cm_cnm_cluster_nas_thread,
    cm_cnm_lunmap_update_period,
    cm_cnm_ntp_sync_thread,
    cm_cnm_storage_alarm_thread,
    cm_cnm_alarm_common_thread,
    cm_cnm_lowdata_volume_stop_thread,
    NULL
};

const cm_main_period_func_t g_cm_period_list_hour[]=
{
    cm_cnm_lowdata_volume_thread,
    NULL
};

const cm_main_period_func_t g_cm_period_list_day[]=
{
    cm_log_backup,
    NULL
};


/******************************************************************************
 * function     :
 * description  :
 *
 * parameter in :
 *
 * parameter out:
 *
 * return type  :
 *
 * author       : ${user}
 * create date  : ${date}
 *****************************************************************************/
sint32 cm_main_init(void)
{
    sint32 iRet = CM_FAIL;
    uint32 iloop = 0;
    const cm_main_init_func_t *initfunc = g_cm_init_list;
    const cm_base_config_t cfg=
    {
        CM_LOG_DIR \
        CM_LOG_FILE, CM_LOG_MODE,CM_LOG_LVL,

        CM_FALSE, CM_TRUE,CM_TRUE, CM_FALSE,

        CM_CMT_MSG_BUTT,g_CmCmtCbks,
        
        CM_OMI_OBJECT_SESSION, CM_OMI_OBJECT_BUTT, 
        g_CmOmiObjCmdNoCheckPtr, CmOmiMap, g_CmOmiObjectCfg,

        cm_node_get_id,cm_node_get_submaster_current
    };

    iRet = cm_base_init(&cfg);
    if(CM_OK != iRet)
    {
        printf("cm base init fail(%d)\n",iRet );
        return CM_FAIL;
    }
    CM_LOG_WARNING(CM_MOD_NONE,"start");
    while(NULL != *initfunc)
    {
        iRet = (*initfunc)();
        if(CM_OK != iRet)
        {
            CM_LOG_ERR(CM_MOD_LOG, "init mod[%d] fail(%d)",iloop,iRet);
            return CM_FAIL;
        }
        initfunc++;
        iloop++;
    }
    CM_LOG_WARNING(CM_MOD_NONE,"end");
    return CM_OK;
}

static void cm_main_period_cbk(const cm_main_period_func_t *list)
{
    while(NULL != *list)
    {
        (*list)();
        list++;
    }
    return;
}

static void* cm_main_report_event(void* arg)

{
    const sint8* myname=NULL;
    while(1)
    {
        CM_SLEEP_S(2);
        if(CM_NODE_ID_NONE != cm_node_get_master())
        {
            break;
        }        
    }
    myname = cm_node_get_name(CM_NODE_ID_LOCAL);
    CM_ALARM_REPORT(CM_ALARM_EVENT_NODE_IN,myname);
    CM_ALARM_RECOVERY(CM_ALARM_FAULT_NODE_OFFLINE,myname);
    return NULL;
}

int main(int argc, const char *argv[])
{
    struct tm pre;
    struct tm now;
    sint32 cnt = cm_exec_int("ps -ef|grep -w ceres_cm |grep -v grep |wc -l");

    if(1 < cnt)
    {
        printf("already run! cnt=%d\n",cnt);
        return CM_FAIL;
    }
    (void)cm_system("/var/cm/script/cm_node.sh preinit");
    
    g_cm_sys_ver = cm_exec_int(CM_SHELL_EXEC" cm_systerm_version_get");
    
    if(0 != fork())
    {
        return 0;
    }
    signal(SIGPIPE,SIG_IGN);
    (void)cm_system("mkdir -p %s 2>/dev/null",CM_DATA_DIR);
    (void)cm_system("mkdir -p %s 2>/dev/null",CM_LOG_DIR);
    (void)cm_system(CM_SHELL_EXEC" cm_disk_update_cache");
    if(CM_OK != cm_main_init())
    {
        return CM_FAIL;
    }
    
    cm_thread_start(cm_main_report_event,NULL);
    
    cm_get_datetime(&pre);
    while(1)
    {
        CM_SLEEP_S(1);        
        cm_main_period_cbk(g_cm_period_list_second);
        
        cm_get_datetime(&now);

        cm_period_20s(now.tm_sec);
        
        if(now.tm_min != pre.tm_min)
        {
            cm_main_period_cbk(g_cm_period_list_munite);
            cm_period_5min(now.tm_min);
        }
        if(now.tm_hour != pre.tm_hour)
        {
            cm_main_period_cbk(g_cm_period_list_hour);
        }
        if(now.tm_mday != pre.tm_mday)
        {
            cm_main_period_cbk(g_cm_period_list_day);
        }
        CM_MEM_CPY(&pre,sizeof(pre),&now,sizeof(pre));
    }
    return 0;
}


static void cm_period_20s(uint32 sec)
{
    sec++;
    if(0 != (sec % 20))
    {
        return;
    }
    
    (void)cm_system(CM_SHELL_EXEC" cm_disk_update_cache");
}

static void cm_period_5min(uint32 minute)
{
    minute++;
    if(0 != (minute % 20))
    {
        return;
    }
    (void)cm_system(CM_SHELL_EXEC" cm_period_5min");
}