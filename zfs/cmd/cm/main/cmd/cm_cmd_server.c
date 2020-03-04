/******************************************************************************
 *          Copyright (c) 2018 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cmd_server.c
 * author     : wbn
 * create date: 2018Äê1ÔÂ8ÈÕ
 * description: TODO:
 *
 *****************************************************************************/
#include "cm_common.h"
#include "cm_cmd_server.h"
#include "cm_log.h"
#include "cm_node.h"
#include "cm_alarm.h"
#include "cm_omi.h"
#include "cm_cnm_common.h"

#include <utmpx.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

static sint32 cm_cmd_help(const sint8 **ppParam, uint32 ParamNum, void **ppAckData, uint32 *pAckLen);
static sint32 cm_cmd_master(const sint8 **ppParam, uint32 ParamNum, void **ppAckData, uint32 *pAckLen);
static sint32 cm_cmd_submaster(const sint8 **ppParam, uint32 ParamNum, void **ppAckData, uint32 *pAckLen);
static sint32 cm_cmd_alarm(const sint8 **ppParam, uint32 ParamNum, void **ppAckData, uint32 *pAckLen);
static sint32 cm_cmd_node(const sint8 **ppParam, uint32 ParamNum, void **ppAckData, uint32 *pAckLen);
static sint32 cm_cmd_log(const sint8 **ppParam, uint32 ParamNum, void **ppAckData, uint32 *pAckLen);
static sint32 cm_cmd_exec(const sint8 **ppParam, uint32 ParamNum, void **ppAckData, uint32 *pAckLen);
static sint32 cm_cmd_count(const sint8 **ppParam, uint32 ParamNum, void **ppAckData, uint32 *pAckLen);
static sint32 cm_cmd_setuptime(const sint8 **ppParam, uint32 ParamNum, void **ppAckData, uint32 *pAckLen);
static sint32 cm_cmd_getuptime(const sint8 **ppParam, uint32 ParamNum, void **ppAckData, uint32 *pAckLen);
static sint32 cm_cmd_starttask(const sint8 **ppParam, uint32 ParamNum, void **ppAckData, uint32 *pAckLen);

const cm_cmd_cbk_cfg_t g_cm_cmd_cfg[] = 
{
    {"help", cm_cmd_help},
    {"master", cm_cmd_master},
    {"submaster", cm_cmd_submaster},
    {"alarm", cm_cmd_alarm},
    {"node",cm_cmd_node},
    {"log",cm_cmd_log},
    {"exec",cm_cmd_exec},
    {"count",cm_cmd_count},
    {"setuptime",cm_cmd_setuptime},
    {"getuptime",cm_cmd_getuptime},
    {"starttask",cm_cmd_starttask},
};

sint32 cm_cmd_init(void)
{
    return CM_OK;
}

static sint32 cm_cmd_msg_decode(sint8 *Msg, uint32 len, 
    sint8 **ppParam, uint32 *pCnt)
{
    uint32 cnt = (uint32)atoi(Msg);
    uint32 index = 0;
    uint32 tmp_len = strlen(Msg)+1;
    /* 3 aa bb cc  */
    while((len > tmp_len) && (index < cnt))
    {
        Msg += tmp_len;
        len -= tmp_len;
        *ppParam = Msg;
        index++;
        ppParam++;
        tmp_len = strlen(Msg)+1;
    }
    *pCnt = index;
    return CM_OK;  
}

static sint32 cm_cmd_subcmd(const cm_cmd_cbk_cfg_t *pCfg, uint32 CfgNum,
    const sint8 **ppParam, uint32 ParamNum, void **ppAckData, uint32 *pAckLen)
{
    if(0 == ParamNum)
    {
        return CM_PARAM_ERR;
    }
    
    while(CfgNum > 0)
    {
        CfgNum--;
        if(0 == strcmp(pCfg->pname, ppParam[0]))
        {
            return pCfg->cbk(ppParam+1,ParamNum-1,ppAckData,pAckLen);
        }
        pCfg++;
    }
    return CM_ERR_NOT_SUPPORT;
}

