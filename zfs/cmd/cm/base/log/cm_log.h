/******************************************************************************
 *          Copyright (c) 2017 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_log.h
 * author     : wbn
 * create date: 2017Äê10ÔÂ23ÈÕ
 * description: TODO:
 *
 *****************************************************************************/
#ifndef BASE_LOG_CM_LOG_H_
#define BASE_LOG_CM_LOG_H_
#include "cm_define.h"
#include "cm_common.h"

extern sint32 cm_log_init(uint8 logmod);

extern void cm_log_print(cm_moudule_e Module,uint8 lvl, const sint8 *pFunction, sint32 Line, const sint8
*pFormat,...);

extern sint32 cm_log_level_set(cm_moudule_e Module, uint8 lvl);

extern sint32 cm_log_level_get(cm_moudule_e Module, uint8 *plvl);

extern sint32 cm_log_all_mode_set(bool_t isEnabel);

extern bool_t cm_log_all_mode_get(void);

extern void cm_log_backup_set(bool_t isEnabel);

extern bool_t cm_log_backup_get(void);

extern void cm_log_backup(void);

#define CM_LOG_ERR(MOD,...) \
    cm_log_print(MOD, CM_LOG_LVL_ERR,__FUNCTION__,__LINE__,__VA_ARGS__)

#define CM_LOG_WARNING(MOD, ...) \
    cm_log_print(MOD, CM_LOG_LVL_WARNING,__FUNCTION__,__LINE__,__VA_ARGS__)

#define CM_LOG_INFO(MOD, ...) \
    cm_log_print(MOD, CM_LOG_LVL_INFO,__FUNCTION__,__LINE__,__VA_ARGS__)

#define CM_LOG_DEBUG(MOD, ...) \
    cm_log_print(MOD, CM_LOG_LVL_DEBUG,__FUNCTION__,__LINE__,__VA_ARGS__)


#endif /* BASE_LOG_CM_LOG_H_ */
