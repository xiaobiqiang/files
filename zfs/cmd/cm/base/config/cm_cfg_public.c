/******************************************************************************
 *          Copyright (c) 2017 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cfg_public.c
 * author     : wbn
 * create date: 2017Äê10ÔÂ26ÈÕ
 * description: TODO:
 *
 *****************************************************************************/
#include "cm_common.h"

const sint8 *g_CmLogModuleNames[CM_MOD_BUTT] =
{
    "None",
    "Log",
    "Queue",
    "Db",
    "Rpc",
    "Cmt",
    "Omi",
    "Node",
    "Htbt",
    "Cnm",
    "Sync",
    "Pmm",
    "Cmd",
    "Alarm",
    "User",
    "Sche",
    "Task",
};

const sint8 *g_CmLogLevelNames[CM_LOG_LVL_NUM] =
{
    "NONE","lvl1","lvl2","ERR","WARN","lvl5","INFO","DEBUG"
};

cm_system_version_e g_cm_sys_ver=CM_SYS_VER_DEFAULT;