sint32 cm_cmd_cbk_cmt(
 void *pMsg, uint32 Len, void** ppAck, uint32 *pAckLen)
{
    sint8 *pParam[CM_CMD_PARAM_MAX_NUM] = {NULL};
    uint32 ParamNum = CM_CMD_PARAM_MAX_NUM;
    
    (void)cm_cmd_msg_decode((sint8*)pMsg,Len,pParam,&ParamNum);
    
    return cm_cmd_subcmd(g_cm_cmd_cfg,sizeof(g_cm_cmd_cfg)/sizeof(cm_cmd_cbk_cfg_t),
        pParam,ParamNum,ppAck,pAckLen);
}

static sint32 cm_cmd_help(const sint8 **ppParam, uint32 ParamNum, void **ppAckData, uint32 *pAckLen)
{
    const cm_cmd_cbk_cfg_t *pCfg = g_cm_cmd_cfg;
    uint32 CfgNum = sizeof(g_cm_cmd_cfg)/sizeof(cm_cmd_cbk_cfg_t);
    sint8 *pdata = CM_MALLOC(CM_STRING_512);
    uint32 len = 0;

    if(NULL == pdata)
    {
        CM_LOG_ERR(CM_MOD_CMD,"malloc fail");
        return CM_FAIL;
    }
    while(CfgNum > 0)
    {
        CfgNum--;
        len += CM_VSPRINTF(pdata+len,CM_STRING_512-len-1,"%s\n",pCfg->pname);
        pCfg++;
    }
    *ppAckData = pdata;
    *pAckLen = len+1;
    return CM_OK;
}

static sint32 cm_cmd_get_master(bool_t isSub,void **ppAckData, uint32 *pAckLen)
{
    cm_node_info_t node;
    sint32 iRet = CM_FAIL;
    sint8 *pdata = CM_MALLOC(CM_STRING_512);
    uint32 len = 0;
    uint32 nid = CM_NODE_ID_NONE;
    uint32 domain_id = 0;

    if(NULL == pdata)
    {
        CM_LOG_ERR(CM_MOD_CMD,"malloc fail");
        return CM_FAIL;
    }
    *ppAckData = pdata;
    
    if(CM_FALSE == isSub)
    {
        nid = cm_node_get_master();
        domain_id = CM_MASTER_SUBDOMAIN_ID;
    }
    else
    {
        nid = cm_node_get_submaster_current();
        domain_id = cm_node_get_subdomain_id();
    }
    
    do
    {
        if(CM_NODE_ID_NONE == nid)
        {
            len += CM_VSPRINTF(pdata+len,CM_STRING_512-len-1,"none\n");
            break;
        }
        iRet = cm_node_get_info(domain_id,nid,&node);
        if(CM_OK != iRet)
        {
            len += CM_VSPRINTF(pdata+len,CM_STRING_512-len-1,
                "error:getinfo nid[%u] subdomain[%u]\n",nid,domain_id);
            break;
        }        

        len += CM_VSPRINTF(pdata+len,CM_STRING_512-len-1,
                "node_id   : %u\n",node.id);
        len += CM_VSPRINTF(pdata+len,CM_STRING_512-len-1,
                "node_name : %s\n",node.name);
        len += CM_VSPRINTF(pdata+len,CM_STRING_512-len-1,
                "node_ip   : %s\n",node.ip_addr);
    }while(0);
    
    *pAckLen = len+1;
    return CM_OK;
}

static sint32 cm_cmd_master(
    const sint8 **ppParam, uint32 ParamNum, void **ppAckData, uint32 *pAckLen)
{
    return cm_cmd_get_master(CM_FALSE,ppAckData,pAckLen);
}

static sint32 cm_cmd_submaster(
    const sint8 **ppParam, uint32 ParamNum, void **ppAckData, uint32 *pAckLen)
{
    return cm_cmd_get_master(CM_TRUE,ppAckData,pAckLen);
}

