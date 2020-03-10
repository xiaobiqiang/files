/******************************************************************************
 *          Copyright (c) 2018 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cnm_nas.c
 * author     : wbn
 * create date: 2018.07.07
 * description: TODO:
 *
 *****************************************************************************/
#include "cm_cnm_nas.h"
#include "cm_log.h"
#include "cm_xml.h"
#include "cm_node.h"
#include "cm_omi.h"
#include "cm_db.h"
#include "cm_sche.h"
#include "cm_sync.h"
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>


extern const cm_omi_map_enum_t CmOmiMapLunSyncTypeCfg;

/*,,sharenfs,,sharesmb,,,compression,sync,zfs:
single_data*/
static const sint8 *g_cm_cnm_nas_get_cmd = NULL;

sint32 cm_cnm_nas_init(void)
{
    switch(g_cm_sys_ver)
    {
        case CM_SYS_VER_SOLARIS_V7R16:
            g_cm_cnm_nas_get_cmd = "zfs list -H -t filesystem -o "
                "name,used,avail,quota,recordsize,"
                "compression,sync,dedup,sharesmb,"
                "aclinherit";
            break;
        default:
            g_cm_cnm_nas_get_cmd = "zfs list -H -t filesystem -o "
                "name,used,avail,quota,recordsize,"
                "compression,sync,dedup,sharesmb,origin:sync,nasavsbw,bandwidth,"
                "aclinherit";
            break;
    }
    return CM_OK;
}

static sint32 cm_cnm_nas_decode_check_blocksize(void *val)
{
    uint16 bs = *((uint16*)val);
    if(!cm_cnm_check_blocksize(bs))
    {
        CM_LOG_ERR(CM_MOD_CNM,"blocksize[%u]",bs);
        return CM_PARAM_ERR;
    }
    return CM_OK;
}

static sint32 cm_cnm_nas_decode_check_write_policy(void *val)
{
    uint8 write_policy = *((uint8*)val);
    const sint8* str = cm_cnm_get_enum_str(&CmOmiMapLunSyncTypeCfg,write_policy);
    if(NULL == str)
    {
        CM_LOG_ERR(CM_MOD_CNM,"write_policy[%u]",write_policy);
        return CM_PARAM_ERR;
    }
    
    return CM_OK;
}

static sint32 cm_cnm_nas_decode_check_quota(void *val)
{
    sint8 *quota = val;
    if(CM_OK == cm_regex_check(quota,"^0[.]{0,1}[0]{0,3}[KMTGPkmtgp]{0,1}"))
    {
        /* 如果设置为0时，表示清除 */
        strcpy(quota,"none");
    }
    
    return CM_OK;
}

static sint32 cm_cnm_nas_decode_ext(
    const cm_omi_obj_t ObjParam,void* data,cm_omi_field_flag_t *set)
{
    sint32 iRet = CM_OK;
    cm_cnm_nas_info_t *info = data;
    cm_cnm_decode_param_t param_str[] = 
    {
        {CM_OMI_FIELD_NAS_NAME,sizeof(info->name),info->name,NULL},
        {CM_OMI_FIELD_NAS_POOL,sizeof(info->pool),info->pool,NULL},
        {CM_OMI_FIELD_NAS_QOS,sizeof(info->qos_val),info->qos_val,NULL},
        {CM_OMI_FIELD_NAS_QUOTA,sizeof(info->quota),info->quota,cm_cnm_nas_decode_check_quota},
        {CM_OMI_FIELD_NAS_QOS_AVS,sizeof(info->qos_avs),info->qos_avs,NULL},
    };

    cm_cnm_decode_param_t param_num[] = 
    {
        {CM_OMI_FIELD_NAS_BLOCKSIZE,sizeof(info->blocksize),&info->blocksize,cm_cnm_nas_decode_check_blocksize},
        {CM_OMI_FIELD_NAS_ACCESS,sizeof(info->access),&info->access,NULL},
        {CM_OMI_FIELD_NAS_WRITE_POLICY,sizeof(info->write_policy),&info->write_policy,cm_cnm_nas_decode_check_write_policy},
        {CM_OMI_FIELD_NAS_IS_COMP,sizeof(info->is_compress),&info->is_compress,cm_cnm_decode_check_bool},
        {CM_OMI_FIELD_NAS_DEDUP,sizeof(info->dedup),&info->dedup,cm_cnm_decode_check_bool},
        {CM_OMI_FIELD_NAS_SMB,sizeof(info->smb),&info->smb,cm_cnm_decode_check_bool},
        {CM_OMI_FIELD_NAS_NID,sizeof(info->nid),&info->nid,NULL},
        {CM_OMI_FIELD_NAS_ABE,sizeof(info->abe),&info->abe,cm_cnm_decode_check_bool},
        {CM_OMI_FIELD_NAS_ACLINHERIT,sizeof(info->aclinherit),&info->aclinherit,NULL},
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

sint32 cm_cnm_nas_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam)
{
    return cm_cnm_decode_comm(ObjParam,sizeof(cm_cnm_nas_info_t),
        cm_cnm_nas_decode_ext,ppDecodeParam);
}

static void cm_cnm_nas_encode_each(cm_omi_obj_t item,void *eachdata,void *arg)
{
    cm_omi_field_flag_t *field = arg;
    cm_cnm_nas_info_t *info = eachdata;
    cm_cnm_map_value_str_t cols_str[] = 
    {
        {CM_OMI_FIELD_NAS_NAME,     info->name},
        {CM_OMI_FIELD_NAS_POOL,     info->pool},
        {CM_OMI_FIELD_NAS_SPACE_AVAIL,    info->space_avail},
        {CM_OMI_FIELD_NAS_SPACE_USED,     info->space_used},
        {CM_OMI_FIELD_NAS_QOS,  info->qos_val},
        {CM_OMI_FIELD_NAS_QUOTA,  info->quota},
        {CM_OMI_FIELD_NAS_QOS_AVS,  info->qos_avs},
    };
    cm_cnm_map_value_num_t cols_num[] = 
    {
        {CM_OMI_FIELD_NAS_NID,          info->nid},
        {CM_OMI_FIELD_NAS_BLOCKSIZE,    (uint32)info->blocksize},
        {CM_OMI_FIELD_NAS_WRITE_POLICY, (uint32)info->write_policy},
        {CM_OMI_FIELD_NAS_ACCESS,       (uint32)info->access},
        {CM_OMI_FIELD_NAS_IS_COMP,      (uint32)info->is_compress},
        {CM_OMI_FIELD_NAS_DEDUP,      (uint32)info->dedup},
        {CM_OMI_FIELD_NAS_SMB,      (uint32)info->smb},
        {CM_OMI_FIELD_NAS_ABE,      (uint32)info->abe},
        {CM_OMI_FIELD_NAS_ACLINHERIT,      (uint32)info->aclinherit},
    };
    cm_cnm_encode_str(item,field,cols_str,sizeof(cols_str)/sizeof(cm_cnm_map_value_str_t));
    cm_cnm_encode_num(item,field,cols_num,sizeof(cols_num)/sizeof(cm_cnm_map_value_num_t)); 
    return;
}

cm_omi_obj_t cm_cnm_nas_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen)
{
    return cm_cnm_encode_comm_ext(pDecodeParam,pAckData,AckLen,
        sizeof(cm_cnm_nas_info_t),cm_cnm_nas_encode_each);
}

#define cm_cnm_nas_req(cmd,param,ppAck,plen) \
    cm_cnm_request_comm(CM_OMI_OBJECT_NAS,cmd,sizeof(cm_cnm_nas_info_t),param,ppAck,plen)

sint32 cm_cnm_nas_getbatch(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_cnm_nas_req(CM_OMI_CMD_GET_BATCH,pDecodeParam,ppAckData,pAckLen);
}    

sint32 cm_cnm_nas_get(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_cnm_nas_req(CM_OMI_CMD_GET,pDecodeParam,ppAckData,pAckLen);
}    

static sint32 cm_cnm_nas_requst(uint32 cmd,const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    const cm_cnm_decode_info_t *decode = pDecodeParam;
    if(NULL  == decode)
    {
        return CM_PARAM_ERR;
    }
    if(CM_NODE_ID_NONE == decode->nid)
    {
        CM_LOG_ERR(CM_MOD_CNM,"nid null");
        return CM_PARAM_ERR;
    }
    if(!CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_NAS_POOL))
    {
        CM_LOG_ERR(CM_MOD_CNM,"pool null");
        return CM_PARAM_ERR;
    }
    if(!CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_NAS_NAME))
    {
        CM_LOG_ERR(CM_MOD_CNM,"name null");
        return CM_PARAM_ERR;
    }
    return cm_cnm_nas_req(cmd,pDecodeParam,ppAckData,pAckLen);
}
sint32 cm_cnm_nas_create(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_cnm_nas_requst(CM_OMI_CMD_ADD,pDecodeParam,ppAckData,pAckLen);
}    
    
sint32 cm_cnm_nas_update(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_cnm_nas_requst(CM_OMI_CMD_MODIFY,pDecodeParam,ppAckData,pAckLen);
}    
    
sint32 cm_cnm_nas_count(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_cnm_nas_req(CM_OMI_CMD_COUNT,pDecodeParam,ppAckData,pAckLen);
}    
    
sint32 cm_cnm_nas_delete(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_cnm_nas_requst(CM_OMI_CMD_DELETE,pDecodeParam,ppAckData,pAckLen);
}    

#if 0
static sint32 cm_cnm_nas_local_get_each(void *arg, sint8 **cols, uint32 col_num)
{
    cm_cnm_nas_info_t *info = arg;
    const uint32 def_num = (g_cm_sys_ver == CM_SYS_VER_SOLARIS_V7R16)? 10:13;    
    
    if(def_num != col_num)
    {
        CM_LOG_WARNING(CM_MOD_CNM,"col_num[%u] def_num[%u]",col_num,def_num);
        return CM_FAIL;
    }
    CM_MEM_ZERO(info,sizeof(cm_cnm_nas_info_t));

    col_num = sscanf(cols[0],"%[a-zA-Z0-9_.-]/%[a-zA-Z0-9_.-]",info->pool,info->name);
    if(col_num < 2)
    {
        CM_LOG_WARNING(CM_MOD_CNM,"name[%s]",cols[0]); 
        return CM_FAIL;
    }
    info->nid = cm_node_get_id();
    CM_VSPRINTF(info->space_used,sizeof(info->space_used),"%s",cols[1]);
    CM_VSPRINTF(info->space_avail,sizeof(info->space_avail),"%s",cols[2]);
    CM_VSPRINTF(info->quota,sizeof(info->quota),"%s",cols[3]);
    info->blocksize = (uint16)atoi(cols[4]);

    info->is_compress = (0 == strcmp(cols[5],"on")) ? CM_TRUE : CM_FALSE;    
    info->write_policy = (uint8)cm_cnm_get_enum(&CmOmiMapLunSyncTypeCfg,cols[6],0);
    info->dedup = (0 == strcmp(cols[7],"on")) ? CM_TRUE : CM_FALSE;    
    
    if(0 == strcmp(cols[8],"off"))
    {
        info->smb = CM_FALSE;
        info->abe = CM_FALSE;
    }
    else
    {
        info->smb = CM_TRUE;
        if(0 == strcmp(cols[8],"abe=true"))
        {
            info->abe = CM_TRUE;
        }
    }
    
    if(g_cm_sys_ver != CM_SYS_VER_SOLARIS_V7R16)
    {
        CM_VSPRINTF(info->qos_avs,sizeof(info->qos_avs),"%s",cols[10]);
        CM_VSPRINTF(info->qos_val,sizeof(info->qos_val),"%s",cols[11]);
        info->aclinherit = (uint8)cm_cnm_get_enum(&CmOmiMapAclInheritType,cols[12],0);
    }
    else
    {
        info->aclinherit = (uint8)cm_cnm_get_enum(&CmOmiMapAclInheritType,cols[9],0);
    }
    
    info->access = (uint16)cm_exec_int("stat `zfs list -o mountpoint -H %s` 2>/dev/null"
        "|grep Uid |cut -b 11-13",cols[0]);
    return CM_OK;
}

sint32 cm_cnm_nas_local_getbatch(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    sint32 iRet = CM_OK;
    sint8 cmd[CM_STRING_512] = {0};
    CM_LOG_INFO(CM_MOD_CNM,"offset[%llu] total[%u]",offset,total);

    CM_VSPRINTF(cmd,sizeof(cmd),"%s |grep '/'",g_cm_cnm_nas_get_cmd);
    iRet = cm_cnm_exec_get_list(cmd,cm_cnm_nas_local_get_each,
        (uint32)offset,sizeof(cm_cnm_nas_info_t),ppAck,&total);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]",iRet);
        return iRet;
    }
    *pAckLen = total * sizeof(cm_cnm_nas_info_t);
    return CM_OK;
}

sint32 cm_cnm_nas_local_get(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    sint32 iRet = CM_OK;
    const cm_cnm_decode_info_t *req = param;
    const cm_cnm_nas_info_t *info = NULL;
    cm_cnm_nas_info_t *data = NULL;

    if(NULL == req)
    {
        return CM_PARAM_ERR;
    }
    info = (const cm_cnm_nas_info_t *)req->data;

    data = CM_MALLOC(sizeof(cm_cnm_nas_info_t));
    if(NULL == data)
    {
        CM_LOG_ERR(CM_MOD_CNM,"malloc fail");
        return CM_FAIL;
    }
    
    CM_LOG_INFO(CM_MOD_CNM,"%s/%s",info->pool,info->name);
    
    iRet = cm_cnm_exec_get_col(cm_cnm_nas_local_get_each,data,
        "%s %s/%s",g_cm_cnm_nas_get_cmd,info->pool,info->name);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]",iRet);
        CM_FREE(data);
        return CM_OK;
    }
    *ppAck = data;
    *pAckLen = sizeof(cm_cnm_nas_info_t);
    return CM_OK;
}    
#else
static sint32 cm_cnm_nas_local_get_each(void *arg, sint8 **cols, uint32 col_num)
{
    cm_cnm_nas_info_t *info = arg;
    const uint32 def_num=11;
    
    if(def_num != col_num)
    {
        CM_LOG_WARNING(CM_MOD_CNM,"col_num[%u] def_num[%u]",col_num,def_num);
        return CM_FAIL;
    }
    /* name,used,avail,quota,recordsize,compression,sync,dedup,sharesmb,aclinherit,
bandwidth,nasavsbw */
    CM_MEM_ZERO(info,sizeof(cm_cnm_nas_info_t));
    sscanf(cols[0],"%[a-zA-Z0-9_.-]/%[a-zA-Z0-9_.-]",info->pool,info->name);
    info->nid = cm_node_get_id();
    CM_VSPRINTF(info->space_used,sizeof(info->space_used),"%s",cols[1]);
    CM_VSPRINTF(info->space_avail,sizeof(info->space_avail),"%s",cols[2]);
    CM_VSPRINTF(info->quota,sizeof(info->quota),"%s",cols[3]);
    info->blocksize = (uint16)atoi(cols[4]);

    info->is_compress = (0 == strcmp(cols[5],"on")) ? CM_TRUE : CM_FALSE;    
    info->write_policy = (uint8)cm_cnm_get_enum(&CmOmiMapLunSyncTypeCfg,cols[6],0);
    info->dedup = (0 == strcmp(cols[7],"on")) ? CM_TRUE : CM_FALSE;   

    if(0 == strcmp(cols[8],"off"))
    {
        info->smb = CM_FALSE;
        info->abe = CM_FALSE;
    }
    else
    {
        info->smb = CM_TRUE;
        if(0 == strcmp(cols[8],"abe=true"))
        {
            info->abe = CM_TRUE;
        }
    }

    info->aclinherit = (uint8)cm_cnm_get_enum(&CmOmiMapAclInheritType,cols[9],0);

    CM_VSPRINTF(info->qos_avs,sizeof(info->qos_avs),"%s",cols[10]);
    //CM_VSPRINTF(info->qos_val,sizeof(info->qos_val),"%s",cols[11]);
    //info->access = (uint16)atoi(cols[12]);
    return CM_OK;
}

sint32 cm_cnm_nas_local_getbatch(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    sint32 iRet = CM_OK;
    
    CM_LOG_INFO(CM_MOD_CNM,"offset[%llu] total[%u]",offset,total);
    iRet = cm_exec_get_list(cm_cnm_nas_local_get_each,
        (uint32)offset,sizeof(cm_cnm_nas_info_t),ppAck,&total,
        CM_SCRIPT_DIR"cm_cnm.sh nas_getbatch");
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]",iRet);
        return iRet;
    }
    *pAckLen = total * sizeof(cm_cnm_nas_info_t);
    return CM_OK;
}

