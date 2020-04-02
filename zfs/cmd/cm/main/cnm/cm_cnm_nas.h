/******************************************************************************
 *          Copyright (c) 2018 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cnm_nas.h
 * author     : wbn
 * create date: 2018.07.07
 * description: TODO:
 *
 *****************************************************************************/
#ifndef MAIN_CNM_CM_CNM_NAS_H_
#define MAIN_CNM_CM_CNM_NAS_H_
#include "cm_cnm_common.h"


/*===========================================================================*/

typedef struct
{
    sint8 name[CM_NAME_LEN_NAS];
    sint8 pool[CM_NAME_LEN_POOL];
    uint32 nid;
    uint16 blocksize;
    uint16 access; /*权限*/
    uint8 write_policy;
    
    uint8 is_compress;
    uint8 dedup;
    uint8 smb;
    
    sint8 qos_val[CM_STRING_32];    
    sint8 space_avail[CM_STRING_32];
    sint8 space_used[CM_STRING_32];
    sint8 quota[CM_STRING_32];   
    sint8 qos_avs[CM_STRING_32]; 

    uint8 abe;
    uint8 aclinherit;
}cm_cnm_nas_info_t;

extern sint32 cm_cnm_nas_init(void);

extern sint32 cm_cnm_nas_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam);
    
extern cm_omi_obj_t cm_cnm_nas_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen);