static sint32 cm_cmd_alarm(
    const sint8 **ppParam, uint32 ParamNum, void **ppAckData, uint32 *pAckLen)
{
    const sint8 *alarm_type[] = {"report","recovery"};
    uint32 cnt = sizeof(alarm_type)/sizeof(sint8*);
    uint32 index = 0;
    uint32 alarm_id = 0;

    /* alarm <report|recovery> <alarm_id> <alarm_param> */
    if(3 != ParamNum)
    {
        CM_LOG_ERR(CM_MOD_CMD,"ParamNum[%u]",ParamNum);
        return CM_PARAM_ERR;
    }
    while(index < cnt)
    {
        if(0 == strcmp(alarm_type[index], ppParam[0]))
        {
            break;
        }
        index++;
    }
    if(index == cnt)
    {
        CM_LOG_ERR(CM_MOD_CMD,"alarm_type=%s",ppParam[0]);
        return CM_PARAM_ERR;
    }

    alarm_id = (uint32)atoi(ppParam[1]);
    if(0 == alarm_id)
    {
        CM_LOG_ERR(CM_MOD_CMD,"alarm_id=%s",ppParam[1]);
        return CM_PARAM_ERR;
    }

    if(strlen(ppParam[2]) >= (CM_ALARM_MAX_PARAM_LEN -1))
    {      
        return CM_PARAM_ERR;
    }
    return cm_alarm_report(alarm_id,ppParam[2],index);
}

#define CM_CMD_NODE_FORMAT_HEAD "%-5s %-10s %-15s %-30s %-10s %-5s\n"
#define CM_CMD_NODE_FORMAT_DATA "%-5u %-10u %-15s %-30s %-10s %-5u\n" 

extern const cm_omi_map_enum_t CmOmiMapEnumNodeState;

static sint32 cm_cmd_node_get_each(
    cm_node_info_t *pinfo,sint8 *pdata)
{
    uint32 len = strlen(pdata);
    const sint8* pstate = (pinfo->state < CmOmiMapEnumNodeState.num) ? \
        CmOmiMapEnumNodeState.value[pinfo->state].pname : "unkown";
    
    CM_VSPRINTF(pdata+len,CM_STRING_2K-len,CM_CMD_NODE_FORMAT_DATA,
        pinfo->subdomain_id,pinfo->id,pinfo->name,pinfo->ip_addr,pstate,pinfo->inter_id);
    return CM_OK;
}

static sint32 cm_cmd_node(const sint8 **ppParam, uint32 ParamNum, void **ppAckData, uint32 *pAckLen)
{
    const sint8* titles[] = {"GID","NID","NAME","IP","STATE","LID"};
    sint8 *pdata = CM_MALLOC(CM_STRING_2K);

    if(NULL == pdata)
    {
        CM_LOG_ERR(CM_MOD_CMD,"malloc fail");
        return CM_FAIL;
    }
    *ppAckData = pdata;
    CM_VSPRINTF(pdata,CM_STRING_2K,CM_CMD_NODE_FORMAT_HEAD,
        titles[0],titles[1],titles[2],titles[3],titles[4],titles[5]);
    if(ParamNum >= 1)
    {
        (void)cm_node_traversal_node((uint32)atoi(ppParam[0]),
            (cm_node_trav_func_t)cm_cmd_node_get_each,pdata,CM_FALSE);
    }
    else
    {
        (void)cm_node_traversal_all(
            (cm_node_trav_func_t)cm_cmd_node_get_each,pdata,CM_FALSE);
    }
    *pAckLen = strlen(pdata) + 1;
    return CM_OK;
}

#define CM_CMD_LOG_FORMAT_HEAD "%-5s %-10s %-15s\n"
#define CM_CMD_LOG_FORMAT_DATA "%-5u %-10s %-15s\n" 
extern const sint8 *g_CmLogModuleNames[CM_MOD_BUTT];
extern const sint8 *g_CmLogLevelNames[CM_LOG_LVL_NUM];

