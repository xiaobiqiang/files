/******************************************************************************
 *          Copyright (c) 2018 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cnm_user.h
 * author     : xar
 * create date: 2018.08.10
 * description: TODO:
 *
 *****************************************************************************/

#ifndef MAIN_USER_H
#define MAIN_USER_H
#include "cm_cnm_common.h"
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

typedef struct
{   
    uint32 id;
    uint32 gid;
    sint8 name[CM_NAME_LEN];
    sint8 path[CM_NAME_LEN];
    sint8 pwd[CM_NAME_LEN];
}cm_cnm_user_info_t;

typedef struct
{
    bool_t flag;
    sint8 passwd[CM_STRING_256];
    sint8 shadow[CM_STRING_256];
    sint8 smbpasswd[CM_STRING_256];
    cm_cnm_user_info_t user;
}cm_cnm_user_infos_t;

typedef struct
{   
    uint32 id;
    sint8 name[CM_NAME_LEN];
}cm_cnm_group_info_t;

extern sint32 cm_cnm_user_init(void);

extern sint32 cm_cnm_user_decode(
    const cm_omi_obj_t  ObjParam,void** ppDecodeParam);

extern cm_omi_obj_t cm_cnm_user_encode(
    const void* pDecodeParam,void* pAckData,uint32 AckLen);

extern sint32 cm_cnm_user_create(
    const void* pDecodeParam,void** ppAckData,uint32* pAckLen);

extern sint32 cm_cnm_user_delete(
    const void* pDecodeParam,void** ppAckData,uint32* pAckLen);

extern sint32 cm_cnm_user_update(
    const void* pDecodeParam,void** ppAckData,uint32* pAckLen);

extern sint32 cm_cnm_user_get(
    const void* pDecodeParam,void** ppAckData,uint32* pAckLen);

extern sint32 cm_cnm_user_getbatch(
    const void* pDecodeParam,void** ppAckData,uint32* pAckLen);

extern sint32 cm_cnm_user_count(
    const void* pDecodeParam,void** ppAckData,uint32* pAckLen);

extern sint32 cm_cnm_user_sync_request(uint64 data_id, void *pdata, uint32 len);

extern sint32 cm_cnm_user_sync_get(uint64 data_id, void **pdata, uint32 *plen);

extern sint32 cm_cnm_user_sync_delete(uint64 id);

extern void cm_cnm_user_oplog_create(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result);   

extern void cm_cnm_user_oplog_update(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result);  

extern void cm_cnm_user_oplog_delete(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result);


/*group*/

extern sint32 cm_cnm_group_init(void);

extern sint32 cm_cnm_group_decode(
    const cm_omi_obj_t  ObjParam,void** ppDecodeParam);

extern cm_omi_obj_t cm_cnm_group_encode(
    const void* pDecodeParam,void* pAckData,uint32 AckLen);

extern sint32 cm_cnm_group_create(
    const void* pDecodeParam,void** ppAckData,uint32* pAckLen);

extern sint32 cm_cnm_group_delete(
    const void* pDecodeParam,void** ppAckData,uint32* pAckLen);

extern sint32 cm_cnm_group_get(
    const void* pDecodeParam,void** ppAckData,uint32* pAckLen);

extern sint32 cm_cnm_group_getbatch(
    const void* pDecodeParam,void** ppAckData,uint32* pAckLen);

extern sint32 cm_cnm_group_count(
    const void* pDecodeParam,void** ppAckData,uint32* pAckLen);

extern sint32 cm_cnm_group_sync_request(uint64 data_id, void *pdata, uint32 len);

extern sint32 cm_cnm_group_sync_get(uint64 data_id, void **pdata, uint32 *plen);

extern sint32 cm_cnm_group_sync_delete(uint64 id);

extern void cm_cnm_group_oplog_create(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result);   

extern void cm_cnm_group_oplog_delete(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result);  


/********************************************************
                  
*********************************************************/

typedef struct
{
    uint32 nid;
    uint32 permission;
    uint32 type;
    uint64 size;
    cm_time_t atime; 
    cm_time_t mtime; 
    cm_time_t ctime; 
    sint8 name[CM_STRING_256];
    sint8 dir[CM_NAME_LEN_DIR];
    sint8 user[CM_STRING_64];
    sint8 group[CM_STRING_64];
    sint8 find[CM_STRING_64];
    sint8 flag[CM_STRING_64];
}cm_cnm_explorer_info_t;

extern sint32 cm_cnm_explorer_init(void);

extern sint32 cm_cnm_explorer_decode(
    const cm_omi_obj_t  ObjParam,void** ppDecodeParam);

extern cm_omi_obj_t cm_cnm_explorer_encode(
    const void* pDecodeParam,void* pAckData,uint32 AckLen);

extern sint32 cm_cnm_explorer_getbatch(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_explorer_create(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_explorer_count(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_explorer_get(
    const void *pDecodeParam, void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_explorer_local_getbatch(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_explorer_local_count(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);
extern sint32 cm_cnm_explorer_local_get(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);    

extern sint32 cm_cnm_explorer_local_create(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_explorer_delete(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_explorer_local_delete(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_explorer_modify(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_explorer_local_modify(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);


/*************domian  user**************/

typedef struct
{
    sint8 domain_user[CM_NAME_LEN];
    sint8 local_user[CM_NAME_LEN];
}cm_cnm_domain_user_info_t;

extern sint32 cm_cnm_domain_user_init(void);

extern sint32 cm_cnm_domain_user_decode(
    const cm_omi_obj_t  ObjParam,void** ppDecodeParam);

extern cm_omi_obj_t cm_cnm_domain_user_encode(
    const void* pDecodeParam,void* pAckData,uint32 AckLen);

extern sint32 cm_cnm_domain_user_create(
    const void* pDecodeParam,void** ppAckData,uint32* pAckLen);

extern sint32 cm_cnm_domain_user_delete(
    const void* pDecodeParam,void** ppAckData,uint32* pAckLen);

extern sint32 cm_cnm_domain_user_getbatch(
    const void* pDecodeParam,void** ppAckData,uint32* pAckLen);

extern sint32 cm_cnm_domain_user_count(
    const void* pDecodeParam,void** ppAckData,uint32* pAckLen);

extern sint32 cm_cnm_domain_user_sync_request(uint64 data_id, void *pdata, uint32 len);

extern sint32 cm_cnm_domain_user_sync_delete(uint64 id);

extern void cm_cnm_domain_user_oplog_delete(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result);

extern void cm_cnm_domain_user_oplog_create(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result);


typedef struct
{
    uint32 domain;
    uint32 type;
    sint8 id[CM_STRING_32];
    sint8 name[CM_NAME_LEN];
}cm_cnm_ucache_info_t;

extern sint32 cm_cnm_ucache_decode(
    const cm_omi_obj_t  ObjParam,void** ppDecodeParam);

extern cm_omi_obj_t cm_cnm_ucache_encode(
    const void* pDecodeParam,void* pAckData,uint32 AckLen);

extern sint32 cm_cnm_ucache_getbatch(
    const void* pDecodeParam,void** ppAckData,uint32* pAckLen);

extern sint32 cm_cnm_ucache_count(
    const void* pDecodeParam,void** ppAckData,uint32* pAckLen);

extern sint32 cm_cnm_ucache_local_count(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_ucache_local_getbatch(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_ucache_test(
    const void* pDecodeParam,void** ppAckData,uint32* pAckLen);

extern sint32 cm_cnm_ucache_local_test(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);



#endif /* MAIN_USER_H */

