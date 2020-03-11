 /******************************************************************************
 *          Copyright (c) 2018 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cnm_nas.h
 * author     : wbn
 * create date: 2018.07.07
 * description: TODO:
 *
 *****************************************************************************/

#include "cm_cnm_clusternas.h"
#include "cm_log.h"
#include "cm_xml.h"
#include "cm_node.h"
#include "cm_omi.h"
#include "cm_db.h"

const sint8* g_cm_cnm_cluster_nas_script = "/var/cm/script/cm_cnm_cluster_nas.sh";
const sint8* g_cm_cnm_cluster_nas_count = "zfs multiclus -v |grep 'Group number' |awk 'BEGIN{cnt=0}$5>0{cnt=cnt+1}END{print cnt}'";
extern const cm_omi_map_enum_t CmOmiMapNasRoleTypeCfg;
extern const cm_omi_map_enum_t CmOmiMapClusterStatusEnumBoolType;
extern const cm_omi_map_enum_t CmOmiMapNasStatusEnumBoolType;

const sint8* g_cm_cnm_cluster_nas_db = "/tmp/cm_cluster_nas_cache.db";
const sint8* g_cm_cnm_cluster_nas_db_tmp = "/tmp/cm_cluster_nas_cache.dbtmp";

#define CM_CNM_CLUSTER_NAS_USE_DB
static sint32 cm_cnm_nas_cluster_request(uint32 obj, uint32 cmd,
    uint32 eachsize, const void* pDecodeParam, void **ppAck, uint32 *pAckLen);
#define cm_cnm_cluster_nas_request(cmd,param,ppAck,plen) \
        cm_cnm_nas_cluster_request(CM_OMI_OBJECT_CLUSTER_NAS,cmd,sizeof(cm_cnm_cluster_nas_info_t),param,ppAck,plen)
static sint32 cm_cnm_nas_cluster_request(uint32 obj, uint32 cmd,
    uint32 eachsize, const void* pDecodeParam, void **ppAck, uint32 *pAckLen)
{
    const cm_cnm_decode_info_t *pDecode = pDecodeParam;
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
        param.param_len = sizeof(cm_cnm_decode_info_t)+eachsize;
    }
    if(param.nid == CM_NODE_ID_NONE)
    {
        param.nid = cm_node_get_id();
    }
    param.obj = obj;
    param.cmd = cmd;
    param.ppack = ppAck;
    param.acklen = pAckLen;
    return cm_cnm_request(&param);
}

sint32 cm_cnm_cluster_nas_getbatch(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
#ifdef CM_CNM_CLUSTER_NAS_USE_DB
    const cm_cnm_decode_info_t *pDecode = pDecodeParam;
    const cm_cnm_cluster_nas_info_t *info=NULL;
    sint8* sql = CM_MALLOC(CM_STRING_512);

    if(NULL == sql)
    {
        CM_LOG_ERR(CM_MOD_CNM,"malloc fail");
        return CM_FAIL;
    }
    
    CM_MEM_ZERO(sql,CM_STRING_512);
    if((NULL != pDecode)
        && (CM_OMI_FIELDS_FLAG_ISSET(&pDecode->set,CM_OMI_FIELD_CLUSTER_NAS_GROUP_NAME)))
    {
        info = (const cm_cnm_cluster_nas_info_t*)pDecode->data;
        CM_SNPRINTF_ADD(sql,CM_STRING_512,"SELECT "
            "nas as f3,nid as f1,status as f5, role as f4, avail as f6,used as f7"
            " FROM nas_t WHERE groupn='%s' LIMIT %llu,%u",
            info->group_name,pDecode->offset,pDecode->total);
    }
    else
    {
        CM_SNPRINTF_ADD(sql,CM_STRING_512,"SELECT "
            "groupn as f0,nid as f1,status as f2, nasnum as f8"
            " FROM group_t");
        if(NULL == pDecode)
        {
            CM_SNPRINTF_ADD(sql,CM_STRING_512," LIMIT %u",CM_CNM_MAX_RECORD);
        }
        else
        {
            CM_SNPRINTF_ADD(sql,CM_STRING_512," LIMIT %llu,%u",pDecode->offset,pDecode->total);
        }
    }
    *ppAckData = sql;
    *pAckLen = CM_STRING_512;
    return CM_OK;
#else
    return cm_cnm_cluster_nas_request(CM_OMI_CMD_GET_BATCH,pDecodeParam,ppAckData,pAckLen);
#endif    
}

