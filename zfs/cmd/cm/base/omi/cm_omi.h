/******************************************************************************
 *          Copyright (c) 2017 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_omi.h
 * author     : wbn
 * create date: 2017Äê10ÔÂ23ÈÕ
 * description: TODO:
 *
 *****************************************************************************/
#ifndef BASE_OMI_CM_OMI_H_
#define BASE_OMI_CM_OMI_H_
#include "cm_common.h"
#include "cm_omi_types.h"

extern sint32 cm_omi_init(bool_t isclient);

extern sint32 cm_omi_close(void);

extern void cm_omi_reconnect(void);

extern void cm_omi_session_cbk_reg(cm_omi_cbk_session_t cbk);

extern void cm_omi_level_cbk_reg(cm_omi_cbk_level_t cbk);

extern sint32 cm_omi_request(cm_omi_obj_t req, cm_omi_obj_t *pAck, uint32 timeout);

extern sint32 cm_omi_request_str(const sint8 *pReq, sint8 **ppAckData, uint32 *pAckLen, uint32 timeout);

extern void cm_omi_free(sint8 *pAckData);

extern cm_omi_obj_t cm_omi_encode_count(const void *pDecodeParam, void *pAckData, uint32
AckLen);

extern cm_omi_obj_t    cm_omi_obj_parse(const sint8 *pStrData);
extern cm_omi_obj_t    cm_omi_obj_new(void);
extern cm_omi_obj_t    cm_omi_obj_new_array(void);
extern uint32              cm_omi_obj_array_size(cm_omi_obj_t obj);
extern sint32              cm_omi_obj_array_add(cm_omi_obj_t obj, cm_omi_obj_t val);
extern cm_omi_obj_t    cm_omi_obj_array_idx(cm_omi_obj_t obj, uint32 index);
extern void             cm_omi_obj_delete(cm_omi_obj_t obj);
extern const sint8*        cm_omi_obj_tostr(cm_omi_obj_t obj);
extern const sint8*        cm_omi_obj_value(cm_omi_obj_t obj);
extern cm_omi_obj_t    cm_omi_obj_key_find(cm_omi_obj_t obj, const sint8* key);
extern sint32 cm_omi_obj_key_add(cm_omi_obj_t obj, const sint8* key, cm_omi_obj_t subobj);
extern void cm_omi_obj_key_delete(cm_omi_obj_t obj, const sint8* key);
extern sint32 cm_omi_obj_key_set_str(cm_omi_obj_t obj, const sint8* key, const sint8* val);
extern sint32 cm_omi_obj_key_get_str(cm_omi_obj_t obj, const sint8* key, sint8* val, uint32 len);

extern sint32 cm_omi_obj_key_get_s32(cm_omi_obj_t obj, const sint8* key, sint32 *val);
extern sint32 cm_omi_obj_key_set_s32(cm_omi_obj_t obj, const sint8* key, sint32 val);
extern sint32 cm_omi_obj_key_get_u32(cm_omi_obj_t obj, const sint8* key, uint32 *val);
extern sint32 cm_omi_obj_key_set_u32(cm_omi_obj_t obj, const sint8* key, uint32 val);
extern sint32 cm_omi_obj_key_get_u64(cm_omi_obj_t obj, const sint8* key, uint64 *val);
extern sint32 cm_omi_obj_key_set_u64(cm_omi_obj_t obj, const sint8* key, uint64 val);


extern sint32 cm_omi_obj_key_set_str_ex(cm_omi_obj_t obj, uint32 key, const sint8* val);
extern sint32 cm_omi_obj_key_get_str_ex(cm_omi_obj_t obj, uint32 key, sint8* val, uint32 len);

extern sint32 cm_omi_obj_key_get_s32_ex(cm_omi_obj_t obj, uint32 key, sint32 *val);
extern sint32 cm_omi_obj_key_set_s32_ex(cm_omi_obj_t obj, uint32 key, sint32 val);
extern sint32 cm_omi_obj_key_get_u32_ex(cm_omi_obj_t obj, uint32 key, uint32 *val);
extern sint32 cm_omi_obj_key_set_u32_ex(cm_omi_obj_t obj, uint32 key, uint32 val);
extern sint32 cm_omi_obj_key_get_u64_ex(cm_omi_obj_t obj, uint32 key, uint64 *val);
extern sint32 cm_omi_obj_key_set_u64_ex(cm_omi_obj_t obj, uint32 key, uint64 val);
extern cm_omi_obj_t cm_omi_obj_key_find_ex(cm_omi_obj_t obj, uint32 key);

extern sint32 cm_omi_obj_key_set_double_ex(cm_omi_obj_t obj, uint32 key, double val);

extern uint32 cm_omi_get_fields(const sint8 *fields_str, sint8 **fields, uint32 max);

extern const cm_omi_map_object_t* cm_omi_get_map_obj_cfg(uint32 obj);

#define CM_OMI_FIELDS_FLAG_SET(p,key) \
    ((p)->bits[(key)>>3] |= 1 << ((key) & 0x07))

#define CM_OMI_FIELDS_FLAG_ISSET(p,key) \
    ((p)->bits[(key)>>3] & (1 << ((key) & 0x07)))

#define CM_OMI_FIELDS_FLAG_CLR(p,key) \
    ((p)->bits[(key)>>3] &= ~(1 << ((key) & 0x07)))

#define CM_OMI_FIELDS_FLAG_CLR_ALL(p) CM_MEM_ZERO(p,sizeof(*(p)))
#define CM_OMI_FIELDS_FLAG_SET_ALL(p) memset((p), 0xFF, sizeof(*(p)))

extern void cm_omi_decode_fields_flag(cm_omi_obj_t obj_param, 
    cm_omi_field_flag_t *pflag);

extern sint32 cm_omi_encode_db_record_each(
    void *arg, sint32 col_cnt, sint8 **col_vals, sint8 **col_names);

extern void cm_omi_make_select_field(const sint8** all_fields, uint32 cnt,
    const cm_omi_field_flag_t *set_flag,sint8 *sql, uint32 buf_size);
    
extern void cm_omi_make_select_cond(
    const uint32 *col_ids, 
    const sint8** col_names,
    const uint32 *col_vals,
    uint32 col_cnt,
    const cm_omi_field_flag_t *set_flag,sint8 *sql, uint32 buf_size);

extern sint32 cm_omi_check_permission(uint32 obj, uint32 cmd);    
extern sint32 cm_omi_check_permission_obj(uint32 obj);

extern sint32 cm_omi_remote_connect(const sint8* ipaddr);

extern sint32 cm_omi_remote_request(sint32 handle,const sint8 *pReq, 
    sint8 **ppAckData, uint32 *pAckLen, uint32 timeout);

extern sint32 cm_omi_remote_close(sint32 handle);

extern cm_omi_obj_t cm_omi_encode_errormsg(
    const void *pDecodeParam, void *pAckData, uint32 AckLen);

extern cm_omi_obj_t cm_omi_encode_taskid(
    const void *pDecodeParam, void *pAckData, uint32 AckLen);


#endif /* BASE_OMI_CM_OMI_H_ */
