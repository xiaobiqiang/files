/******************************************************************************
 *          Copyright (c) 2018 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cnm_pool.c
 * author     : wbn
 * create date: 2018.05.07
 * description: TODO:
 *
 *****************************************************************************/
#include "cm_cnm_common.h"
#include "cm_log.h"
#include "cm_omi.h"
#include "cm_cnm_pool.h"
#include "cm_node.h"
#include "cm_xml.h"

/*==============================================================================
zpool create [-o autoreplace=on -f] <poolname> <raid> <disk1> [<disk2> ...]
zpool add [-f] <poolname> <raid> <disk1> [<disk2> ...]
zpool 
==============================================================================*/
extern const cm_omi_map_enum_t CmOmiMapEnumPoolStatusType;
extern const cm_omi_map_enum_t CmCnmPoolMapErrCfg;
extern const cm_omi_map_enum_t CmCnmPoolMapDiskTypeCfg;
extern const cm_omi_map_enum_t CmOmiMapEnumPoolRaidType;
extern const cm_omi_map_enum_t CmCnmPoolMapDiskTypeUserCfg;

#define CM_CNM_POOL_READ_FROM_FILE CM_TRUE
#if(CM_FALSE == CM_CNM_POOL_READ_FROM_FILE)
static const sint8* g_cm_cnm_pool_list_cmd = "zpool list -H -o name,health";
/* zpool status 命令会生成/tmp/pool.xml 
防止并发操作，加锁
*/
static cm_mutex_t g_cm_cnm_pool_mutex;

#else
/* FMD模块维护，CM这边只读取 */
#define CM_CNM_POOL_FILE "/tmp/pool.xml"
#endif
const cm_omi_map_cfg_t g_cm_raid_map[] = 
{
    {"",CM_RAID0},
    {"mirror",CM_RAID1},
    {"raidz1",CM_RAID5},
    {"raidz2",CM_RAID6},
    {"raidz3",CM_RAID7},
    {"raidz_s1",CM_RAIDZ5},
    {"raidz_s2",CM_RAIDZ6},
    {"raidz_s3",CM_RAIDZ7},
};

const sint8 * g_cm_cnm_pool_get_scrub_status = "echo `zpool %s %s`|"
    "awk -F: '{i = 1;while(i <= NF){if($i~/^ scrub..../)"
    "{print $i\":\"$++i\":\"$++i}i++}}'";

extern void cm_cnm_disk_set_busy(sint8 *diskids);
extern sint32 cm_cnm_disk_get_sn(const sint8* id,sint8* sn, uint32 len);
extern sint32 cm_cnm_disk_get_id(const sint8* sn,sint8* id, uint32 len);
extern uint32 cm_cnm_disk_enid_exchange(uint32 enid);
extern void* cm_cnm_disk_update_thread(void* arg);

sint32 cm_cnm_pool_init(void)
{
#if(CM_FALSE == CM_CNM_POOL_READ_FROM_FILE)
    CM_MUTEX_INIT(&g_cm_cnm_pool_mutex);
#endif    
    return CM_OK;
}
    
sint32 cm_cnm_pool_decode(const cm_omi_obj_t ObjParam, void **ppDecodeParam)
{
    cm_cnm_pool_list_param_t *param = CM_MALLOC(sizeof(cm_cnm_pool_list_param_t));

    if(NULL == param)
    {
        CM_LOG_ERR(CM_MOD_CNM,"malloc fail");
        return CM_FAIL;
    }
    CM_MEM_ZERO(param,sizeof(cm_cnm_pool_list_param_t));
    param->nid = CM_NODE_ID_NONE;
    param->offset = 0;
    param->total = CM_CNM_MAX_RECORD;
    if(CM_OK == cm_omi_obj_key_get_u32_ex(ObjParam,CM_OMI_FIELD_POOL_NID,&param->nid))
    {
        CM_OMI_FIELDS_FLAG_SET(&param->set,CM_OMI_FIELD_POOL_NID);
    }
    
    if(CM_OK == cm_omi_obj_key_get_u32_ex(ObjParam,CM_OMI_FIELD_FROM,&param->offset))
    {
        CM_OMI_FIELDS_FLAG_SET(&param->set,CM_OMI_FIELD_FROM);
    }
    if(CM_OK == cm_omi_obj_key_get_u32_ex(ObjParam,CM_OMI_FIELD_TOTAL,&param->total))
    {
        CM_OMI_FIELDS_FLAG_SET(&param->set,CM_OMI_FIELD_TOTAL);
    }
    if(CM_OK == cm_omi_obj_key_get_u32_ex(ObjParam,CM_OMI_FIELD_POOL_RAID,&param->raid))
    {
        CM_OMI_FIELDS_FLAG_SET(&param->set,CM_OMI_FIELD_POOL_RAID);
    }
    if(CM_OK == cm_omi_obj_key_get_u32_ex(ObjParam,CM_OMI_FIELD_POOL_GROUP,&param->group))
    {
        CM_OMI_FIELDS_FLAG_SET(&param->set,CM_OMI_FIELD_POOL_GROUP);
    }
    if(CM_OK == cm_omi_obj_key_get_u32_ex(ObjParam,CM_OMI_FIELD_POOL_OPS,&param->operation))
    {
        CM_OMI_FIELDS_FLAG_SET(&param->set,CM_OMI_FIELD_POOL_OPS);
    }
    if(CM_OK == cm_omi_obj_key_get_str_ex(ObjParam,CM_OMI_FIELD_PARAM,param->disk_ids,sizeof(param->disk_ids)))
    {
        CM_OMI_FIELDS_FLAG_SET(&param->set,CM_OMI_FIELD_PARAM);
    }
    if(CM_OK == cm_omi_obj_key_get_str_ex(ObjParam,CM_OMI_FIELD_POOL_NAME,param->name,sizeof(param->name)))
    {
        CM_OMI_FIELDS_FLAG_SET(&param->set,CM_OMI_FIELD_POOL_NAME);
    }
    if(CM_OK == cm_omi_obj_key_get_str_ex(ObjParam,CM_OMI_FIELD_POOL_VAR,param->args,sizeof(param->args)))
    {
        CM_OMI_FIELDS_FLAG_SET(&param->set,CM_OMI_FIELD_POOL_VAR);
    }

    if(0 == param->group)
    {
        param->group = 1;
    }
    *ppDecodeParam = param;
    return CM_OK;
}

static void cm_cnm_pool_encode_each(cm_omi_obj_t item,void *eachdata,void *arg)
{
    cm_cnm_pool_list_t *pool = eachdata;
    (void)cm_omi_obj_key_set_str_ex(item,CM_OMI_FIELD_POOL_NAME,pool->name);
    (void)cm_omi_obj_key_set_u32_ex(item,CM_OMI_FIELD_POOL_NID,pool->nid);
    (void)cm_omi_obj_key_set_u32_ex(item,CM_OMI_FIELD_POOL_NID_SRC,cm_node_get_id_by_inter(pool->src_nid));
    (void)cm_omi_obj_key_set_u32_ex(item,CM_OMI_FIELD_POOL_STATUS,pool->status);
    (void)cm_omi_obj_key_set_str_ex(item,CM_OMI_FIELD_POOL_AVAIL,pool->avail);
    (void)cm_omi_obj_key_set_str_ex(item,CM_OMI_FIELD_POOL_USED,pool->used);
    (void)cm_omi_obj_key_set_str_ex(item,CM_OMI_FIELD_POOL_RDCIP,pool->rdcip);
    return;
}

cm_omi_obj_t cm_cnm_pool_encode(const void *pDecodeParam, void *pAckData, uint32 AckLen)
{
    return cm_cnm_encode_comm(pAckData,AckLen,
        sizeof(cm_cnm_pool_list_t),cm_cnm_pool_encode_each,NULL);
}

static void cm_cnm_pool_status_encode_each(cm_omi_obj_t item,void *eachdata,void *arg)
{
    cm_cnm_pool_list_t *pool = eachdata;
    (void)cm_omi_obj_key_set_double_ex(item,CM_OMI_FIELD_POOL_PROG,pool->prog);
    (void)cm_omi_obj_key_set_double_ex(item,CM_OMI_FIELD_POOL_SPEED,pool->speed);
    (void)cm_omi_obj_key_set_u32_ex(item,CM_OMI_FIELD_POOL_REPAIRED,pool->repaired);
    (void)cm_omi_obj_key_set_u32_ex(item,CM_OMI_FIELD_POOL_ERRORS,pool->errors);
    (void)cm_omi_obj_key_set_u32_ex(item,CM_OMI_FIELD_POOL_ZSTATUS,pool->zstatus);
    (void)cm_omi_obj_key_set_str_ex(item,CM_OMI_FIELD_POOL_TIME,pool->time);
    return;
}


cm_omi_obj_t cm_cnm_pool_status_encode(const void *pDecodeParam, void *pAckData, uint32 AckLen)
{
    return cm_cnm_encode_comm(pAckData,AckLen,
        sizeof(cm_cnm_pool_list_t),cm_cnm_pool_status_encode_each,NULL);
}

static sint32 cm_cnm_pool_requst(uint32 cmd,const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    const cm_cnm_pool_list_param_t *pDecode = pDecodeParam;
    cm_cnm_req_param_t param;
    CM_MEM_ZERO(&param,sizeof(cm_cnm_req_param_t));

    if(NULL == pDecode)
    {
        param.total = CM_CNM_MAX_RECORD;
    }
    else
    {
        param.nid = pDecode->nid;
        param.offset = pDecode->offset;
        param.total = pDecode->total;
        param.param = pDecode;
        param.param_len = sizeof(cm_cnm_pool_list_param_t);
    }
    param.obj = CM_OMI_OBJECT_POOL;
    param.cmd = cmd;
    param.ppack = ppAckData;
    param.acklen = pAckLen;
    param.sbb_only = CM_FALSE;
#if 0  /* 2018.09.15 wbn san双活需要创建同名池 */ 
    if(cmd == CM_OMI_CMD_ADD)
    {
        if(CM_TRUE == cm_cnm_check_exist(&param))
        {
            /* 前面检查过 */
            CM_LOG_ERR(CM_MOD_CNM,"name[%s]",pDecode->name);
            return CM_ERR_POOL_ALREADY_EXISTS;
        }
    }
#endif    
    return cm_cnm_request(&param);
}

sint32 cm_cnm_pool_getbatch(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_cnm_pool_requst(CM_OMI_CMD_GET_BATCH,pDecodeParam,ppAckData,pAckLen);    
}