#ifdef CM_CNM_CLUSTER_NAS_USE_DB
static cm_db_handle_t cm_cnm_cluster_nas_db_new(const sint8* path)
{
    cm_db_handle_t handle;
    const sint8* create_tables[]=
    {
        "CREATE TABLE IF NOT EXISTS group_t ("
            "groupn VARCHAR(128),"
            "nid INT,"
            "status TINYINT,"
            "nasnum INT)",
        "CREATE TABLE IF NOT EXISTS nas_t ("
            "groupn VARCHAR(128),"
            "nas VARCHAR(128),"
            "nid INT,"
            "status INT,"
            "role INT,"
            "avail VARCHAR(32),"
            "used VARCHAR(32))",
    };
    uint32 iloop = 0;
    uint32 table_cnt = sizeof(create_tables)/(sizeof(sint8*));
   
    sint32 iRet = cm_db_open_ext(path,&handle);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"open %s fail",path);
        return NULL;
    }

    while(iloop < table_cnt)
    {
        iRet = cm_db_exec_ext(handle,"%s",create_tables[iloop]);
        if(CM_OK != iRet)
        {
            cm_db_close(handle);
            return NULL;
        }
        iloop++;
    }
    return handle;
}
#endif

sint32 cm_cnm_cluster_nas_init(void)
{
#ifdef CM_CNM_CLUSTER_NAS_USE_DB
    cm_db_handle_t handle = cm_cnm_cluster_nas_db_new(g_cm_cnm_cluster_nas_db);
    if(NULL == handle)
    {        
        return CM_FAIL;
    }
    cm_db_close(handle);
#endif    
    return CM_OK;
}

#ifdef CM_CNM_CLUSTER_NAS_USE_DB
static void cm_cnm_cluster_nas_group_update(cm_db_handle_t handle, 
    cm_cnm_decode_info_t *pDecode,cm_cnm_cluster_nas_info_t* pGroup)
{
    sint32 iRet = CM_OK;
    void* pAck = NULL;
    uint32 len = 0;
    uint64 cnt = 0;
    uint32 getcnt = 0;
    cm_cnm_cluster_nas_info_t* pNas = (cm_cnm_cluster_nas_info_t*)pDecode->data;

    pDecode->offset = 0;
    CM_OMI_FIELDS_FLAG_SET(&pDecode->set,CM_OMI_FIELD_CLUSTER_NAS_GROUP_NAME);
    CM_MEM_CPY(pNas->group_name,sizeof(pNas->group_name),pGroup->group_name,sizeof(pNas->group_name));

    while(getcnt < pGroup->nas_num)
    {
        len = 0;
        iRet = cm_cnm_cluster_nas_request(CM_OMI_CMD_GET_BATCH,pDecode,&pAck,&len);
        if(CM_OK != iRet)
        {
            CM_LOG_ERR(CM_MOD_CNM,"nid:%u offset:%llu total:%u iRet:%d",
                pDecode->nid,pDecode->offset,pDecode->total,iRet);
            break;
        }
        
        if((NULL == pAck) || (0 == len))
        {
            break;
        }
        
        for(pNas = pAck,len = len/sizeof(cm_cnm_cluster_nas_info_t),getcnt+=len;
            len>0; pNas++,len--)
        {
            cnt = 0;
            (void)cm_db_exec_get_count(handle,&cnt,
                "SELECT COUNT(nas) FROM nas_t WHERE groupn='%s' AND nas='%s'",
                pGroup->group_name,pNas->nas);
            if(0 != cnt)
            {
                continue;
            }
            (void)cm_db_exec_ext(handle,"INSERT INTO nas_t "
                "VALUES('%s','%s',%u,%u,%u,'%s','%s')",pGroup->group_name,pNas->nas,
                pNas->nid, pNas->status, pNas->role, pNas->avail,pNas->used);
        }
        CM_FREE(pAck);
        pAck = NULL;

        pDecode->offset += pDecode->total;
    }
    return;
}

