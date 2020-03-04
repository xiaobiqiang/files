/******************************************************************************
 *          Copyright (c) 2018 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cfg_global.h
 * author     : wbn
 * create date: 2018Äê1ÔÂ8ÈÕ
 * description: TODO:
 *
 *****************************************************************************/
#ifndef CMD_CONFIG_CM_CFG_GLOBAL_H_
#define CMD_CONFIG_CM_CFG_GLOBAL_H_

#include "cm_define.h"

/* ================= start CFG for cm_log ===================================*/
#define CM_LOG_MODE CM_LOG_MODE_CMD
#define CM_LOG_LVL CM_LOG_LVL_ERR
#define CM_LOG_FILE "ceres_exec.log"

/* ================= end CFG for cm_log =====================================*/

/* ================= start CFG for cm_rpc ===================================*/
#define CM_RPC_SUPPORT_SERVER CM_OFF
/* ================= end   CFG for cm_rpc ===================================*/
#define CM_CLIENT_SUPPORT_MUTIL_REQ CM_OFF


#endif /* CMD_CONFIG_CM_CFG_GLOBAL_H_ */