sint32 cm_cnm_pool_create(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    const cm_cnm_pool_list_param_t *pDecode = pDecodeParam;
    sint32 iRet = CM_OK;
    sint8 disk_ids[CM_STRING_1K] = {0};
    if((NULL == pDecode) || ('\0' == pDecode->name[0]))
    {        
        return CM_PARAM_ERR;
    }

    iRet = cm_cnm_pool_requst(CM_OMI_CMD_ADD,pDecodeParam,ppAckData,pAckLen);
    if(CM_OK == iRet)
    {
        /* 修改问题单 0005426 */
        CM_MEM_CPY(disk_ids,sizeof(disk_ids),pDecode->disk_ids,sizeof(pDecode->disk_ids));
        cm_cnm_disk_set_busy(disk_ids);
    }
    
    return iRet;
}

static uint32 cm_cnm_pool_task_cmd(
    const sint8* cmd,const sint8* pro,
    const sint8* pool,sint8* arg,uint32 task_type,uint32 nid)
{
    uint32 iRet = CM_OK;
    sint8 cmd_buff[CM_STRING_256] =  {0};
    if(NULL == arg)
    {
        arg = "";
    }
    CM_VSPRINTF(cmd_buff,sizeof(cmd_buff),"zpool %s %s %s %s",cmd,pro,pool,arg);
    CM_LOG_WARNING(CM_MOD_CNM,"[%s]",cmd_buff);
    //runing in task
    iRet = cm_task_add(nid, task_type, cmd_buff,cmd);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"[%s] fail [iRet = %d]",cmd_buff,iRet);
    }
    
    return iRet;
}


sint32 cm_cnm_pool_update(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    uint32 iRet = CM_OK;
    cm_cnm_pool_list_param_t *info = pDecodeParam;
    switch(info->operation)
    {
        case CM_POOL_IMPORT:
            iRet = cm_cnm_pool_task_cmd("import","-f",info->name,"",CM_TASK_TYPE_ZPOOL_IMPROT,info->nid);               
            break;
        case CM_POOL_EXPORT:
            iRet = cm_cnm_pool_task_cmd("export","-f",info->name,"",CM_TASK_TYPE_ZPOOL_EXPROT,info->nid);
            break;
        case CM_POOL_SCRUB:
        case CM_POOL_CLEAR:
        case CM_POOL_SWITCH:
        case CM_POOL_DESTROY:
            iRet = cm_cnm_pool_requst(CM_OMI_CMD_MODIFY,pDecodeParam,ppAckData,pAckLen);
            break;
        default:           
            break;
    }
    return iRet;
}

sint32 cm_cnm_pool_delete(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_cnm_pool_requst(CM_OMI_CMD_DELETE,pDecodeParam,ppAckData,pAckLen);
}

sint32 cm_cnm_pool_count(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_cnm_pool_requst(CM_OMI_CMD_COUNT,pDecodeParam,ppAckData,pAckLen);
}

sint32 cm_cnm_pool_scan(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_cnm_pool_requst(CM_OMI_CMD_SCAN,pDecodeParam,ppAckData,pAckLen);
}

static uint32 cm_cnm_pool_exec_cmd(
    const sint8* cmd,const sint8* pro,
    const sint8* pool,sint8* arg)
{
    uint32 iRet = CM_OK;
    sint8 cmd_buff[CM_STRING_256] =  {0};
    sint8 back_buff[CM_STRING_256] = {0};
    if(NULL == arg)
    {
        arg = "";
    }
    CM_VSPRINTF(cmd_buff,sizeof(cmd_buff),"zpool %s %s %s %s 2>&1",cmd,pro,pool,arg);
    CM_LOG_WARNING(CM_MOD_CNM,"[%s]",cmd_buff);
    iRet = cm_exec_tmout(back_buff,sizeof(back_buff),20,"%s",cmd_buff);
    CM_LOG_WARNING(CM_MOD_CNM,"back:[%s]",back_buff);
    if(CM_ERR_TIMEOUT == iRet)
    {
        return iRet;
    }
    else if(strstr(back_buff,"currently scrubbing"))
    {
        return CM_ERR_BUSY;
    }
    else if(strstr(back_buff,"no such pool"))
    {
        return CM_ERR_NOT_EXISTS;
    }
    return iRet;
}

static uint32 cm_cnm_zpool_find_str(
    sint8 *des,const sint8* src,uint32 src_size,
    const sint8 *pro,const sint8* aft)
{
    sint8* tmp_src = CM_MALLOC(src_size);
    sint8* tmp_pro = NULL;
    sint8* tmp_aft = NULL;
    uint32 pro_str_len = strlen(pro);
    CM_MEM_CPY(tmp_src,src_size,src,src_size);
    if(pro == NULL)
    {
        tmp_pro = tmp_src;
    }
    else if(NULL == (tmp_pro = strstr(tmp_src,pro)))
    {
        CM_FREE(tmp_src);
        tmp_src = NULL;
        CM_LOG_ERR(CM_MOD_CNM,"NULL tmp_pro");
        return CM_FAIL;
    }
    if(aft == NULL)
    {
        tmp_aft = tmp_src + (src_size - 1);
    }
    else if(NULL == (tmp_aft = strstr(tmp_src,aft)))
    {
        CM_FREE(tmp_src);
        tmp_src = NULL;
        tmp_pro = NULL;
        CM_LOG_ERR(CM_MOD_CNM,"NULL tmp_aft");
        return CM_FAIL;
    }
    *tmp_aft = '\0';
    tmp_pro+=pro_str_len;
    if(tmp_pro >= tmp_aft)
    {
        CM_FREE(tmp_src);
        CM_LOG_ERR(CM_MOD_CNM,"tmp_pro >= tmp_aft");
        return CM_FAIL;
    }
    CM_MEM_CPY(des,src_size,tmp_pro,strlen(tmp_pro)+1);
    CM_FREE(tmp_src);
    return CM_OK;
}

static uint32 cm_cnm_pool_exec_cmd_back(
    const sint8* cmd,const sint8* pool,void **ppAck, uint32 *pAckLen)
{
    uint32 iRet = CM_OK;
    sint8 cmd_buff[CM_STRING_256] =  {0};
    sint8 back_buff[CM_STRING_256] = {0};
    
    sint8 buff[CM_STRING_256] = {0};
    uint32 len = 0;
    uint32 back_size = sizeof(cm_cnm_pool_list_t);    
    cm_cnm_pool_list_t *pinfo = CM_MALLOC(back_size);
    
    //CM_VSPRINTF(cmd_buff,sizeof(cmd_buff),"ret=$(zpool %s %s);iret=${ret##*scan:};echo ${iret%%config*}",cmd,pool);
    
    CM_VSPRINTF(cmd_buff,sizeof(cmd_buff),g_cm_cnm_pool_get_scrub_status,cmd,pool);
    
    iRet = cm_exec_tmout(back_buff,sizeof(back_buff),20,"%s",cmd_buff);
    if(('\0' == back_buff[0])||(NULL == strstr(back_buff,"scrub")))
    {
		CM_FREE(pinfo);
        CM_LOG_ERR(CM_MOD_CNM,"cmd:[%s]",cmd_buff);
        return CM_OK;
    }
    else if(strstr(back_buff,"scrub repaired"))
    {
        
        pinfo->prog = 100.0;
        pinfo->speed = 0.0;
        pinfo->zstatus = CM_CNM_STATUS_SCRUBED;
        if(CM_OK != cm_cnm_zpool_find_str(buff,back_buff,sizeof(back_buff)," on "," config"))
        {
            CM_FREE(pinfo);
            CM_LOG_ERR(CM_MOD_CNM," on && NULL");
            return CM_FAIL;
        }
        CM_MEM_CPY(pinfo->time,sizeof(pinfo->time),buff,strlen(buff)+1);
        if(CM_OK != cm_cnm_zpool_find_str(buff,back_buff,sizeof(back_buff)," repaired "," in "))
        {
            CM_FREE(pinfo);
            CM_LOG_ERR(CM_MOD_CNM," repaired && in back:%s",back_buff);
            return CM_FAIL;
        }
        pinfo->repaired = atoi(buff);
        if(CM_OK != cm_cnm_zpool_find_str(buff,back_buff,sizeof(back_buff)," with "," errors "))
        {
            CM_FREE(pinfo);
            CM_LOG_ERR(CM_MOD_CNM," with && errors");
            return CM_FAIL;
        }
        pinfo->errors = atoi(buff);
    }
    else if(strstr(back_buff,"scrub canceled"))
    {
        pinfo->prog = 0.0;
        pinfo->speed = 0.0;
        pinfo->zstatus = CM_CNM_STATUS_CANCELED;
        pinfo->repaired = 0;
        pinfo->errors = 0;
        if(CM_OK != cm_cnm_zpool_find_str(buff,back_buff,sizeof(back_buff)," on "," config"))
        {
            CM_FREE(pinfo);
            return CM_FAIL;
        }
        CM_MEM_CPY(pinfo->time,sizeof(pinfo->time),buff,strlen(buff)+1);
            
    }
    else if(strstr(back_buff,"scrub in progress"))
    {
        pinfo->zstatus = CM_CNM_STATUS_SCRUBING;
        pinfo->errors = 0;
        if(CM_OK != cm_cnm_zpool_find_str(buff,back_buff,sizeof(back_buff)," repaired, "," done config"))
        {
            CM_FREE(pinfo);
            CM_LOG_ERR(CM_MOD_CNM," repaired, && done");
            return CM_FAIL;
        }
        pinfo->prog = atof(buff);
        if(CM_OK != cm_cnm_zpool_find_str(buff,back_buff,sizeof(back_buff)," at ","M/s, "))
        {
            CM_FREE(pinfo);
            CM_LOG_ERR(CM_MOD_CNM," at && M/s,");
            return CM_FAIL;
        }
        pinfo->speed = atof(buff);
        if(CM_OK != cm_cnm_zpool_find_str(buff,back_buff,sizeof(back_buff)," go "," repaired, "))
        {
            CM_FREE(pinfo);
            CM_LOG_ERR(CM_MOD_CNM," go && repaired");
            return CM_FAIL;
        }
        pinfo->repaired = atoi(buff);
        if(CM_OK != cm_cnm_zpool_find_str(buff,back_buff,sizeof(back_buff)," since "," scanned "))
        {
            CM_FREE(pinfo);
            CM_LOG_ERR(CM_MOD_CNM," since && scanned");
            return CM_FAIL;
        }
        len = strlen(buff);
        for(;buff[len] != ' '&& len > 0;len--);
        if(len == 0)
        {
            CM_FREE(pinfo);
            return CM_FAIL;
        }
        buff[len] = '\0';
        CM_MEM_CPY(pinfo->time,sizeof(pinfo->time),buff,strlen(buff)+1);      
    }
    *ppAck = pinfo;
    *pAckLen = back_size;
    return iRet;
}