static sint32 cm_cmd_log_show(const sint8 **ppParam, uint32 ParamNum, void **ppAckData, uint32 *pAckLen)
{
    const sint8* titles[] = {"MID","NAME","LEVEL"};
    const sint8* *pLvl = g_CmLogLevelNames;
    cm_moudule_e iloop = CM_MOD_NONE+1; 
    uint32 len = 0;
    uint8 level = 0;
    sint8 *pdata = CM_MALLOC(CM_STRING_2K);

    if(NULL == pdata)
    {
        CM_LOG_ERR(CM_MOD_CMD,"malloc fail");
        return CM_FAIL;
    }
    *ppAckData = pdata;
    len = CM_VSPRINTF(pdata,CM_STRING_2K,CM_CMD_LOG_FORMAT_HEAD,
        titles[0],titles[1],titles[2]);

    while(iloop<CM_MOD_BUTT)
    {
        (void)cm_log_level_get(iloop,&level);
        len += CM_VSPRINTF(pdata+len,CM_STRING_2K-len,CM_CMD_LOG_FORMAT_DATA,
            (uint32)iloop,g_CmLogModuleNames[iloop],pLvl[level]);
        iloop++;
    }
    *pAckLen = len+1;
    return CM_OK;
}

static uint32 cm_cmd_log_get_index(const sint8 **ppNames, uint32 cnt, const sint8 *pName)
{
    uint32 index = 0;
    
    while(index < cnt)
    {
        if(0 == strcasecmp(*ppNames, pName))
        {
            return index;
        }
        ppNames++;
        index++;
    }
    return index;
}
static sint32 cm_cmd_log_set(const sint8 **ppParam, uint32 ParamNum, void **ppAckData, uint32 *pAckLen)
{
    uint32 mod = 0;
    uint32 level = 0;
    
    if(2 != ParamNum)
    {
        return CM_PARAM_ERR;
    }
    
    mod = cm_cmd_log_get_index(g_CmLogModuleNames,CM_MOD_BUTT,ppParam[0]);
    if(CM_MOD_BUTT == mod)
    {
        return CM_PARAM_ERR;
    }
    
    level = cm_cmd_log_get_index(g_CmLogLevelNames,CM_LOG_LVL_NUM,ppParam[1]);
    if(CM_LOG_LVL_NUM == level)
    {
        return CM_PARAM_ERR;
    }
    return cm_log_level_set((cm_moudule_e)mod,(uint8)level);
}

static sint32 cm_cmd_log_reset(const sint8 **ppParam, uint32 ParamNum, void **ppAckData, uint32 *pAckLen)
{
    uint32 mod = 0;
    
    if(1 != ParamNum)
    {
        return CM_PARAM_ERR;
    }
    
    mod = cm_cmd_log_get_index(g_CmLogModuleNames,CM_MOD_BUTT,ppParam[0]);
    if(CM_MOD_BUTT == mod)
    {
        return CM_PARAM_ERR;
    }
    return cm_log_level_set((cm_moudule_e)mod,(uint8)CM_LOG_LVL);
}

static sint32 cm_cmd_log_enable(const sint8 **ppParam, uint32 ParamNum, void **ppAckData, uint32 *pAckLen)
{
    return cm_log_all_mode_set(CM_TRUE);
}

static sint32 cm_cmd_log_disable(const sint8 **ppParam, uint32 ParamNum, void **ppAckData, uint32 *pAckLen)
{
    return cm_log_all_mode_set(CM_FALSE);
}

static sint32 cm_cmd_log_backup(const sint8 **ppParam, uint32 ParamNum, void **ppAckData, uint32 *pAckLen)
{
    bool_t state = CM_FALSE;
    const sint8* stateval[] = {"off", "on"};
    
    if(ParamNum == 0)
    {
        /* get */
        uint8 *Ack = CM_MALLOC(CM_STRING_32);
        if(NULL == Ack)
        {
            return CM_FAIL;
        }
        state = cm_log_backup_get();
        CM_VSPRINTF(Ack,CM_STRING_32,"backup : %s\n",stateval[state]);
        *ppAckData = Ack;
        *pAckLen = strlen(Ack)+1;
        return CM_OK;   
    }
    if(0 == strcasecmp(ppParam[0],stateval[1]))
    {
        state = CM_TRUE;
    }
    cm_log_backup_set(state);
    return CM_OK;
}