static sint32 cm_cnm_cluster_nas_node(cm_node_info_t *pNode, void *pArg)
{
    cm_db_handle_t handle = pArg;
    const uint32 max_size = 20;
    sint32 iRet = CM_OK;
    void* pAck = NULL;
    uint64 cnt = 0;
    uint64 offset = 0;
    cm_cnm_cluster_nas_info_t* pGroup = NULL;
    uint32 len = sizeof(cm_cnm_decode_info_t)+sizeof(cm_cnm_cluster_nas_info_t);
    cm_cnm_decode_info_t *pDecode = CM_MALLOC(len);

    if(NULL == pDecode)
    {
        CM_LOG_ERR(CM_MOD_CNM,"malloc fail");
        return CM_FAIL;
    }
    CM_MEM_ZERO(pDecode,len);
    pDecode->nid = pNode->id;
    pDecode->total = max_size;
    while(1)
    {
        /* 清除下发组名，循环查询组信息 */
        CM_OMI_FIELDS_FLAG_CLR(&pDecode->set,CM_OMI_FIELD_CLUSTER_NAS_GROUP_NAME);
        
        len = 0;
        iRet = cm_cnm_cluster_nas_request(CM_OMI_CMD_GET_BATCH,pDecode,&pAck,&len);
        if(CM_OK != iRet)
        {
            CM_LOG_ERR(CM_MOD_CNM,"nid:%u offset:%llu total:%u iRet:%d",
                pDecode->nid,pDecode->offset,pDecode->total,iRet);
            break;
        }
        
        if((NULL == pAck) || (0 == len))
        {
            break;
        }
        offset = pDecode->offset;
        for(pGroup = pAck,len = len/sizeof(cm_cnm_cluster_nas_info_t);
            len>0; pGroup++,len--)
        {
            cnt = 0;
            (void)cm_db_exec_get_count(handle,&cnt,
                "SELECT COUNT(groupn) FROM group_t WHERE groupn='%s'",pGroup->group_name);
            if(0 != cnt)
            {
                continue;
            }
            (void)cm_db_exec_ext(handle,"INSERT INTO group_t "
                "VALUES('%s',%u,%u,%u)",pGroup->group_name,pDecode->nid,
                pGroup->cluster_status, pGroup->nas_num);
            cm_cnm_cluster_nas_group_update(handle,pDecode,pGroup);
        }
        CM_FREE(pAck);
        pAck = NULL;

        pDecode->offset = offset + pDecode->total;
    }
    CM_FREE(pDecode);
    return iRet;
}
#endif
void cm_cnm_cluster_nas_thread(void)
{
#ifdef CM_CNM_CLUSTER_NAS_USE_DB
    cm_db_handle_t handle = NULL;
#ifndef CM_OMI_LOCAL    
    if(cm_node_get_master() != cm_node_get_id())
    {
        /* 仅主节点更新 */
        return;
    }
#endif
    cm_system("rm -f %s",g_cm_cnm_cluster_nas_db_tmp);
    handle = cm_cnm_cluster_nas_db_new(g_cm_cnm_cluster_nas_db_tmp);
    if(NULL == handle)
    {
        return;
    }
    (void)cm_node_traversal_all(cm_cnm_cluster_nas_node,handle,CM_FALSE);
    cm_db_close(handle);
    cm_system("mv %s %s",g_cm_cnm_cluster_nas_db_tmp,g_cm_cnm_cluster_nas_db);
#endif    
    return;    
}

sint32  cm_cnm_cluster_nas_local_create(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    const cm_cnm_decode_info_t *req = param;
    const cm_cnm_cluster_nas_info_t *info = NULL;
    uint32 nascheck = 0;
    uint32 cnt=0;
    sint8 buff[CM_STRING_1K] = {0};
    if(NULL == req)
    {
        return CM_PARAM_ERR;
    }
    info = (const cm_cnm_cluster_nas_info_t *)req->data;
    nascheck = cm_exec_int("zfs list -H -t filesystem -o name %s 2>/dev/null |wc -l",info->nas);
    if(0==nascheck)
    {
        return CM_ERR_NOT_EXISTS; 
    }
    cnt = cm_exec_int("zfs multiclus -v | grep -w '%s' | wc -l",info->nas);
    if(cnt != 0)
    {
        return CM_ERR_ALREADY_EXISTS; 
    }
    
    return cm_exec_out(ppAck,pAckLen,CM_CMT_REQ_TMOUT_NEVER,
            "zfs multiclus create %s %s 2>&1",info->group_name,info->nas);
}    