sint32 cm_cnm_pool_local_scan(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    cm_cnm_pool_list_param_t *info = param;
    return cm_cnm_pool_exec_cmd_back("status",info->name,ppAck,pAckLen);
}

static sint32 cm_cnm_pool_local_get_used(void *arg, sint8 **cols, uint32 col_num)
{
    cm_cnm_pool_list_t *pool = arg;

    /*    
    AVAIL   USED  RDC:IP_OWNER
    1.77T  10.4G  igb0,192.168.36.119,255.255.252.0
    */
    if(col_num != 3)
    {
        CM_LOG_ERR(CM_MOD_CNM,"col_num[%u]",col_num);
        return CM_FAIL;
    }
    CM_MEM_CPY(pool->avail,sizeof(pool->avail),cols[0],strlen(cols[0])+1);
    CM_MEM_CPY(pool->used,sizeof(pool->used),cols[1],strlen(cols[1])+1);
    CM_VSPRINTF(pool->rdcip,sizeof(pool->rdcip),"%s",cols[2]);
    return CM_OK;
}

static sint32 cm_cnm_pool_local_get_each(void *arg, sint8 **cols, uint32 col_num)
{
    cm_cnm_pool_list_t *pool = arg;

    CM_MEM_ZERO(pool,sizeof(cm_cnm_pool_list_t));
#if(CM_FALSE == CM_CNM_POOL_READ_FROM_FILE)      
    /* 
    NAME   HEALTH
    poolb  ONLINE
    tank   ONLINE
    */
    if(col_num != 2)
    {
        CM_LOG_ERR(CM_MOD_CNM,"col_num[%u]",col_num);
        return CM_FAIL;
    }
    CM_MEM_CPY(pool->name,sizeof(pool->name),cols[0],strlen(cols[0])+1);
    pool->status = cm_cnm_get_enum(&CmOmiMapEnumPoolStatusType,cols[1],0);
    pool->src_nid = (uint32)cm_exec_int("zpool status %s |grep 'real owner' "
        "|awk '{printf $3}'", pool->name);
#else
    sint8 buff[CM_STRING_64] = {0};
    /*    
    <pool node="dfb" name="poola" state="ONLINE" rid="1" cid="2" available="2.72T" used="68.8G">
    <pool node="dfb" name="poolb" state="ONLINE" rid="2" cid="2" available="1.81T" used="170M">
    <pool node="dfb" name="tank" state="ONLINE" rid="1" cid="2" available="556G" used="38.8G">
    */
    if(col_num != 8)
    {
        CM_LOG_ERR(CM_MOD_CNM,"col_num[%u]",col_num);
        return CM_FAIL;
    }
    col_num = sscanf(cols[2],"name=\"%[a-zA-Z0-9_.-]\"",pool->name);
    if(1 != col_num)
    {
        CM_LOG_ERR(CM_MOD_CNM,"get name fail");
        return CM_FAIL;
    }
    col_num = sscanf(cols[3],"state=\"%[a-zA-Z]\"",buff);
    if(1 != col_num)
    {
        CM_LOG_ERR(CM_MOD_CNM,"get name fail");
    }
    pool->status = cm_cnm_get_enum(&CmOmiMapEnumPoolStatusType,buff,0);
    col_num = sscanf(cols[4],"rid=\"%u\"",&pool->src_nid);
#endif
    pool->nid = cm_node_get_id();
    (void)cm_cnm_exec_get_col(cm_cnm_pool_local_get_used,arg,
        "zfs list -H -o avail,used,rdc:ip_owner %s", pool->name);
    
    return CM_OK;
}

sint32 cm_cnm_pool_local_getbatch(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    sint32 iRet = CM_OK;

    CM_LOG_INFO(CM_MOD_CNM,"offset[%llu] total[%u]",offset,total);
    
    iRet = cm_cnm_exec_get_list(
#if(CM_FALSE == CM_CNM_POOL_READ_FROM_FILE)     
        g_cm_cnm_pool_list_cmd,
#else
        "grep '<pool ' "CM_CNM_POOL_FILE" 2>/dev/null",
#endif
        cm_cnm_pool_local_get_each,(uint32)offset,sizeof(cm_cnm_pool_list_t),
        ppAck,&total);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]",iRet);
        return iRet;
    }
    *pAckLen = total * sizeof(cm_cnm_pool_list_t);
    return CM_OK;
}

sint32 cm_cnm_pool_local_count(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    cm_cnm_pool_list_param_t *req = param;
    uint64 cnt = 0;
    
    if((NULL == req) || ('\0' == req->name[0]))
#if(CM_FALSE == CM_CNM_POOL_READ_FROM_FILE)    
    {
        cnt = (uint64)cm_exec_double("%s 2>/dev/null |wc -l",g_cm_cnm_pool_list_cmd);
    }
    else
    {
        cnt = (uint64)cm_exec_double("%s %s 2>/dev/null |wc -l",g_cm_cnm_pool_list_cmd,req->name);
    }
#else
    {
        cnt = (uint64)cm_exec_double("grep '<pool ' "CM_CNM_POOL_FILE" 2>/dev/null |wc -l");
    }
    else
    {
        cnt = (uint64)cm_exec_double("grep '<pool ' "CM_CNM_POOL_FILE" 2>/dev/null |grep 'name=\"%s\"' |wc -l",req->name);
    }
#endif
    return cm_cnm_ack_uint64(cnt,ppAck,pAckLen);
}

static void cm_cnm_pool_local_make_raidx(sint8* buff,uint32 len,
    sint8 *disk_ids,uint32 groupnum, const sint8* raid)
{
    sint8 *pdisk[CM_STRING_256] = {NULL};
    uint32 index = 0;
    uint32 disk_num = 0;
    uint32 each_num = 0;
    sint8* *ppdisk = NULL;

    if(0 == groupnum)
    {
        return;
    }
    pdisk[disk_num] = strtok(disk_ids,",");
    while(NULL != pdisk[disk_num])
    {
        if(0 < strlen(pdisk[disk_num]))
        {
            disk_num++;
        }        
        pdisk[disk_num] = strtok(NULL,",");
    }

    if((0 == disk_num) || (disk_num < groupnum))
    {
        CM_LOG_ERR(CM_MOD_CNM,"disk_num[%u] groupnum[%u]",disk_num,groupnum);
        return;
    }
    each_num = disk_num/groupnum;
    ppdisk = pdisk;
    while(groupnum > 0)
    {
        CM_SNPRINTF_ADD(buff,len," %s",raid);
        index = each_num;
        while(index > 0)
        {
            CM_SNPRINTF_ADD(buff,len," %s",*ppdisk);
            index--;
            ppdisk++;
        }
        groupnum--;
    }
    return;
}

static uint32 cm_cnm_pool_raid_judge(const cm_cnm_pool_list_param_t *info)
{
    uint32 cut = 1;
    sint8* str = info->disk_ids;
    while('\0' != *str)
    {
        if(',' == *str)
        {
            cut++;
        }
        str++;
    }

    switch(info->raid)
    {
        case CM_RAID1:
            if(cut < 2)
            {
                return CM_FALSE;    
            }
            break;
        case CM_RAID5:
        case CM_RAIDZ5:
            if(cut<3)
            {
                return CM_FALSE;    
            }
            break;
        case CM_RAID6:
        case CM_RAIDZ6:
            if(cut<4)
            {
                return CM_FALSE;    
            }
            break;
        case CM_RAID7:
        case CM_RAIDZ7:
            if(cut<5)
            {
                return CM_FALSE;    
            }
            break;
        case CM_RAID10:
            if(0 != cut%(info->group) || 2 > (cut/(info->group)))
            {
                return CM_FALSE;    
            }
            break;
        case CM_RAID50:
        case CM_RAIDZ50:
            if(0 != cut%(info->group) || (cut/(info->group)<3))
            {
                return CM_FALSE;    
            }
            break;
        case CM_RAID60:
        case CM_RAIDZ60:
            if(0 != cut%(info->group) || (cut/(info->group)<4))
            {
                return CM_FALSE;    
            }
            break;
        case CM_RAID70:
        case CM_RAIDZ70:
            if(0 != cut%(info->group) || (cut/(info->group)<5))
            {
                return CM_FALSE;    
            }
            break;
        default:
            break;
    }

    return CM_TRUE;
} 

sint32 cm_cnm_pool_local_create(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    cm_cnm_pool_list_param_t *req = param;
    const sint8* raid = NULL;
    const cm_omi_map_enum_t raidcfg = {
        sizeof(g_cm_raid_map)/sizeof(cm_omi_map_cfg_t),
        g_cm_raid_map
    };
    sint8 cmd[CM_STRING_1K] = {0};
    uint32 buf_len = sizeof(cmd);
    uint32 cur_len = 0;
    sint8 disk_ids[CM_STRING_1K] = {0};
    sint32 iRet = CM_OK;
    uint32 raid_num = 0;
    bool_t raid_x = CM_FALSE;
    uint32 cut = 0;
   
    if((NULL == req) 
        || ('\0' == req->disk_ids[0])
        || ('\0' == req->name[0]))
    {
        return CM_PARAM_ERR;
    }

    switch(req->raid)
    {
        case CM_RAID10:
            raid_num = CM_RAID1;
            raid_x = CM_TRUE;
            break;
        case CM_RAID50:
            raid_num = CM_RAID5;
            raid_x = CM_TRUE;
            break;
        case CM_RAID60:
            raid_num = CM_RAID6;
            raid_x = CM_TRUE;
            break;
        case CM_RAID70:
            raid_num = CM_RAID7;
            raid_x = CM_TRUE;
            break;
        case CM_RAIDZ50:
            raid_num = CM_RAIDZ5;
            raid_x = CM_TRUE;
            break;
        case CM_RAIDZ60:
            raid_num = CM_RAIDZ6;
            raid_x = CM_TRUE;
            break;
        case CM_RAIDZ70:
            raid_num = CM_RAIDZ7;
            raid_x = CM_TRUE;
            break;
        default:
            raid_num = req->raid;
            break;
    }
    raid = cm_cnm_get_enum_str(&raidcfg, raid_num);
    if(NULL == raid)
    {
        CM_LOG_ERR(CM_MOD_CNM,"raid[%u]",raid_num);
        return CM_PARAM_ERR;
    }   
    
    cur_len = CM_VSPRINTF(cmd,buf_len,"zpool create -f %s",req->name);
    cut = cm_cnm_pool_raid_judge(req);
    if(CM_FALSE == cut)
    {
        CM_LOG_ERR(CM_MOD_CNM,"raid create fail");
        return CM_ERR_POOL_DISK_NUM;
    }
    CM_MEM_CPY(disk_ids,sizeof(disk_ids),req->disk_ids,sizeof(req->disk_ids));
    if(CM_FALSE == raid_x)
    {
        cm_cnm_comma2blank(disk_ids);
        cur_len += CM_VSPRINTF(cmd+cur_len,buf_len-cur_len," %s %s",raid,disk_ids);
    }
    else
    {
        cm_cnm_pool_local_make_raidx(cmd,buf_len,disk_ids,req->group,raid);       
    }
    CM_LOG_WARNING(CM_MOD_CNM,"%s",cmd);
    CM_SNPRINTF_ADD(cmd,buf_len," 2>&1");
    
    return cm_exec_out(ppAck,pAckLen,CM_CMT_REQ_TMOUT_NEVER,cmd);
}