sint32 cm_cnm_nas_local_get(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    sint32 iRet = CM_OK;
    const cm_cnm_decode_info_t *req = param;
    const cm_cnm_nas_info_t *info = (const cm_cnm_nas_info_t *)req->data;
    
    total = 1;
    CM_LOG_INFO(CM_MOD_CNM,"offset[%llu] total[%u]",offset,total);
    iRet = cm_exec_get_list(cm_cnm_nas_local_get_each,
        (uint32)0,sizeof(cm_cnm_nas_info_t),ppAck,&total,
        CM_SCRIPT_DIR"cm_cnm.sh nas_getbatch %s/%s",info->pool,info->name);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]",iRet);
        return iRet;
    }
    *pAckLen = total * sizeof(cm_cnm_nas_info_t);
    return CM_OK;
}

#endif
sint32 cm_cnm_nas_local_count(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    uint64 cnt = 0;
    const sint8 *cmd = "zfs list -t filesystem -H -o name |grep '/'";
    
    cnt = (uint64)cm_exec_double("%s 2>/dev/null |wc -l",cmd);
    
    return cm_cnm_ack_uint64(cnt,ppAck,pAckLen);
}    

#if 0
static sint32 cm_cnm_nas_local_set(const cm_omi_field_flag_t *set,
    const cm_cnm_nas_info_t *info)
{
    uint32 failnum = 0;
    sint32 iRet = CM_OK;
    sint8 buff[CM_STRING_1K] = {0};
    const sint8 *str = NULL;

    if(0 == cm_exec_int("zfs list -H -o name %s/%s 2>/dev/null |wc -l",info->pool,info->name))
    {
        CM_LOG_ERR(CM_MOD_CNM," %s/%s not exists",info->pool,info->name);
        return CM_ERR_NOT_EXISTS;
    }
    
    if(CM_OMI_FIELDS_FLAG_ISSET(set,CM_OMI_FIELD_NAS_BLOCKSIZE))
    {
        iRet = cm_exec_tmout(buff,sizeof(buff),CM_CMT_REQ_TMOUT_NEVER,
            "zfs set recordsize=%uK %s/%s 2>&1",info->blocksize,info->pool,info->name);
        if(CM_OK != iRet)
        {
            CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]\n%s",iRet,buff);
            failnum++;
        }
    }

    if(CM_OMI_FIELDS_FLAG_ISSET(set,CM_OMI_FIELD_NAS_ACCESS))
    {
        iRet = cm_exec_tmout(buff,sizeof(buff),CM_CMT_REQ_TMOUT_NEVER,
            "chmod %u `zfs list -H -o mountpoint %s/%s 2>/dev/null` 2>/dev/null",
            info->access,info->pool,info->name);
        if(CM_OK != iRet)
        {
            failnum++;
        }
    }

    if(CM_OMI_FIELDS_FLAG_ISSET(set,CM_OMI_FIELD_NAS_WRITE_POLICY))
    {
        str = cm_cnm_get_enum_str(&CmOmiMapLunSyncTypeCfg,info->write_policy);
        if(NULL == str)
        {
            CM_LOG_ERR(CM_MOD_CNM,"policy[%u]",info->write_policy);
            return CM_PARAM_ERR;
        }
        iRet = cm_exec_tmout(buff,sizeof(buff),CM_CMT_REQ_TMOUT_NEVER,
            "zfs set origin:sync=%s %s/%s 2>&1",
            str,info->pool,info->name);
        if(CM_OK != iRet)
        {
            CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]\n%s",iRet,buff);
            failnum++;
        }
    }

    if(CM_OMI_FIELDS_FLAG_ISSET(set,CM_OMI_FIELD_NAS_IS_COMP))
    {
        str = (CM_TRUE == info->is_compress)? "on" : "off";
        iRet = cm_exec_tmout(buff,sizeof(buff),CM_CMT_REQ_TMOUT_NEVER,
            "zfs set compression=%s %s/%s 2>&1",
            str,info->pool,info->name);
        if(CM_OK != iRet)
        {
            CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]\n%s",iRet,buff);
            failnum++;
        }
    }

    if(CM_OMI_FIELDS_FLAG_ISSET(set,CM_OMI_FIELD_NAS_QOS)
        && (g_cm_sys_ver != CM_SYS_VER_SOLARIS_V7R16))
    {
        iRet = cm_exec_tmout(buff,sizeof(buff),CM_CMT_REQ_TMOUT_NEVER,
            "zfs set bandwidth=%s %s/%s 2>&1",
            info->qos_val,info->pool,info->name);
        if(CM_OK != iRet)
        {
            CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]\n%s",iRet,buff);
            failnum++;
        }
    }

    if(CM_OMI_FIELDS_FLAG_ISSET(set,CM_OMI_FIELD_NAS_QUOTA))
    {
        iRet = cm_exec_tmout(buff,sizeof(buff),CM_CMT_REQ_TMOUT_NEVER,
            "zfs set quota=%s %s/%s 2>&1",
            info->quota,info->pool,info->name);
        if(CM_OK != iRet)
        {
            CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]\n%s",iRet,buff);
            failnum++;
        }
    }
    
    if(CM_OMI_FIELDS_FLAG_ISSET(set,CM_OMI_FIELD_NAS_DEDUP))
    {
        str = (CM_TRUE == info->dedup)? "on" : "off";
        iRet = cm_exec_tmout(buff,sizeof(buff),CM_CMT_REQ_TMOUT_NEVER,
            "zfs set dedup=%s %s/%s 2>&1",
            str,info->pool,info->name);
        if(CM_OK != iRet)
        {
            CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]\n%s",iRet,buff);
            failnum++;
        }
    }

    if(CM_OMI_FIELDS_FLAG_ISSET(set,CM_OMI_FIELD_NAS_SMB))
    {
        uint8 abe = CM_FALSE;
        if(!CM_OMI_FIELDS_FLAG_ISSET(set,CM_OMI_FIELD_NAS_ABE))
        {
            str = (CM_TRUE == info->smb)? "on" : "off";
        }
        else
        {
            if((CM_TRUE == info->smb) && (CM_TRUE == info->abe))
            {
                str="abe=true";
                abe=CM_TRUE;
            }
            else if((CM_TRUE == info->smb) && (CM_FALSE == info->abe))
            {
                str="on";
            }
            else
            {
                str="off";
            }
        }
        iRet = cm_exec_tmout(buff,sizeof(buff),CM_CMT_REQ_TMOUT_NEVER,
            CM_SHELL_EXEC" cm_zfs_set %s/%s sharesmb %s 2>&1",
            info->pool,info->name, str);
        if(CM_OK != iRet)
        {
            CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]\n%s",iRet,buff);
            failnum++;
        }
        else
        {
            (void)cm_system(CM_SHELL_EXEC" cm_set_abe %s/%s %u",
                info->pool,info->name,abe);
        }        
    }
    
    if(CM_OMI_FIELDS_FLAG_ISSET(set,CM_OMI_FIELD_NAS_QOS_AVS)
        && (g_cm_sys_ver != CM_SYS_VER_SOLARIS_V7R16))
    {
        iRet = cm_exec_tmout(buff,sizeof(buff),CM_CMT_REQ_TMOUT_NEVER,
            "zfs set nasavsbw=%s %s/%s 2>&1",
            info->qos_avs,info->pool,info->name);
        if(CM_OK != iRet)
        {
            CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]\n%s",iRet,buff);
            failnum++;
        }
    }

    if(CM_OMI_FIELDS_FLAG_ISSET(set,CM_OMI_FIELD_NAS_ACLINHERIT))
    {
        str = cm_cnm_get_enum_str(&CmOmiMapAclInheritType,info->aclinherit);
        iRet = cm_exec_tmout(buff,sizeof(buff),CM_CMT_REQ_TMOUT_NEVER,
            "zfs set aclinherit=%s %s/%s 2>&1",
            str,info->pool,info->name);
        if(CM_OK != iRet)
        {
            CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]\n%s",iRet,buff);
            failnum++;
        }
    }
    
    return (0 == failnum)? CM_OK: CM_FAIL;
}


sint32  cm_cnm_nas_local_create(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    const cm_cnm_decode_info_t *req = param;
    const cm_cnm_nas_info_t *info = NULL;
    sint32 iRet = CM_OK;
    sint8 buff[CM_STRING_1K] = {0};
    if(NULL == req)
    {
        return CM_PARAM_ERR;
    }
    info = (const cm_cnm_nas_info_t *)req->data;
    iRet = cm_exec_tmout(buff,sizeof(buff),CM_CMT_REQ_TMOUT_NEVER,
        "zfs create %s/%s 2>&1",info->pool,info->name);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]\n%s",iRet,buff);
        return cm_cnm_get_errcode(&CmCnmMapCommErrCfg,buff,iRet);
    }

    /* 设置可选属性 */
    (void)cm_cnm_nas_local_set(&req->set,info);
    return CM_OK;
}    

sint32 cm_cnm_nas_local_update(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    const cm_cnm_decode_info_t *req = param;
    const cm_cnm_nas_info_t *info = NULL;

    if(NULL == req)
    {
        return CM_PARAM_ERR;
    }
    info = (const cm_cnm_nas_info_t *)req->data;
    return cm_cnm_nas_local_set(&req->set,info);
}    

sint32 cm_cnm_nas_local_delete(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    const cm_cnm_decode_info_t *req = param;
    const cm_cnm_nas_info_t *info = NULL;
    sint32 iRet = CM_OK;
    sint8 buff[CM_STRING_1K] = {0};

    if(NULL == req)
    {
        return CM_PARAM_ERR;
    }
    info = (const cm_cnm_nas_info_t *)req->data;
    
    /* zfs destroy -rRf */
    iRet = cm_exec_tmout(buff,sizeof(buff),CM_CMT_REQ_TMOUT_NEVER,
        "zfs destroy -rRf %s/%s 2>&1",info->pool,info->name);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]\n%s",iRet,buff);
        return cm_cnm_get_errcode(&CmCnmMapCommErrCfg,buff,iRet);
    }
    return CM_OK;
}    
#else
static sint32 cm_cnm_nas_local_exec(const sint8* act,
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    uint32 taskid=0;
    sint8 buf[CM_STRING_1K] = {0};
    sint32 buflen=sizeof(buf);
    sint32 iRet = CM_OK;
    const cm_cnm_decode_info_t *req = (const cm_cnm_decode_info_t *)param;
    const cm_cnm_nas_info_t *info = (const cm_cnm_nas_info_t *)req->data;
    cm_cnm_map_value_num_t optnum[] = 
        {
            {CM_OMI_FIELD_NAS_BLOCKSIZE, info->blocksize},
            {CM_OMI_FIELD_NAS_ACCESS, info->access},
            {CM_OMI_FIELD_NAS_WRITE_POLICY, info->write_policy},
            {CM_OMI_FIELD_NAS_IS_COMP, info->is_compress},
            {CM_OMI_FIELD_NAS_DEDUP, info->dedup},
            {CM_OMI_FIELD_NAS_SMB, info->smb},
            {CM_OMI_FIELD_NAS_ABE, info->abe},
            {CM_OMI_FIELD_NAS_ACLINHERIT, info->aclinherit},
        };
    cm_cnm_map_value_str_t optstr[] = 
        {
            {CM_OMI_FIELD_NAS_QUOTA,info->quota},
            {CM_OMI_FIELD_NAS_QOS,info->qos_val},
            {CM_OMI_FIELD_NAS_QOS_AVS,info->qos_avs},
        };
    
    CM_SNPRINTF_ADD(buf,buflen,"%s/%s",info->pool,info->name);
    cm_cnm_mkparam_num(buf,buflen,&req->set,optnum,sizeof(optnum)/sizeof(cm_cnm_map_value_num_t));
    cm_cnm_mkparam_str(buf,buflen,&req->set,optstr,sizeof(optstr)/sizeof(cm_cnm_map_value_str_t));
    
    iRet = cm_exec_out(ppAck,pAckLen,CM_CMT_REQ_TMOUT_NEVER,
        CM_SCRIPT_DIR"cm_cnm.sh nas_%s '%s' 2>&1",act,buf);
    if((CM_OK != iRet) || (*ppAck == NULL) || (*pAckLen == 0))
    {
        return iRet;
    }
    taskid = atoi((sint8*)*ppAck);
    CM_FREE(*ppAck);
    *ppAck = NULL;
    *pAckLen = 0;
    if(taskid > 0)
    {
        return cm_cnm_ack_uint64(taskid,ppAck,pAckLen);
    }
    return CM_OK;
}

sint32 cm_cnm_nas_local_create(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    return cm_cnm_nas_local_exec("create",param,len,offset,total,ppAck,pAckLen);
}

sint32 cm_cnm_nas_local_update(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    return cm_cnm_nas_local_exec("update",param,len,offset,total,ppAck,pAckLen);
}

sint32 cm_cnm_nas_local_delete(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    return cm_cnm_nas_local_exec("delete",param,len,offset,total,ppAck,pAckLen);
}

#endif
static void cm_cnm_nas_oplog_report(
    const sint8* sessionid, const void *pDecodeParam, uint32 alarmid)
{
    const cm_cnm_decode_info_t *req = pDecodeParam;
    const uint32 cnt = 14;

    /*node,pool,nas,blocksize,access,w_policy,is_comp,qos,quota,dedup,avsbw,smb*/
    if(NULL == pDecodeParam)
    {   
        cm_cnm_oplog_report(sessionid,alarmid,NULL,cnt,NULL);
    }    
    else    
    {       
        const cm_cnm_nas_info_t *info = (const cm_cnm_nas_info_t*)req->data;
        const sint8* nodename = cm_node_get_name(info->nid);
        cm_cnm_oplog_param_t params[14] = {
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_NAS_NID,strlen(nodename),nodename},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_NAS_POOL,strlen(info->pool),info->pool},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_NAS_NAME,strlen(info->name),info->name},
            {CM_OMI_DATA_INT,CM_OMI_FIELD_NAS_BLOCKSIZE,sizeof(info->blocksize),&info->blocksize},
            {CM_OMI_DATA_INT,CM_OMI_FIELD_NAS_ACCESS,sizeof(info->access),&info->access},
            
            {CM_OMI_DATA_INT,CM_OMI_FIELD_NAS_WRITE_POLICY,sizeof(info->write_policy),&info->write_policy},
            {CM_OMI_DATA_INT,CM_OMI_FIELD_NAS_IS_COMP,sizeof(info->is_compress),&info->is_compress},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_NAS_QOS,strlen(info->qos_val),info->qos_val},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_NAS_QUOTA,strlen(info->quota),info->quota},
            {CM_OMI_DATA_INT,CM_OMI_FIELD_NAS_DEDUP,sizeof(info->dedup),&info->dedup},
            
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_NAS_QOS_AVS,strlen(info->qos_avs),info->qos_avs},
            {CM_OMI_DATA_INT,CM_OMI_FIELD_NAS_SMB,sizeof(info->smb),&info->smb},
            {CM_OMI_DATA_INT,CM_OMI_FIELD_NAS_ABE,sizeof(info->abe),&info->abe},
            {CM_OMI_DATA_INT,CM_OMI_FIELD_NAS_ACLINHERIT,sizeof(info->aclinherit),&info->aclinherit},
        };
        
        cm_cnm_oplog_report(sessionid,alarmid,params,cnt,&req->set);
    }    
    
    return;
}    

void cm_cnm_nas_oplog_update(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK)? CM_ALARM_LOG_NAS_UPDATE_OK : CM_ALARM_LOG_NAS_UPDATE_FAIL;
    
    cm_cnm_nas_oplog_report(sessionid,pDecodeParam,alarmid);
    return;
}
void cm_cnm_nas_oplog_create(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK)? CM_ALARM_LOG_NAS_CREATR_OK : CM_ALARM_LOG_NAS_CREATR_FAIL;
    
    cm_cnm_nas_oplog_report(sessionid,pDecodeParam,alarmid);
    return;
}

void cm_cnm_nas_oplog_delete(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK)? CM_ALARM_LOG_NAS_DELETE_OK : CM_ALARM_LOG_NAS_DELETE_FAIL;
    const cm_cnm_decode_info_t *req = pDecodeParam;
    const uint32 cnt = 3;

    /*node,pool,nas*/
    if(NULL == pDecodeParam)
    {   
        cm_cnm_oplog_report(sessionid,alarmid,NULL,cnt,NULL);
    }    
    else    
    {       
        const cm_cnm_nas_info_t *info = (const cm_cnm_nas_info_t*)req->data;
        const sint8* nodename = cm_node_get_name(info->nid);
        cm_cnm_oplog_param_t params[3] = {
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_NAS_NID,strlen(nodename),nodename},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_NAS_POOL,strlen(info->pool),info->pool},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_NAS_NAME,strlen(info->name),info->name},
        };
        
        cm_cnm_oplog_report(sessionid,alarmid,params,cnt,&req->set);
    }   
    return; 
}    

