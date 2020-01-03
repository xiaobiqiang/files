/******************************************************************************
 *          Copyright (c) 2017 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cfg_local.h
 * author     : wbn
 * create date: 2017Äê10ÔÂ26ÈÕ
 * description: TODO:
 *
 *****************************************************************************/
#ifndef MAIN_CONFIG_CM_CFG_LOCAL_H_
#define MAIN_CONFIG_CM_CFG_LOCAL_H_

/* ================= start CFG for cm_htbt ==================================*/
#define CM_HTBT_SUBDOMAIN_NODE_MAX 32

#define CM_HTBT_SUBDOMAIN_MAX 8

#define CM_HTBT_MASTER_SUBDUMAIN_ID 0

#define CM_HTBT_MSG_PERIOD_LOCAL 1

#define CM_HTBT_MSG_PERIOD_SUBDOMAIN 2

#define CM_HTBT_STABLE_NUM 5
#define CM_HTBT_LONELY_NUM 3

#define CM_HTBT_OFFLINE_NUM 20

#define CM_HTBT_OFFLINE_LONG_NUM 300

/* ================= end CFG for cm_htbt  ===================================*/

/* ================= start CFG for cm_sync ==================================*/
#define CM_SYNC_SEND_QUEUE_LEN 64
#define CM_SYNC_RECV_QUEUE_LEN 64

#define CM_SYNC_SEND_TMOUT 10

/* ================= end   CFG for cm_sync ==================================*/


#endif /* MAIN_CONFIG_CM_CFG_LOCAL_H_ */