sint32 cm_cnm_pool_local_update(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    sint32 iRet = CM_OK;
    sint8 buff[CM_STRING_1K] = {0};
    cm_cnm_pool_list_param_t *info = param;
    CM_LOG_WARNING(CM_MOD_CNM,"[ops = %d] [name = %s] [var = %s]",info->operation,info->name,info->args);
    switch(info->operation)
    {
        case CM_POOL_SCRUB:
            iRet = cm_cnm_pool_exec_cmd("scrub",info->args,info->name,"");
            break;
        case CM_POOL_CLEAR:
            iRet = cm_cnm_pool_exec_cmd("clear","",info->name,info->args);
            break;
        case CM_POOL_SWITCH:
            iRet = cm_cnm_pool_exec_cmd("release","-c",info->name,"");
            break;
        case CM_POOL_DESTROY:
            (void)cm_system(CM_SCRIPT_DIR"cm_pool.sh destroy_xml '%s'",info->name);
            iRet = cm_system(CM_SCRIPT_DIR"cm_pool.sh destroy '%s' &",info->name);
            CM_LOG_WARNING(CM_MOD_CNM,"destroy %s %d",info->name,iRet);
            break;
        default:
            break;
    }
    
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]\n%s",iRet,buff);
        iRet = cm_cnm_get_errcode(&CmCnmPoolMapErrCfg,buff,iRet);
    }
    
    return iRet;
}

sint32 cm_cnm_pool_local_delete(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    cm_cnm_pool_list_param_t *req = param;
    sint32 iRet = CM_OK;
    sint8 cmd[CM_STRING_1K] = {0};
    uint32 buf_len = sizeof(cmd);
    uint32 luncnt = 0;
    uint32 nascnt = 0;
    if((NULL == req) || ('\0' == req->name[0]))
    {
        return CM_PARAM_ERR;
    }
    
    CM_LOG_WARNING(CM_MOD_CNM,"zpool destroy %s",req->name);

    iRet = cm_exec_tmout(cmd,buf_len,CM_CMT_REQ_TMOUT_NEVER,
        "zfs list -H -o name,mountpoint|grep -v _bit0 |egrep \"^%s/\" "
        "|awk 'BEGIN{lun=0;nas=0}$2==\"-\"{lun+=1}{nas+=1}END{nas=nas-lun;print lun\" \"nas}'",
        req->name);
        
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"get count fail[%d]",req->name,iRet);
        return iRet;
    }
    sscanf(cmd,"%u %u",&luncnt,&nascnt);
    
    if(0 != luncnt)
    {
        CM_LOG_ERR(CM_MOD_CNM,"pool[%s] lun[%u]",req->name,luncnt);
        return CM_ERR_POOL_LUN_EXISTS;
    }

    if(0 != nascnt)
    {
        CM_LOG_ERR(CM_MOD_CNM,"pool[%s] nas[%u]",req->name,nascnt);
        return CM_ERR_POOL_NAS_EXISTS;
    }
    
    return cm_exec_out(ppAck,pAckLen,CM_CMT_REQ_TMOUT_NEVER,
        "zpool destroy -f %s 2>&1",req->name);
}

sint32 cm_cnm_zpool_eximport_task_report
    (cm_task_send_state_t *pSnd, cm_task_cmt_echo_t **pproc)
{
    cm_task_cmt_echo_t *pAck = NULL;
    sint32 prog = cm_exec_int("sqlite3 %s 'select prog from record_t "
                              "WHERE id=%u'", CM_TASK_DB_FILE, pSnd->task_id);
    prog = (prog >= 90 ? 90 : prog + 10);

    pAck = CM_MALLOC(sizeof(cm_task_cmt_echo_t));

    if(NULL == pAck)
    {
        CM_LOG_ERR(CM_MOD_CNM, "malloc fail");
        return CM_FAIL;
    }

    pAck->task_pid = pSnd->task_pid;
    pAck->task_prog = prog;
    pAck->task_state = CM_TASK_STATE_RUNNING;
    pAck->task_end = 0;
    *pproc = pAck;
    return CM_OK;
}

void cm_cnm_pool_oplog_create(const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK)? CM_ALARM_LOG_POOL_CREATR_OK : CM_ALARM_LOG_POOL_CREATR_FAIL;
    
    const cm_cnm_pool_list_param_t *info = pDecodeParam;
    const uint32 cnt = 5;

    /*<nid> <raid> <name> <param> [-group group]*/
    if(NULL == pDecodeParam)
    {   
        cm_cnm_oplog_report(sessionid,alarmid,NULL,cnt,NULL);
    }    
    else    
    {       
        const sint8* nodename = cm_node_get_name(info->nid);
        cm_cnm_oplog_param_t params[] = {
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_POOL_NID,strlen(nodename),nodename},
            {CM_OMI_DATA_ENUM,CM_OMI_FIELD_POOL_RAID,sizeof(info->raid),&info->raid},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_POOL_NAME,strlen(info->name),info->name},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_PARAM,strlen(info->disk_ids),info->disk_ids},
            {CM_OMI_DATA_INT,CM_OMI_FIELD_POOL_GROUP,sizeof(info->group),&info->group},
        };
        
        cm_cnm_oplog_report(sessionid,alarmid,params,cnt,&info->set);
        if(CM_OK == Result)
        {
            (void)cm_thread_start(cm_cnm_disk_update_thread,nodename);
        }
    }   
    return;
}

void cm_cnm_pool_oplog_delete(const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{   
    uint32 alarmid = (Result == CM_OK) ? CM_ALARM_LOG_POOL_DELETE_OK : CM_ALARM_LOG_POOL_DELETE_FAIL;
    const cm_cnm_pool_list_param_t* info = pDecodeParam;
    const uint32 cnt = 2;
    
    if(NULL == info)
    {
        cm_cnm_oplog_report(sessionid,alarmid,NULL,cnt,NULL);
    }
    else
    {
        const sint8* nodename = cm_node_get_name(info->nid);
        cm_cnm_oplog_param_t params[] = {
        	{CM_OMI_DATA_STRING,CM_OMI_FIELD_POOL_NID,strlen(nodename),nodename},
        	{CM_OMI_DATA_STRING,CM_OMI_FIELD_POOL_NAME,strlen(info->name),info->name},
        };
        cm_cnm_oplog_report(sessionid,alarmid,params,cnt,&info->set);
        if(CM_OK == Result)
        {
            (void)cm_thread_start(cm_cnm_disk_update_thread,nodename);
        }
    }
   
    return;
}

/*<nid> <name> <operation> [-var var]*/

void cm_cnm_pool_oplog_update(const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK) ? CM_ALARM_LOG_POOL_UPDATE_OK : CM_ALARM_LOG_POOL_UPDATE_FAIL;
    const uint32 cnt = 4;
    const cm_cnm_pool_list_param_t* info = pDecodeParam;
    if(NULL == info)
    {
        cm_cnm_oplog_report(sessionid,alarmid,NULL,cnt,NULL);
    }
    else
    {
        const sint8* nodename = cm_node_get_name(info->nid);
        cm_cnm_oplog_param_t params[] = {
        	{CM_OMI_DATA_STRING,CM_OMI_FIELD_POOL_NID,strlen(nodename),nodename},
        	{CM_OMI_DATA_STRING,CM_OMI_FIELD_POOL_NAME,strlen(info->name),info->name},
        	{CM_OMI_DATA_INT,CM_OMI_FIELD_POOL_OPS,sizeof(info->operation),&info->operation},
        	{CM_OMI_DATA_STRING,CM_OMI_FIELD_POOL_VAR,strlen(info->args),info->args},
        };
        cm_cnm_oplog_report(sessionid,alarmid,params,cnt,&info->set);
    }
   
    return;
}

/*============================================================================*/

static const sint8* g_cm_cnm_pooldisk_type[] = {
    "data","meta","spare","metaspare","log","logdata",
        "logdataspare","cache","low","lowspare","mirrorspare"
    };
sint32 cm_cnm_pooldisk_decode(const cm_omi_obj_t ObjParam, void **ppDecodeParam)
{
    cm_cnm_pool_list_param_t *param = CM_MALLOC(sizeof(cm_cnm_pool_list_param_t));

    if(NULL == param)
    {
        CM_LOG_ERR(CM_MOD_CNM,"malloc fail");
        return CM_FAIL;
    }
    CM_MEM_ZERO(param,sizeof(cm_cnm_pool_list_param_t));
    param->nid = CM_NODE_ID_NONE;
    param->offset = 0;
    param->total = CM_CNM_MAX_RECORD;
    param->raid = CM_RAID_BUTT;
    param->type = CM_POOL_DISK_BUTT;
    if(CM_OK != cm_omi_obj_key_get_u32_ex(ObjParam,CM_OMI_FIELD_POOLDISK_NID,&param->nid))
    {
        CM_FREE(param);
        CM_LOG_ERR(CM_MOD_CNM,"nid null");
        return CM_PARAM_ERR;
    }
    else
    {
        CM_OMI_FIELDS_FLAG_SET(&param->set,CM_OMI_FIELD_POOLDISK_NID);
    }

    if(CM_OK != cm_omi_obj_key_get_str_ex(ObjParam,CM_OMI_FIELD_POOLDISK_NAME,param->name,sizeof(param->name)))
    {
        CM_FREE(param);
        CM_LOG_ERR(CM_MOD_CNM,"pool null");
        return CM_PARAM_ERR;
    }
    else
    {
        CM_OMI_FIELDS_FLAG_SET(&param->set,CM_OMI_FIELD_POOLDISK_NAME);
    }
    
    (void)cm_omi_obj_key_get_u32_ex(ObjParam,CM_OMI_FIELD_FROM,&param->offset);
    (void)cm_omi_obj_key_get_u32_ex(ObjParam,CM_OMI_FIELD_TOTAL,&param->total);
 
    if(CM_OK == cm_omi_obj_key_get_u32_ex(ObjParam,CM_OMI_FIELD_POOLDISK_RAID,&param->raid))
    {
        CM_OMI_FIELDS_FLAG_SET(&param->set,CM_OMI_FIELD_POOLDISK_RAID);
    }
    if(CM_OK == cm_omi_obj_key_get_u32_ex(ObjParam,CM_OMI_FIELD_POOLDISK_TYPE,&param->type))
    {
        CM_OMI_FIELDS_FLAG_SET(&param->set,CM_OMI_FIELD_POOLDISK_TYPE);
    }
    if(CM_OK == cm_omi_obj_key_get_u32_ex(ObjParam,CM_OMI_FIELD_POOLDISK_GROUP,&param->group))
    {
        CM_OMI_FIELDS_FLAG_SET(&param->set,CM_OMI_FIELD_POOLDISK_GROUP);
    }
    if(CM_OK == cm_omi_obj_key_get_str_ex(ObjParam,CM_OMI_FIELD_PARAM,param->disk_ids,sizeof(param->disk_ids)))
    {
        CM_OMI_FIELDS_FLAG_SET(&param->set,CM_OMI_FIELD_PARAM);
    }

    if(0 == param->group)
    {
        param->group = 1;
    }
    
    param->total = CM_MIN(param->total,CM_CNM_MAX_RECORD);
    *ppDecodeParam = param;
    return CM_OK;
}