/*================================= CIFS =====================================*/
/*参考 ZFS文件系统-使用ACL和属性保护Oracle Solaris ZFS文件*/
sint32 cm_cnm_cifs_init(void)
{
    return CM_OK;
}
static sint32 cm_cnm_cifs_decode_check_domain(void *val)
{
    uint8 value = *((uint8*)val);

    if(value >= CM_DOMAIN_BUTT)
    {
        CM_LOG_ERR(CM_MOD_CNM,"domain[%u]",value);
        return CM_PARAM_ERR;
    }
    return CM_OK;
}

static sint32 cm_cnm_cifs_decode_check_permission(void *val)
{
    uint8 value = *((uint8*)val);
    if(value >= CM_NAS_PERISSION_BUTT)
    {
        CM_LOG_ERR(CM_MOD_CNM,"permission[%u]",value);
        return CM_PARAM_ERR;
    }
    return CM_OK;
}

static sint32 cm_cnm_cifs_decode_check_nametype(void *val)
{
    uint8 value = *((uint8*)val);
    if(value >= CM_NAME_BUTT)
    {
        CM_LOG_ERR(CM_MOD_CNM,"nametype[%u]",value);
        return CM_PARAM_ERR;
    }
    return CM_OK;
}


static sint32 cm_cnm_cifs_decode_ext(
    const cm_omi_obj_t ObjParam,void* data,cm_omi_field_flag_t *set)
{
    cm_cnm_cifs_param_t *param = data;
    cm_cnm_cifs_info_t *info = &param->info;
    sint32 iRet = CM_OK;
    uint32 nid = 0;
    cm_cnm_decode_param_t param_str[] = 
    {
        {CM_OMI_FIELD_CIFS_DIR,sizeof(param->dir),param->dir,NULL},
        {CM_OMI_FIELD_CIFS_NAME,sizeof(info->name),info->name,NULL},
    };

    cm_cnm_decode_param_t param_num[] = 
    {
        {CM_OMI_FIELD_CIFS_NID,sizeof(uint32),&nid,NULL},
        {CM_OMI_FIELD_CIFS_NAME_TYPE,sizeof(info->name_type),&info->name_type,cm_cnm_cifs_decode_check_nametype},
        {CM_OMI_FIELD_CIFS_DOMAIN,sizeof(info->domain_type),&info->domain_type,cm_cnm_cifs_decode_check_domain},
        {CM_OMI_FIELD_CIFS_PERMISSION,sizeof(info->permission),&info->permission,cm_cnm_cifs_decode_check_permission},
    };
    info->domain_type = CM_DOMAIN_LOCAL; /*default*/
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

sint32 cm_cnm_cifs_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam)
{
    return cm_cnm_decode_comm(ObjParam,sizeof(cm_cnm_cifs_param_t),
        cm_cnm_cifs_decode_ext,ppDecodeParam);
}

static void cm_cnm_cifs_encode_each(cm_omi_obj_t item,void *eachdata,void *arg)
{
    cm_omi_field_flag_t *field = arg;
    cm_cnm_cifs_info_t *info = eachdata;
    cm_cnm_map_value_str_t cols_str[] = 
    {
        {CM_OMI_FIELD_CIFS_NAME,     info->name},
    };
    cm_cnm_map_value_num_t cols_num[] = 
    {
        {CM_OMI_FIELD_CIFS_NAME_TYPE,     (uint32)info->name_type},
        {CM_OMI_FIELD_CIFS_DOMAIN,        (uint32)info->domain_type},
        {CM_OMI_FIELD_CIFS_PERMISSION,    (uint32)info->permission},
    };
    
    cm_cnm_encode_str(item,field,cols_str,sizeof(cols_str)/sizeof(cm_cnm_map_value_str_t));
    cm_cnm_encode_num(item,field,cols_num,sizeof(cols_num)/sizeof(cm_cnm_map_value_num_t)); 
    return;
}

cm_omi_obj_t cm_cnm_cifs_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen)
{
    return cm_cnm_encode_comm_ext(pDecodeParam,pAckData,AckLen,
        sizeof(cm_cnm_cifs_info_t),cm_cnm_cifs_encode_each);
}    

static sint32 cm_cnm_cifs_requst(uint32 cmd,const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    const cm_cnm_decode_info_t *decode = pDecodeParam;
    
    if(NULL  == decode)
    {
        return CM_PARAM_ERR;
    }
    if(CM_NODE_ID_NONE == decode->nid)
    {
        CM_LOG_ERR(CM_MOD_CNM,"nid null");
        return CM_PARAM_ERR;
    }
    if(!CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_CIFS_DIR))
    {
        CM_LOG_ERR(CM_MOD_CNM,"dir null");
        return CM_PARAM_ERR;
    }
    switch(cmd)
    {
        case CM_OMI_CMD_ADD:
        case CM_OMI_CMD_MODIFY:
        {
            if(!CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_CIFS_PERMISSION))
            {
                CM_LOG_ERR(CM_MOD_CNM,"permission null");
                return CM_PARAM_ERR;
            }
        }    
        case CM_OMI_CMD_DELETE:
        {
            if(!CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_CIFS_NAME_TYPE))
            {
                CM_LOG_ERR(CM_MOD_CNM,"nametype null");
                return CM_PARAM_ERR;
            }
            if(!CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_CIFS_NAME))
            {
                CM_LOG_ERR(CM_MOD_CNM,"user null");
                return CM_PARAM_ERR;
            }
            break;
        }            
        default:
            break;
    }
    return cm_cnm_request_comm(CM_OMI_OBJECT_CIFS,cmd,sizeof(cm_cnm_cifs_param_t),pDecodeParam,ppAckData,pAckLen);
}

sint32 cm_cnm_cifs_getbatch(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_cnm_cifs_requst(CM_OMI_CMD_GET_BATCH,pDecodeParam,ppAckData,pAckLen);
}     

sint32 cm_cnm_cifs_get(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_cnm_cifs_requst(CM_OMI_CMD_GET,pDecodeParam,ppAckData,pAckLen);
}
   
sint32 cm_cnm_cifs_create(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_cnm_cifs_requst(CM_OMI_CMD_ADD,pDecodeParam,ppAckData,pAckLen);
}    
    
sint32 cm_cnm_cifs_update(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_cnm_cifs_requst(CM_OMI_CMD_MODIFY,pDecodeParam,ppAckData,pAckLen);
}    
    
sint32 cm_cnm_cifs_count(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_cnm_cifs_requst(CM_OMI_CMD_COUNT,pDecodeParam,ppAckData,pAckLen);
}    
    
sint32 cm_cnm_cifs_delete(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_cnm_cifs_requst(CM_OMI_CMD_DELETE,pDecodeParam,ppAckData,pAckLen);
}    

const sint8* g_cm_cnm_cifs_script=CM_SCRIPT_DIR"cm_cnm_cifs.sh";
/*
# 批量查询  : cm_cnm_cifs.sh getbatch <dir> <offset> <total>
# 记录数查询: cm_cnm_cifs.sh count    <dir>
# 添加记录  : cm_cnm_cifs.sh add      <dir> <domain> <nametype> <name> <permission>
# 修改记录  : cm_cnm_cifs.sh update   <dir> <domain> <nametype> <name> <permission>
# 删除记录  : cm_cnm_cifs.sh delete   <dir> <domain> <nametype> <name>
# 精确查询  : cm_cnm_cifs.sh get      <dir> <domain> <nametype> <name>

*/
static sint32 cm_cnm_cifs_local_get_each(void *arg, sint8 **cols, uint32 col_num)
{
    cm_cnm_cifs_info_t *info = arg;
    const uint32 def_num = 5;  
    
    /* index domain nametype name permission */
    if(def_num > col_num)
    {
        CM_LOG_WARNING(CM_MOD_CNM,"col_num[%u] def_num[%u]",col_num,def_num);
        return CM_FAIL;
    }
    info->domain_type = (uint8)atoi(cols[2]);
    info->name_type = (uint8)atoi(cols[3]);    
    info->permission = (uint8)atoi(cols[1]);
    CM_VSPRINTF(info->name,sizeof(info->name),"%s",cols[4]);
    for(cols+=def_num,col_num-=def_num;col_num>0;col_num--,cols++)
    {
        CM_SNPRINTF_ADD(info->name,sizeof(info->name)," %s",*cols);
    }
    return CM_OK;
}

sint32 cm_cnm_cifs_local_getbatch(
    void *param, uint32 len,
    uint64 offset, uint32 total,  
    void **ppAck, uint32 *pAckLen)
{
    const cm_cnm_decode_info_t *decode = param;
    const cm_cnm_cifs_param_t *req = (const cm_cnm_cifs_param_t*)decode->data;
    sint32 iRet = CM_OK;
    sint8 cmd[CM_STRING_512] = {0};
    CM_LOG_INFO(CM_MOD_CNM,"offset[%llu] total[%u]",offset,total);

    CM_VSPRINTF(cmd,sizeof(cmd),"%s getbatch '%s' %llu %u",
        g_cm_cnm_cifs_script,req->dir,offset,total);
    iRet = cm_cnm_exec_get_list(cmd,cm_cnm_cifs_local_get_each,
        (uint32)0,sizeof(cm_cnm_cifs_info_t),ppAck,&total);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]",iRet);
        return iRet;
    }
    *pAckLen = total * sizeof(cm_cnm_cifs_info_t);
    return CM_OK;
}    

sint32 cm_cnm_cifs_local_get(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    const cm_cnm_decode_info_t *decode = param;
    const cm_cnm_cifs_param_t *req = (const cm_cnm_cifs_param_t*)decode->data;
    const cm_cnm_cifs_info_t *info = &req->info;
    cm_cnm_cifs_info_t *data=CM_MALLOC(sizeof(cm_cnm_cifs_info_t));
    sint32 iRet = CM_OK;
    
    CM_LOG_INFO(CM_MOD_CNM,"'%s' %u %u '%s'",req->dir,
        info->domain_type,info->name_type,info->name);
    if(NULL == data)
    {
        CM_LOG_ERR(CM_MOD_CNM,"malloc fail");
        return CM_FAIL;
    }

    iRet = cm_cnm_exec_get_col(cm_cnm_cifs_local_get_each,data,
        "%s get '%s' %u %u '%s'",
        g_cm_cnm_cifs_script, req->dir,info->domain_type,
        info->name_type,info->name);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]",iRet);
        CM_FREE(data);
        return CM_OK;
    }
    *ppAck = data;
    *pAckLen = sizeof(cm_cnm_cifs_info_t);
    return CM_OK;
}    

sint32 cm_cnm_cifs_local_count(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    uint64 cnt = 0;
    const cm_cnm_decode_info_t *decode = param;
    const cm_cnm_cifs_param_t *req = (const cm_cnm_cifs_param_t*)decode->data;
    
    cnt = (uint64)cm_exec_double("%s count '%s'",g_cm_cnm_cifs_script,req->dir);
    
    return cm_cnm_ack_uint64(cnt,ppAck,pAckLen);
}    

sint32 cm_cnm_cifs_local_create(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    const cm_cnm_decode_info_t *decode = param;
    const cm_cnm_cifs_param_t *req = (const cm_cnm_cifs_param_t*)decode->data;
    const cm_cnm_cifs_info_t *info = &req->info;

    CM_LOG_WARNING(CM_MOD_CNM,"'%s' %u %u '%s' %u",req->dir,
        info->domain_type,info->name_type,info->name,info->permission);
    
    return cm_system("%s add '%s' %u %u '%s' %u",
        g_cm_cnm_cifs_script, req->dir,info->domain_type,
        info->name_type,info->name,info->permission);
}    

sint32 cm_cnm_cifs_local_update(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    const cm_cnm_decode_info_t *decode = param;
    const cm_cnm_cifs_param_t *req = (const cm_cnm_cifs_param_t*)decode->data;
    const cm_cnm_cifs_info_t *info = &req->info;

    CM_LOG_WARNING(CM_MOD_CNM,"'%s' %u %u '%s' %u",req->dir,
        info->domain_type,info->name_type,info->name,info->permission);
    
    return cm_system("%s update '%s' %u %u '%s' %u",
        g_cm_cnm_cifs_script, req->dir,info->domain_type,
        info->name_type,info->name,info->permission);
}    

sint32  cm_cnm_cifs_local_delete(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    const cm_cnm_decode_info_t *decode = param;
    const cm_cnm_cifs_param_t *req = (const cm_cnm_cifs_param_t*)decode->data;
    const cm_cnm_cifs_info_t *info = &req->info;

    CM_LOG_WARNING(CM_MOD_CNM,"'%s' %u %u '%s'",req->dir,
        info->domain_type,info->name_type,info->name);
    
    return cm_system("%s delete '%s' %u %u '%s'",
        g_cm_cnm_cifs_script, req->dir,info->domain_type,
        info->name_type,info->name);
}    

static void cm_cnm_cifs_oplog_report(
    const sint8* sessionid, const void *pDecodeParam, uint32 alarmid)
{
    const cm_cnm_decode_info_t *decode = pDecodeParam;
    const uint32 cnt = 5;
    if(NULL == pDecodeParam)
    {   
        cm_cnm_oplog_report(sessionid,alarmid,NULL,cnt,NULL);
    }    
    else    
    {       
        const cm_cnm_cifs_param_t *req = (const cm_cnm_cifs_param_t*)decode->data;
        const cm_cnm_cifs_info_t *info = &req->info;
        const sint8* nodename = cm_node_get_name(decode->nid);
        cm_cnm_oplog_param_t params[5] = {
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_CIFS_NID,strlen(nodename),nodename},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_CIFS_DIR,strlen(req->dir),req->dir},
            {CM_OMI_DATA_INT,   CM_OMI_FIELD_CIFS_NAME_TYPE,sizeof(info->name_type),&info->name_type},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_CIFS_NAME,strlen(info->name),info->name},
            {CM_OMI_DATA_INT,   CM_OMI_FIELD_CIFS_PERMISSION,sizeof(info->permission),&info->permission},   
        };
        cm_cnm_oplog_report(sessionid,alarmid,params,cnt,&decode->set);
    }     
    return;
}    
void cm_cnm_cifs_oplog_create(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK)? CM_ALARM_LOG_CIFS_INSERT_OK : CM_ALARM_LOG_CIFS_INSERT_FAIL;
    cm_cnm_cifs_oplog_report(sessionid,pDecodeParam,alarmid);
    return;
}     

void cm_cnm_cifs_oplog_update(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK)? CM_ALARM_LOG_CIFS_UPDATE_OK : CM_ALARM_LOG_CIFS_UPDATE_FAIL;
    cm_cnm_cifs_oplog_report(sessionid,pDecodeParam,alarmid);
    return;
}       
    
void cm_cnm_cifs_oplog_delete(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK)? CM_ALARM_LOG_CIFS_DELETE_OK : CM_ALARM_LOG_CIFS_DELETE_FAIL;
    const cm_cnm_decode_info_t *decode = pDecodeParam;
    const uint32 cnt = 4;
    if(NULL == pDecodeParam)
    {   
        cm_cnm_oplog_report(sessionid,alarmid,NULL,cnt,NULL);
    }    
    else    
    {       
        const cm_cnm_cifs_param_t *req = (const cm_cnm_cifs_param_t*)decode->data;
        const cm_cnm_cifs_info_t *info = &req->info;
        const sint8* nodename = cm_node_get_name(decode->nid);
        cm_cnm_oplog_param_t params[4] = {
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_CIFS_NID,strlen(nodename),nodename},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_CIFS_DIR,strlen(req->dir),req->dir},
            {CM_OMI_DATA_INT,   CM_OMI_FIELD_CIFS_NAME_TYPE,sizeof(info->name_type),&info->name_type},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_CIFS_NAME,strlen(info->name),info->name},           
        };
        cm_cnm_oplog_report(sessionid,alarmid,params,cnt,&decode->set);
    }    
    return;
}    

/******************************************************************************
 * description  : share nfs
 * author       : zjd
 * create date  : 2018.8.13
 *****************************************************************************/
