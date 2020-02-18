/******************************************************************************
 *          Copyright (c) 2017 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_common.h
 * author     : wbn
 * create date: 2017Äê10ÔÂ23ÈÕ
 * description: TODO:
 *
 *****************************************************************************/
#ifndef BASE_COMMON_CM_COMMON_H_
#define BASE_COMMON_CM_COMMON_H_

#include "cm_define.h"
#include "cm_cfg_public.h"
#include "cm_platform.h"
#include "cm_sys.h"
#include "cm_types.h"

extern cm_time_t cm_get_time(void);
extern sint32 cm_set_time(cm_time_t tval);

/* format: 20171211180500 */
extern cm_time_t cm_mktime(const sint8 *strtime);

extern void cm_get_datetime(struct tm *tin);
extern void cm_get_datetime_ext(struct tm *tin,cm_time_t utc);
extern void cm_utc_to_str(cm_time_t utc, sint8 *str, uint32 len);

/* 20171229172330 => 20171229170000*/
extern cm_time_t cm_get_time_hour_floor(cm_time_t utc);

extern sint32 cm_exec_tmout(sint8 *buff, sint32 size,sint32 tmout, const sint8* cmdforamt,...);
extern sint32 cm_system_in(uint32 tmout, const sint8* cmdforamt,...);

//extern sint32 cm_exec_cmd_for_str(const sint8 *cmd, sint8 *buff, sint32 size, sint32 tmout);

#define cm_exec_cmd_for_str(cmd,buff,size,tmout) cm_exec_tmout(buff,size,tmout,cmd)

#define cm_exec_cmd(cmd, tmout) cm_system_in(tmout,cmd);

extern uint32 cm_exec_cmd_for_num(const sint8 *cmd);

extern sint32 cm_get_local_nid(uint32 *pNid);

extern uint32 cm_ipaddr_to_nid(const sint8 *pAddr);
extern uint32 cm_get_local_nid_x(void);
extern sint32 cm_regex_check(const sint8* str, const sint8* pattern);
extern sint32 cm_regex_check_num(const sint8* strnum,uint8 Bytes);

//extern sint32 cm_exec(sint8 *buff, sint32 size,const sint8* cmdforamt,...);
#define cm_exec(buff,size,...) cm_exec_tmout(buff,size,CM_CMT_REQ_TMOUT,__VA_ARGS__)

extern double cm_exec_double(const sint8* cmdforamt,...);
extern sint32 cm_exec_int(const sint8* cmdforamt,...);

#define cm_system(...)  cm_system_in(CM_CMT_REQ_TMOUT,__VA_ARGS__)
#define cm_system_no_tmout(...)  cm_system_in(CM_CMT_REQ_TMOUT_NEVER,__VA_ARGS__)


extern bool_t cm_file_exist(const sint8* filename);

extern sint32 cm_get_peer_addr(sint32 fd, sint8 *buff, sint32 size);

extern const sint8* cm_get_peer_addr_str(sint32 fd);

extern sint32 cm_get_md5sum(const sint8* str,sint8 *buff, sint32 size);

extern sint32 cm_exec_get_subpid(const sint8 *cmd, uint32 *subPID);

#define CM_MAX(a,b) ((a)>(b)?(a):(b))
#define CM_MIN(a,b) ((a)>(b)?(b):(a))

#define CM_ARGS_TO_BUF(buf,buf_size, cmdformat) \
    do{\
        va_list args_; \
        va_start(args_, (cmdformat)); \
        (void)vsnprintf((buf),(buf_size)-1,(cmdformat),args_); \
        va_end(args_); \
    }while(0)

#define cm_check_char(c) (((c)>='!' && (c)<='~') || ((c)&0x80))

extern sint32 cm_thread_start(cm_thread_func_t thread, void* arg);

typedef sint32 (*cm_exec_cbk_col_t)(void *arg, sint8 **cols, uint32 col_num);

extern uint32 cm_get_cols(sint8 *col_line, sint8 **cols, uint32 max_cols);

extern sint32 cm_exec_get_list(cm_exec_cbk_col_t cbk, 
    uint32 row_offset, uint32 each_size,void **ppbuff,uint32 *row_cnt,
    const sint8 *cmdformat, ...);

extern uint64 cm_hrtime_delta(cm_hrtime_t old, cm_hrtime_t new);

#endif /* BASE_COMMON_CM_COMMON_H_ */