static void cm_cnm_pooldisk_encode_each(cm_omi_obj_t item,void *eachdata,void *arg)
{
    cm_cnm_pooldisk_info_t *disk = eachdata;
    (void)cm_omi_obj_key_set_str_ex(item,CM_OMI_FIELD_POOLDISK_ID,disk->disk);
    (void)cm_omi_obj_key_set_u32_ex(item,CM_OMI_FIELD_POOLDISK_RAID,disk->raid);
    (void)cm_omi_obj_key_set_u32_ex(item,CM_OMI_FIELD_POOLDISK_TYPE,disk->type);
    (void)cm_omi_obj_key_set_u32_ex(item,CM_OMI_FIELD_POOLDISK_ERR_READ,disk->err_read);
    (void)cm_omi_obj_key_set_u32_ex(item,CM_OMI_FIELD_POOLDISK_ERR_WRITE,disk->err_write);
    (void)cm_omi_obj_key_set_u32_ex(item,CM_OMI_FIELD_POOLDISK_ERR_SUM,disk->err_sum);
    (void)cm_omi_obj_key_set_u32_ex(item,CM_OMI_FIELD_POOLDISK_ENID,disk->enid);
    (void)cm_omi_obj_key_set_u32_ex(item,CM_OMI_FIELD_POOLDISK_SLOTID,disk->slotid);
    (void)cm_omi_obj_key_set_u32_ex(item,CM_OMI_FIELD_POOLDISK_STATUS,disk->status);
    (void)cm_omi_obj_key_set_u32_ex(item,CM_OMI_FIELD_POOLDISK_GROUP,disk->group_id);
    (void)cm_omi_obj_key_set_str_ex(item,CM_OMI_FIELD_POOLDISK_SIZE,disk->size);
    return;
}

cm_omi_obj_t cm_cnm_pooldisk_encode(const void *pDecodeParam, void *pAckData, uint32 AckLen)
{
    return cm_cnm_encode_comm(pAckData,AckLen,
        sizeof(cm_cnm_pooldisk_info_t),cm_cnm_pooldisk_encode_each,NULL);
}

static sint32 cm_cnm_pooldisk_requst(uint32 cmd,const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    const cm_cnm_pool_list_param_t *pDecode = pDecodeParam;
    cm_cnm_req_param_t param;
    CM_MEM_ZERO(&param,sizeof(cm_cnm_req_param_t));
    
    if(NULL == pDecode)
    {
        CM_LOG_ERR(CM_MOD_CNM,"pDecodeParam null");
        return CM_PARAM_ERR;
    }
    param.nid = pDecode->nid;
    param.offset = pDecode->offset;
    param.total = pDecode->total;
    param.param = pDecode;
    param.param_len = sizeof(cm_cnm_pool_list_param_t);
    param.obj = CM_OMI_OBJECT_POOLDISK;
    param.cmd = cmd;
    param.ppack = ppAckData;
    param.acklen = pAckLen;
    return cm_cnm_request(&param);
}

sint32 cm_cnm_pooldisk_getbatch(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_cnm_pooldisk_requst(CM_OMI_CMD_GET_BATCH,pDecodeParam,ppAckData,pAckLen);
}
sint32 cm_cnm_pooldisk_add(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    const cm_cnm_pool_list_param_t *pDecode = pDecodeParam;
    sint32 iRet = cm_cnm_pooldisk_requst(CM_OMI_CMD_ADD,pDecodeParam,ppAckData,pAckLen);
    sint8 disk_ids[CM_STRING_1K] = {0};
    
    if(CM_OK == iRet)
    {
        /* 修改问题单 0005426 */
        CM_MEM_CPY(disk_ids,sizeof(disk_ids),pDecode->disk_ids,sizeof(pDecode->disk_ids));
        cm_cnm_disk_set_busy(disk_ids);
    }
    return iRet;
}
sint32 cm_cnm_pooldisk_count(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_cnm_pooldisk_requst(CM_OMI_CMD_COUNT,pDecodeParam,ppAckData,pAckLen);
}
sint32 cm_cnm_pooldisk_delete(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_cnm_pooldisk_requst(CM_OMI_CMD_DELETE,pDecodeParam,ppAckData,pAckLen);
}

static sint32 cm_cnm_pooldisk_info_get(const sint8* poolname, cm_xml_node_ptr *root)
{
    const sint8* xmlfile = "/tmp/pool.xml";
    sint32 iRet = CM_OK;
    cm_xml_node_ptr xmlinfo = NULL;
#if(CM_FALSE == CM_CNM_POOL_READ_FROM_FILE)
    sint8 cmd[CM_STRING_1K] = {0};
    
    CM_VSPRINTF(cmd,sizeof(cmd),"zpool status -x %s", poolname);
    CM_MUTEX_LOCK(&g_cm_cnm_pool_mutex);
    iRet = cm_exec_cmd_for_str(cmd,cmd,sizeof(cmd),CM_CMT_REQ_TMOUT);
    if(CM_OK == iRet)
    {
        xmlinfo = cm_xml_parse_file(xmlfile);
        if(NULL == xmlinfo)
        {
            CM_LOG_ERR(CM_MOD_CNM,"load %s fail",xmlfile);
            iRet = CM_ERR_POOL_DISK_XML;
        }
    }
    else
    {
        CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]\n%s",iRet,cmd);
        iRet = cm_cnm_get_errcode(&CmCnmPoolMapErrCfg,cmd,iRet);
    }
    CM_MUTEX_UNLOCK(&g_cm_cnm_pool_mutex);
#else
    xmlinfo = cm_xml_parse_file(xmlfile);
    if(NULL == xmlinfo)
    {
        CM_LOG_ERR(CM_MOD_CNM,"load %s fail",xmlfile);
        iRet = CM_ERR_POOL_DISK_XML;
    }
#endif

    *root = xmlinfo;
    return iRet;
}

sint32 cm_cnm_pooldisk_get_raid_count(cm_xml_node_ptr curnode,void *arg)
{
    uint32* pcount = arg;
    if(NULL == curnode)
    {
        return CM_FAIL;
    }
    (*pcount)++;
    return CM_OK;
}


static uint32 cm_cnm_pooldisk_get_raid(cm_xml_node_ptr curnode)
{
    
    const sint8* valstr = NULL;
    sint32 raid = -1;
    uint32 count = 0;    
    sint8* str = NULL;
    cm_xml_node_ptr node = curnode;
    cm_xml_node_ptr parent = NULL;
    if(NULL == node)
    {
        return CM_FAIL;
    }
    parent = cm_xml_get_parent(node);
    if(NULL == parent)
    {
        return CM_FAIL;
    }
    
    str = cm_xml_get_name(parent);
    if(NULL == str)
    {
        return CM_FAIL;
    }
    if(0 != strcmp(str,"vdev"))
    {
        valstr = cm_xml_get_attr(parent,"name");
        if(NULL == valstr)
        {
            return CM_RAID0;
        }
        else
        {
            return cm_cnm_get_enum(&CmOmiMapEnumPoolRaidType,valstr,CM_RAID0);
        } 
    }

    /* start for 6026 修改热备盘顶替场景 */
    valstr = cm_xml_get_attr(parent,"type");
    if((NULL != valstr) && (0 == strcmp(valstr,"spare")))
    {
        parent = cm_xml_get_parent(parent);
    }
    /* end for 6026 修改热备盘顶替场景 */

    valstr = cm_xml_get_attr(parent,"name");
    if(NULL == valstr)
    {
        return CM_RAID0;
    }
    else
    {
        raid = cm_cnm_get_enum(&CmOmiMapEnumPoolRaidType,valstr,CM_RAID0);
    }
    cm_xml_find_node(cm_xml_get_parent(parent),"vdev","name",\
                    valstr,cm_cnm_pooldisk_get_raid_count,&count);
    if(count > 1)
    {
        switch(raid)
        {
            case CM_RAID1:
                raid=CM_RAID10;
                break;
            case CM_RAID5:
                raid=CM_RAID50;
                break;
            case CM_RAID6:
                raid=CM_RAID60;
                break;
            case CM_RAID7:
                raid=CM_RAID70;
                break;
            case CM_RAIDZ5:
                raid=CM_RAIDZ50;
                break;
            case CM_RAIDZ6:
                raid=CM_RAIDZ60;
                break;
            case CM_RAIDZ7:
                raid=CM_RAIDZ70;
                break;
            default:
                break;
        }
    }
    
    return raid;
}

static uint32 cm_cnm_pooldisk_local_gettype(cm_xml_node_ptr curnode)
{
    cm_xml_node_ptr parent = cm_xml_get_parent(curnode);
    const sint8* str = NULL;
    while(NULL != parent)
    {
        str = cm_xml_get_name(parent);
        if(NULL == str)
        {
            CM_LOG_WARNING(CM_MOD_CNM,"get parentname fail");
            break;
        }
        if(0 == strcmp(str,"vdev"))
        {
            parent = cm_xml_get_parent(parent);
            continue;
        }
        //CM_LOG_WARNING(CM_MOD_CNM,"%p:%s",parent,str);
        return cm_cnm_get_enum(&CmCnmPoolMapDiskTypeCfg,str,CM_POOL_DISK_DATA);
    }
    return CM_POOL_DISK_DATA;
}
static sint32 cm_cnm_pooldisk_local_check(cm_xml_node_ptr curnode,void *arg)
{
    cm_cnm_pool_list_param_t *req = arg;
    uint32 val = 0;
    const sint8* str = NULL;

    /* 校验disk id */
    if(0 != req->disk_ids[0])
    {
        str = cm_xml_get_attr(curnode,"name");
        if(NULL == str)
        {
            return CM_FAIL;
        }
        str = strstr(req->disk_ids,str);
        if(NULL == str)
        {
            return CM_FAIL;
        }
    }
    
    if(CM_POOL_DISK_BUTT > req->type)
    {
        val = cm_cnm_pooldisk_local_gettype(curnode);
        if(req->type != val)
        {
            return CM_FAIL;
        }
    }

    if(CM_RAID_BUTT > req->raid)
    {
        val = cm_cnm_pooldisk_get_raid(curnode);
        if(req->raid != val)
        {
            return CM_FAIL;
        }
    }    
    
    return CM_OK;
}

