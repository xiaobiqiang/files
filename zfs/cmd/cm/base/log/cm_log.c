/******************************************************************************
 *          Copyright (c) 2017 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_log.c
 * author     : wbn
 * create date: 2017Äê10ÔÂ23ÈÕ
 * description: TODO:
 *
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/time.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "cm_log.h"

#include "cm_queue.h"
static cm_queue_t *g_cmlogqueue=NULL;
static void* cm_log_thread(void* pArg);

static uint8 cm_log_out[CM_LOG_LINE_MAX] = {0};

static uint8 g_CmLogLvls[CM_MOD_BUTT];
static bool_t g_CmLogAllEnable = CM_FALSE;

extern const sint8 *g_CmLogModuleNames[CM_MOD_BUTT];
extern const sint8 *g_CmLogLevelNames[CM_LOG_LVL_NUM];

static bool_t cmlogisinit = CM_FALSE;
static bool_t g_CmLogIsbackup = CM_FALSE;
static uint8 g_CmLogMod=CM_LOG_MODE_NORMAL;
static void cm_log_write(const char *str);

extern uint8 g_CmLogLvlDef;
extern const sint8* g_CmLogFile;

sint32 cm_log_init(uint8 logmod)
{
    uint32 iloop = 0;
    sint32 iRet = CM_FAIL;
    cm_thread_t Handle;
    g_CmLogMod = logmod;
    while(iloop < (uint32)CM_MOD_BUTT)
    {
        g_CmLogLvls[iloop] = g_CmLogLvlDef;
        iloop++;
    }

    g_CmLogAllEnable = CM_TRUE;

    if(g_CmLogMod == CM_LOG_MODE_NORMAL)
    {
        iRet = cm_queue_init(&g_cmlogqueue,CM_LOG_QUEUE_LEN);
        if(CM_OK != iRet)
        {
            return CM_FAIL;
        }

        iRet = CM_THREAD_CREATE(&Handle, cm_log_thread, g_cmlogqueue);
        if(CM_OK != iRet)
        {
            cm_queue_destroy(g_cmlogqueue);
            return CM_FAIL;
        }
    }
    cmlogisinit = CM_TRUE;
    return CM_OK;
}

void cm_log_print(cm_moudule_e Module, uint8 lvl, const sint8 *pFunction, sint32 Line, const sint8 *pFormat,...)
{
    va_list args;
    sint32 len=0;
    struct timeval tv;
    struct tm *t;
    const sint8* *pLvl = g_CmLogLevelNames;
    uint8 *pBuf = NULL;

    if((Module >= CM_MOD_BUTT) || (CM_FALSE == cmlogisinit)
        || (CM_FALSE == g_CmLogAllEnable))
    {
        return;
    }

    if(lvl > g_CmLogLvls[Module])
    {
        return;
    }

    if(g_CmLogMod != CM_LOG_MODE_NORMAL)
    {
        pBuf = cm_log_out;
    }
    else
    {
        pBuf = malloc(CM_LOG_LINE_MAX);
        if(NULL == pBuf)
        {
            return;
        }
    }

    (void)gettimeofday(&tv, NULL);
    t = localtime(&tv.tv_sec);

    snprintf(pBuf,CM_LOG_LINE_MAX-1,
        "[%04d%02d%02d%02d%02d%02d.%06lu][%s.%s]",
        t->tm_year +1900, t->tm_mon + 1, t->tm_mday,
        t->tm_hour, t->tm_min, t->tm_sec, tv.tv_usec,
        g_CmLogModuleNames[Module],pLvl[lvl]);

    va_start(args, pFormat);
    len=vsnprintf(pBuf+strlen(pBuf),
        CM_LOG_LINE_MAX-strlen(pBuf)-1,
        pFormat,args);
    va_end(args);

    len=snprintf(pBuf+strlen(pBuf),
        CM_LOG_LINE_MAX-strlen(pBuf)-1,
        "[%s,%d][%u.%u]\n",pFunction,Line,CM_THREAD_MYID(),(uint32)getpid());

    if(g_CmLogMod != CM_LOG_MODE_NORMAL)
    {
        cm_log_write(pBuf);
    }
    else
    {
        if(CM_OK != cm_queue_add(g_cmlogqueue, pBuf))
        {
            free(pBuf);
        }
    }
}

sint32 cm_log_level_set(cm_moudule_e Module, uint8 lvl)
{
    if(Module >= CM_MOD_BUTT)
    {
        return CM_PARAM_ERR;
    }
    switch(lvl)
    {
        case CM_LOG_LVL_ERR:
        case CM_LOG_LVL_WARNING:
        case CM_LOG_LVL_INFO:
        case CM_LOG_LVL_DEBUG:
            break;
        default:
            return CM_PARAM_ERR;
    }
    g_CmLogLvls[Module] = lvl;
    return CM_OK;
}

sint32 cm_log_level_get(cm_moudule_e Module, uint8 *plvl)
{
    if(Module >= CM_MOD_BUTT)
    {
        return CM_PARAM_ERR;
    }
    *plvl = g_CmLogLvls[Module];
    return CM_OK;
}

sint32 cm_log_all_mode_set(bool_t isEnabel)
{
    g_CmLogAllEnable = isEnabel;
    return CM_OK;
}

bool_t cm_log_all_mode_get(void)
{
    return g_CmLogAllEnable;
}

void cm_log_backup(void)
{
    if(CM_FALSE == g_CmLogIsbackup)
    {
        cm_system(CM_SCRIPT_DIR"cm_shell_exec.sh cm_log_clear");
    }
    else
    {
        cm_system(CM_SCRIPT_DIR"cm_shell_exec.sh cm_log_backup");
    }
    return;
}

static void cm_log_write(const char *str)
{
    uint32 file_size  = 0; 
    const char *filename = g_CmLogFile;
    FILE *fp = fopen(filename,"a+");
    if(NULL == fp)
    {
        return;
    }
    
    fprintf(fp,"%s",str);
    
    fseek(fp,0,SEEK_END);
    file_size = ftell(fp);
    fclose(fp);
    if(file_size <= CM_LOG_FILE_MAX_SIZE)
    {
        return;
    }
    cm_log_backup();
    return;
}

static void* cm_log_thread(void* pArg)
{
    cm_queue_t *pQueue = pArg;
    sint32 iRet = CM_FAIL;
    uint8 *pMsg = NULL;

    while(1)
    {
        iRet = cm_queue_get(pQueue, (void**)&pMsg);
        if(CM_OK != iRet)
        {
            continue;
        }
        cm_log_write((char*)pMsg);
        free(pMsg);
        pMsg = NULL;
    }

    return NULL;
}

void cm_log_backup_set(bool_t isEnabel)
{
    g_CmLogIsbackup = isEnabel;
}

bool_t cm_log_backup_get(void)
{
    return g_CmLogIsbackup;
}