sint32  cm_cnm_cluster_nas_local_add(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    const cm_cnm_decode_info_t *req = param;
    const cm_cnm_cluster_nas_info_t *info = NULL;
    const sint8* str = NULL;
    uint32 cnt = 0;
    uint32 nascheck = 0;
    sint8 buff[CM_STRING_1K] = {0};
    sint32 iRet = CM_OK;
    
    if(NULL == req)
    {
        return CM_PARAM_ERR;
    }
    info = (const cm_cnm_cluster_nas_info_t *)req->data;
    nascheck = cm_exec_int("zfs list -H -t filesystem -o name %s 2>/dev/null |wc -l",info->nas);
    if(0==nascheck)
    {
        return CM_ERR_NOT_EXISTS; 
    }
    cnt = cm_exec_int("zfs multiclus -v | grep -w '%s' | wc -l",info->nas);
    if(0==cnt)
    {
        iRet = cm_exec_out(ppAck,pAckLen,CM_CMT_REQ_TMOUT_NEVER,
            "zfs multiclus add %s %s 2>&1",info->group_name,info->nas);
    }
    if(CM_OMI_FIELDS_FLAG_ISSET(&req->set,CM_OMI_FIELD_CLUSTER_NAS_ZFS_ROLE))
    {
        str = cm_cnm_get_enum_str(&CmOmiMapNasRoleTypeCfg,info->role);
        (void)cm_exec_tmout(buff,sizeof(buff),CM_CMT_REQ_TMOUT,
            "%s setrole %s %s %s",g_cm_cnm_cluster_nas_script,str,info->group_name,info->nas);
        if(NULL != strstr(buff,"Fail"))
        {
            CM_LOG_ERR(CM_MOD_CNM,"%s",buff);
        } 
    }
    if((cnt != 0)
        &&(!CM_OMI_FIELDS_FLAG_ISSET(&req->set,CM_OMI_FIELD_CLUSTER_NAS_ZFS_ROLE)))
    { 
        return CM_ERR_ALREADY_EXISTS; 
    }
    return iRet;
}  

sint32  cm_cnm_cluster_nas_local_delete(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    const cm_cnm_decode_info_t *req = param;
    const cm_cnm_cluster_nas_info_t *info = NULL;
    const sint8* str = NULL;
    uint32 cnt = 0;
    uint32 nascheck = 0;
    sint8 buff[CM_STRING_1K] = {0};
    sint32 iRet = CM_OK;
    if(NULL == req)
    {
        return CM_PARAM_ERR;
    }
    info = (const cm_cnm_cluster_nas_info_t *)req->data;
    nascheck = cm_exec_int("zfs list -H -t filesystem -o name %s 2>/dev/null |wc -l",info->nas);
    if(0==nascheck)
    {
       CM_LOG_ERR(CM_MOD_CNM,"there is no nas named %s",info->nas); 
       return CM_ERR_NOT_EXISTS; 
    }
    cnt = cm_exec_int("zfs multiclus -v | grep -w '%s' | wc -l",info->nas);
    if(cnt > 0)
    {
        iRet = cm_exec_out(ppAck,pAckLen,CM_CMT_REQ_TMOUT,
            "%s delete %s %s 2>&1",g_cm_cnm_cluster_nas_script,info->group_name,info->nas);
    }
    if(cnt == 0)
    { 
        CM_LOG_ERR(CM_MOD_CNM,"nas %s is no member in cluster %s",info->nas,info->group_name);
        return CM_ERR_NOT_EXISTS; 
    }
    return iRet;
}  

sint32 cm_cnm_cluster_nas_create(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_cnm_cluster_nas_request(CM_OMI_CMD_ADD,pDecodeParam,ppAckData,pAckLen);
}  

sint32 cm_cnm_cluster_nas_add(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_cnm_cluster_nas_request(CM_OMI_CMD_MODIFY,pDecodeParam,ppAckData,pAckLen);
}    

sint32 cm_cnm_cluster_nas_delete(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_cnm_cluster_nas_request(CM_OMI_CMD_DELETE,pDecodeParam,ppAckData,pAckLen);
}

sint32 cm_cnm_cluster_nas_count(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
#ifdef CM_CNM_CLUSTER_NAS_USE_DB
    const cm_cnm_decode_info_t *pDecode = pDecodeParam;
    const cm_cnm_cluster_nas_info_t *info=NULL;
    uint64 cnt = 0;

    if(NULL != pDecode)
    {
        sint8 buff[CM_STRING_256] = {0};
        info = (const cm_cnm_cluster_nas_info_t*)pDecode->data;
        if((CM_OMI_FIELDS_FLAG_ISSET(&pDecode->set,CM_OMI_FIELD_CLUSTER_NAS_GROUP_NAME))
            &&(CM_OMI_FIELDS_FLAG_ISSET(&pDecode->set,CM_OMI_FIELD_CLUSTER_NAS_ZFS_ROLE)))
        {
            CM_SNPRINTF_ADD(buff,sizeof(buff),
                "SELECT COUNT(nas) FROM nas_t WHERE groupn='%s' AND role>=%u",
                info->group_name,info->role);
        }
        else if(CM_OMI_FIELDS_FLAG_ISSET(&pDecode->set,CM_OMI_FIELD_CLUSTER_NAS_GROUP_NAME))
        {
            CM_SNPRINTF_ADD(buff,sizeof(buff),
                "SELECT COUNT(nas) FROM nas_t WHERE groupn='%s'",
                info->group_name);
        }
        else if(CM_OMI_FIELDS_FLAG_ISSET(&pDecode->set,CM_OMI_FIELD_CLUSTER_NAS_ZFS_ROLE))
        {
            CM_SNPRINTF_ADD(buff,sizeof(buff),
                "SELECT COUNT(nas) FROM nas_t WHERE role>=%u",
                info->role);
        }
        else
        {
            CM_SNPRINTF_ADD(buff,sizeof(buff),
                "SELECT COUNT(nas) FROM nas_t");
        }
        (void)cm_db_exec_file_count(g_cm_cnm_cluster_nas_db,&cnt,buff);
    }
    else
    {
        (void)cm_db_exec_file_count(g_cm_cnm_cluster_nas_db,&cnt,
            "SELECT COUNT(groupn) FROM group_t");
    }
    
    return cm_cnm_ack_uint64(cnt,ppAckData,pAckLen);
    
#else
    return cm_cnm_cluster_nas_request(CM_OMI_CMD_COUNT,pDecodeParam,ppAckData,pAckLen);
#endif
}    