/* start for 7029 */
uint32 cm_cnm_pooldisk_local_get_groupid(cm_xml_node_ptr curnode)
{
    uint32 groupid=0;
    cm_xml_node_ptr ptmp = cm_xml_get_parent(curnode);
    if(NULL == ptmp)
    {
        return 0;
    }
    for(ptmp = cm_xml_get_child(ptmp),groupid=1;
        NULL!=ptmp;
        ptmp = cm_xml_get_next_sibling(ptmp),groupid++)
    {
        if (ptmp == curnode)
        {
            return groupid;
        }
    }
    return 0;
}
/* end for 7029 */

static sint32 cm_cnm_pooldisk_local_get(cm_xml_node_ptr curnode,void *arg)
{
    cm_cnm_pooldisk_info_t *disk = arg;
    const sint8* attrs[] = {"read","write","sum","en_id","slot_id"};
    uint32* vals[] = {&disk->err_read, &disk->err_write, &disk->err_sum,&disk->enid,&disk->slotid};
    const sint8* valstr = NULL;
    uint32 index = sizeof(vals)/sizeof(uint32*);
    cm_xml_node_ptr parent= NULL;

    /*
    <vdev name="c0t5000C50070EBCE0Be5" state="ONLINE" 
        type="disk" read="0" write="0" sum="0" en_id="22" 
        slot_id="26" alloc="38.8G" total="556G"/>*/
    valstr = cm_xml_get_attr(curnode,"name");
    if(NULL == valstr)
    {
        CM_LOG_WARNING(CM_MOD_CNM,"get diskname fail");
        return CM_FAIL;
    }
    CM_VSPRINTF(disk->disk,sizeof(disk->disk),"%s",valstr);

    valstr = cm_xml_get_attr(curnode,"state");
    if(NULL == valstr)
    {
        CM_LOG_WARNING(CM_MOD_CNM,"get disk[%s] state fail",disk->disk);
        return CM_FAIL;
    }
    disk->status = cm_cnm_get_enum(&CmOmiMapEnumPoolStatusType,valstr,0);

    valstr = cm_xml_get_attr(curnode,"total");
    if(NULL == valstr)
    {
        CM_LOG_WARNING(CM_MOD_CNM,"get total fail");
        return CM_FAIL;
    }
    CM_VSPRINTF(disk->size,sizeof(disk->size),"%s",valstr);
    while(index > 0)
    {
        index--;
        valstr = cm_xml_get_attr(curnode,attrs[index]);
        if(NULL == valstr)
        {
            CM_LOG_WARNING(CM_MOD_CNM,"get disk[%s] %s fail",disk->disk,attrs[index]);
            return CM_FAIL;
        }
        *(vals[index]) = (uint32)atoi(valstr);
    }
    
    disk->raid = cm_cnm_pooldisk_get_raid(curnode);
    disk->type = cm_cnm_pooldisk_local_gettype(curnode);
    
    curnode = cm_xml_get_parent(curnode);
    if(NULL == curnode)
    {
        CM_LOG_WARNING(CM_MOD_CNM,"get disk[%s] parent fail",disk->disk);
        return CM_FAIL;
    }

    valstr = cm_xml_get_name(curnode);
    if(NULL == valstr)
    {
        CM_LOG_WARNING(CM_MOD_CNM,"get disk[%s] parentname fail",disk->disk);
        return CM_FAIL;
    }
    if(0 == strcmp(valstr,"vdev"))
    {
        parent = cm_xml_get_parent(curnode);
        valstr = cm_xml_get_name(parent);
    }
    else
    {
        parent = curnode;
    }    

    /* start for 6026 修改热备盘顶替场景 */
    valstr = cm_xml_get_attr(curnode,"type");
    if((NULL != valstr) && (0 == strcmp(valstr,"spare")))
    {
        disk->group_id = cm_cnm_pooldisk_local_get_groupid(cm_xml_get_parent(curnode));
    }
    else
    {
        disk->group_id = cm_cnm_pooldisk_local_get_groupid(curnode);
    }
    /* end for 6026 修改热备盘顶替场景 */
    
    disk->enid = cm_cnm_disk_enid_exchange(disk->enid);
    return CM_OK;
}

sint32 cm_cnm_pooldisk_local_getbatch(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    cm_xml_node_ptr xmlroot = NULL;
    cm_cnm_pool_list_param_t *req = param;
    sint32 iRet = CM_OK;
    cm_xml_request_param_t xmlparam;

    if(NULL == req)
    {
        CM_LOG_ERR(CM_MOD_CNM,"param null");
        return CM_PARAM_ERR;
    }
    CM_LOG_INFO(CM_MOD_CNM,"pool[%s] type[%d] raid[%d] offset[%u] total[%u]",
        req->name,req->type,req->raid,req->offset,req->total); 
    iRet = cm_cnm_pooldisk_info_get(req->name, &xmlroot);
    if(CM_OK != iRet)
    {
        return iRet;
    }
    CM_MEM_ZERO(&xmlparam,sizeof(xmlparam));
#if(CM_FALSE == CM_CNM_POOL_READ_FROM_FILE)    
    xmlparam.root = xmlroot;
#else
    xmlparam.root = cm_xml_get_node(xmlroot,"pool","name",req->name);
    if(NULL == xmlparam.root)
    {
        cm_xml_free(xmlroot);
        return iRet;
    }
#endif
    xmlparam.nodename = "vdev";
    xmlparam.attr = "type";
    xmlparam.value = "disk"; 
    xmlparam.param = req;
    xmlparam.offset = (uint32)offset;
    xmlparam.total = total;
    xmlparam.eachsize = sizeof(cm_cnm_pooldisk_info_t);
    xmlparam.check = cm_cnm_pooldisk_local_check;
    xmlparam.geteach = cm_cnm_pooldisk_local_get;
    xmlparam.ppAck = ppAck;
    xmlparam.pAckLen = pAckLen;
    iRet = cm_xml_request(&xmlparam);
    cm_xml_free(xmlroot);
    return iRet;
}

sint32 cm_cnm_pooldisk_local_count(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    cm_xml_node_ptr xmlroot = NULL;
    cm_cnm_pool_list_param_t *req = param;
    sint32 iRet = CM_OK;
    cm_xml_request_param_t xmlparam;
    
    if(NULL == req)
    {
        CM_LOG_ERR(CM_MOD_CNM,"param null");
        return CM_PARAM_ERR;
    }
    CM_LOG_INFO(CM_MOD_CNM,"pool[%s] type[%d] raid[%d]",
        req->name,req->type,req->raid); 
    iRet = cm_cnm_pooldisk_info_get(req->name, &xmlroot);
    if(CM_OK != iRet)
    {
        return iRet;
    }
    CM_MEM_ZERO(&xmlparam,sizeof(xmlparam));
#if(CM_FALSE == CM_CNM_POOL_READ_FROM_FILE)    
    xmlparam.root = xmlroot;
#else
    xmlparam.root = cm_xml_get_node(xmlroot,"pool","name",req->name);
    if(NULL == xmlparam.root)
    {
        cm_xml_free(xmlroot);
        return iRet;
    }
#endif
    xmlparam.nodename = "vdev";
    xmlparam.attr = "type";
    xmlparam.value = "disk"; 
    xmlparam.param = req;
    xmlparam.check = cm_cnm_pooldisk_local_check;
    xmlparam.ppAck = ppAck;
    xmlparam.pAckLen = pAckLen;
    iRet = cm_xml_request_count(&xmlparam);
    cm_xml_free(xmlroot);
    return iRet;;
}

sint32 cm_cnm_pooldisk_local_add(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    cm_cnm_pool_list_param_t *req = param;
    const sint8* raid = "";
    const sint8* disktype = "";
    const cm_omi_map_enum_t raidcfg = {
        sizeof(g_cm_raid_map)/sizeof(cm_omi_map_cfg_t),
        g_cm_raid_map
    };
    sint8 cmd[CM_STRING_1K] = {0};
    uint32 buf_len = sizeof(cmd);
    sint8 disk_ids[CM_STRING_1K] = {0};
    sint32 iRet = CM_OK;
    uint32 raid_num = 0;
    bool_t raid_x = CM_FALSE;
    uint32 cut = 0;
    
    if((NULL == req) 
        || ('\0' == req->disk_ids[0])
        || ('\0' == req->name[0]))
    {
        return CM_PARAM_ERR;
    }
    CM_SNPRINTF_ADD(cmd,buf_len,"zpool add -f %s",req->name);

    if((CM_POOL_DISK_DATA < req->type) && (req->type < CM_POOL_DISK_BUTT))
    {
        disktype = cm_cnm_get_enum_str(&CmCnmPoolMapDiskTypeUserCfg,req->type);
        if(NULL == disktype)
        {
            CM_LOG_ERR(CM_MOD_CNM,"type[%u]",req->type);
            return CM_PARAM_ERR;
        }
        CM_SNPRINTF_ADD(cmd,buf_len," %s",disktype);
    }
    
    if(req->raid < CM_RAID_BUTT)
    {
        switch(req->raid)
        {
            case CM_RAID10:
                raid_num = CM_RAID1;
                raid_x = CM_TRUE;
                break;
            case CM_RAID50:
                raid_num = CM_RAID5;
                raid_x = CM_TRUE;
                break;
            case CM_RAID60:
                raid_num = CM_RAID6;
                raid_x = CM_TRUE;
                break;
            case CM_RAID70:
                raid_num = CM_RAID7;
                raid_x = CM_TRUE;
                break;
            default:
                raid_num = req->raid;
                break;
        }
        raid = cm_cnm_get_enum_str(&raidcfg, raid_num);
        if(NULL == raid)
        {
            CM_LOG_ERR(CM_MOD_CNM,"raid[%u]",req->raid);
            return CM_PARAM_ERR;
        }
    }

    cut = cm_cnm_pool_raid_judge(req);
    if(CM_FALSE == cut)
    {
        CM_LOG_ERR(CM_MOD_CNM,"raid create fail");
        return CM_ERR_POOL_DISK_NUM;
    }
    
    CM_MEM_CPY(disk_ids,sizeof(disk_ids),req->disk_ids,sizeof(req->disk_ids));
    if(CM_FALSE == raid_x)
    {
        cm_cnm_comma2blank(disk_ids);    
        CM_SNPRINTF_ADD(cmd,buf_len," %s %s 2>&1",raid,disk_ids);
    }
    else
    {
        cm_cnm_pool_local_make_raidx(cmd,buf_len,disk_ids,req->group,raid);   
        CM_SNPRINTF_ADD(cmd,buf_len," 2>&1");
    }
    CM_LOG_WARNING(CM_MOD_CNM,"%s",cmd);   
    
    return cm_exec_out(ppAck,pAckLen,CM_CMT_REQ_TMOUT_NEVER,cmd);
}