static sint32 cm_cnm_nfs_decode_ext(
        const cm_omi_obj_t ObjParam,void* data,cm_omi_field_flag_t *set)
{
    sint32 iRet = CM_OK;
    cm_cnm_nfs_info_t* nfs_info = data;
    cm_cnm_decode_param_t param_str[] =
    {
        {CM_OMI_NFS_CFG_DIR_PATH,sizeof(nfs_info->nfs_dir_path),nfs_info->nfs_dir_path,NULL},
        {CM_OMI_NFS_CFG_NFS_IP,sizeof(nfs_info->nfs_ip),nfs_info->nfs_ip,NULL},
    };

    cm_cnm_decode_param_t param_num[] = 
    {  
        {CM_OMI_NFS_CFG_NID,sizeof(nfs_info->nid),&nfs_info->nid,NULL},
        {CM_OMI_NFS_CFG_LIMIT,sizeof(nfs_info->nfs_limit),&nfs_info->nfs_limit,NULL},
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

sint32 cm_cnm_nfs_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam)
{
    return cm_cnm_decode_comm(ObjParam,sizeof(cm_cnm_nfs_info_t),
        cm_cnm_nfs_decode_ext,ppDecodeParam);
}

static void cm_cnm_nfs_encode_each(cm_omi_obj_t item,void *eachdata,void *arg)
{
    cm_omi_field_flag_t *field = arg;
    cm_cnm_nfs_info_t * nfs_info = eachdata;
    cm_cnm_map_value_str_t cols_str[] = 
    {
        {CM_OMI_NFS_CFG_NFS_IP,nfs_info->nfs_ip},
    };

    cm_cnm_map_value_num_t cols_num[] = 
    {
        {CM_OMI_NFS_CFG_LIMIT,nfs_info->nfs_limit},
    };

    cm_cnm_encode_str(item,field,cols_str,sizeof(cols_str)/sizeof(cm_cnm_map_value_str_t));
    cm_cnm_encode_num(item,field,cols_num,sizeof(cols_num)/sizeof(cm_cnm_map_value_num_t));
    return;
}

cm_omi_obj_t cm_cnm_nfs_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen)
{
    return cm_cnm_encode_comm_ext(pDecodeParam,pAckData,AckLen,
        sizeof(cm_cnm_nfs_info_t),cm_cnm_nfs_encode_each);
}

/****************************************************************************************/

#define cm_cnm_nfs_req(cmd,param,ppAck,plen) \
    cm_cnm_request_comm(CM_OMI_OBJECT_SHARE_NFS,cmd,sizeof(cm_cnm_nfs_info_t),param,ppAck,plen)
const sint8* g_cm_cnm_nfs_script = "/var/cm/script/cm_cnm_nfs.sh";
    
sint32 cm_cnm_nfs_init(void)
{
	return CM_OK;
}

sint32 cm_cnm_nfs_getbatch(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{   
    if( NULL == pDecodeParam)
    {
        CM_LOG_ERR(CM_MOD_CNM,"cm_cnm_nfs_getbatch");
        return CM_FAIL; 
    }

    return cm_cnm_nfs_req(CM_OMI_CMD_GET_BATCH,pDecodeParam,ppAckData,pAckLen);
}
   
sint32 cm_cnm_nfs_add(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    if( NULL == pDecodeParam)
    {
        CM_LOG_ERR(CM_MOD_CNM,"cm_cnm_nfs_add");
        return CM_FAIL; 
    }

    return cm_cnm_nfs_req(CM_OMI_CMD_ADD,pDecodeParam,ppAckData,pAckLen);
}



sint32 cm_cnm_nfs_count(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    if( NULL == pDecodeParam )
    {
        CM_LOG_ERR(CM_MOD_CNM,"cm_cnm_nfs_count");
        return CM_FAIL; 
    }

    return cm_cnm_nfs_req(CM_OMI_CMD_COUNT,pDecodeParam,ppAckData,pAckLen);
}
    
sint32 cm_cnm_nfs_delete(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    if( NULL == pDecodeParam )
    {
        CM_LOG_ERR(CM_MOD_CNM,"cm_cnm_nfs_delete");
        return CM_FAIL;
    }

    return cm_cnm_nfs_req(CM_OMI_CMD_DELETE,pDecodeParam,ppAckData,pAckLen);	
}



/*************************************************/

static uint32 cm_cnm_nfs_get_permission_num(uint8* str)
{
    uint32 nfs_permi_num = 0;
    sint8* nfs_permi[] = {"rw","ro","-"};
    if(NULL == str)
    {
        return 3;
    }
    for(;(0 != strcmp(str,nfs_permi[nfs_permi_num]) && (nfs_permi_num <= 2));++nfs_permi_num);
    return nfs_permi_num;
}

static sint32 cm_cnm_nfs_local_get_each(void *arg, sint8 **cols, uint32 col_num)
{
    cm_cnm_nfs_info_t *nfs_info = arg;
    const uint32 def_num = 3;    
    
    if(def_num != col_num)
    {
        CM_LOG_WARNING(CM_MOD_CNM,"col_num[%u] def_num[%u]",col_num,def_num);
        return CM_FAIL;
    }
    CM_MEM_ZERO(nfs_info,sizeof(cm_cnm_nfs_info_t));

    nfs_info->nfs_limit = (uint32)cm_cnm_nfs_get_permission_num(cols[0]);
    CM_VSPRINTF(nfs_info->nfs_ip,sizeof(nfs_info->nfs_ip),"%s",cols[1]);
    return CM_OK;
}

sint32 cm_cnm_nfs_local_getbatch(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    sint32 iRet = CM_OK;
    sint8 cmd[CM_STRING_512] = {0};
    const cm_cnm_nfs_info_t *nfs_info = NULL;
    cm_cnm_decode_info_t* req = param;

    if(NULL == req)
    {
        return CM_PARAM_ERR;
    }
    nfs_info = (const cm_cnm_nfs_info_t*)req->data;

    CM_LOG_INFO(CM_MOD_CNM,"offset[%llu] total[%u]",offset,total);
    CM_VSPRINTF(cmd,sizeof(cmd),"%s getbatch %s %llu %u",g_cm_cnm_nfs_script,nfs_info->nfs_dir_path,offset,total);
    iRet = cm_cnm_exec_get_list(cmd,cm_cnm_nfs_local_get_each,
        0,sizeof(cm_cnm_nfs_info_t),ppAck,&total);

    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]",iRet);
        return iRet;
    }
    *pAckLen = total * sizeof(cm_cnm_nfs_info_t);

    return CM_OK;

}

sint32 cm_cnm_nfs_local_count(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    uint64 cnt = 0;
    const cm_cnm_nfs_info_t *nfs_info = NULL;
    cm_cnm_decode_info_t* req = param; 
    if(NULL == req)
    {
        return CM_PARAM_ERR;
    }
    nfs_info = (const cm_cnm_nfs_info_t*)req->data;  
    cnt = (uint64)cm_exec_double("%s count %s",g_cm_cnm_nfs_script,nfs_info->nfs_dir_path);    
    return cm_cnm_ack_uint64(cnt,ppAck,pAckLen);
}

sint32 cm_cnm_nfs_local_add(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    cm_cnm_decode_info_t* req = param;
    sint8* nfs_permi[] = {"rw","ro","-"};
    const cm_cnm_nfs_info_t *nfs_info = NULL;	
    sint32 iRet = CM_OK;
    sint8 buff[CM_STRING_1K] = {0};
    if(NULL == req)
    {
        return CM_PARAM_ERR;
    }

    nfs_info = (const cm_cnm_nfs_info_t *)req->data;

    iRet = cm_exec_tmout(buff,sizeof(buff),CM_CMT_REQ_TMOUT,
        "%s add %s %s %s",g_cm_cnm_nfs_script,nfs_info->nfs_dir_path,nfs_permi[nfs_info->nfs_limit],nfs_info->nfs_ip);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]",iRet);
    }

    return iRet;
}

sint32 cm_cnm_nfs_local_delete(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    cm_cnm_decode_info_t* req = param;
    const cm_cnm_nfs_info_t *nfs_info = NULL;
    sint32 iRet = CM_OK;
    sint8 buff[CM_STRING_1K] = {0};
    if(NULL == req)
    {
        return CM_PARAM_ERR;
    }
    nfs_info = (const cm_cnm_nfs_info_t *)req->data;
    iRet = cm_exec_tmout(buff,sizeof(buff),CM_CMT_REQ_TMOUT,
        "%s delet %s %s",g_cm_cnm_nfs_script,nfs_info->nfs_dir_path,nfs_info->nfs_ip);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]\n%s",iRet,buff);
        return cm_cnm_get_errcode(&CmCnmMapCommErrCfg,buff,iRet);
    }
    
    return CM_OK;
}

void cm_cnm_nfs_oplog_add(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK)? CM_ALARM_LOG_NFS_CREATR_OK : CM_ALARM_LOG_NFS_CREATR_FAIL;   
    const cm_cnm_decode_info_t *req = pDecodeParam;
    const uint32 cnt = 4;

    /*node,pool,nas,nfs_limit*/
    if(NULL == pDecodeParam)
    {   
        cm_cnm_oplog_report(sessionid,alarmid,NULL,cnt,NULL);
    }    
    else    
    {       
        const cm_cnm_nfs_info_t *info = (const cm_cnm_nfs_info_t*)req->data;
        const sint8* nodename = cm_node_get_name(info->nid);
        cm_cnm_oplog_param_t params[] = {
            {CM_OMI_DATA_STRING,CM_OMI_NFS_CFG_NID,strlen(nodename),nodename},
            {CM_OMI_DATA_STRING,CM_OMI_NFS_CFG_DIR_PATH,strlen(info->nfs_dir_path),info->nfs_dir_path},
            {CM_OMI_DATA_STRING,CM_OMI_NFS_CFG_NFS_IP,strlen(info->nfs_ip),info->nfs_ip},
            {CM_OMI_DATA_ENUM,CM_OMI_NFS_CFG_LIMIT,sizeof(info->nfs_limit),&info->nfs_limit},
        };
        CM_LOG_ERR(CM_MOD_CNM,"node = %s,dir = %s,limit = %d,ip = %s",nodename,info->nfs_dir_path,info->nfs_limit,info->nfs_ip);
        cm_cnm_oplog_report(sessionid,alarmid,params,cnt,&req->set);
    }    
    return;
}

void cm_cnm_nfs_oplog_delete(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK)? CM_ALARM_LOG_NFS_DELETE_OK : CM_ALARM_LOG_NFS_DELETE_FAIL;
    const cm_cnm_decode_info_t *req = pDecodeParam;
    const uint32 cnt = 3;

    /*node,pool,nas*/
    if(NULL == pDecodeParam)
    {   
        cm_cnm_oplog_report(sessionid,alarmid,NULL,cnt,NULL);
    }    
    else    
    {       
        const cm_cnm_nfs_info_t *info = (const cm_cnm_nfs_info_t*)req->data;
        const sint8* nodename = cm_node_get_name(info->nid);
        cm_cnm_oplog_param_t params[] = {
            {CM_OMI_DATA_STRING,CM_OMI_NFS_CFG_NID,strlen(nodename),nodename},
            {CM_OMI_DATA_STRING,CM_OMI_NFS_CFG_DIR_PATH,strlen(info->nfs_dir_path),info->nfs_dir_path},
            {CM_OMI_DATA_STRING,CM_OMI_NFS_CFG_NFS_IP,strlen(info->nfs_ip),info->nfs_ip},
        };
        
        cm_cnm_oplog_report(sessionid,alarmid,params,cnt,&req->set);
    }   
    return; 
}    


/**************************************************************
                        nas_shadow
***************************************************************/

#define cm_cnm_nas_shadow_req(cmd,param,ppAck,plen) \
    cm_cnm_request_comm(CM_OMI_OBJECT_CNM_NAS_SHADOW,cmd,sizeof(cm_cnm_nas_shadow_info_t),param,ppAck,plen)

extern const cm_omi_map_enum_t CmOmiMapShadowOperationTypeCfg;


sint32 cm_cnm_nas_shadow_init(void)
{
    return CM_OK;
}

static sint32 cm_cnm_nas_shadow_decode_ext(
    const cm_omi_obj_t ObjParam,void* data,cm_omi_field_flag_t *set)
{
    cm_cnm_nas_shadow_info_t *info = data;
    sint32 iRet = CM_OK;
    cm_cnm_decode_param_t param_str[] = 
    {
        {CM_OMI_FIELD_NAS_SHADOW_DST,sizeof(info->dst),info->dst,NULL},
        {CM_OMI_FIELD_NAS_SHADOW_SRC,sizeof(info->src),info->src,NULL},
        {CM_OMI_FIELD_NAS_SHADOW_MOUNT,sizeof(info->mount),info->mount,NULL},
    };

    cm_cnm_decode_param_t param_num[] = 
    {
        {CM_OMI_FIELD_NAS_SHADOW_NID,sizeof(info->nid),&info->nid,NULL},
        {CM_OMI_FIELD_NAS_SHADOW_STATUS,sizeof(info->status),&info->status,NULL},
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


sint32 cm_cnm_nas_shadow_decode(const cm_omi_obj_t ObjParam,void **ppDecodeParam)
{
    return cm_cnm_decode_comm(ObjParam,sizeof(cm_cnm_nas_shadow_info_t),
        cm_cnm_nas_shadow_decode_ext,ppDecodeParam);
}

static void cm_cnm_nas_shadow_encode_each(cm_omi_obj_t item,void *eachdata,void *arg)
{
    cm_omi_field_flag_t *field = arg;
    cm_cnm_nas_shadow_info_t *info = eachdata;
    cm_cnm_map_value_str_t cols_str[] = 
    {
        {CM_OMI_FIELD_NAS_SHADOW_STATE,     info->shadow_status},
        {CM_OMI_FIELD_NAS_SHADOW_MIRROR,     info->mirror_status},
        {CM_OMI_FIELD_NAS_SHADOW_MOUNT,     info->mount},
        {CM_OMI_FIELD_NAS_SHADOW_DST,     info->dst},
        {CM_OMI_FIELD_NAS_SHADOW_SRC,     info->src},
    };

    cm_cnm_map_value_str_t cols_num[] =
    {
        {CM_OMI_FIELD_NAS_SHADOW_NID,     info->nid},
    };

    cm_cnm_encode_str(item,field,cols_str,sizeof(cols_str)/sizeof(cm_cnm_map_value_str_t));
    cm_cnm_encode_num(item,field,cols_num,sizeof(cols_num)/sizeof(cm_cnm_map_value_num_t)); 
    return;
}

cm_omi_obj_t cm_cnm_nas_shadow_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen)
{
    return cm_cnm_encode_comm_ext(pDecodeParam,pAckData,AckLen,
        sizeof(cm_cnm_nas_shadow_info_t),cm_cnm_nas_shadow_encode_each);
}    

sint32 cm_cnm_nas_shadow_get(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_cnm_nas_shadow_req(CM_OMI_CMD_GET,pDecodeParam,ppAckData,pAckLen);
}

sint32 cm_cnm_nas_shadow_getbatch(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_cnm_nas_shadow_req(CM_OMI_CMD_GET_BATCH,pDecodeParam,ppAckData,pAckLen);
}

sint32 cm_cnm_nas_shadow_count(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_cnm_nas_shadow_req(CM_OMI_CMD_COUNT,pDecodeParam,ppAckData,pAckLen);
}

sint32 cm_cnm_nas_shadow_create(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_cnm_nas_shadow_req(CM_OMI_CMD_ADD,pDecodeParam,ppAckData,pAckLen);
}    
    
sint32 cm_cnm_nas_shadow_update(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_cnm_nas_shadow_req(CM_OMI_CMD_MODIFY,pDecodeParam,ppAckData,pAckLen);
}    

sint32 cm_cnm_nas_shadow_delete(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_cnm_nas_shadow_req(CM_OMI_CMD_DELETE,pDecodeParam,ppAckData,pAckLen);
} 

static sint32 cm_cnm_nas_shadow_local_get_each(void *arg, sint8 **cols, uint32 col_num)
{
    cm_cnm_nas_shadow_info_t *info = arg;
    uint32 cut = 0;

    CM_VSPRINTF(info->dst,sizeof(info->dst),"%s",cols[0]);
    CM_VSPRINTF(info->src,sizeof(info->src),"%s",cols[1]);

    CM_VSPRINTF(info->shadow_status,sizeof(info->shadow_status),"ON");
    cut = cm_exec_int("mount | grep %s |grep :/|wc -l",info->src);
    if(0 != cut)
    {
        (void)cm_exec_tmout(info->mount,sizeof(info->mount),CM_CMT_REQ_TMOUT,
            "mount| grep %s|awk '{printf $3}'",info->src);
    }else
    {
        CM_VSPRINTF(info->mount,sizeof(info->mount)," ");
    }
    
    cut = cm_exec_int("zfs shadow mirror_status| grep 'OFF'|wc -l");
    if(0 == cut)
    {
        CM_VSPRINTF(info->mirror_status,sizeof(info->mirror_status),"ON");
    }else
    {
        CM_VSPRINTF(info->mirror_status,sizeof(info->mirror_status),"OFF");
    }
    info->nid = cm_node_get_id();

    return CM_OK;
}

sint32 cm_cnm_nas_shadow_local_get(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    sint32 iRet = CM_OK;
    uint32 cut = 0;
    uint32 cut_mirror = 0;
    cm_cnm_nas_shadow_info_t *info = NULL;

    cut = cm_exec_int("zfs shadow status|grep 'No Migration'|wc -l");
    cut_mirror = cm_exec_int("zfs shadow  mirror_status| grep 'OFF'|wc -l");
    if(0 != cut || 0 != cut_mirror)
    {
        *ppAck = NULL;
        *pAckLen = 0;
        return CM_OK;
    }
   
    info = CM_MALLOC(sizeof(cm_cnm_nas_shadow_info_t));
    if(NULL == info)
    {
        CM_LOG_ERR(CM_MOD_CNM,"malloc fail");
        return CM_FAIL;
    }

    iRet = cm_cnm_exec_get_col(cm_cnm_nas_shadow_local_get_each,info,
        "zfs shadow mirror_status|egrep -v 'Shadow'|awk -F':' '{printf $2\" \"}'");

    *ppAck = info;
    *pAckLen = sizeof(cm_cnm_nas_shadow_info_t);

    return CM_OK;
}

sint32 cm_cnm_nas_shadow_local_count(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    uint32 cut = 0;
    uint32 cut_mirror = 0;
    cut = cm_exec_int("zfs shadow status|grep 'No Migration'|wc -l");
    cut_mirror = cm_exec_int("zfs shadow  mirror_status| grep 'OFF'|wc -l");
    if(0 == cut && 0 == cut_mirror)
    {
        return cm_cnm_ack_uint64(1,ppAck,pAckLen);
    }

    return cm_cnm_ack_uint64(0,ppAck,pAckLen);
}

sint32 cm_cnm_nas_shadow_local_create(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    cm_cnm_decode_info_t* decode = param;
    cm_cnm_nas_shadow_info_t* info = (cm_cnm_nas_shadow_info_t* )decode->data;
    sint8 dst_test[CM_STRING_128];
    sint32 iRet = CM_OK;
    uint32 cut = 0;

    if(CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_NAS_SHADOW_MOUNT))
    {
        iRet = cm_system("mount -o vers=3 %s %s",info->mount,info->src);
        if(CM_OK != iRet)
        {
            CM_LOG_ERR(CM_MOD_CNM,"mount fail");
            return CM_FAIL;
        }
    }

    strncpy(dst_test,info->dst+1,sizeof(info->dst)-1);
    iRet= cm_system("zfs get type %s",dst_test);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"%s no exist",info->dst);
        return CM_PARAM_ERR;
    }
    
    iRet = cm_system("zfs shadow create %s %s",info->dst,info->src);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"zfs shadow create %s %s fail",info->dst,info->src);
        return CM_FAIL;
    }
        
    return CM_OK;
}