sint32 cm_cnm_cluster_nas_local_count(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    uint64 cnt = 0;
    cm_cnm_decode_info_t *dec = param;
    cm_cnm_cluster_nas_info_t *info=NULL;
    uint32 groupcheck=0;
    sint8 cmd[CM_STRING_512] = {0};
    if((dec != NULL)
        && (CM_OMI_FIELDS_FLAG_ISSET(&dec->set,CM_OMI_FIELD_CLUSTER_NAS_GROUP_NAME)))     
    {
        info = (cm_cnm_cluster_nas_info_t*)dec->data;
        groupcheck = cm_exec_int("zfs multiclus -v | grep -w '%s' 2>/dev/null |wc -l",info->group_name);
        if(groupcheck==0)
        {
            return CM_PARAM_ERR; 
        }
        CM_VSPRINTF(cmd,CM_STRING_128,"%s count %s",g_cm_cnm_cluster_nas_script,info->group_name);
    }
    else
    {
        CM_VSPRINTF(cmd,CM_STRING_128,"%s",g_cm_cnm_cluster_nas_count);
    }
    cnt = (uint64)cm_exec_int("%s 2>/dev/null",cmd);
     
    return cm_cnm_ack_uint64(cnt,ppAck,pAckLen);
}    

static sint32 cm_cnm_cluster_nas_local_get_each_member(void *arg, sint8 **cols, uint32 col_num)
{
    cm_cnm_cluster_nas_info_t *info = arg;
    const uint32 def_num = 6;       
    if(def_num != col_num)
    {
        CM_LOG_WARNING(CM_MOD_CNM,"col_num[%u] def_num[%u]",col_num,def_num);
        return CM_FAIL;
    }
    CM_MEM_ZERO(info,sizeof(cm_cnm_cluster_nas_info_t));
    CM_VSPRINTF(info->nas,sizeof(info->nas),"%s",cols[0]);
    info->role = (uint8)cm_cnm_get_enum(&CmOmiMapNasRoleTypeCfg,cols[1],0);
    info->status=(0 == strcmp(cols[2],"online")) ? CM_TRUE : CM_FALSE;
    CM_VSPRINTF(info->avail,sizeof(info->avail),"%s",cols[3]);
    CM_VSPRINTF(info->used,sizeof(info->used),"%s",cols[4]);
    info->nid = cm_node_get_id_by_inter((uint32)atoi(cols[5]));
    return CM_OK;
}
static sint32 cm_cnm_cluster_nas_local_get_each_group(void *arg, sint8 **cols, uint32 col_num)
{
    cm_cnm_cluster_nas_info_t *info = arg;
    const uint32 def_num = 4;       
    if(def_num != col_num)
    {
        CM_LOG_INFO(CM_MOD_CNM,"col_num[%u] def_num[%u]",col_num,def_num);
        return CM_FAIL;
    }
    CM_MEM_ZERO(info,sizeof(cm_cnm_cluster_nas_info_t));     
    CM_VSPRINTF(info->group_name,sizeof(info->group_name),"%s",cols[0]);
    info->cluster_status=(0 == strcmp(cols[1],"up")) ? CM_TRUE : CM_FALSE;
    info->nas_num = atoi(cols[2]);
    info->nid = cm_node_get_id_by_inter((uint32)atoi(cols[3]));
    return CM_OK;
}