static sint32 cm_cmd_log(const sint8 **ppParam, uint32 ParamNum, void **ppAckData, uint32 *pAckLen)
{
    const cm_cmd_cbk_cfg_t subcmd[] = 
    {
        {"show",cm_cmd_log_show},
        {"set",cm_cmd_log_set},
        {"reset",cm_cmd_log_reset},
        {"enable",cm_cmd_log_enable},
        {"disable",cm_cmd_log_disable},
        {"backup",cm_cmd_log_backup},
    };
    uint32 CfgNum = sizeof(subcmd)/sizeof(cm_cmd_cbk_cfg_t);

    return cm_cmd_subcmd(subcmd,CfgNum,ppParam,ParamNum,ppAckData,pAckLen);
}

static sint32 cm_cmd_exec(const sint8 **ppParam, uint32 ParamNum, void **ppAckData, uint32 *pAckLen)
{
    uint32 nid  = 0;
    const sint8* cmd = NULL;
    sint8 *pbuff = NULL;
    sint32 iRet = CM_OK;
    if(ParamNum < 1)
    {
        /* ceres_cmd exec <cmd> <nid>*/
        return CM_PARAM_ERR;
    }
    if(ParamNum > 1)
    {
        nid = (uint32)atoi(ppParam[1]);
    }
    
    cmd = ppParam[0];

    pbuff = CM_MALLOC(CM_STRING_4K);
    if(NULL == pbuff)
    {
        return CM_FAIL;
    }
    
    iRet = cm_cnm_exec_remote(nid,CM_FALSE,pbuff,CM_STRING_4K,cmd);
    if(0 == strlen(pbuff))
    {
        CM_FREE(pbuff);
        return iRet;
    }
    *ppAckData = pbuff;
    *pAckLen = strlen(pbuff)+1;
    return iRet;
}

static sint32 cm_cmd_count(const sint8 **ppParam, uint32 ParamNum, void **ppAckData, uint32 *pAckLen)
{
    uint32 nid  = 0;
    const sint8* cmd = NULL;
    sint8 *pbuff = NULL;
    uint64 cnt = 0;
    if(ParamNum < 1)
    {
        /* ceres_cmd exec <cmd> <nid>*/
        return CM_PARAM_ERR;
    }
    if(ParamNum > 1)
    {
        nid = (uint32)atoi(ppParam[1]);
    }
    cmd = ppParam[0];
    pbuff = CM_MALLOC(CM_STRING_32);
    if(NULL == pbuff)
    {
        return CM_FAIL;
    }
    
    cnt = cm_cnm_exec_count(nid,cmd);
    CM_VSPRINTF(pbuff,CM_STRING_32,"%llu\n",cnt);
    
    *ppAckData = pbuff;
    *pAckLen = strlen(pbuff)+1;
    return CM_OK;
}


#ifdef __linux__
static sint32 cm_cmd_setuptime(const sint8 **ppParam, uint32 ParamNum, void **ppAckData, uint32 *pAckLen)
{
    return CM_OK;
}

static sint32 cm_cmd_getuptime(const sint8 **ppParam, uint32 ParamNum, void **ppAckData, uint32 *pAckLen)
{
    return CM_OK;
}  
#else
static sint32 cm_cmd_update_utmpx_bak(int size,time_t u_time)
{
    const char* file = "/var/adm/utmpx";
    const char* bak_file = "/var/adm/utmpx_bak";
    sint32 n = size/sizeof(struct futmpx);
    sint32 i=0;
    struct futmpx upt[n];
    struct futmpx* p;
    FILE* fpr,*fpw;
	
    fpr = fopen(file,"rb");
    if(fpr == NULL)
	{
        CM_LOG_ERR(CM_MOD_CMD,"open file fail");
        return CM_FAIL;
    }
    p = (struct futmpx*)&upt;
    fread(p,sizeof(struct futmpx),n,fpr);
    fclose(fpr);
    for(;i<n;i++,p++)
    {
        if(p->ut_type == BOOT_TIME)
        {
             p->ut_tv.tv_sec = u_time;
        }
    }
	
    fpw = fopen(bak_file,"wb");
    if(fpw == NULL)
    {
	    CM_LOG_ERR(CM_MOD_CMD,"open file fail");
        return CM_FAIL;
	}
    fwrite(&upt,sizeof(struct futmpx),n,fpw);
    fclose(fpw);
    
	return CM_OK;
}