sint32 cm_cnm_nas_shadow_local_update(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    sint32 iRet = CM_OK;
    cm_cnm_decode_info_t* decode = param;
    cm_cnm_nas_shadow_info_t* info = (cm_cnm_nas_shadow_info_t* )decode->data;

    if(CM_OPERATION_STOP== info->status)
    {
        iRet = cm_system("zfs shadow suspend");
    }else
    {
        iRet = cm_system("zfs shadow resume");
    }

    return iRet;
}

sint32 cm_cnm_nas_shadow_local_delete(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    sint32 iRet = CM_OK;
    sint8 src[CM_STRING_64] = {0};
    sint8 mount[CM_STRING_64] = {0};

    (void)cm_exec_tmout(src,sizeof(src),CM_CMT_REQ_TMOUT,
        "zfs shadow mirror_status|grep 'To:'|awk -F':' '{printf $2}'");
    (void)cm_exec_tmout(mount,sizeof(mount),CM_CMT_REQ_TMOUT,
        "mount| grep %s|grep ':/'|awk '{printf $3}'",src);
    
    iRet = cm_system("umount %s",mount);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"umount fail");
    }
    iRet = cm_system("zfs shadow mirror_clear");
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"mirror_cleart fail");
        return CM_FAIL;
    }
    iRet = cm_system("zfs shadow stop");
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"shadow stop  fail");
        return CM_FAIL;
    }
    
    return CM_OK;
}

static void cm_cnm_nas_shadow_oplog_report(
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
        const cm_cnm_nas_shadow_info_t *info = (const cm_cnm_nas_shadow_info_t*)req->data;
        const sint8* nodename = cm_node_get_name(info->nid);
        cm_cnm_oplog_param_t params[4] = {
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_NAS_SHADOW_NID,strlen(nodename),nodename},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_NAS_SHADOW_DST,strlen(info->dst),info->dst},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_NAS_SHADOW_SRC,strlen(info->src),info->src},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_NAS_SHADOW_MOUNT,strlen(info->mount),info->mount},
        };
        
        cm_cnm_oplog_report(sessionid,alarmid,params,cnt,&req->set);
    }    
    
    return;
}    

void cm_cnm_nas_shadow_oplog_update(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK)? CM_ALARM_LOG_NAS_SHADOW_UPDATE_OK : CM_ALARM_LOG_NAS_SHADOW_UPDATE_FAIL;
    
    cm_cnm_nas_shadow_oplog_report(sessionid,pDecodeParam,alarmid);
    return;
}
void cm_cnm_nas_shadow_oplog_create(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{  
    uint32 alarmid = (Result == CM_OK)? CM_ALARM_LOG_NAS_SHADOW_CREATR_OK : CM_ALARM_LOG_NAS_SHADOW_CREATR_FAIL;
    
    cm_cnm_nas_shadow_oplog_report(sessionid,pDecodeParam,alarmid);
    return;
}

void cm_cnm_nas_shadow_oplog_delete(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK)? CM_ALARM_LOG_NAS_SHADOW_DELETE_OK : CM_ALARM_LOG_NAS_SHADOW_DELETE_FAIL;
    const cm_cnm_decode_info_t *req = pDecodeParam;
    const uint32 cnt = 1;

    if(NULL == pDecodeParam)
    {   
        cm_cnm_oplog_report(sessionid,alarmid,NULL,cnt,NULL);
    }    
    else    
    {       
        const cm_cnm_nas_shadow_info_t *info = (const cm_cnm_nas_shadow_info_t*)req->data;
        
        cm_cnm_oplog_param_t params[1] = {
            {CM_OMI_DATA_INT,CM_OMI_FIELD_NAS_SHADOW_NID,sizeof(info->nid),&info->nid},
        };
        
        cm_cnm_oplog_report(sessionid,alarmid,params,cnt,&req->set);
    }    
    
    return;

}

/****************lowdata*********************/

#define cm_cnm_lowdata_req(cmd,param,ppAck,plen) \
    cm_cnm_request_comm(CM_OMI_OBJECT_LOWDATA,cmd,sizeof(cm_cnm_lowdata_info_t),param,ppAck,plen)
static sint8* cm_cnm_lowdata_sh = "/var/cm/script/cm_cnm_lowdata.sh";

extern const cm_omi_map_enum_t CmOmiMapLowdataStatusTypeCfg;
extern const cm_omi_map_enum_t CmOmiMapLowdataUnitTypeCfg;
extern const cm_omi_map_enum_t CmOmiMapLowdataCriTypeCfg;
extern const cm_omi_map_enum_t CmOmiMapLowdataSwitchTypeCfg;


sint32 cm_cnm_lowdata_init(void)
{
    return CM_OK;
}

static sint32 cm_cnm_lowdata_decode_ext(
    const cm_omi_obj_t ObjParam,void* data,cm_omi_field_flag_t *set)
{
    cm_cnm_lowdata_info_t *info = data;
    sint32 iRet = CM_OK;
    cm_cnm_decode_param_t param_str[] = 
    {
        {CM_OMI_FIELD_LOWDATA_NAME,sizeof(info->name),info->name,NULL},
    };

    cm_cnm_decode_param_t param_num[] = 
    {
        {CM_OMI_FIELD_LOWDATA_NID,sizeof(info->nid),&info->nid,NULL},
        {CM_OMI_FIELD_LOWDATA_STATUS,sizeof(info->status),&info->status,NULL},
        {CM_OMI_FIELD_LOWDATA_UNIT,sizeof(info->time_unit),&info->time_unit,NULL},
        {CM_OMI_FIELD_LOWDATA_CRI,sizeof(info->criteria),&info->criteria,NULL},
        {CM_OMI_FIELD_LOWDATA_TIMEOUT,sizeof(info->timeout),&info->timeout,NULL},
        {CM_OMI_FIELD_LOWDATA_DELETE,sizeof(info->delete_time),&info->delete_time,NULL},
        {CM_OMI_FIELD_LOWDATA_SWITCH,sizeof(info->switchs),&info->switchs,NULL},
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


sint32 cm_cnm_lowdata_decode(const cm_omi_obj_t ObjParam, void**ppDecodeParam)
{
    return cm_cnm_decode_comm(ObjParam,sizeof(cm_cnm_lowdata_info_t),
        cm_cnm_lowdata_decode_ext,ppDecodeParam);
}
static void cm_cnm_lowdata_encode_ext(cm_omi_obj_t item,void *eachdata,void *arg)
{
    cm_omi_field_flag_t *field = arg;
    cm_cnm_lowdata_info_t *info = eachdata;
    cm_cnm_map_value_str_t cols_str[] = 
    {
        {CM_OMI_FIELD_LOWDATA_NAME,     info->name},
    };

    cm_cnm_map_value_str_t cols_num[] =
    {
        {CM_OMI_FIELD_LOWDATA_NID,     info->nid},
        {CM_OMI_FIELD_LOWDATA_STATUS,     info->status},
        {CM_OMI_FIELD_LOWDATA_UNIT,     info->time_unit},
        {CM_OMI_FIELD_LOWDATA_CRI,     info->criteria},
        {CM_OMI_FIELD_LOWDATA_TIMEOUT,     info->timeout},
        {CM_OMI_FIELD_LOWDATA_DELETE,     info->delete_time},
        {CM_OMI_FIELD_LOWDATA_PROCESS,     info->process},
    };

    cm_cnm_encode_str(item,field,cols_str,sizeof(cols_str)/sizeof(cm_cnm_map_value_str_t));
    cm_cnm_encode_num(item,field,cols_num,sizeof(cols_num)/sizeof(cm_cnm_map_value_num_t)); 
    return;
}

cm_omi_obj_t cm_cnm_lowdata_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen)
{
    return cm_cnm_encode_comm_ext(pDecodeParam,pAckData,AckLen,
        sizeof(cm_cnm_lowdata_info_t),cm_cnm_lowdata_encode_ext);
}

sint32 cm_cnm_lowdata_get(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_cnm_lowdata_req(CM_OMI_CMD_GET,pDecodeParam,ppAckData,pAckLen);
}


sint32 cm_cnm_lowdata_getbatch(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_cnm_lowdata_req(CM_OMI_CMD_GET_BATCH,pDecodeParam,ppAckData,pAckLen);
}

sint32 cm_cnm_lowdata_count(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_cnm_lowdata_req(CM_OMI_CMD_COUNT,pDecodeParam,ppAckData,pAckLen);
}

sint32 cm_cnm_lowdata_update(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_cnm_lowdata_req(CM_OMI_CMD_MODIFY,pDecodeParam,ppAckData,pAckLen);
}    

static sint32 cm_cnm_lowdata_local_get_each(void *arg, sint8 **cols, uint32 col_num)
{
    cm_cnm_lowdata_info_t *info = arg;
    const uint32 def_num = 8;  
    
    /* index domain nametype name permission */
    if(def_num > col_num)
    {
        CM_LOG_WARNING(CM_MOD_CNM,"col_num[%u] def_num[%u]",col_num,def_num);
        return CM_FAIL;
    }

    CM_VSPRINTF(info->name,sizeof(info->name),"%s",cols[0]);

    if(strcmp(cols[1],"off") == 0)
    {
        info->status = 0;
    }else if(strcmp(cols[1],"migrate") == 0)
    {
        info->status = 1;
    }else
    {
        info->status = 2;
    }

    info->timeout= atoi(cols[2]);
    info->delete_time = atoi(cols[3]);

    if(strcmp(cols[4],"second") == 0)
    {
        info->time_unit = 0;
    }else if(strcmp(cols[4],"minute") == 0)
    {
         info->time_unit = 1;
    }else if(strcmp(cols[4],"hour") == 0)
    {
        info->time_unit = 2;
    }else
    {
        info->time_unit = 3;
    }

    if(strcmp(cols[5],"atime") == 0)
    {
        info->criteria = 0;
    }else
    {
        info->criteria = 1;
    }

    if(atoi(cols[6]) == 0)
    {
        info->process = 0;
    }else
    {
        info->process = (uint32)(atof(cols[7])/atof(cols[6]) * 100);
    }

    info->nid = cm_node_get_id();
    
    return CM_OK;
}

sint32 cm_cnm_lowdata_local_getbatch(
    void *param, uint32 len,
    uint64 offset, uint32 total,  
    void **ppAck, uint32 *pAckLen)
{
    sint32 iRet = CM_OK;
    sint8 cmd[CM_STRING_512] = {0};
    
    CM_VSPRINTF(cmd,CM_STRING_512,"%s getbatch",cm_cnm_lowdata_sh);

    iRet = cm_cnm_exec_get_list(cmd,cm_cnm_lowdata_local_get_each,
        offset,sizeof(cm_cnm_lowdata_info_t),ppAck,&total);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]",iRet);
        return iRet;
    }
    *pAckLen = total * sizeof(cm_cnm_lowdata_info_t);
    return CM_OK;
}

sint32 cm_cnm_lowdata_local_get(
    void *param, uint32 len,
    uint64 offset, uint32 total,  
    void **ppAck, uint32 *pAckLen)
{
    sint32 iRet = CM_OK;
    sint8 cmd[CM_STRING_512] = {0};
    cm_cnm_decode_info_t* decode = param;
    cm_cnm_lowdata_info_t* info = (cm_cnm_lowdata_info_t* )decode->data;
    cm_cnm_lowdata_info_t* data = CM_MALLOC(sizeof(cm_cnm_lowdata_info_t));

    if(data == NULL)
    {
        CM_LOG_ERR(CM_MOD_CNM,"malloc fail");
        return CM_FAIL;
    }
    CM_VSPRINTF(cmd,CM_STRING_512,"%s getbatch",cm_cnm_lowdata_sh);

    iRet = cm_cnm_exec_get_col(cm_cnm_lowdata_local_get_each,data,
        "%s get %s",cm_cnm_lowdata_sh,info->name);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]",iRet);
        CM_FREE(data);
        return iRet;
    }
    *ppAck = data;
    *pAckLen = sizeof(cm_cnm_lowdata_info_t);
    return CM_OK;
}


sint32 cm_cnm_lowdata_local_count(
    void *param, uint32 len,
    uint64 offset, uint32 total,  
    void **ppAck, uint32 *pAckLen)
{
    uint32 cut = 0;

    cut = cm_exec_int("zfs list -H -t filesystem -o name|grep '/'|wc -l");
    
    return cm_cnm_ack_uint64(cut,ppAck,pAckLen);
}