sint32 cm_cnm_pooldisk_local_delete(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    cm_cnm_pool_list_param_t *req = param;
    sint8 cmd[CM_STRING_1K] = {0};
    uint32 buf_len = sizeof(cmd);
    sint8 disk_ids[CM_STRING_1K] = {0};
    sint32 iRet = CM_OK;
   
    if((NULL == req) 
        || ('\0' == req->disk_ids[0])
        || ('\0' == req->name[0]))
    {
        return CM_PARAM_ERR;
    }
    
    CM_SNPRINTF_ADD(cmd,buf_len,"zpool remove %s",req->name);
    
    CM_MEM_CPY(disk_ids,sizeof(disk_ids),req->disk_ids,sizeof(req->disk_ids));
    cm_cnm_comma2blank(disk_ids);
    
    CM_SNPRINTF_ADD(cmd,buf_len," %s 2>&1",disk_ids);
    CM_LOG_WARNING(CM_MOD_CNM,"%s",cmd);   
    
    return cm_exec_out(ppAck,pAckLen,CM_CMT_REQ_TMOUT_NEVER,cmd);
}

void cm_cnm_pooldisk_oplog_add(const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK)? CM_ALARM_LOG_POOL_DISK_CREATR_OK : CM_ALARM_LOG_POOL_DISK_CREATR_FAIL;
    
    const cm_cnm_pool_list_param_t *info = (const cm_cnm_pool_list_param_t*)pDecodeParam;
    const uint32 cnt = 6;
    
    /*<nid> <pool> <param> [-type type] [-raid raid] [-group group]*/
    if(NULL == pDecodeParam)
    {   
        cm_cnm_oplog_report(sessionid,alarmid,NULL,cnt,NULL);
    }    
    else    
    {       
        const sint8* nodename = cm_node_get_name(info->nid);
        cm_cnm_oplog_param_t params[] = {
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_POOLDISK_NID,strlen(nodename),nodename},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_POOLDISK_NAME,strlen(info->name),info->name},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_PARAM,strlen(info->disk_ids),info->disk_ids},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_POOLDISK_TYPE,strlen(g_cm_cnm_pooldisk_type[info->type]),g_cm_cnm_pooldisk_type[info->type]},
            {CM_OMI_DATA_ENUM,CM_OMI_FIELD_POOLDISK_RAID,sizeof(info->raid),(const void*)&info->raid},            
            {CM_OMI_DATA_INT,CM_OMI_FIELD_POOLDISK_GROUP,sizeof(info->group),&info->group},
        };
        
        cm_cnm_oplog_report(sessionid,alarmid,params,cnt,&info->set);
        if(CM_OK == Result)
        {
            (void)cm_thread_start(cm_cnm_disk_update_thread,nodename);
        }
    }   
    return;
}

void cm_cnm_pooldisk_oplog_delete(const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK) ? CM_ALARM_LOG_POOL_DISK_DELETE_OK : CM_ALARM_LOG_POOL_DISK_DELETE_FAIL;
    const uint32 cnt = 3;
    const cm_cnm_pool_list_param_t *info = pDecodeParam;
    if(NULL == info)
    {
        cm_cnm_oplog_report(sessionid,alarmid,NULL,cnt,NULL);
    }
    else
    {
        const sint8* nodename = cm_node_get_name(info->nid);
        cm_cnm_oplog_param_t params[] = {
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_POOL_NID,strlen(nodename),nodename},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_POOL_NAME,strlen(info->name),info->name},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_PARAM,strlen(info->disk_ids),info->disk_ids},
        };
        cm_cnm_oplog_report(sessionid,alarmid,params,cnt,&info->set);

        if(CM_OK == Result)
        {
            (void)cm_thread_start(cm_cnm_disk_update_thread,nodename);
        }
    }
   
    return;
}

static sint32 cm_cnm_pool_get_each(cm_node_info_t *pNode, void *pArg)
{
    sint32 iRet = CM_OK;
    cm_cnm_req_param_t *param = pArg;
    
    param->nid = pNode->id;
    iRet = cm_cnm_request(param);
    if((CM_OK != iRet) || (NULL == *(param->ppack)))
    {
        /* 为了继续查询 */
        return CM_OK;
    }

    /* 让跳出循环 */
    return CM_FAIL;
}

uint32 cm_cnm_pool_getbydisk(const sint8* disk, sint8* pool, uint32 len,uint32 *ptype)
{
    cm_cnm_pool_list_param_t req;
    cm_cnm_req_param_t param;
    cm_cnm_pool_list_t *pAck  = NULL;
    uint32 AckLen = 0;
    
    CM_MEM_ZERO(&param,sizeof(cm_cnm_req_param_t));
    CM_MEM_ZERO(&req,sizeof(req));
    
    CM_VSPRINTF(req.disk_ids,sizeof(req.disk_ids),"%s",disk);
    param.total = 1;
    param.param = &req;
    param.param_len = sizeof(cm_cnm_pool_list_param_t);
    param.obj = CM_OMI_OBJECT_POOL;
    param.cmd = CM_OMI_CMD_GET;
    param.ppack = (void**)&pAck;
    param.acklen = &AckLen;

    (void)cm_node_traversal_all(cm_cnm_pool_get_each,&param,CM_TRUE);
    if(NULL != pAck)
    {
        CM_VSPRINTF(pool,len,"%s",pAck->name);
        *ptype=pAck->status; 
        CM_FREE(pAck);
        return param.nid;
    }
    return CM_NODE_ID_NONE;
}

sint32 cm_cnm_pool_local_get(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    cm_cnm_pool_list_param_t *req = param;
    cm_cnm_pool_list_t *info = NULL;
    sint32 cnt = 0;
    cm_xml_node_ptr xmlroot = NULL;
    cm_xml_node_ptr node = NULL;
    sint8 diskid[CM_NAME_LEN_DISK] = {0};
    
    if((NULL == req) 
        || ('\0' == req->disk_ids[0]))
    {
        return CM_PARAM_ERR;
    }

    /* diskid为 sn， 这里先转换为id  */
    cnt = cm_cnm_disk_get_id(req->disk_ids,diskid,sizeof(diskid));
    if(CM_OK != cnt)
    {
        return CM_OK;
    }
    cnt = cm_exec_int("grep -w %s "CM_CNM_POOL_FILE" 2>/dev/null |wc -l", diskid);
    if(0 == cnt)
    {
        return CM_OK;
    }
    info = CM_MALLOC(sizeof(cm_cnm_pool_list_t));
    if(NULL == info)
    {
        CM_LOG_ERR(CM_MOD_CNM,"malloc fail");
        return CM_FAIL;
    }
    xmlroot = cm_xml_parse_file(CM_CNM_POOL_FILE);
    if(NULL == xmlroot)
    {
        CM_LOG_ERR(CM_MOD_CNM,"parse %s fail",CM_CNM_POOL_FILE);
        CM_FREE(info);
        return CM_FAIL;
    }

    node = cm_xml_get_node(xmlroot,"vdev","name",diskid);
    if(NULL == node)
    {
        CM_LOG_ERR(CM_MOD_CNM,"find %s fail",diskid);
        CM_FREE(info);
        cm_xml_free(xmlroot);
        return CM_FAIL;
    }
    /* 获取硬盘类型 */
    info->status = cm_cnm_pooldisk_local_gettype(node);
    do
    {
        node = cm_xml_get_parent(node);        
    }while((NULL != node) && (0 != strcmp("pool", cm_xml_get_name(node))));
    
    CM_VSPRINTF(info->name,sizeof(info->name),"%s",cm_xml_get_attr(node,"name"));
    info->nid = cm_node_get_id();
    /* 其他字段暂时不用，就不取了 */
    cm_xml_free(xmlroot);
    *ppAck = info;
    *pAckLen = sizeof(cm_cnm_pool_list_t);
    return CM_OK;
}
const sint8* g_cm_cnm_pool_reconstruct_script = "/var/cm/script/cm_cnm_pool_reconstruct.sh";

sint32 cm_cnm_pool_reconstruct_init(void)
{
    return CM_OK;
}
static sint32 cm_cnm_pool_reconstruct_local_get_each(void *arg, sint8 **cols, uint32 col_num)
{
    cm_cnm_pool_reconstruct_info_t *info = arg;
    const uint32 def_num = 5;       
    if(def_num != col_num)
    {
        CM_LOG_WARNING(CM_MOD_CNM,"col_num[%u] def_num[%u]",col_num,def_num);
        return CM_FAIL;
    }
    CM_MEM_ZERO(info,sizeof(cm_cnm_pool_reconstruct_info_t));     
    CM_VSPRINTF(info->name,sizeof(info->name),"%s",cols[0]);
    info->nid = cm_node_get_id();
    CM_VSPRINTF(info->status,sizeof(info->status),"%s",cols[1]);
    CM_VSPRINTF(info->speed,sizeof(info->speed),"%s",cols[2]); 
    CM_VSPRINTF(info->time_cost,sizeof(info->time_cost),"%s",cols[3]); 
    CM_VSPRINTF(info->progress,sizeof(info->progress),"%s",cols[4]); 
    return CM_OK;
}
sint32 cm_cnm_pool_reconstruct_local_getbatch( 
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    sint32 iRet = CM_OK;
    sint8 cmd[CM_STRING_512] = {0};
    CM_VSPRINTF(cmd,CM_STRING_128,"%s getbatch",g_cm_cnm_pool_reconstruct_script);
    iRet = cm_cnm_exec_get_list(cmd,cm_cnm_pool_reconstruct_local_get_each,
    (uint32)offset,sizeof(cm_cnm_pool_reconstruct_info_t),ppAck,&total); 
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]",iRet);
        return iRet;
    }
    *pAckLen = total * sizeof(cm_cnm_pool_reconstruct_info_t);
    return CM_OK;
}    

#define cm_cnm_pool_reconstruct_request(cmd,param,ppAck,plen) \
    cm_cnm_request_comm(CM_OMI_OBJECT_POOL_RECONSTRUCT,cmd,sizeof(cm_cnm_pool_reconstruct_info_t),param,ppAck,plen)