sint32 cm_cnm_cluster_nas_local_getbatch(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    sint32 iRet = CM_OK;
    uint32 groupcheck=0;
    sint8 cmd[CM_STRING_512] = {0};
    cm_cnm_decode_info_t *dec=param;
    cm_cnm_cluster_nas_info_t *info=NULL;

    if((dec != NULL)
        &&(CM_OMI_FIELDS_FLAG_ISSET(&dec->set,CM_OMI_FIELD_CLUSTER_NAS_GROUP_NAME)))
    {
        info = (cm_cnm_cluster_nas_info_t*)dec->data;
        groupcheck = cm_exec_int("zfs multiclus -v | grep -w '%s' 2>/dev/null |wc -l",info->group_name);
        if(groupcheck==0)
        {
            return CM_PARAM_ERR; 
        }
        CM_VSPRINTF(cmd,CM_STRING_128,"%s get %s",g_cm_cnm_cluster_nas_script,info->group_name);
        iRet = cm_cnm_exec_get_list(cmd,cm_cnm_cluster_nas_local_get_each_member,
            (uint32)offset,sizeof(cm_cnm_cluster_nas_info_t),ppAck,&total);
    }
    else
    {
        CM_VSPRINTF(cmd,CM_STRING_128,"%s group",g_cm_cnm_cluster_nas_script);
        iRet = cm_cnm_exec_get_list(cmd,cm_cnm_cluster_nas_local_get_each_group,
    (uint32)offset,sizeof(cm_cnm_cluster_nas_info_t),ppAck,&total); 
    }
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]",iRet);
        return iRet;
    }
    *pAckLen = total * sizeof(cm_cnm_cluster_nas_info_t);
    return CM_OK;
    
}    

static sint32 cm_cnm_cluster_nas_decode_check_role(void *val)
{
    uint8 role = *((uint8*)val);
    const sint8* str = cm_cnm_get_enum_str(&CmOmiMapNasRoleTypeCfg,role);
    if(NULL == str)
    {
        CM_LOG_ERR(CM_MOD_CNM,"role[%u]",role);
        return CM_PARAM_ERR;
    }
    
    return CM_OK;
}

static sint32 cm_cnm_cluster_nas_decode_check_status(void *val)
{
    uint8 status = *((uint8*)val);
    const sint8* str = cm_cnm_get_enum_str(&CmOmiMapClusterStatusEnumBoolType,status);
    if(NULL == str)
    {
        CM_LOG_ERR(CM_MOD_CNM,"status[%u]",status);
        return CM_PARAM_ERR;
    }
    
    return CM_OK;
}
static sint32 cm_cnm_cluster_nas_decode_check_nas_status(void *val)
{
    uint8 nassta = *((uint8*)val);
    const sint8* str = cm_cnm_get_enum_str(&CmOmiMapNasStatusEnumBoolType,nassta);
    if(NULL == str)
    {
        CM_LOG_ERR(CM_MOD_CNM,"nassta[%u]",nassta);
        return CM_PARAM_ERR;
    }
    
    return CM_OK;
}