sint32 cm_cnm_lowdata_local_update(
    void *param, uint32 len,
    uint64 offset, uint32 total,  
    void **ppAck, uint32 *pAckLen)
{
    cm_cnm_decode_info_t* decode = param;
    cm_cnm_lowdata_info_t* info = (cm_cnm_lowdata_info_t* )decode->data;
    sint32 iRet = CM_OK;
    
    if(!CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_LOWDATA_STATUS)&&
        !CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_LOWDATA_UNIT)&&
        !CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_LOWDATA_CRI)&&
        !CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_LOWDATA_TIMEOUT)&&
        !CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_LOWDATA_DELETE)&&
        !CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_LOWDATA_SWITCH))
    {
        return CM_FAIL;
    }


    if(CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_LOWDATA_STATUS))
    {
        iRet = cm_system("zfs set lowdata=%s %s",
            cm_cnm_get_enum_str(&CmOmiMapLowdataStatusTypeCfg,info->status),info->name);

        if(iRet != CM_OK)
        {
            CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]",iRet);
            return CM_FAIL;
        }
    }

    if(CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_LOWDATA_UNIT))
    {
        iRet = cm_system("zfs set lowdata_period_unit=%s %s",
            cm_cnm_get_enum_str(&CmOmiMapLowdataUnitTypeCfg,info->time_unit),info->name);

        if(iRet != CM_OK)
        {
            CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]",iRet);
            return CM_FAIL;
        }
    }

    if(CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_LOWDATA_CRI))
    {
        iRet = cm_system("zfs set lowdata_criteria=%s %s",
            cm_cnm_get_enum_str(&CmOmiMapLowdataCriTypeCfg,info->criteria),info->name);

        if(iRet != CM_OK)
        {
            CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]",iRet);
            return CM_FAIL;
        }
    }

    if(CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_LOWDATA_TIMEOUT))
    {
        iRet = cm_system("zfs set lowdata_period=%u %s",info->timeout,info->name);

        if(iRet != CM_OK)
        {
            CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]",iRet);
            return CM_FAIL;
        }
    }

    if(CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_LOWDATA_DELETE))
    {

        iRet = cm_system("zfs set lowdata_delete_period=%u %s",info->delete_time,info->name);
        if(iRet != CM_OK)
        {
            CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]",iRet);
            return CM_FAIL;
        }
        
    }

    
    if(CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_LOWDATA_SWITCH))
    {
        if(info->switchs == 2)
        {
            iRet = cm_system("zfs migrate start %s -all",info->name);
        }else
        {
            iRet = cm_system("zfs migrate %s %s",
                cm_cnm_get_enum_str(&CmOmiMapLowdataSwitchTypeCfg,info->switchs),info->name);
        }

        if(iRet != CM_OK)
        {
            CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]",iRet);
            return CM_FAIL;
        }
    }
    
    return CM_OK;
}


void cm_cnm_lowdata_oplog_update(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK)? CM_ALARM_LOG_LOWDATA_UPDATE_OK: CM_ALARM_LOG_LOWDATA_UPDATE_FAIL;
    const cm_cnm_decode_info_t *req = pDecodeParam;
    const uint32 cnt = 8;

    if(NULL == pDecodeParam)
    {   
        cm_cnm_oplog_report(sessionid,alarmid,NULL,cnt,NULL);
    }    
    else    
    {
        const cm_cnm_lowdata_info_t *info = (const cm_cnm_lowdata_info_t*)req->data;
        const sint8* nodename = cm_node_get_name(info->nid);
        cm_cnm_oplog_param_t params[8] = {
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_LOWDATA_NAME,strlen(info->name),info->name},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_LOWDATA_NID,strlen(nodename),nodename},
            {CM_OMI_DATA_INT,CM_OMI_FIELD_LOWDATA_STATUS,sizeof(info->status),&info->status},
            {CM_OMI_DATA_INT,CM_OMI_FIELD_LOWDATA_UNIT,sizeof(info->time_unit),&info->time_unit},
            {CM_OMI_DATA_INT,CM_OMI_FIELD_LOWDATA_CRI,sizeof(info->criteria),&info->criteria},
            {CM_OMI_DATA_INT,CM_OMI_FIELD_LOWDATA_TIMEOUT,sizeof(info->timeout),&info->timeout},
            {CM_OMI_DATA_INT,CM_OMI_FIELD_LOWDATA_DELETE,sizeof(info->delete_time),&info->delete_time},
            {CM_OMI_DATA_INT,CM_OMI_FIELD_LOWDATA_SWITCH,sizeof(info->switchs),&info->switchs},
        };
        
        cm_cnm_oplog_report(sessionid,alarmid,params,cnt,&req->set);
    }    
    
    return;

}


/*****************dirlowdata*******************/

#define cm_cnm_dirlowdata_req(cmd,param,ppAck,plen) \
    cm_cnm_request_comm(CM_OMI_OBJECT_DIRLOWDATA,cmd,sizeof(cm_cnm_dirlowdata_info_t),param,ppAck,plen)

sint32 cm_cnm_dirlowdata_init(void)
{
    return CM_OK;
}

static sint32 cm_cnm_dirlowdata_decode_ext(
    const cm_omi_obj_t ObjParam,void* data,cm_omi_field_flag_t *set)
{
    cm_cnm_dirlowdata_info_t *info = data;
    sint32 iRet = CM_OK;
    cm_cnm_decode_param_t param_str[] = 
    {
        {CM_OMI_FIELD_DIRLOWDATA_NAME,sizeof(info->name),info->name,NULL},
        {CM_OMI_FIELD_DIRLOWDATA_DIR,sizeof(info->dir),info->dir,NULL}, 
        {CM_OMI_FIELD_DIRLOWDATA_CLUSTER,sizeof(info->cluster),info->cluster,NULL}, 
    };

    cm_cnm_decode_param_t param_num[] = 
    {
        {CM_OMI_FIELD_DIRLOWDATA_NID,sizeof(info->nid),&info->nid,NULL},
        {CM_OMI_FIELD_DIRLOWDATA_STATUS,sizeof(info->status),&info->status,NULL},
        {CM_OMI_FIELD_DIRLOWDATA_UNIT,sizeof(info->time_unit),&info->time_unit,NULL},
        {CM_OMI_FIELD_DIRLOWDATA_CRI,sizeof(info->criteria),&info->criteria,NULL},
        {CM_OMI_FIELD_DIRLOWDATA_TIMEOUT,sizeof(info->timeout),&info->timeout,NULL},
        {CM_OMI_FIELD_DIRLOWDATA_DELETE,sizeof(info->delete_time),&info->delete_time,NULL},
        {CM_OMI_FIELD_DIRLOWDATA_SWITCH,sizeof(info->switchs),&info->switchs,NULL},
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


sint32 cm_cnm_dirlowdata_decode(const cm_omi_obj_t ObjParam, void**ppDecodeParam)
{
    return cm_cnm_decode_comm(ObjParam,sizeof(cm_cnm_dirlowdata_info_t),
        cm_cnm_dirlowdata_decode_ext,ppDecodeParam);
}
static void cm_cnm_dirlowdata_encode_ext(cm_omi_obj_t item,void *eachdata,void *arg)
{
    cm_omi_field_flag_t *field = arg;
    cm_cnm_dirlowdata_info_t *info = eachdata;
    cm_cnm_map_value_str_t cols_str[] = 
    {
        {CM_OMI_FIELD_DIRLOWDATA_DIR,     info->dir},
    };

    cm_cnm_map_value_str_t cols_num[] =
    {
        {CM_OMI_FIELD_DIRLOWDATA_NID,     info->nid},
        {CM_OMI_FIELD_DIRLOWDATA_STATUS,     info->status},
        {CM_OMI_FIELD_DIRLOWDATA_UNIT,     info->time_unit},
        {CM_OMI_FIELD_DIRLOWDATA_CRI,     info->criteria},
        {CM_OMI_FIELD_DIRLOWDATA_TIMEOUT,     info->timeout},
        {CM_OMI_FIELD_DIRLOWDATA_DELETE,     info->delete_time},
    };

    cm_cnm_encode_str(item,field,cols_str,sizeof(cols_str)/sizeof(cm_cnm_map_value_str_t));
    cm_cnm_encode_num(item,field,cols_num,sizeof(cols_num)/sizeof(cm_cnm_map_value_num_t)); 
    return;
}

cm_omi_obj_t cm_cnm_dirlowdata_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen)
{
    return cm_cnm_encode_comm_ext(pDecodeParam,pAckData,AckLen,
        sizeof(cm_cnm_dirlowdata_info_t),cm_cnm_dirlowdata_encode_ext);
}

static sint32 cm_cnm_dirlowdata_cluster_get_each(void *arg, sint8 **cols, uint32 col_num)
{
    cm_cnm_dirlowdata_info_t *info = arg;
    sint8 *p = NULL;
    int count = 0;

    info->status = cm_cnm_get_enum(&CmOmiMapLowdataStatusTypeCfg,cols[0],3);
    info->timeout = atoi(cols[1]);
    info->time_unit = cm_cnm_get_enum(&CmOmiMapLowdataUnitTypeCfg,cols[2],0);
    info->delete_time = atoi(cols[3]);
    info->criteria =  cm_cnm_get_enum(&CmOmiMapLowdataCriTypeCfg,cols[4],0);
    /*从绝对路径中取出目录名*/
    /*
    p = cols[5];
    for(;*p!='\0';p++)
    {
        if(*p == '/')
        {
            if(++count == 3)
            {
                p++;
                break;
            }
        }
    }*/
    CM_VSPRINTF(info->dir,sizeof(info->dir),"%s",cols[5]);
    info->nid = cm_node_get_id();
    
    return CM_OK;
}

sint32 cm_cnm_dirlowdata_getbatch(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_cnm_dirlowdata_req(CM_OMI_CMD_GET_BATCH,pDecodeParam,ppAckData,pAckLen);
}

sint32 cm_cnm_dirlowdata_get(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_cnm_dirlowdata_req(CM_OMI_CMD_GET,pDecodeParam,ppAckData,pAckLen);
}


sint32 cm_cnm_dirlowdata_count(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_cnm_dirlowdata_req(CM_OMI_CMD_COUNT,pDecodeParam,ppAckData,pAckLen);
}

sint32 cm_cnm_dirlowdata_update(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_cnm_dirlowdata_req(CM_OMI_CMD_MODIFY,pDecodeParam,ppAckData,pAckLen);
}    

static sint32 cm_cnm_dirlowdata_local_get_each(void *arg, sint8 **cols, uint32 col_num)
{
    cm_cnm_dirlowdata_info_t *info = arg;
    const uint32 def_num = 5;  
    
    /* index domain nametype name permission */
    if(def_num != col_num)
    {
        CM_LOG_WARNING(CM_MOD_CNM,"col_num[%u] def_num[%u]",col_num,def_num);
        return CM_FAIL;
    }

    info->status = atoi(cols[0]);
    info->timeout= atoi(cols[1]);
    info->time_unit = atoi(cols[2]);
    info->delete_time= atoi(cols[3]);
    info->criteria = atoi(cols[4]);

    info->nid = cm_node_get_id();
    
    return CM_OK;
}


static sint32 cm_cnm_dirlowdata_get_each(void* ptemp,const sint8* dir,const sint8* name,const sint8* nas,sint32 status)
{
    sint8* p = NULL;
    uint32 count = 0;
    sint8 list_dl[CM_STRING_128] = {0};
    cm_cnm_dirlowdata_info_t *info = (cm_cnm_dirlowdata_info_t*)ptemp;


    CM_MEM_ZERO(info,sizeof(cm_cnm_dirlowdata_info_t));
    CM_VSPRINTF(info->dir,sizeof(info->dir),"%s",name);
    (void)cm_exec_tmout(list_dl,CM_STRING_128,CM_CMT_REQ_TMOUT,"%s dirget %s %s",cm_cnm_lowdata_sh,nas,name);
    
    p = strtok(list_dl," ");
    while(p != NULL)
    {
        if(count == 0)
        {
            if(status != -1 && status != atoi(p))
            {
                return 1;
            }
            info->status = atoi(p);
        }else if(count == 1)
        {
            info->timeout = atoi(p);
        }else if(count == 2)
        {
            info->time_unit = atoi(p);
        }else if(count == 3)
        {
            info->delete_time= atoi(p);
        }else if(count == 4)
        {
            info->criteria = atoi(p);
        }else
        {
            break;
        }
        p = strtok(NULL," ");
        count++;
    }
    info->nid = cm_node_get_id();

    return 0;
}

static sint32 cm_cnm_dirlowdata_local_getlist(
    sint32 status,const sint8* dir,const sint8* nas,uint64 offset,
    uint32 each_size,void **ppAck,uint32 *total)
{
    sint8* ptemp = NULL;
    DIR* pdir = NULL;
    struct dirent *entry;
    sint8 dir_file[CM_STRING_128] = {0};
    uint32 cut = 0;
    struct stat buf;

    *ppAck = CM_MALLOC((*total) * each_size);
    if(NULL == ppAck)
    {
        CM_LOG_ERR(CM_MOD_CNM,"mollac fail");
        return CM_FAIL;
    }
    ptemp = *ppAck;

    pdir = opendir(dir);
    if(NULL == pdir)
    {
        CM_FREE(ptemp);
        *ppAck = NULL;
        CM_LOG_ERR(CM_MOD_CNM,"dir error");
        return CM_ERR_NOT_EXISTS;
    }  

    while(entry=readdir(pdir))
    {
        uint32 ret = 0;
        if(entry->d_name[0] == '.')
        {
            continue;
        }
        CM_VSPRINTF(dir_file,sizeof(dir_file),"%s/%s",dir,entry->d_name);
        stat(dir_file,&buf);
        if(!S_ISDIR(buf.st_mode))
        {
            continue;
        }
        if(cut < offset)
        {
            cut++;
            continue;
        }
        ret = cm_cnm_dirlowdata_get_each((void*)ptemp,dir_file,entry->d_name,nas,status);
        if(ret != 0)
        {
            continue;
        }
        cut++;
        ptemp += each_size;

        if((cut - offset) == *total)
        {
            break;
        }
    }
    closedir(pdir);
    
    if(cut < offset)
    {
        *total = 0;
         CM_FREE(*ppAck);
        *ppAck = NULL;
    }
    
    if(cut - offset < *total)
    {
        *total = cut - offset;
    }

    return CM_OK;
}

sint32 cm_cnm_dirlowdata_local_getbatch(
    void *param, uint32 len,
    uint64 offset, uint32 total,  
    void **ppAck, uint32 *pAckLen)
{
    sint32 iRet = CM_OK;
    sint8 dir[CM_STRING_128] = {0};
    sint8 nas[CM_STRING_128] = {0};
    sint8* p = NULL;
    uint32 count = 0;
    sint32 status = -1;
    cm_cnm_decode_info_t* decode = param;
    cm_cnm_dirlowdata_info_t* info = (cm_cnm_dirlowdata_info_t* )decode->data;
    
    CM_VSPRINTF(dir,sizeof(dir),"/%s",info->name);
    if(CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_DIRLOWDATA_CLUSTER))
    {
        sint8 cmd[CM_STRING_128] = {0};
        sint32 iRet = CM_OK;
        CM_LOG_ERR(CM_MOD_CNM,"cluster lowdata");

        CM_VSPRINTF(cmd,sizeof(cmd),"%s cluster",cm_cnm_lowdata_sh);
        iRet = cm_cnm_exec_get_list(cmd,cm_cnm_dirlowdata_cluster_get_each,
            offset,sizeof(cm_cnm_dirlowdata_info_t),ppAck,&total);
        if(CM_OK != iRet)
        {
            CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]",iRet);
            return iRet;
        }
        *pAckLen = total * sizeof(cm_cnm_dirlowdata_info_t);
        return CM_OK;
    }

    if(CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_DIRLOWDATA_STATUS))
    {
        status = info->status;
    }
    
    p = info->name;
    for(int i=0;i<CM_STRING_128;i++)
    {
        if(p == NULL || *p == '\0')
        {
            break;
        }
        if(p == '/')
        {
            count++;
        }
        if(count > 1)
        {
            break;
        }
        
        nas[i] = *p; 
        p++;
    }

    iRet = cm_cnm_dirlowdata_local_getlist(status,dir,nas,offset,sizeof(cm_cnm_dirlowdata_info_t),ppAck,&total);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]",iRet);
        return iRet;
    }
    *pAckLen = total * sizeof(cm_cnm_dirlowdata_info_t);
    return CM_OK;
}

sint32 cm_cnm_dirlowdata_local_get(
    void *param, uint32 len,
    uint64 offset, uint32 total,  
    void **ppAck, uint32 *pAckLen)
{
    sint32 iRet = CM_OK;
    sint8 cmd[CM_STRING_512] = {0};
    cm_cnm_decode_info_t* decode = param;
    cm_cnm_dirlowdata_info_t* info = (cm_cnm_dirlowdata_info_t* )decode->data;
    cm_cnm_dirlowdata_info_t* data = CM_MALLOC(sizeof(cm_cnm_dirlowdata_info_t));

    if(data == NULL)
    {
        CM_LOG_ERR(CM_MOD_CNM,"malloc fail");
        return CM_FAIL;
    }
    CM_VSPRINTF(cmd,CM_STRING_512,"%s getbatch",cm_cnm_lowdata_sh);

    iRet = cm_cnm_exec_get_col(cm_cnm_dirlowdata_local_get_each,data,
        "%s dirget %s '%s'",cm_cnm_lowdata_sh,info->name,info->dir);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]",iRet);
        CM_FREE(data);
        return iRet;
    }

    CM_VSPRINTF(data->dir,sizeof(data->dir),"%s",info->dir);
    *ppAck = data;
    *pAckLen = sizeof(cm_cnm_dirlowdata_info_t);
    return CM_OK;
}


