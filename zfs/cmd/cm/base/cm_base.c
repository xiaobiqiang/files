/******************************************************************************
 *          Copyright (c) 2019 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_base.c
 * author     : wbn
 * create date: 2019Äê9ÔÂ29ÈÕ
 * description: TODO:
 *
 *****************************************************************************/
#include "cm_base.h"

const cm_cmt_config_t* g_CmCmtCbksCfg = NULL;
uint32 g_CmCmtMaxId = 0;

const cm_omi_object_cfg_t** g_CmOmiObjectCfgPtr =NULL;
const cm_omi_map_object_t* g_CmOmiObjCfg = NULL;
uint32 g_CmOmiMaxId = 0;
uint32 g_CmOmiSessionObjId = 0;
bool_t g_CmOmiCliMulti = CM_FALSE;
const uint32* g_CmOmiNoCheck = NULL;

uint8 g_CmLogLvlDef = CM_LOG_LVL_WARNING;
const sint8* g_CmLogFile=NULL;

cm_get_node_func_t g_CmGetNodeIdFunc;
cm_get_node_func_t g_CmGetMasterIdFunc;

sint32 cm_base_init(const cm_base_config_t* cfg)
{
    sint32 iRet = CM_OK;
    if(NULL == cfg)
    {
        return CM_FAIL;
    }
    g_CmLogFile = cfg->logfile;
    g_CmLogLvlDef = cfg->loglvldef;
    g_CmGetNodeIdFunc = cfg->getmyid;
    g_CmGetMasterIdFunc = cfg->getmasterid;
    iRet = cm_log_init(cfg->logmode);
    if(CM_OK != iRet)
    {
        return iRet;
    }

    iRet = cm_rpc_init(cfg->isclient);
    if(CM_OK != iRet)
    {
        return iRet;
    }

    if(CM_TRUE==cfg->usecmt)
    {
        g_CmCmtMaxId = cfg->cmtmsgmax;
        g_CmCmtCbksCfg = cfg->cmtcfg;
        iRet = cm_cmt_init(cfg->isclient);
        if(CM_OK != iRet)
        {
            return iRet;
        }
    }

    if(CM_TRUE==cfg->useomi)
    {
        g_CmOmiMaxId = cfg->omiobjmax;
        g_CmOmiObjectCfgPtr = cfg->omiobjcbk;
        g_CmOmiSessionObjId = cfg->omisessionid;
        g_CmOmiObjCfg = cfg->omiobj;
        g_CmOmiCliMulti = cfg->omiclimulti;
        g_CmOmiNoCheck = cfg->ominocheck;
        iRet = cm_omi_init(cfg->isclient);
        if(CM_OK != iRet)
        {
            return iRet;
        }
    }
    
    return CM_OK;
}