static sint32 cm_cnm_cluster_nas_decode_ext(
    const cm_omi_obj_t ObjParam,void* data,cm_omi_field_flag_t *set)
{
    sint32 iRet = CM_OK;
    cm_cnm_cluster_nas_info_t *info = data;
    cm_cnm_decode_param_t param_str[] = 
    {
        {CM_OMI_FIELD_CLUSTER_NAS_GROUP_NAME,sizeof(info->group_name),info->group_name,NULL},
        {CM_OMI_FIELD_CLUSTER_NAS_ZFS_NAME,sizeof(info->nas),info->nas,NULL},
        {CM_OMI_FIELD_CLUSTER_NAS_ZFS_AVAIL,sizeof(info->avail),info->avail,NULL},
        {CM_OMI_FIELD_CLUSTER_NAS_ZFS_USED,sizeof(info->used),info->used,NULL},    
    };

    cm_cnm_decode_param_t param_num[] = 
    {
        {CM_OMI_FIELD_CLUSTER_NAS_STATUS,sizeof(info->cluster_status),&info->cluster_status,cm_cnm_cluster_nas_decode_check_status},
        {CM_OMI_FIELD_CLUSTER_NAS_ZFS_ROLE,sizeof(info->role),&info->role,cm_cnm_cluster_nas_decode_check_role}, 
        {CM_OMI_FIELD_CLUSTER_NAS_ZFS_STATUS,sizeof(info->status),&info->status,cm_cnm_cluster_nas_decode_check_nas_status},
        {CM_OMI_FIELD_CLUSTER_NAS_NID,sizeof(info->nid),&info->nid,NULL},
        {CM_OMI_FIELD_CLUSTER_NAS_NUM,sizeof(info->nas_num),&info->nas_num,NULL},
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

sint32 cm_cnm_cluster_nas_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam)
{
    return cm_cnm_decode_comm(ObjParam,sizeof(cm_cnm_cluster_nas_info_t),
        cm_cnm_cluster_nas_decode_ext,ppDecodeParam);
}

#ifdef CM_CNM_CLUSTER_NAS_USE_DB
cm_omi_obj_t cm_cnm_cluster_nas_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen)
{
    cm_omi_obj_t items = NULL;
    sint8 *sql = pAckData;
    cm_db_handle_t handle = NULL;
    sint32 iRet = CM_OK;
    if(NULL == sql)
    {
        return NULL;
    }
    
    items = cm_omi_obj_new_array();
    if(NULL == items)
    {
        CM_LOG_ERR(CM_MOD_CNM,"new items fail");
        return NULL;
    }
    iRet = cm_db_open(g_cm_cnm_cluster_nas_db,&handle);
    if(CM_OK == iRet)
    {
        (void)cm_db_exec(handle,cm_omi_encode_db_record_each,items,"%s",sql);
        cm_db_close(handle);
    }    
    return items;
}
#else
static void cm_cnm_cluster_nas_encode_each(cm_omi_obj_t item,void *eachdata,void *arg)
{
    cm_omi_field_flag_t *field = arg;
    cm_cnm_cluster_nas_info_t *info = eachdata;
    cm_cnm_map_value_str_t cols_str[] = 
    {
        {CM_OMI_FIELD_CLUSTER_NAS_GROUP_NAME,     info->group_name},
        {CM_OMI_FIELD_CLUSTER_NAS_ZFS_NAME,     info->nas},
        {CM_OMI_FIELD_CLUSTER_NAS_ZFS_AVAIL,  info->avail},
        {CM_OMI_FIELD_CLUSTER_NAS_ZFS_USED,  info->used},
    };
    cm_cnm_map_value_num_t cols_num[] = 
    {
        {CM_OMI_FIELD_CLUSTER_NAS_NID,          info->nid},
        {CM_OMI_FIELD_CLUSTER_NAS_STATUS,    (uint32)info->cluster_status},
        {CM_OMI_FIELD_CLUSTER_NAS_ZFS_STATUS, (uint32)info->status},
        {CM_OMI_FIELD_CLUSTER_NAS_ZFS_ROLE, (uint32)info->role},
        {CM_OMI_FIELD_CLUSTER_NAS_NUM,    (uint32)info->nas_num},
    };
    cm_cnm_encode_str(item,field,cols_str,sizeof(cols_str)/sizeof(cm_cnm_map_value_str_t));
    cm_cnm_encode_num(item,field,cols_num,sizeof(cols_num)/sizeof(cm_cnm_map_value_num_t)); 
    return;
}

cm_omi_obj_t cm_cnm_cluster_nas_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen)
{
    cm_omi_field_flag_t field;
    const cm_cnm_decode_info_t *param = pDecodeParam;
    CM_OMI_FIELDS_FLAG_CLR_ALL(&field);
    if((param != NULL)
        && (CM_OMI_FIELDS_FLAG_ISSET(&param->set,CM_OMI_FIELD_CLUSTER_NAS_GROUP_NAME)))
    {
        CM_OMI_FIELDS_FLAG_SET(&field,CM_OMI_FIELD_CLUSTER_NAS_ZFS_NAME);
        CM_OMI_FIELDS_FLAG_SET(&field,CM_OMI_FIELD_CLUSTER_NAS_NID);
        CM_OMI_FIELDS_FLAG_SET(&field,CM_OMI_FIELD_CLUSTER_NAS_ZFS_STATUS);
        CM_OMI_FIELDS_FLAG_SET(&field,CM_OMI_FIELD_CLUSTER_NAS_ZFS_ROLE);
        CM_OMI_FIELDS_FLAG_SET(&field,CM_OMI_FIELD_CLUSTER_NAS_ZFS_USED);
        CM_OMI_FIELDS_FLAG_SET(&field,CM_OMI_FIELD_CLUSTER_NAS_ZFS_AVAIL);
    }
    else
    {
        CM_OMI_FIELDS_FLAG_SET(&field,CM_OMI_FIELD_CLUSTER_NAS_GROUP_NAME);
        CM_OMI_FIELDS_FLAG_SET(&field,CM_OMI_FIELD_CLUSTER_NAS_NID);
        CM_OMI_FIELDS_FLAG_SET(&field,CM_OMI_FIELD_CLUSTER_NAS_STATUS);
        CM_OMI_FIELDS_FLAG_SET(&field,CM_OMI_FIELD_CLUSTER_NAS_NUM);
    }
    return cm_cnm_encode_comm(pAckData,AckLen,sizeof(cm_cnm_cluster_nas_info_t),cm_cnm_cluster_nas_encode_each,&field);
}
#endif
static void cm_cnm_cluster_nas_oplog_report(
    const sint8* sessionid, const void *pDecodeParam, uint32 alarmid)
{
    const cm_cnm_decode_info_t *req = pDecodeParam;
    const uint32 cnt = 4;
    if(NULL == pDecodeParam)
    {   
        cm_cnm_oplog_report(sessionid,alarmid,NULL,cnt,NULL);
    }    
    else    
    {       
        const cm_cnm_cluster_nas_info_t *info = (const cm_cnm_cluster_nas_info_t*)req->data;
        const sint8* nodename = cm_node_get_name(info->nid);
        cm_cnm_oplog_param_t params[4] = {
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_CLUSTER_NAS_GROUP_NAME,strlen(info->group_name),info->group_name},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_CLUSTER_NAS_NID,strlen(nodename),nodename},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_CLUSTER_NAS_ZFS_NAME,strlen(info->nas),info->nas},
            {CM_OMI_DATA_INT,   CM_OMI_FIELD_CLUSTER_NAS_ZFS_ROLE,sizeof(info->role),&info->role},
        };
        cm_cnm_oplog_report(sessionid,alarmid,params,cnt,&req->set);