sint32 cm_cnm_dirlowdata_local_count(
    void *param, uint32 len,
    uint64 offset, uint32 total,  
    void **ppAck, uint32 *pAckLen)
{
    cm_cnm_decode_info_t* decode = param;
    cm_cnm_dirlowdata_info_t* info = (cm_cnm_dirlowdata_info_t* )decode->data;
    uint32 cut = 0;

    if(CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_DIRLOWDATA_CLUSTER))
    {
        cut = cm_exec_int("%s cluster|wc -l",cm_cnm_lowdata_sh);
        return cm_cnm_ack_uint64(cut,ppAck,pAckLen);
    }

    cut = cm_exec_int("ls -l /%s |grep '^d' |wc -l",info->name);
    
    return cm_cnm_ack_uint64(cut,ppAck,pAckLen);
}

sint32 cm_cnm_dirlowdata_local_update(
    void *param, uint32 len,
    uint64 offset, uint32 total,  
    void **ppAck, uint32 *pAckLen)
{
    cm_cnm_decode_info_t* decode = param;
    cm_cnm_dirlowdata_info_t* info = (cm_cnm_dirlowdata_info_t* )decode->data;
    sint32 iRet = CM_OK;
    uint32 cut = 0;
    sint8 status[CM_STRING_32] = {0};

    if(!CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_DIRLOWDATA_STATUS)&&
        !CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_DIRLOWDATA_UNIT)&&
        !CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_DIRLOWDATA_CRI)&&
        !CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_DIRLOWDATA_TIMEOUT)&&
        !CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_DIRLOWDATA_DELETE)&&
        !CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_DIRLOWDATA_SWITCH))
    {
        return CM_FAIL;
    }

    cut = cm_exec_int("ls -al /%s/%s| egrep -v '^d|^total'|wc -l",info->name,info->dir);
    (void)cm_exec_tmout(status,sizeof(status),CM_CMT_REQ_TMOUT,
        "zfs get dirlowdata@%s %s | egrep -v NAME|awk '{printf $3}'",info->dir,info->name);

    if(CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_DIRLOWDATA_STATUS))
    {
        iRet = cm_system("zfs set dirlowdata@%s=%s %s",
            info->dir,cm_cnm_get_enum_str(&CmOmiMapLowdataStatusTypeCfg,info->status),info->name);

        if(iRet != CM_OK && cut != 0 && strcmp(status,"-") == 0)
        {
            return CM_DIRLOWDATA_FILE_EXISTS;
        }

        if(iRet != CM_OK)
        {
            CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]",iRet);
            return CM_FAIL;
        }
    }

    if(CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_DIRLOWDATA_UNIT))
    {
        iRet = cm_system("zfs set dirlowdata_period_unit@%s=%s %s",
            info->dir,cm_cnm_get_enum_str(&CmOmiMapLowdataUnitTypeCfg,info->time_unit),info->name);

        if(iRet != CM_OK)
        {
            CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]",iRet);
            return CM_FAIL;
        }
    }

    if(CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_DIRLOWDATA_CRI))
    {
        iRet = cm_system("zfs set dirlowdata_criteria@%s=%s %s",
            info->dir,cm_cnm_get_enum_str(&CmOmiMapLowdataCriTypeCfg,info->criteria),info->name);

        if(iRet != CM_OK)
        {
            CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]",iRet);
            return CM_FAIL;
        }
    }

    if(CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_DIRLOWDATA_TIMEOUT))
    {
        iRet = cm_system("zfs set dirlowdata_period@%s=%u %s",info->dir,info->timeout,info->name);

        if(iRet != CM_OK)
        {
            CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]",iRet);
            return CM_FAIL;
        }
    }

    if(CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_DIRLOWDATA_DELETE))
    {
        iRet = cm_system("zfs set dirlowdata_delete_period@%s=%u %s",info->dir,info->delete_time,info->name);
       
        if(iRet != CM_OK)
        {
            CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]",iRet);
            return CM_FAIL;
        }
    }

    if(CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_DIRLOWDATA_SWITCH))
    {
        if(info->switchs != 0)
        {
            return CM_PARAM_ERR;
        }
        iRet = cm_system("zfs migrate %s %s %s",
            cm_cnm_get_enum_str(&CmOmiMapLowdataSwitchTypeCfg,info->switchs),info->name,info->dir);

        if(iRet != CM_OK)
        {
            CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]",iRet);
            return CM_FAIL;
        }
    }
    
    return CM_OK;
}

void cm_cnm_dirlowdata_oplog_update(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK)? CM_ALARM_LOG_DIRLOWDATA_UPDATE_OK: CM_ALARM_LOG_DIRLOWDATA_UPDATE_FAIL;
    const cm_cnm_decode_info_t *req = pDecodeParam;
    const uint32 cnt = 9;

    if(NULL == pDecodeParam)
    {   
        cm_cnm_oplog_report(sessionid,alarmid,NULL,cnt,NULL);
    }    
    else    
    {
        const cm_cnm_dirlowdata_info_t *info = (const cm_cnm_dirlowdata_info_t*)req->data;
        const sint8* nodename = cm_node_get_name(info->nid);
        cm_cnm_oplog_param_t params[9] = {
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_DIRLOWDATA_NAME,strlen(info->name),info->name},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_DIRLOWDATA_NID,strlen(nodename),nodename},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_DIRLOWDATA_DIR,strlen(info->dir),info->dir},
            {CM_OMI_DATA_INT,CM_OMI_FIELD_DIRLOWDATA_STATUS,sizeof(info->status),&info->status},
            {CM_OMI_DATA_INT,CM_OMI_FIELD_DIRLOWDATA_UNIT,sizeof(info->time_unit),&info->time_unit},
            {CM_OMI_DATA_INT,CM_OMI_FIELD_DIRLOWDATA_CRI,sizeof(info->criteria),&info->criteria},
            {CM_OMI_DATA_INT,CM_OMI_FIELD_DIRLOWDATA_TIMEOUT,sizeof(info->timeout),&info->timeout},
            {CM_OMI_DATA_INT,CM_OMI_FIELD_DIRLOWDATA_DELETE,sizeof(info->delete_time),&info->delete_time},
            {CM_OMI_DATA_INT,CM_OMI_FIELD_DIRLOWDATA_SWITCH,sizeof(info->switchs),&info->switchs},
        };
        
        cm_cnm_oplog_report(sessionid,alarmid,params,cnt,&req->set);
    }    
    
    return;

}

/*******************lowdata sche**********************/

sint32 cm_cnm_lowdata_sche_cbk(const sint8* name, const sint8* param)
{
    sint32 iRet = CM_OK;
    uint32 len = sizeof(cm_cnm_decode_info_t)+sizeof(cm_cnm_lowdata_info_t);
    cm_cnm_decode_info_t *decode = NULL;
    cm_cnm_lowdata_info_t *info = NULL;
    
    uint32 nid = cm_exec_int("%s getnid %s",cm_cnm_lowdata_sh,name);

    CM_LOG_WARNING(CM_MOD_CNM,"nid[%u] name[%s] param[%s]",nid,name, param);
    if(CM_NODE_ID_NONE == nid)
    {
        return CM_OK;
    }
    decode = CM_MALLOC(len);
    if(NULL == decode)
    {
        CM_LOG_ERR(CM_MOD_CNM,"malloc fail");
        return CM_FAIL;
    }
    CM_MEM_ZERO(decode,len);
    decode->nid = nid;
    info = (cm_cnm_lowdata_info_t*)decode->data;
    
    CM_VSPRINTF(info->name,sizeof(info->name),"%s",name);
    CM_OMI_FIELDS_FLAG_SET(&decode->set,CM_OMI_FIELD_LOWDATA_NAME);

    info->switchs = 2; /*migrate all*/
    CM_OMI_FIELDS_FLAG_SET(&decode->set,CM_OMI_FIELD_LOWDATA_SWITCH);

    iRet = cm_cnm_lowdata_req(CM_OMI_CMD_MODIFY,decode,NULL,NULL);
    CM_FREE(decode);

    return iRet;
}

#define CM_CNM_SCHE_BIT_HOUR 0
#define CM_CNM_SCHE_BIT_WEEK_DAY 1
#define CM_CNM_SCHE_BIT_MONTH_DAY 2

static uint32 cm_cnm_lowdata_sche_decode_bits(uint32 type,
    const cm_omi_obj_t ObjParam,uint32 id, cm_omi_field_flag_t *set)
{
    uint32 value = 0;
    sint32 iRet = CM_OK;
    sint8 str[CM_STRING_256] = {0};
    sint8 *pfield[31] = {NULL};
    uint32 cnt = 0;
    uint32 tmp = 0;
    uint32 max_num = 0;
    /*
      hour 0-23
      weekday 0-6,对应周日 到周六 
      monthday 1-32 对应1-31号，*/
    iRet = cm_omi_obj_key_get_str_ex(ObjParam,id,str,sizeof(str));
    if(CM_OK != iRet)
    {
        return 0;
    }
    CM_OMI_FIELDS_FLAG_SET(set,id);
    cnt = cm_omi_get_fields(str,pfield,31);
    if(CM_CNM_SCHE_BIT_MONTH_DAY == type)
    {
        while(cnt > 0)
        {
            cnt--;
            tmp = (uint32)atol(pfield[cnt]);
            if((1 <= tmp) && (31 >= tmp))
            {
                value |= 1<<(tmp-1);
            }
        }
    }
    else
    {   
        max_num = (CM_CNM_SCHE_BIT_HOUR==type)? 23 : 6;
        while(cnt > 0)
        {
            cnt--;
            tmp = (uint32)atol(pfield[cnt]);
            if(max_num >= tmp)
            {
                value |= 1<<tmp;
            }
        }
    }
    return value;
}

static sint32 cm_cnm_lowdata_sche_decode_ext(
    const cm_omi_obj_t ObjParam,void* data,cm_omi_field_flag_t *set)
{
    sint32 iRet = CM_OK;  
    cm_sche_info_t *info = data;

    info->obj = CM_OMI_OBJECT_LOWDATA_SCHE;
    cm_cnm_decode_param_t param_str[] =
    {
        {CM_OMI_FIELD_LOWDATA_SCHE_NAME,sizeof(info->name),info->name,NULL},
    };

    cm_cnm_decode_param_t param_num[] = 
    {
        {CM_OMI_FIELD_LOWDATA_SCHE_EXPIRED,sizeof(uint32),&info->expired,NULL},
        {CM_OMI_FIELD_LOWDATA_SCHE_TYPE,sizeof(info->type),&info->type,NULL},
        {CM_OMI_FIELD_LOWDATA_SCHE_DAYTYPE,sizeof(info->day_type),&info->day_type,NULL},
        {CM_OMI_FIELD_LOWDATA_SCHE_MINUTE,sizeof(info->minute),&info->minute,NULL},
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
    
    info->hours = cm_cnm_lowdata_sche_decode_bits(
        CM_CNM_SCHE_BIT_HOUR,ObjParam,CM_OMI_FIELD_LOWDATA_SCHE_HOURS,set);
    
    switch(info->day_type)
    {
        case CM_SCHE_DAY_MONTH:
            info->days = cm_cnm_lowdata_sche_decode_bits(CM_CNM_SCHE_BIT_MONTH_DAY,
                ObjParam,CM_OMI_FIELD_LOWDATA_SCHE_DAYS,set);
            break;
        case CM_SCHE_DAY_WEEK:
            info->days = cm_cnm_lowdata_sche_decode_bits(CM_CNM_SCHE_BIT_WEEK_DAY,
                ObjParam,CM_OMI_FIELD_LOWDATA_SCHE_DAYS,set);
            break;
        default:
            break;
    }

    return CM_OK;
}

sint32 cm_cnm_lowdata_sche_decode(
    const cm_omi_obj_t ObjParam, void** ppDecodeParam)
{
    return cm_cnm_decode_comm(ObjParam,sizeof(cm_sche_info_t),
        cm_cnm_lowdata_sche_decode_ext,ppDecodeParam);
}

static void cm_cnm_lowdata_sche_encode_bits(uint32 type,uint32 val, sint8 *buf,uint32 len)
{
    uint32 iloop = 0;

    if(CM_CNM_SCHE_BIT_MONTH_DAY == type)
    {
        while(val)
        {
            iloop++;
            if(val & 1)
            {
                CM_SNPRINTF_ADD(buf,len,",%u",iloop);
            }
            val >>= 1;            
        }
        return;
    }
    
    while(val)
    {            
        if(val & 1)
        {
            CM_SNPRINTF_ADD(buf,len,",%u",iloop);
        }
        val >>= 1; 
        iloop++;
    }
    return;
}

static void cm_cnm_lowdata_sche_encode_each(cm_omi_obj_t item,void *eachdata,void *arg)
{
    cm_sche_info_t *info = eachdata;
    cm_omi_field_flag_t *field = arg;
    sint8 hours[CM_STRING_256] = {0};
    sint8 days[CM_STRING_256] = {0};
    
    cm_cnm_map_value_str_t cols_str[] = {
        {CM_OMI_FIELD_LOWDATA_SCHE_NAME,    info->name},
        {CM_OMI_FIELD_LOWDATA_SCHE_HOURS,   &hours[1]},
        {CM_OMI_FIELD_LOWDATA_SCHE_DAYS,    &days[1]},
    };
    cm_cnm_map_value_num_t cols_num[] = {
        {CM_OMI_FIELD_LOWDATA_SCHE_EXPIRED,     (uint32)info->expired},
        {CM_OMI_FIELD_LOWDATA_SCHE_TYPE,    (uint32)info->type},
        {CM_OMI_FIELD_LOWDATA_SCHE_DAYTYPE,    (uint32)info->day_type},
        {CM_OMI_FIELD_LOWDATA_SCHE_MINUTE,    (uint32)info->minute},
    };

    cm_cnm_lowdata_sche_encode_bits(CM_CNM_SCHE_BIT_HOUR,info->hours,hours,sizeof(hours));
    switch(info->day_type)
    {
        case CM_SCHE_DAY_MONTH:
            cm_cnm_lowdata_sche_encode_bits(CM_CNM_SCHE_BIT_MONTH_DAY,info->days,days,sizeof(days));
            break;
        case CM_SCHE_DAY_WEEK:
            cm_cnm_lowdata_sche_encode_bits(CM_CNM_SCHE_BIT_WEEK_DAY,info->days,days,sizeof(days));
            break;
        default:
            break;
    }
    
    if(CM_OMI_FIELDS_FLAG_ISSET(field,CM_OMI_FIELD_LOWDATA_SCHE_ID))
    {
        (void)cm_omi_obj_key_set_u64_ex(item,CM_OMI_FIELD_LOWDATA_SCHE_ID,info->id);
    }
    cm_cnm_encode_str(item,field,cols_str,sizeof(cols_str)/sizeof(cm_cnm_map_value_str_t));
    cm_cnm_encode_num(item,field,cols_num,sizeof(cols_num)/sizeof(cm_cnm_map_value_num_t)); 
    return;
}

cm_omi_obj_t cm_cnm_lowdata_sche_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen)
{
    return cm_cnm_encode_comm_ext(pDecodeParam,pAckData,AckLen,
        sizeof(cm_sche_info_t),cm_cnm_lowdata_sche_encode_each);
}

