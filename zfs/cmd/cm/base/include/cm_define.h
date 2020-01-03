/******************************************************************************
 *          Copyright (c) 2017 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_define.h
 * author     : wbn
 * create date: 2017Äê10ÔÂ26ÈÕ
 * description: TODO:
 *
 *****************************************************************************/
#ifndef BASE_INCLUDE_CM_DEFINE_H_
#define BASE_INCLUDE_CM_DEFINE_H_

#define CM_ON   1
#define CM_OFF  0

#define CM_NODE_ID_NONE 0x00
#define CM_MASTER_SUBDOMAIN_ID 0x00
#define CM_NODE_ID_LOCAL CM_NODE_ID_NONE

#define CM_STRING_32 32
#define CM_STRING_64 64
#define CM_STRING_128 128
#define CM_STRING_256 256
#define CM_STRING_512 512
#define CM_STRING_1K 1024
#define CM_STRING_2K 2048
#define CM_STRING_4K 4096
#define CM_STRING_8K 8092



/* ================= start define for cm_log ================================*/
#define CM_LOG_MODE_NORMAL 0
#define CM_LOG_MODE_CMD 1

#define CM_LOG_LVL_ERR      3
#define CM_LOG_LVL_WARNING  4
#define CM_LOG_LVL_INFO     6
#define CM_LOG_LVL_DEBUG    7
#define CM_LOG_LVL_NUM      8

/* ================= end define for cm_log ==================================*/
#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64
#endif

#endif /* BASE_INCLUDE_CM_DEFINE_H_ */