#ifdef CM_CNM_CLUSTER_NAS_USE_DB
        if(alarmid == CM_ALARM_LOG_NAS_CLUSTER_UPDATE_OK)
        {
            cm_db_handle_t handle = NULL;
            alarmid = cm_db_open(g_cm_cnm_cluster_nas_db,&handle);
            if(CM_OK == alarmid)
            {
                uint64 cnt=0;
                cm_db_exec_get_count(handle,&cnt,
                    "SELECT COUNT(*) FROM nas_t WHERE groupn='%s' AND nas='%s'",
                    info->group_name,info->nas);
                if(0 == cnt)
                {                
                    (void)cm_db_exec_ext(handle,"UPDATE group_t SET nasnum=nasnum "
                        "WHERE groupn='%u'",info->group_name);
                    (void)cm_db_exec_ext(handle,"INSERT INTO nas_t "
                        "VALUES('%s','%s',%u,%u,%u,'-','-')",info->group_name,info->nas,
                        info->nid, 1, 0);    
                }
                cm_db_close(handle);
            }  
        }
#endif     
    }    
    return;
}    
void cm_cnm_cluster_nas_oplog_update(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK)? CM_ALARM_LOG_NAS_CLUSTER_UPDATE_OK : CM_ALARM_LOG_NAS_CLUSTER_UPDATE_FAIL;
    cm_cnm_cluster_nas_oplog_report(sessionid,pDecodeParam,alarmid);
    return;
}
void cm_cnm_cluster_nas_oplog_create(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK)? CM_ALARM_LOG_NAS_CLUSTER_CREATE_OK : CM_ALARM_LOG_NAS_CLUSTER_CREATE_FAIL;
    const cm_cnm_decode_info_t *req = pDecodeParam;
    const uint32 cnt = 3;
    if(NULL == pDecodeParam)
    {   
        cm_cnm_oplog_report(sessionid,alarmid,NULL,cnt,NULL);
    }    
    else    
    {       
        const cm_cnm_cluster_nas_info_t *info = (const cm_cnm_cluster_nas_info_t*)req->data;
        const sint8* nodename = cm_node_get_name(info->nid);
        cm_cnm_oplog_param_t params[3] = {
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_CLUSTER_NAS_GROUP_NAME,strlen(info->group_name),info->group_name},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_CLUSTER_NAS_NID,strlen(nodename),nodename},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_CLUSTER_NAS_ZFS_NAME,strlen(info->nas),info->nas},
        };
        cm_cnm_oplog_report(sessionid,alarmid,params,cnt,&req->set);

#ifdef CM_CNM_CLUSTER_NAS_USE_DB
        if(Result == CM_OK)
        {
            cm_db_handle_t handle = NULL;
            Result = cm_db_open(g_cm_cnm_cluster_nas_db,&handle);
            if(CM_OK == Result)
            {
                uint64 cnt=0;
                cm_db_exec_get_count(handle,&cnt,
                    "SELECT COUNT(*) FROM nas_t WHERE groupn='%s' AND nas='%s'",
                    info->group_name,info->nas);
                if(0 == cnt)
                {   
                    (void)cm_db_exec_ext(handle,"INSERT INTO group_t "
                        "VALUES('%s',%u,%u,%u)",info->group_name,info->nid,1,1);
                    (void)cm_db_exec_ext(handle,"INSERT INTO nas_t "
                        "VALUES('%s','%s',%u,%u,%u,'-','-')",info->group_name,info->nas,
                        info->nid, 1, 1);  
                }
                cm_db_close(handle);
            }  
        }
#endif
    }   
    return; 
}