sint32 cm_cnm_lowdata_sche_getbatch(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    sint32 iRet = CM_OK;
    const cm_cnm_decode_info_t *decode = pDecodeParam;
    const cm_sche_info_t *info = NULL;
    info = (cm_sche_info_t* )decode->data;

    *pAckLen = 0;
    if(NULL == decode)
    {
        iRet = cm_sche_getbatch(CM_OMI_OBJECT_LOWDATA_SCHE,NULL,0,CM_CNM_MAX_RECORD,
            (cm_sche_info_t**)ppAckData,pAckLen);
    }
    else
    {
        iRet = cm_sche_getbatch(CM_OMI_OBJECT_LOWDATA_SCHE,info->name,
            (uint32)decode->offset,decode->total,
            (cm_sche_info_t**)ppAckData,pAckLen);
    }
    *pAckLen *= sizeof(cm_sche_info_t);
    return iRet;
}

   
sint32 cm_cnm_lowdata_sche_update(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    const cm_cnm_decode_info_t *decode = pDecodeParam;
    const cm_sche_info_t *info = NULL;
    uint32 set_flag = 0;
    uint32 id = 0;
    sint32 iRet = CM_OK;
    cm_cnm_map_value_num_t mapflag[] = {
        {CM_OMI_FIELD_LOWDATA_SCHE_NAME, CM_SCHE_FLAG_NAME},
        {CM_OMI_FIELD_LOWDATA_SCHE_EXPIRED, CM_SCHE_FLAG_EXPIRED},
        {CM_OMI_FIELD_LOWDATA_SCHE_TYPE, CM_SCHE_FLAG_TYPE},
        {CM_OMI_FIELD_LOWDATA_SCHE_DAYTYPE, CM_SCHE_FLAG_DAYTYPE},
        {CM_OMI_FIELD_LOWDATA_SCHE_MINUTE, CM_SCHE_FLAG_MINUTE},
        {CM_OMI_FIELD_LOWDATA_SCHE_HOURS, CM_SCHE_FLAG_HOURS},
        {CM_OMI_FIELD_LOWDATA_SCHE_DAYS, CM_SCHE_FLAG_DAYS},
    };
    uint32 iloop = sizeof(mapflag)/sizeof(cm_cnm_map_value_num_t);
    
    if(NULL == decode)
    {
        return CM_PARAM_ERR;
    }    

    while(iloop > 0)
    {
        iloop--;
        if(CM_OMI_FIELDS_FLAG_ISSET(&decode->set,mapflag[iloop].id))
        {
            set_flag |= mapflag[iloop].value;
        }
    }
    
    if(0 == set_flag)
    {
        CM_LOG_ERR(CM_MOD_CNM,"param null");
        return CM_PARAM_ERR;
    }
    
    info = (const cm_sche_info_t*)decode->data;
    id = cm_exec_int("%s getdbid %s %u",cm_cnm_lowdata_sh,info->name,CM_OMI_OBJECT_LOWDATA_SCHE);
    return cm_sche_update(id,set_flag,(cm_sche_info_t*)info);
}
    
sint32 cm_cnm_lowdata_sche_create(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    const cm_cnm_decode_info_t *decode = pDecodeParam;
    const cm_sche_info_t *info = NULL;
    uint32 id = 0;

    if(NULL == decode)
    {
        return CM_PARAM_ERR;
    }
    info = (const cm_sche_info_t*)decode->data;
    if((!CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_LOWDATA_SCHE_NAME))
        || (0 == strlen(info->name)))
    {
        CM_LOG_ERR(CM_MOD_CNM,"name null");
        return CM_PARAM_ERR;
    }
    
    if(!CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_LOWDATA_SCHE_EXPIRED))
    {
        CM_LOG_ERR(CM_MOD_CNM,"expired null:%lu",info->expired);
        return CM_PARAM_ERR;
    }
    id = cm_exec_int("%s getdbid %s %u",cm_cnm_lowdata_sh,info->name,CM_OMI_OBJECT_LOWDATA_SCHE);
    if(id != 0)
    {
        return CM_ERR_ALREADY_EXISTS;
    }
    CM_VSPRINTF(info->param,sizeof(info->param),"%s",info->name);
    
    return cm_sche_create((cm_sche_info_t*)info);
}
    
sint32 cm_cnm_lowdata_sche_count(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    uint64 cnt = 0;
    const cm_cnm_decode_info_t *decode = pDecodeParam;
    const cm_sche_info_t *info = NULL;

    cnt = cm_sche_count(CM_OMI_OBJECT_LOWDATA_SCHE,NULL);

    return cm_cnm_ack_uint64(cnt,ppAckData,pAckLen);
}
    
sint32 cm_cnm_lowdata_sche_delete(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    const cm_cnm_decode_info_t *decode = pDecodeParam;
    const cm_sche_info_t *info = NULL;
    uint32 id = 0;
    
    if(NULL == decode)
    {
        return CM_PARAM_ERR;
    }
    info = (const cm_sche_info_t*)decode->data;

    id = cm_exec_int("%s getdbid %s %u",cm_cnm_lowdata_sh,info->name,CM_OMI_OBJECT_LOWDATA_SCHE);
    if(id == 0)
    {
        return CM_PARAM_ERR;
    }
    
    return cm_sche_delete(id);
}

/*============================================================================*/

sint32 cm_cnm_nas_client_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam)
{
    return cm_cnm_decode_comm(ObjParam,0,NULL,ppDecodeParam);
}

static void cm_cnm_nas_client_encode_ext(cm_omi_obj_t item,void *eachdata,void *arg)
{
    cm_omi_field_flag_t *field = arg;
    cm_cnm_nas_client_info_t *info = eachdata;
    cm_cnm_map_value_str_t cols_str[] = 
    {
        {CM_OMI_FIELD_NAS_CLIENT_IP,     info->ipaddr},
        {CM_OMI_FIELD_NAS_CLIENT_PROTO,     info->proto},
    };
    cm_cnm_encode_str(item,field,cols_str,sizeof(cols_str)/sizeof(cm_cnm_map_value_str_t));
    return;
}
cm_omi_obj_t cm_cnm_nas_client_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen)
{
    return cm_cnm_encode_comm_ext(pDecodeParam,pAckData,AckLen,
        sizeof(cm_cnm_nas_client_info_t),cm_cnm_nas_client_encode_ext);
}

static sint32 cm_cnm_nas_client_get_each(void *arg, sint8 **cols, uint32 col_num)
{
    cm_cnm_nas_client_info_t *info = arg;
    const uint32 def_num = 2;    
    
    if(def_num != col_num)
    {
        CM_LOG_WARNING(CM_MOD_CNM,"col_num[%u] def_num[%u]",col_num,def_num);
        return CM_FAIL;
    }
    CM_MEM_ZERO(info,sizeof(cm_cnm_nas_client_info_t));

    CM_VSPRINTF(info->ipaddr,sizeof(info->ipaddr),"%s",cols[0]);
    CM_VSPRINTF(info->proto,sizeof(info->proto),"%s",cols[1]);
    return CM_OK;
}

sint32 cm_cnm_nas_client_getbatch(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    sint32 iRet = CM_OK;
    const cm_cnm_decode_info_t* decode = pDecodeParam;
    uint32 total = CM_CNM_MAX_RECORD;
    uint32 offset = 0;
    if (NULL != decode)
    {
        offset = (uint32)decode->offset;
        total = decode->total;
    }
    
    iRet = cm_cnm_exec_get_list(CM_SCRIPT_DIR"cm_cnm.sh client_stat",
             cm_cnm_nas_client_get_each, offset, 
             sizeof(cm_cnm_nas_client_info_t), ppAckData, &total);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]",iRet);
        return iRet;
    }
    *pAckLen = total * sizeof(cm_cnm_nas_client_info_t);
    return CM_OK;
}

sint32 cm_cnm_nas_client_count(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    uint64 cnt = 0;
    
    cnt = cm_exec_int(CM_SCRIPT_DIR"cm_cnm.sh client_stat |wc -l");

    return cm_cnm_ack_uint64(cnt,ppAckData,pAckLen);
}


/*********************** lowdata_volume ****************************/

#define CM_LOWDATA_INI CM_DATA_DIR"lowdata_config.ini"

sint32 cm_cnm_lowdata_volume_init(void)
{
    sint32 iRet;
    void* handle;
    handle = cm_ini_open(CM_LOWDATA_INI);

    if(NULL == handle)
    {
        iRet = cm_system("touch "CM_LOWDATA_INI);

        if(CM_OK != iRet)
        {
            CM_LOG_ERR(CM_MOD_CNM, "touch CM_LOWDATA_INI fail[%d]", iRet);
            return CM_FAIL;
        }
    }
    else
    {
        cm_ini_free(handle);
    }    

    return CM_OK;
}

static sint32 cm_cnm_lowdata_volume_decode_ext(
    const cm_omi_obj_t ObjParam,void* data,cm_omi_field_flag_t *set)
{
    cm_cnm_lowdata_volume_info_t *info = data;
    sint32 iRet = CM_OK;
    cm_cnm_decode_param_t param_num[] = 
    {
        {CM_OMI_FIELD_LOWDATA_VOLUME_PERCENT,sizeof(info->percent),&info->percent,NULL},
        {CM_OMI_FIELD_LOWDATA_VOLUME_STOP,sizeof(info->stop),&info->stop,NULL},
    };

    iRet = cm_cnm_decode_num(ObjParam,param_num,
        sizeof(param_num)/sizeof(cm_cnm_decode_param_t),set);
    if(iRet != CM_OK)
    {
        return iRet;
    }
    return CM_OK;
}


sint32 cm_cnm_lowdata_volume_decode(const cm_omi_obj_t ObjParam, void**ppDecodeParam)
{
    return cm_cnm_decode_comm(ObjParam,sizeof(cm_cnm_lowdata_volume_info_t),
        cm_cnm_lowdata_volume_decode_ext,ppDecodeParam);
}
static void cm_cnm_lowdata_volume_encode_ext(cm_omi_obj_t item,void *eachdata,void *arg)
{
    cm_omi_field_flag_t *field = arg;
    cm_cnm_lowdata_volume_info_t *info = eachdata;

    cm_cnm_map_value_str_t cols_num[] =
    {
        {CM_OMI_FIELD_LOWDATA_VOLUME_PERCENT,     info->percent},
        {CM_OMI_FIELD_LOWDATA_VOLUME_STOP,     info->stop},
    };

    cm_cnm_encode_num(item,field,cols_num,sizeof(cols_num)/sizeof(cm_cnm_map_value_num_t)); 
    return;
}

cm_omi_obj_t cm_cnm_lowdata_volume_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen)
{
    return cm_cnm_encode_comm_ext(pDecodeParam,pAckData,AckLen,
        sizeof(cm_cnm_lowdata_volume_info_t),cm_cnm_lowdata_volume_encode_ext);
}

sint32 cm_cnm_lowdata_volume_get(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    sint32 iRet = CM_OK;
    cm_cnm_lowdata_volume_info_t* info = NULL;

    info = CM_MALLOC(sizeof(cm_cnm_lowdata_volume_info_t));
    if(info == NULL)
    {
        CM_LOG_ERR(CM_MOD_CNM, "malloc fail");
        return CM_FAIL; 
    }
    
    iRet = cm_ini_get_ext_uint32(CM_LOWDATA_INI, "config","start", &info->percent);
    if(iRet != CM_OK)
    {
        info->percent = 0;
    }
    iRet = cm_ini_get_ext_uint32(CM_LOWDATA_INI, "config","stop", &info->stop);
    if(iRet != CM_OK)
    {
        info->stop = 0;
    }
    *ppAckData = info;
    *pAckLen = sizeof(cm_cnm_lowdata_volume_info_t);
    
    return CM_OK;
}

sint32 cm_cnm_lowdata_volume_create(const void * pDecodeParam,void ** ppAckData,uint32 * pAckLen)
{
    sint32 iRet = CM_OK;
    const cm_cnm_decode_info_t *decode = pDecodeParam;
    cm_cnm_lowdata_volume_info_t info;
    uint32 start = 0;
    uint32 stop = 0;

    if(!CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_LOWDATA_VOLUME_PERCENT) &&
        !CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_LOWDATA_VOLUME_STOP))
    {
        CM_LOG_ERR(CM_MOD_CNM,"null param");
        return CM_FAIL;
    }

    CM_MEM_CPY(&info,sizeof(info),decode->data,sizeof(info));
    cm_ini_get_ext_uint32(CM_LOWDATA_INI, "config","start", &start);
    cm_ini_get_ext_uint32(CM_LOWDATA_INI, "config","stop", &stop);

    if(!CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_LOWDATA_VOLUME_PERCENT))
    {
        info.percent= start;
    }
    
    if(!CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_LOWDATA_VOLUME_STOP))
    {
        info.stop = stop;
    }

    if(info.stop >= info.percent && info.percent != 0)
    {
        return CM_PARAM_ERR;
    }

    if(0 > info.percent || 100 < info.percent)
    {
        return CM_PARAM_ERR;
    }

    if(0 > info.stop || 100 < info.stop)
    {
        return CM_PARAM_ERR;
    }
    
    return cm_sync_request(CM_SYNC_OBJ_LOWDATA_VOLUME,0,(void* )&info,sizeof(cm_cnm_decode_info_t));
}

sint32 cm_cnm_lowdata_volume_sync_request(uint64 data_id, void *pdata, uint32 len)
{
    cm_cnm_lowdata_volume_info_t* info = (cm_cnm_lowdata_volume_info_t*)pdata;

    (void)cm_ini_set_ext_uint32(CM_LOWDATA_INI, "config","start", info->percent);
    (void)cm_ini_set_ext_uint32(CM_LOWDATA_INI, "config","stop", info->stop);

    return CM_OK;
}

void cm_cnm_lowdata_volume_thread(void)
{
    sint8* script = "/var/cm/script/cm_cnm_lowdata.sh";

    (void)cm_system("%s thread",script);
}

void cm_cnm_lowdata_volume_stop_thread(void)
{
    //static uint32 cut = 0;
    sint8* script = "/var/cm/script/cm_cnm_lowdata.sh";
    /*
    cut++;
    if( cut%5 != 0)
    {
        return;
    }
    */
    (void)cm_system("%s thread_stop",script);
}


/***********************************copy***********************************/

#define CM_NASCOPY_DB_DIR "/var/cm/data/cm_nascopy.db"

const sint8* g_cm_cnm_copy_script = "/var/cm/script/cm_cnm.sh";

static cm_db_handle_t nascopy_db_handle = NULL;

sint32 cm_cnm_nas_copy_init(void)
{

    sint32 iRet = CM_OK;
    const sint8* table = "CREATE TABLE IF NOT EXISTS copy_t ("
                         "nas VARCHAR(128),"
                         "copynum INT)";
    
    iRet = cm_db_open_ext(CM_NASCOPY_DB_DIR,&nascopy_db_handle);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"open %s fail",CM_NASCOPY_DB_DIR);
        return iRet;
    }
    iRet = cm_db_exec_ext(nascopy_db_handle, table);
    if(iRet != CM_OK)
    {
        CM_LOG_ERR(CM_MOD_CNM, "create copy_t fail[%d]", iRet);
        return iRet;
    }
    return CM_OK;
}

sint32 cm_cnm_nas_copy_update(const void * pDecodeParam,void **ppAckData,uint32 * pAckLen)
{
    sint32 iRet = CM_OK;
    const cm_cnm_decode_info_t *decode = pDecodeParam;
    cm_cnm_nas_copy_info_t *info = NULL;

    if(decode == NULL)
    {
        return CM_PARAM_ERR;
    }
    info = (const cm_cnm_nas_copy_info_t*)decode->data;
    
    iRet = cm_system("%s copy_update %s %u %u",g_cm_cnm_copy_script,
        info->nas,info->nid,info->copynum);
    if(iRet != CM_OK)
    {
        CM_LOG_ERR(CM_MOD_CNM, "update copy[%s] fail[%d],", info->nas, iRet);
        return iRet;
    }
    return CM_OK;
}

sint32 cm_cnm_nas_copy_count(const void * pDecodeParam,void **ppAckData,uint32 * pAckLen)
{
    uint64 cnt = 0;
    sint32 iRet = CM_OK;
    const cm_cnm_decode_info_t *decode = pDecodeParam;
    if(decode == NULL)
    {
        CM_LOG_ERR(CM_MOD_CNM,"decode param null");
        return CM_PARAM_ERR;
    }
    cm_cnm_nas_copy_info_t *info = (const cm_cnm_nas_copy_info_t*)decode->data;
    
    iRet = cm_db_exec_get_count(nascopy_db_handle,&cnt,"SELECT copynum FROM" 
        " copy_t WHERE nas='%s'",info->nas);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM, "get count failed[%d]", iRet);
        return CM_FAIL;
    }
    if(cnt == 0)
    {
        cnt = 1;
    }
    return cm_cnm_ack_uint64(cnt,ppAckData,pAckLen);
}

static sint32 cm_cnm_nas_copy_decode_ext(
    const cm_omi_obj_t ObjParam,void* data,cm_omi_field_flag_t *set)
{
    sint32 iRet = CM_OK;
    cm_cnm_nas_copy_info_t *info = data;
    cm_cnm_decode_param_t param_str[] = 
    {
        {CM_OMI_FIELD_NASCOPY_NAS,sizeof(info->nas),info->nas,NULL},
    };
    cm_cnm_decode_param_t param_num[] = 
    {
        {CM_OMI_FIELD_NASCOPY_NID,sizeof(info->nid),&info->nid,NULL},
        {CM_OMI_FIELD_NASCOPY_NUM,sizeof(info->copynum),&info->copynum,NULL}
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
sint32 cm_cnm_nas_copy_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam)
{
    return cm_cnm_decode_comm(ObjParam,sizeof(cm_cnm_nas_copy_info_t),
        cm_cnm_nas_copy_decode_ext,ppDecodeParam);
}