extern sint32 cm_cnm_nas_getbatch(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_nas_get(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
   
extern sint32 cm_cnm_nas_create(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
    
extern sint32 cm_cnm_nas_update(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
    
extern sint32 cm_cnm_nas_count(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
    
extern sint32 cm_cnm_nas_delete(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_nas_local_getbatch(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_nas_local_get(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_nas_local_count(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_nas_local_create(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_nas_local_update(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);    

extern sint32 cm_cnm_nas_local_delete(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern void cm_cnm_nas_oplog_create(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result);   

extern void cm_cnm_nas_oplog_update(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result);  

extern void cm_cnm_nas_oplog_delete(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result);

typedef struct
{    
    sint8 nfs_dir_path[CM_NAS_PATH_LEN]; //文件的路径
    uint32 nid; 
    uint32 nfs_limit; //共享的权限
    sint8 nfs_ip[CM_IP_LEN]; //主机ip
}cm_cnm_nfs_info_t;//nfs

extern sint32 cm_cnm_nfs_init(void);

extern sint32 cm_cnm_nfs_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam);

extern cm_omi_obj_t cm_cnm_nfs_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen);

extern sint32 cm_cnm_nfs_getbatch(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
  
extern sint32 cm_cnm_nfs_add(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
    
extern sint32 cm_cnm_nfs_count(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
    
extern sint32 cm_cnm_nfs_delete(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_nfs_local_getbatch(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_nfs_local_count(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_nfs_local_add(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_nfs_local_delete(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern void cm_cnm_nfs_oplog_add(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result);

extern void cm_cnm_nfs_oplog_delete(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result);

/*===========================================================================*/


/*============================================================================*/
/* CIFS */

typedef struct
{
    sint8 name[CM_NAME_LEN];
    uint8 name_type;   /* cm_name_type_e */
    uint8 domain_type; /* cm_domain_type_e */
    uint8 permission;  /* cm_nas_permission_e */ 
}cm_cnm_cifs_info_t;

typedef struct
{
    sint8 dir[CM_NAME_LEN_DIR];
    cm_cnm_cifs_info_t info;
}cm_cnm_cifs_param_t;

extern sint32 cm_cnm_cifs_init(void);

extern sint32 cm_cnm_cifs_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam);
    
extern cm_omi_obj_t cm_cnm_cifs_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen);

extern sint32 cm_cnm_cifs_getbatch(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_cifs_get(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
   
extern sint32 cm_cnm_cifs_create(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
    
extern sint32 cm_cnm_cifs_update(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
    
extern sint32 cm_cnm_cifs_count(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
    
extern sint32 cm_cnm_cifs_delete(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_cifs_local_getbatch(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_cifs_local_get(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_cifs_local_count(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_cifs_local_create(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_cifs_local_update(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);    

extern sint32 cm_cnm_cifs_local_delete(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern void cm_cnm_cifs_oplog_create(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result);   

extern void cm_cnm_cifs_oplog_update(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result);  

extern void cm_cnm_cifs_oplog_delete(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result);


/**************************************************
                    nas_shadow
***************************************************/

typedef struct 
{
    uint32 status;
    uint32 nid;
    sint8 shadow_status[CM_STRING_128];
    sint8 mirror_status[CM_STRING_128];
    sint8 dst[CM_STRING_128];
    sint8 src[CM_STRING_128];
    sint8 mount[CM_STRING_128];
}cm_cnm_nas_shadow_info_t;

extern sint32 cm_cnm_nas_shadow_init(void);

extern sint32 cm_cnm_nas_shadow_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam);
    
extern cm_omi_obj_t cm_cnm_nas_shadow_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen);

extern sint32 cm_cnm_nas_shadow_get(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_nas_shadow_getbatch(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_nas_shadow_count(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_nas_shadow_create(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
    
extern sint32 cm_cnm_nas_shadow_update(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_nas_shadow_delete(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_nas_shadow_local_get(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_nas_shadow_local_getbatch(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen); 

extern sint32 cm_cnm_nas_shadow_local_count(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen); 

extern sint32 cm_cnm_nas_shadow_local_create(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_nas_shadow_local_update(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen); 

extern sint32 cm_cnm_nas_shadow_local_delete(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen); 

extern void cm_cnm_nas_shadow_oplog_create(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result);   

extern void cm_cnm_nas_shadow_oplog_update(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result);  

extern void cm_cnm_nas_shadow_oplog_delete(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result);


/*************lowdata****************/

typedef struct
{
    uint32 status;
    uint32 nid;
    uint32 time_unit;
    uint32 criteria;
    uint32 timeout;
    uint32 delete_time;
    uint32 switchs;
    uint32 process;
    sint8 name[CM_STRING_256];
}cm_cnm_lowdata_info_t;

extern sint32 cm_cnm_lowdata_init(void);

extern sint32 cm_cnm_lowdata_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam);
    
extern cm_omi_obj_t cm_cnm_lowdata_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen);

extern sint32 cm_cnm_lowdata_getbatch(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_lowdata_get(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_lowdata_update(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_lowdata_count(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_lowdata_local_getbatch(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen); 

extern sint32 cm_cnm_lowdata_local_get(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen); 

extern sint32 cm_cnm_lowdata_local_count(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen); 

extern sint32 cm_cnm_lowdata_local_update(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern void cm_cnm_lowdata_oplog_update(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result);

/***************dirlowdata***********************/

typedef struct
{
    uint32 status;
    uint32 nid;
    uint32 time_unit;
    uint32 criteria;
    uint32 timeout;
    uint32 delete_time;
    uint32 switchs;
    sint8 name[CM_STRING_256];
    sint8 dir[CM_STRING_256];
    sint8 cluster[CM_STRING_128];
}cm_cnm_dirlowdata_info_t;

extern sint32 cm_cnm_dirlowdata_init(void);

extern sint32 cm_cnm_dirlowdata_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam);
    
extern cm_omi_obj_t cm_cnm_dirlowdata_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen);

extern sint32 cm_cnm_dirlowdata_getbatch(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_dirlowdata_get(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_dirlowdata_update(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_dirlowdata_count(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_dirlowdata_local_getbatch(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen); 

extern sint32 cm_cnm_dirlowdata_local_get(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen); 


extern sint32 cm_cnm_dirlowdata_local_count(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen); 

extern sint32 cm_cnm_dirlowdata_local_update(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern void cm_cnm_dirlowdata_oplog_update(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result);

/************lowdata sche**************/

extern sint32 cm_cnm_lowdata_sche_cbk(const sint8* name, const sint8* param);
extern sint32 cm_cnm_lowdata_sche_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam);
    
extern cm_omi_obj_t cm_cnm_lowdata_sche_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen);

extern sint32 cm_cnm_lowdata_sche_getbatch(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
   
extern sint32 cm_cnm_lowdata_sche_create(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
    
extern sint32 cm_cnm_lowdata_sche_update(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
    
extern sint32 cm_cnm_lowdata_sche_count(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
    
extern sint32 cm_cnm_lowdata_sche_delete(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern void cm_cnm_lowdata_sche_oplog_create(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result);   

extern void cm_cnm_lowdata_sche_oplog_update(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result);  

extern void cm_cnm_lowdata_sche_oplog_delete(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result);


/******************************************************************************/
typedef struct
{
    uint32 nid;
    sint8 ipaddr[CM_IP_LEN];
    sint8 proto[CM_STRING_32];
}cm_cnm_nas_client_info_t;

extern sint32 cm_cnm_nas_client_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam);
    
extern cm_omi_obj_t cm_cnm_nas_client_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen);

extern sint32 cm_cnm_nas_client_getbatch(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_nas_client_count(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);


/*********************** lowdata_volume ****************************/

typedef struct
{
    uint32 percent;
    uint32 stop;
}cm_cnm_lowdata_volume_info_t;

extern sint32 cm_cnm_lowdata_volume_init(void);

extern sint32 cm_cnm_lowdata_volume_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam);
    
extern cm_omi_obj_t cm_cnm_lowdata_volume_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen);

extern sint32 cm_cnm_lowdata_volume_create(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_lowdata_volume_get(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_lowdata_volume_sync_request(
    uint64 data_id, void *pdata, uint32 len);

extern sint32 cm_cnm_lowdata_volume_sync_get(uint64 data_id, void **pdata, uint32 
*plen);

extern void cm_cnm_lowdata_volume_thread(void);

extern void cm_cnm_lowdata_volume_stop_thread(void);
/********************************nascopy*************************************/

typedef struct 
{
    sint8 nas[CM_NAME_LEN];
    uint32 nid;
    uint32 copynum;
}cm_cnm_nas_copy_info_t;

extern sint32 cm_cnm_nas_copy_init(void);

extern sint32 cm_cnm_nas_copy_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam);
  
extern sint32 cm_cnm_nas_copy_count(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_nas_copy_update(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
#endif /* MAIN_CNM_CM_CNM_NAS_H_ */