sint32 cm_cnm_pool_reconstruct_getbatch(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_cnm_pool_reconstruct_request(CM_OMI_CMD_GET_BATCH,pDecodeParam,ppAckData,pAckLen);
}
sint32 cm_cnm_pool_reconstruct_local_count(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    uint64 cnt = 0;
    cnt = cm_exec_int("%s count",g_cm_cnm_pool_reconstruct_script);
    return cm_cnm_ack_uint64(cnt,ppAck,pAckLen);
}
sint32 cm_cnm_pool_reconstruct_count(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_cnm_pool_reconstruct_request(CM_OMI_CMD_COUNT,pDecodeParam,ppAckData,pAckLen);
}    
static sint32 cm_cnm_pool_reconstruct_decode_ext(
    const cm_omi_obj_t ObjParam,void* data,cm_omi_field_flag_t *set)
{
    sint32 iRet = CM_OK;
    cm_cnm_pool_reconstruct_info_t *info = data;
    cm_cnm_decode_param_t param_str[] = 
    {
        {CM_OMI_FIELD_POOL_RECONSTRUCT_NAME,sizeof(info->name),info->name,NULL},
        {CM_OMI_FIELD_POOL_RECONSTRUCT_STATUS,sizeof(info->status),info->status,NULL},   
        {CM_OMI_FIELD_POOL_RECONSTRUCT_PROGRESS,sizeof(info->progress),info->progress,NULL},
        {CM_OMI_FIELD_POOL_RECONSTRUCT_SPEED,sizeof(info->speed),info->speed,NULL},
        {CM_OMI_FIELD_POOL_RECONSTRUCT_TIME_COST,sizeof(info->time_cost),info->time_cost,NULL},    
    };
    cm_cnm_decode_param_t param_num[] = 
    {
        {CM_OMI_FIELD_POOL_RECONSTRUCT_NID,sizeof(info->nid),&info->nid,NULL},
    };
    iRet = cm_cnm_decode_str(ObjParam,param_str,
        sizeof(param_str)/sizeof(cm_cnm_decode_param_t),set);
    if(iRet != CM_OK)
    {
        return iRet;
    }
    iRet = cm_cnm_decode_num(ObjParam,param_num,
        sizeof(param_num)/sizeof(cm_cnm_decode_param_t),set);
    if(iRet != CM_OK)
    {
        return iRet;
    }
    return CM_OK;
}
sint32 cm_cnm_pool_reconstruct_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam)
{
    return cm_cnm_decode_comm(ObjParam,sizeof(cm_cnm_pool_reconstruct_info_t),
        cm_cnm_pool_reconstruct_decode_ext,ppDecodeParam);
}
static void cm_cnm_pool_reconstruct_encode_each(cm_omi_obj_t item,void *eachdata,void *arg)
{
    cm_omi_field_flag_t *field = arg;
    cm_cnm_pool_reconstruct_info_t *info = eachdata;
    cm_cnm_map_value_str_t cols_str[] = 
    {
        {CM_OMI_FIELD_POOL_RECONSTRUCT_NAME,  info->name},
        {CM_OMI_FIELD_POOL_RECONSTRUCT_STATUS,  info->status},    
        {CM_OMI_FIELD_POOL_RECONSTRUCT_PROGRESS,  info->progress},
        {CM_OMI_FIELD_POOL_RECONSTRUCT_SPEED,  info->speed},
        {CM_OMI_FIELD_POOL_RECONSTRUCT_TIME_COST,  info->time_cost},
    };
    cm_cnm_map_value_num_t cols_num[] = 
    {
        {CM_OMI_FIELD_POOL_RECONSTRUCT_NID,  info->nid},
    };
    cm_cnm_encode_str(item,field,cols_str,sizeof(cols_str)/sizeof(cm_cnm_map_value_str_t));
    cm_cnm_encode_num(item,field,cols_num,sizeof(cols_num)/sizeof(cm_cnm_map_value_num_t)); 
    return;
}
cm_omi_obj_t cm_cnm_pool_reconstruct_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen)
{
    return cm_cnm_encode_comm_ext(pDecodeParam,pAckData,AckLen,
        sizeof(cm_cnm_pool_reconstruct_info_t),cm_cnm_pool_reconstruct_encode_each);
}


/******************************************************************************/

static sint32 cm_cnm_poolext_decode_ext(
    const cm_omi_obj_t ObjParam,void* data,cm_omi_field_flag_t *set)
{
    sint32 iRet = CM_OK;
    cm_cnm_poolext_info_t *info = data;
    cm_cnm_decode_param_t param_str[] = 
    {
        {CM_OMI_FIELD_POOLEXT_NAME,sizeof(info->name),info->name,NULL},  
    };
    cm_cnm_decode_param_t param_num[] = 
    {
        {CM_OMI_FIELD_POOLEXT_FORCE,sizeof(info->isforce),&info->isforce,NULL},
        {CM_OMI_FIELD_POOLEXT_POLICY,sizeof(info->policy),&info->policy,NULL},
        
        {CM_OMI_FIELD_POOLEXT_DATA_RAID,sizeof(info->data_raid),&info->data_raid,NULL},
        {CM_OMI_FIELD_POOLEXT_DATA_NUM,sizeof(info->data_num),&info->data_num,NULL},
        {CM_OMI_FIELD_POOLEXT_DATA_GROUP,sizeof(info->data_group),&info->data_group,NULL},
        {CM_OMI_FIELD_POOLEXT_DATA_SPARE,sizeof(info->data_spare),&info->data_spare,NULL},

        {CM_OMI_FIELD_POOLEXT_META_RAID,sizeof(info->meta_raid),&info->meta_raid,NULL},
        {CM_OMI_FIELD_POOLEXT_META_NUM,sizeof(info->meta_num),&info->meta_num,NULL},
        {CM_OMI_FIELD_POOLEXT_META_GROUP,sizeof(info->meta_group),&info->meta_group,NULL},
        {CM_OMI_FIELD_POOLEXT_META_SPARE,sizeof(info->meta_spare),&info->meta_spare,NULL},

        {CM_OMI_FIELD_POOLEXT_LOW_RAID,sizeof(info->low_raid),&info->low_raid,NULL},
        {CM_OMI_FIELD_POOLEXT_LOW_NUM,sizeof(info->low_num),&info->low_num,NULL},
        {CM_OMI_FIELD_POOLEXT_LOW_GROUP,sizeof(info->low_group),&info->low_group,NULL},
        {CM_OMI_FIELD_POOLEXT_LOW_SPARE,sizeof(info->low_spare),&info->low_spare,NULL},
        
    };
    iRet = cm_cnm_decode_str(ObjParam,param_str,
        sizeof(param_str)/sizeof(cm_cnm_decode_param_t),set);
    if(iRet != CM_OK)
    {
        return iRet;
    }
    iRet = cm_cnm_decode_num(ObjParam,param_num,
        sizeof(param_num)/sizeof(cm_cnm_decode_param_t),set);
    if(iRet != CM_OK)
    {
        return iRet;
    }
    return CM_OK;
}


sint32 cm_cnm_poolext_decode(const cm_omi_obj_t ObjParam, void**ppDecodeParam)
{
    return cm_cnm_decode_comm(ObjParam,sizeof(cm_cnm_poolext_info_t),
        cm_cnm_poolext_decode_ext,ppDecodeParam);
}

sint32 cm_cnm_poolext_create(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    if(NULL == pDecodeParam)
    {
        CM_LOG_ERR(CM_MOD_CNM,"param null");
        return CM_PARAM_ERR;
    }
    return cm_cnm_request_comm(CM_OMI_OBJECT_POOLEXT,CM_OMI_CMD_ADD,
        sizeof(cm_cnm_poolext_info_t),pDecodeParam,ppAckData,pAckLen);
}

sint32 cm_cnm_poolext_add(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    if(NULL == pDecodeParam)
    {
        CM_LOG_ERR(CM_MOD_CNM,"param null");
        return CM_PARAM_ERR;
    }
    return cm_cnm_request_comm(CM_OMI_OBJECT_POOLEXT,CM_OMI_CMD_MODIFY,
        sizeof(cm_cnm_poolext_info_t),pDecodeParam,ppAckData,pAckLen);
}

static void cm_cnm_poolext_local_gettype(const uint32 *val,sint8* buff, sint32 len)
{
    uint32 raid=val[0];
    uint32 num=val[1];
    uint32 group=val[2];
    uint32 spare=val[3];

    if((0 == num) && (0 == group) && (0 == spare))
    {
        CM_SNPRINTF_ADD(buff,CM_STRING_512," '-'");
        return;
    }
    CM_SNPRINTF_ADD(buff,CM_STRING_512," '%u|%u|%u|%u'",raid,num,group,spare);
    return;
}

static sint32 cm_cnm_poolext_local_exec(uint32 act,void* param)
{
    const cm_cnm_decode_info_t *req = param;
    const cm_cnm_poolext_info_t *info = NULL;
    sint8 buff[CM_STRING_512] = {0};
    if(NULL == req)
    {
        CM_LOG_ERR(CM_MOD_CNM,"param null");
        return CM_PARAM_ERR;
    }
    info = (const cm_cnm_poolext_info_t *)req->data;

    if(0 == strlen(info->name))
    {
        CM_LOG_ERR(CM_MOD_CNM,"poolname null");
        return CM_PARAM_ERR;
    }
    CM_SNPRINTF_ADD(buff,CM_STRING_512,CM_SCRIPT_DIR"cm_pool.sh");
    if(CM_OMI_CMD_ADD == act)
    {
        CM_SNPRINTF_ADD(buff,CM_STRING_512," create %u %u '%s'",info->isforce,
            info->policy,info->name);
    }
    else
    {
        CM_SNPRINTF_ADD(buff,CM_STRING_512," add %u %u '%s'",info->isforce, 
            info->policy,info->name);
    }
    
    cm_cnm_poolext_local_gettype(&info->data_raid,buff,CM_STRING_512);

    cm_cnm_poolext_local_gettype(&info->meta_raid,buff,CM_STRING_512);

    cm_cnm_poolext_local_gettype(&info->low_raid,buff,CM_STRING_512);

    return cm_system(buff);
    
}
sint32 cm_cnm_poolext_local_create(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    return cm_cnm_poolext_local_exec(CM_OMI_CMD_ADD,param);
}

sint32 cm_cnm_poolext_local_add(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    return cm_cnm_poolext_local_exec(CM_OMI_CMD_MODIFY,param);
}

void cm_cnm_poolext_oplog_create(const sint8* sessionid, const void *pDecodeParam, sint32 
Result)
{
    return;
}

void cm_cnm_poolext_oplog_add(const sint8* sessionid, const void *pDecodeParam, sint32 
Result)
{
    return;
}