static sint32 cm_cmd_setuptime(const sint8 **ppParam, uint32 ParamNum, void **ppAckData, uint32 *pAckLen)
{
    time_t u_time;
    int size = 0;
    struct stat sbuf;
    struct tm stm;
    sint32 iRet = CM_OK;

    if(ParamNum != 1)
    {
        CM_LOG_ERR(CM_MOD_CMD,"ParamNum[%u]",ParamNum);
        return CM_PARAM_ERR;
    }

    if(strlen(ppParam[0]) != 19)
    {
        CM_LOG_ERR(CM_MOD_CMD,"len[%u]",strlen(ppParam[0]));
        return CM_PARAM_ERR;
    }
    
    strptime(ppParam[0],"%Y-%m-%d %H:%M:%S",&stm);
	u_time =  mktime(&stm);   

    stat(UTMPX_FILE, &sbuf);
    size = sbuf.st_size;

    iRet = cm_cmd_update_utmpx_bak(size,u_time);
    if(iRet != CM_OK)
    {
        return CM_FAIL;
    }

    cm_system("mv /var/adm/utmpx_bak /var/adm/utmpx");
    return cm_system("chgrp bin /var/adm/utmpx");
}

static sint32 cm_cmd_getuptime(const sint8 **ppParam, uint32 ParamNum, void **ppAckData, uint32 *pAckLen)
{
    const char* file = "/var/adm/utmpx";
    FILE* fp;
    struct futmpx* p;
    struct stat sbuf;
    sint32 i=0,n=0;
    cm_time_t u_time;
    struct tm* tm_t;
    char strtime[CM_STRING_128] = {0};
    
    if(ParamNum != 0)
    {
        CM_LOG_ERR(CM_MOD_CMD,"ParamNum[%u]",ParamNum);
        return CM_PARAM_ERR;
    }

    stat(UTMPX_FILE, &sbuf);
    n = sbuf.st_size / sizeof(struct futmpx);

    struct futmpx upt[n];
    
    fp = fopen(file,"rb");
    if(fp == NULL)
	{
        CM_LOG_ERR(CM_MOD_CMD,"open file fail");
        return CM_FAIL;
    }
    p = (struct futmpx*)&upt;
    fread(p,sizeof(struct futmpx),n,fp);
    fclose(fp);
    for(;i<n;i++,p++)
    {
        if(p->ut_type == BOOT_TIME)
        {
             u_time = p->ut_tv.tv_sec;
        }
    }

    tm_t = localtime(&u_time);
    strftime(strtime,CM_STRING_128,"%F %T\n",tm_t);

    *ppAckData = strtime;
    *pAckLen = strlen(strtime)+1;

    return CM_OK;
}
#endif
static sint32 cm_cmd_starttask(const sint8 **ppParam, uint32 ParamNum, void **ppAckData, uint32 *pAckLen)
{
    uint32 tid = 0;
    sint32 iRet = CM_OK;
    sint8 *pbuff = NULL;
    if(ParamNum < 2)
    {
        return CM_PARAM_ERR;
    }
    pbuff = CM_MALLOC(CM_STRING_128);
    if(NULL == pbuff)
    {
        return CM_FAIL;
    }
    
    iRet = cm_cnm_localtask_create(ppParam[0],ppParam[1],&tid);
    if(CM_OK != iRet)
    {
        CM_FREE(pbuff);
        return iRet;
    }
    CM_VSPRINTF(pbuff,CM_STRING_128,"taskid: %u\n",tid);
    *ppAckData = pbuff;
    *pAckLen = strlen(pbuff)+1;
    return CM_OK;
}


