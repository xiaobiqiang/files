/******************************************************************************
 *          Copyright (c) 2018 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_alarm.h
 * author     : wbn
 * create date: 2018Äê1ÔÂ8ÈÕ
 * description: TODO:
 *
 *****************************************************************************/
#ifndef MAIN_ALARM_CM_ALARM_H_
#define MAIN_ALARM_CM_ALARM_H_
#include "cm_common.h"
#include "cm_alarm_types.h"
#include "cm_cfg_global.h"

extern sint32 cm_alarm_init(void);
extern sint32 cm_alarm_report(uint32 alarm_id,const sint8 *param, bool_t is_recovery);

extern sint32 cm_alarm_cbk_cmt(
 void *pMsg, uint32 Len, void** ppAck, uint32 *pAckLen);

extern sint32 cm_alarm_cbk_sync_request(uint64 data_id, void *pdata, uint32 len);
extern sint32 cm_alarm_cbk_sync_get(uint64 data_id, void **pdata, uint32 *plen);
extern sint32 cm_alarm_cbk_sync_delete(uint64 data_id);

extern sint32 cm_alarm_cfg_get(uint32 alarm_id, cm_alarm_config_t *cfg);

#define CM_ALARM_REPORT(alarm_id, param) \
    cm_alarm_report((alarm_id),(param),CM_FALSE)

#define CM_ALARM_RECOVERY(alarm_id, param) \
    cm_alarm_report((alarm_id),(param),CM_TRUE)

extern sint32 cm_alarm_oplog(uint32 alarm_id,const sint8 *param, const sint8 *sessionid);  

extern sint32 cm_alarm_threshold_get(uint32 alarm_id, uint32 *report, uint32 *recovery);

#endif /* MAIN_ALARM_CM_ALARM_H_ */
