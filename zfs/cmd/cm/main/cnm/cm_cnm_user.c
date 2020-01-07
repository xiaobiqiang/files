/******************************************************************************
 *          Copyright (c) 2018 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cnm_user.c
 * author     : xar
 * create date: 2018.08.10
 * description: TODO:
 *
 *****************************************************************************/
#include "cm_cnm_user.h"
#include "cm_log.h"
#include "cm_sync.h"
#include "cm_node.h"
#include "cm_omi.h"
#include "cm_db.h"
#include "cm_cnm_common.h"

const sint8* cm_cnm_user_sh = "/var/cm/script/cm_cnm_user.sh";

sint32 cm_cnm_user_init(void)
{
    return CM_OK;
}

static sint32 cm_cnm_user_decode_ext(
    const cm_omi_obj_t ObjParam,void* data,cm_omi_field_flag_t* set)
{
    sint32 iRet = CM_OK;
    cm_cnm_user_info_t *info = data;
    cm_cnm_decode_param_t param_str[] = 
    {
        {CM_OMI_FIELD_USER_NAME,sizeof(info->name),info->name,NULL},
        {CM_OMI_FIELD_USER_PATH,sizeof(info->path),info->path,NULL},
        {CM_OMI_FIELD_USER_PWD,sizeof(info->pwd),info->pwd,NULL},
    };
    
    cm_cnm_decode_param_t param_num[] =
    {
        {CM_OMI_FIELD_USER_GID,sizeof(info->gid),&info->gid,NULL},
        {CM_OMI_FIELD_USER_ID,sizeof(info->id),&info->id,NULL},
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

sint32 cm_cnm_user_decode(const cm_omi_obj_t ObjParam,void **ppDecodeParam)
{
    return cm_cnm_decode_comm(ObjParam,sizeof(cm_cnm_user_info_t),cm_cnm_user_decode_ext,ppDecodeParam);
}

static void cm_cnm_user_encode_each(cm_omi_obj_t item,void* eachdata,void* arg)
{
    cm_omi_field_flag_t *field = arg;
    cm_cnm_user_info_t *info = eachdata;
    cm_cnm_map_value_str_t cols_str[] = 
    {
        {CM_OMI_FIELD_USER_NAME,info->name},
        {CM_OMI_FIELD_USER_PATH,info->path},
    };
    cm_cnm_map_value_num_t cols_num[] = 
    {
        {CM_OMI_FIELD_USER_GID,info->gid},
        {CM_OMI_FIELD_USER_ID,info->id},
    };

    cm_cnm_encode_str(item,field,cols_str,sizeof(cols_str)/sizeof(cm_cnm_map_value_str_t));
    cm_cnm_encode_num(item,field,cols_num,sizeof(cols_num)/sizeof(cm_cnm_map_value_str_t));
    return;
}

cm_omi_obj_t cm_cnm_user_encode(const void *pDecodeParam,void *pAckData,uint32 AckLen)
{
    return cm_cnm_encode_comm_ext(pDecodeParam,pAckData,AckLen,
        sizeof(cm_cnm_user_info_t),cm_cnm_user_encode_each);
}

sint32 cm_cnm_user_create(const void *pDecodeParam,void **ppAckData,uint32 * pAckLen)
{
    cm_cnm_decode_info_t *decode = pDecodeParam;
    cm_cnm_user_info_t *info = NULL;
    cm_cnm_user_infos_t infoset;
    uint32 cut = 0;/*判断账号是否存在*/
    uint32 id = 0;
    
    if(decode == NULL)
    {
        return CM_PARAM_ERR;
    }
    infoset.flag = CM_TRUE;
    info = (cm_cnm_user_info_t *)decode->data;
    if(!CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_USER_GID))
    {
        info->gid = 1;
    }
    CM_MEM_CPY(&(infoset.user),sizeof(cm_cnm_user_info_t),info,sizeof(cm_cnm_user_info_t));
    if(!CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_USER_NAME))
    {
        CM_LOG_ERR(CM_MOD_CNM,"name null");
        return CM_PARAM_ERR;
    }
    if(!CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_USER_PWD))
    {
        CM_LOG_ERR(CM_MOD_CNM,"pwd null");
        return CM_PARAM_ERR;
    }
    
    cut = (uint32)cm_exec_int("cat /etc/passwd | grep  '^%s:' | wc -l",info->name);
    if(cut != 0)
    {
        CM_LOG_ERR(CM_MOD_CNM,"name[%s] exits",info->name);
        return CM_ERR_ALREADY_EXISTS;
    }
    
    if(CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_USER_ID))
    {
        id = info->id;
    }else{
        id = (uint32)cm_exec_int("%s maxid",cm_cnm_user_sh);
    }
    return cm_sync_request(CM_SYNC_OBJ_USER,id,(void* )&infoset,sizeof(cm_cnm_user_infos_t));
}

sint32 cm_cnm_user_delete(const void * pDecodeParam,void **ppAckData,uint32 * pAckLen)
{
    const cm_cnm_decode_info_t *decode = pDecodeParam;
    const cm_cnm_user_info_t *info = NULL;
    sint32 cut = 0;
    uint32 id = 0;
    if(decode==NULL)
    {
        return CM_PARAM_ERR;
    }

    info = (const cm_cnm_user_info_t*) decode->data;
    if(!CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_USER_NAME))
    {
        CM_LOG_ERR(CM_MOD_CNM,"name null");
        return CM_PARAM_ERR;
    }

    if(0 == strcmp(info->name,"root"))
    {
        CM_LOG_ERR(CM_MOD_CNM,"name root");
        return CM_FAIL;
    }

    cut = (uint32)cm_exec_int("cat /etc/passwd | grep -w '%s' | wc -l",info->name);
    if(cut == 0)
    {
        CM_LOG_ERR(CM_MOD_CNM,"name[%s] no exits",info->name);
        return CM_ERR_NOT_EXISTS;
    }
    id = (uint32)cm_exec_int("cat /etc/passwd | grep '^%s:' | awk -F':' '{printf $3}'",info->name);
    if(id == 0)
    {
        CM_LOG_ERR(CM_MOD_CNM,"id 0");
        return CM_PARAM_ERR;
    }
    
    return cm_sync_delete(CM_SYNC_OBJ_USER,id);
}


sint32 cm_cnm_user_update(const void * pDecodeParam,void ** ppAckData,uint32 * pAckLen)
{
    cm_cnm_decode_info_t *decode = pDecodeParam;
    cm_cnm_user_info_t *info = NULL;
    cm_cnm_user_infos_t oldinfo;
    uint64 id = 0;

    
    if(decode == NULL)
    {
        return CM_PARAM_ERR;
    }
    oldinfo.flag = CM_TRUE;
    info = (cm_cnm_decode_info_t *)decode->data;
    if(!CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_USER_NAME))
    {
        CM_LOG_ERR(CM_MOD_CNM,"name null");
        return CM_PARAM_ERR;
    }
    if(!CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_USER_PWD)&&!CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_USER_GID)
        &&!CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_USER_PATH)&&!CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_USER_ID))
    {
        CM_LOG_ERR(CM_MOD_CNM,"update param null");
        return CM_PARAM_ERR;
    }
    if(!CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_USER_GID))
    {
        info->gid = 0;
    }
    if(!CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_USER_ID))
    {
        info->id = 0;
    }

    id = (uint64)cm_exec_int("cat /etc/passwd | grep '^%s:' | awk -F':' '{printf $3}'",info->name);
    CM_MEM_CPY(&(oldinfo.user),sizeof(cm_cnm_user_info_t),info,sizeof(cm_cnm_user_info_t));
    (void)cm_exec_tmout(oldinfo.passwd,CM_STRING_256,CM_CMT_REQ_TMOUT,
        "cat /etc/passwd | grep -w '^%s:'",info->name);
    (void)cm_exec_tmout(oldinfo.shadow,CM_STRING_256,CM_CMT_REQ_TMOUT,
        "cat /etc/shadow | grep -w '^%s:'",info->name);
    
    return cm_sync_request(CM_SYNC_OBJ_USER,id,(void* )&oldinfo,sizeof(cm_cnm_user_infos_t));
}

sint32 cm_cnm_user_count(const void * pDecodeParam,void **ppAckData,uint32 *pAckLen)
{
    const cm_cnm_decode_info_t *decode = pDecodeParam;
    const cm_cnm_user_info_t *info = NULL;
    uint64 cut = 0;

    cut = (uint64)cm_exec_int("cat /etc/passwd | awk -F':' '($3>99&&$3<50000) {print $3}'| wc -l");

    if(decode != NULL&&CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_USER_GID))
    {
        info = (const cm_cnm_user_info_t *)decode->data;
        cut = (uint64)cm_exec_int(
            "cat /etc/passwd | awk -F':' '($3>99&&$3<50000)&&$4==%u'| wc -l",info->gid);
    }
    
    return cm_cnm_ack_uint64(cut,ppAckData,pAckLen);
}

static sint32 cm_cnm_user_get_each(void *arg, sint8 **cols, uint32 col_num)
{
    cm_cnm_user_info_t *info = arg;
    if(col_num != 4)
    {
        CM_LOG_ERR(CM_MOD_CNM,"col_num %u",col_num);
        return CM_FAIL;
    }
    info->id = (uint32)atoi(cols[0]);
    info->gid = (uint32)atoi(cols[1]);
    CM_VSPRINTF(info->name,sizeof(info->name),"%s",cols[2]);
    CM_VSPRINTF(info->path,sizeof(info->path),"%s",cols[3]);
    return CM_OK;
}

sint32 cm_cnm_user_getbatch(const void * pDecodeParam,void **ppAckData,uint32 * pAckLen)
{
    sint32 iRet =  CM_OK;
    const cm_cnm_decode_info_t *decode = pDecodeParam;
    const cm_cnm_user_info_t *info = NULL;
    sint8 cmd[CM_STRING_256] = {0};
    uint32 offset = 0;
    uint32 total = CM_CNM_MAX_RECORD;
    if(decode != NULL)
    {
        offset = decode->offset;
        total = decode->total;
        info = (const cm_cnm_user_info_t *)decode->data;
        if(CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_USER_GID))
        {
            CM_VSPRINTF(cmd,CM_STRING_256,"%s getbatch %u",cm_cnm_user_sh,info->gid);
        }
        else
        {
            CM_VSPRINTF(cmd,CM_STRING_256,"%s getbatch null",cm_cnm_user_sh);
        } 
    }else
    {
        CM_VSPRINTF(cmd,CM_STRING_256,"%s getbatch null",cm_cnm_user_sh);
    }

    iRet = cm_cnm_exec_get_list(cmd,cm_cnm_user_get_each,
        offset,sizeof(cm_cnm_user_info_t),ppAckData,&total);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]",iRet);
        return iRet;
    }
    *pAckLen = total * sizeof(cm_cnm_user_info_t);
    return CM_OK;
}
    
sint32 cm_cnm_user_get(
    const void * pDecodeParam,void **ppAckData,uint32 * pAckLen)
{
    const cm_cnm_decode_info_t *decode = pDecodeParam;
    sint8 str[CM_STRING_512] ={0};
    const cm_cnm_user_info_t *info = NULL;
    cm_cnm_user_info_t *data = NULL;
    sint32 iRet = CM_OK;
     
    if((decode == NULL) || 
        (!CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_USER_NAME)))
    {
        return CM_PARAM_ERR;
    }

    info = (const cm_cnm_user_info_t *)decode->data;

    iRet = cm_exec_tmout(str,CM_STRING_512,CM_CMT_REQ_TMOUT,
        "grep -w '%s' /etc/passwd",info->name);
    if(iRet != CM_OK)
    {
        return CM_OK;
    }

    data = CM_MALLOC(sizeof(cm_cnm_user_info_t));
    if(data == NULL)
    {
        CM_LOG_ERR(CM_MOD_CNM,"malloc fail");
        return CM_FAIL;
    }
    
    iRet = cm_cnm_exec_get_col(cm_cnm_user_get_each,data,"%s get %s",cm_cnm_user_sh,info->name);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]",iRet);
        CM_FREE(data);
        return CM_OK;
    }

    *ppAckData = data;
    *pAckLen = sizeof(cm_cnm_user_info_t);
    return CM_OK;
}


void* cm_cnm_user_update_pwd(void* arg)
{
    cm_cnm_user_info_t* info = (cm_cnm_user_info_t *)arg;

    cm_system("%s insert_pwd %s %s",cm_cnm_user_sh,info->name,info->pwd);
    
    CM_FREE(info);
    return NULL;
}

sint32 cm_cnm_user_sync_request(uint64 data_id, void *pdata, uint32 len)
{
    uint64 cut = 0;
    const cm_cnm_user_infos_t *info = pdata;
    cm_cnm_user_info_t* info_user = NULL;
    sint8 cmd[CM_STRING_128] = {0};
    sint32 iRet = CM_OK;
    cut = cm_exec_int("cat /etc/passwd | grep ':%llu:[0-9]' | wc -l",data_id);
    
    
    if(cut == 0)
    {   
        if(info->flag == CM_TRUE)
        {
            info_user = CM_MALLOC(sizeof(cm_cnm_user_info_t));
            if(info_user == NULL)
            {
                CM_LOG_ERR(CM_MOD_CNM,"malloc fail");
                return CM_FAIL;
            }
            CM_MEM_CPY(info_user,sizeof(cm_cnm_user_info_t),&info->user,sizeof(cm_cnm_user_info_t));
  
            if(0 != strlen(info_user->path))
            {
                iRet =cm_system("%s insert %s %s %llu %u",cm_cnm_user_sh,info_user->name,info_user->path,data_id,info_user->gid);
            }else
            {
                iRet =cm_system("%s insert %s null %llu %u",cm_cnm_user_sh,info_user->name,data_id,info_user->gid);
            }
            if(iRet != CM_OK)
            {
                CM_FREE(info_user);
                return iRet;
            }
            iRet = cm_thread_start(cm_cnm_user_update_pwd,(void*)info_user);
            if(iRet != CM_OK)
            {
                CM_FREE(info_user);
                return iRet;
            }

            return iRet;
        }
        else
        {
            return cm_system("%s request_false %s %s %s",cm_cnm_user_sh,info->passwd,info->shadow,info->smbpasswd);
        }
    }
    
    if(info->flag == CM_TRUE)
    {
        if(0 != strlen(info->user.pwd))
        {
            info_user = CM_MALLOC(sizeof(cm_cnm_user_info_t));
            if(info_user == NULL)
            {
                CM_LOG_ERR(CM_MOD_CNM,"malloc fail");
                return CM_FAIL;
            }
            CM_MEM_CPY(info_user,sizeof(cm_cnm_user_info_t),&info->user,sizeof(cm_cnm_user_info_t));
            if(cm_thread_start(cm_cnm_user_update_pwd,(void*)info_user) != CM_OK)
            {
                CM_FREE(info_user);
                return CM_FAIL;
            }
        }
        if(0 != strlen(info->user.path))
        {
            iRet = cm_system("%s update %s %u %s %u",cm_cnm_user_sh,info->user.name,info->user.gid,info->user.path,info->user.id);
        }else
        {
            iRet = cm_system("%s update %s %u null %u",cm_cnm_user_sh,info->user.name,info->user.gid,info->user.id);
        }
        return iRet;
    }
    else
    {      
        return cm_system("%s request_true %s %s %s",cm_cnm_user_sh,info->user.name,info->shadow,info->smbpasswd);
    }
    return CM_OK;
}


sint32 cm_cnm_user_sync_get(uint64 data_id, void **pdata, uint32 *plen)
{
    cm_cnm_user_infos_t *info = CM_MALLOC(sizeof(cm_cnm_user_infos_t));
    sint8 name[CM_NAME_LEN] = {0};
    sint8 iRet = CM_OK;
    uint32 cut = 0;
    
    if(info == NULL)
    {
        CM_LOG_ERR(CM_MOD_CNM,"malloc fail");
        return CM_FAIL;
    }
    
    iRet = cm_system("%s isexist %llu 0",cm_cnm_user_sh,data_id);
    if(iRet != CM_OK)
    {
        return CM_OK;
    }    
    cm_exec_tmout(name,CM_NAME_LEN,CM_CMT_REQ_TMOUT,
        "cat /etc/passwd | awk -F':' '$3==%llu {printf $1}'",data_id);

    iRet = cm_exec_tmout(info->passwd,CM_STRING_256,CM_CMT_REQ_TMOUT,
        "cat /etc/passwd | grep ':%llu:[0-9]'",data_id);
    cut = strlen(info->passwd);
    
    if(info->passwd[cut-1] == '\n')
    {
        info->passwd[cut-1] = '\0';
    }
    
    (void)cm_exec_tmout(name,CM_NAME_LEN,CM_CMT_REQ_TMOUT,
        "cat /etc/passwd | awk -F':' '$3==%llu{printf $1}'",data_id);
    (void)cm_exec_tmout(info->shadow,CM_STRING_256,CM_CMT_REQ_TMOUT,
            "cat /etc/shadow | grep '^%s:'",name);
    cut = strlen(info->shadow);
    if(info->shadow[cut-1] == '\n')
    {
        info->shadow[cut-1] = '\0';
    }
    
    (void)cm_exec_tmout(info->smbpasswd,CM_STRING_256,CM_CMT_REQ_TMOUT,
        "cat /var/smb/smbpasswd | grep '^%s'",name);
    cut = strlen(info->smbpasswd);
    if(info->smbpasswd[cut-1] == '\n')
    {
        info->smbpasswd[cut-1] = '\0';
    }
    
    info->flag = CM_FALSE;
    *pdata = info;
    *plen = sizeof(cm_cnm_user_infos_t);
    return CM_OK;
}

sint32 cm_cnm_user_sync_delete(uint64 id)
{   
    sint32 iRet = CM_OK;

    iRet = cm_system("%s isexist %llu 0",cm_cnm_user_sh,id);
    if(iRet != CM_OK)
    {
        return CM_OK;
    }
    return cm_system("%s delete %llu",cm_cnm_user_sh,id);
}

static void cm_cnm_user_oplog_report(
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
        const cm_cnm_user_info_t *info = (const cm_cnm_user_info_t*)req->data;
        
        cm_cnm_oplog_param_t params[4] = {
            {CM_OMI_DATA_INT,CM_OMI_FIELD_USER_ID,sizeof(info->id),&info->id},
            {CM_OMI_DATA_INT,CM_OMI_FIELD_USER_GID,sizeof(info->gid),&info->gid},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_USER_NAME,strlen(info->name),info->name},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_USER_PATH,strlen(info->path),info->path},
        };
        
        cm_cnm_oplog_report(sessionid,alarmid,params,cnt,&req->set);
    }    
    
    return;
}    

void cm_cnm_user_oplog_update(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK)? CM_ALARM_LOG_USER_UPDATE_OK : CM_ALARM_LOG_USER_UPDATE_FAIL;
    
    cm_cnm_user_oplog_report(sessionid,pDecodeParam,alarmid);
    return;
}
void cm_cnm_user_oplog_create(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK)? CM_ALARM_LOG_USER_CREATR_OK : CM_ALARM_LOG_USER_CREATR_FAIL;
    
    cm_cnm_user_oplog_report(sessionid,pDecodeParam,alarmid);
    return;
}

void cm_cnm_user_oplog_delete(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK)? CM_ALARM_LOG_USER_DELETE_OK : CM_ALARM_LOG_USER_DELETE_FAIL;
    const cm_cnm_decode_info_t *req = pDecodeParam;
    const uint32 cnt = 1;
    
    if(NULL == pDecodeParam)
    {   
        cm_cnm_oplog_report(sessionid,alarmid,NULL,cnt,NULL);
    }    
    else    
    {       
        const cm_cnm_user_info_t *info = (const cm_cnm_user_info_t*)req->data;
         
        cm_cnm_oplog_param_t params[1] = {
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_USER_NAME,strlen(info->name),info->name},
        };
        cm_cnm_oplog_report(sessionid,alarmid,params,cnt,&req->set);
    }  
    return; 
}    


/******************************************************************************
 * 
 * 
 *                       group部分
 * 
 *
 *****************************************************************************/

sint32 cm_cnm_group_init(void)
{
     return CM_OK;
}

static sint32 cm_cnm_group_decode_ext(const cm_omi_obj_t ObjParam,void* data,cm_omi_field_flag_t* set)
{
    sint32 iRet = CM_OK;
    cm_cnm_group_info_t *info = data;
    cm_cnm_decode_param_t param_str[] = 
    {
        {CM_OMI_FIELD_GROUP_NAME,sizeof(info->name),info->name,NULL},
    };
    
    cm_cnm_decode_param_t param_num[] =
    {
        {CM_OMI_FIELD_GROUP_ID,sizeof(info->id),&info->id,NULL},
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

sint32 cm_cnm_group_decode(const cm_omi_obj_t  ObjParam,void** ppDecodeParam)
{
    return cm_cnm_decode_comm(ObjParam,sizeof(cm_cnm_group_info_t),cm_cnm_group_decode_ext,ppDecodeParam);
}

static void cm_cnm_group_encode_each(cm_omi_obj_t item,void* eachdata,void* arg)
{
    cm_omi_field_flag_t *field = arg;
    cm_cnm_group_info_t *info = eachdata;
    cm_cnm_map_value_str_t cols_str[] = 
    {
        {CM_OMI_FIELD_GROUP_NAME,info->name},
    };
    cm_cnm_map_value_num_t cols_num[] = 
    {
        {CM_OMI_FIELD_GROUP_ID,info->id},
    };

    cm_cnm_encode_str(item,field,cols_str,sizeof(cols_str)/sizeof(cm_cnm_map_value_str_t));
    cm_cnm_encode_num(item,field,cols_num,sizeof(cols_num)/sizeof(cm_cnm_map_value_str_t));
    return;
}

cm_omi_obj_t cm_cnm_group_encode(const void *pDecodeParam,void *pAckData,uint32 AckLen)
{
    return cm_cnm_encode_comm_ext(pDecodeParam,pAckData,AckLen,
        sizeof(cm_cnm_group_info_t),cm_cnm_group_encode_each);
}

sint32 cm_cnm_group_get(const void *pDecodeParam,void **ppAckData,uint32 *pAckLen)
{
    const cm_cnm_decode_info_t *decode = pDecodeParam;
    const cm_cnm_group_info_t *info = NULL;
    cm_cnm_group_info_t *data = NULL;
    uint32 cut = 0;
    if((decode == NULL) || 
        (!CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_GROUP_NAME)))
    {
        return CM_PARAM_ERR;
    }

    info = (const cm_cnm_group_info_t *)decode->data;    

    cut = (uint32)cm_exec_int("cat /etc/group| grep -w '%s:' |wc -l",info->name);
    if(cut == 0)
    {
        return CM_OK;
    }
    data = CM_MALLOC(sizeof(cm_cnm_group_info_t));
    if(data == NULL)
    {
        CM_LOG_ERR(CM_MOD_CNM,"malloc fail");
        return CM_FAIL;
    }
    
    CM_MEM_CPY(data->name,CM_NAME_LEN,info->name,CM_NAME_LEN);
    data->id = (uint32)cm_exec_int("cat /etc/group | awk -F':' '$1==\"%s\" {printf $3}'",info->name);

    *ppAckData = data;
    *pAckLen = sizeof(cm_cnm_group_info_t);
    return CM_OK;
}

static sint32 cm_cnm_group_get_each(void *arg, sint8 **cols, uint32 col_num)
{
    cm_cnm_group_info_t *info = arg;
    if(col_num != 2)
    {
        CM_LOG_ERR(CM_MOD_CNM,"col_num %u",col_num);
        return CM_FAIL;
    }

    info->id = (uint32)atoi(cols[0]);
    CM_VSPRINTF(info->name,sizeof(info->name),"%s",cols[1]);
    return CM_OK;
}

sint32 cm_cnm_group_getbatch(const void *pDecodeParam,void **ppAckData,uint32 *pAckLen)
{
    const cm_cnm_decode_info_t *decode = pDecodeParam;
    uint32 offset = 0;
    uint32 total = CM_CNM_MAX_RECORD;
    const sint8 *cmd= "cat /etc/group | awk -F':' '$3==1||($3>99&&$3<50000) {print $3\" \"$1}'";
    sint32 iRet = CM_OK;
    if(decode != NULL)
    {
        offset = decode->offset;
        total = decode->total;
    }

    iRet = cm_cnm_exec_get_list(cmd,cm_cnm_group_get_each,
        offset,sizeof(cm_cnm_group_info_t),ppAckData,&total);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]",iRet);
        return iRet;
    }
    *pAckLen = total * sizeof(cm_cnm_group_info_t);
    return CM_OK;
}

sint32 cm_cnm_group_count(const void *pDecodeParam,void **ppAckData,uint32 *pAckLen)
{
    uint64 cut = 0;

    cut = (uint64)cm_exec_int("cat /etc/group |awk -F':' '$3==1||($3>99&&$3<50000)' |wc -l");

    return cm_cnm_ack_uint64(cut,ppAckData,pAckLen);
}

sint32 cm_cnm_group_create(const void *pDecodeParam,void **ppAckData,uint32 *pAckLen)
{
    const cm_cnm_decode_info_t *decode = pDecodeParam;
    const cm_cnm_group_info_t *info = NULL;
    uint32 cut = 0;
    uint32 maxid = 0;
    if(decode == NULL)
    {
        return CM_PARAM_ERR;
    }
    
    info = (const cm_cnm_user_info_t *)decode->data;
    if(!CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_GROUP_NAME))
    {
        CM_LOG_ERR(CM_MOD_CNM,"name null");
        return CM_PARAM_ERR;
    }

    cut = (uint32)cm_exec_int("cat /etc/group | grep -w '%s' | wc -l",info->name); 
    if(cut!= 0)
    {
        CM_LOG_ERR(CM_MOD_CNM,"name[%s] exits",info->name);
        return CM_ERR_ALREADY_EXISTS;
    }

    if(CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_GROUP_ID))
    {
        maxid = info->id;
    }else
    {
        maxid = (uint32)cm_exec_int("cat /etc/group | awk -F':' 'BEGIN {max=99}{if(($3>99&&$3<50000)&&($3>max)) max=$3} END{printf max}'");
        maxid++;
    }
    cut = (uint32)cm_exec_int("cat /etc/group | grep ':%u:' | wc -l",maxid); 
    if(cut!= 0)
    {
        CM_LOG_ERR(CM_MOD_CNM,"id[%u] exits",maxid);
        return CM_ERR_ALREADY_EXISTS;
    }
    
    return cm_sync_request(CM_SYNC_OBJ_GROUP,maxid,(void*)info,sizeof(cm_cnm_group_info_t));
}

sint32 cm_cnm_group_delete(const void *pDecodeParam,void **ppAckData,uint32 *pAckLen)
{
    const cm_cnm_decode_info_t *decode = pDecodeParam;
    const cm_cnm_group_info_t *info = NULL;
    uint32 cut = 0;
    uint64 id = 0;

    if(decode==NULL)
    {
        return CM_PARAM_ERR;
    }

    info = (const cm_cnm_user_info_t*) decode->data;
    if(!CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_GROUP_NAME))
    {
        CM_LOG_ERR(CM_MOD_CNM,"name null");
        return CM_PARAM_ERR;
    }

    cut = (uint32)cm_exec_int("cat /etc/group | grep -w '%s' | wc -l",info->name);
    if(cut == 0)
    {
        CM_LOG_ERR(CM_MOD_CNM,"name[%s] no exits",info->name);
        return CM_ERR_NOT_EXISTS;
    }
    id = (uint64)cm_exec_int("cat /etc/group | grep '^%s:' | awk -F':' '{printf $3}'",info->name);
    if(id<100||id>1000)
    {
        return CM_PARAM_ERR;
    }
    
    return cm_sync_delete(CM_SYNC_OBJ_GROUP,id);
}

sint32 cm_cnm_group_sync_request(uint64 data_id,void *pdata,uint32 len)
{
    const cm_cnm_group_info_t *info = pdata;
    uint32 cut = 0;
    if(info == NULL)
    {
        return CM_PARAM_ERR;
    }
    cut = (uint32)cm_exec_int("cat /etc/group | grep -w '%s' | wc -l",info->name); 
    if(cut!= 0)
    {
        CM_LOG_ERR(CM_MOD_CNM,"name[%s] exist",info->name);
        return CM_OK;
    }
    
    cm_system("groupadd -g %llu %s",data_id,info->name);
     
    return CM_OK;
}

sint32 cm_cnm_group_sync_get(uint64 data_id,void **pdata,uint32 *plen)
{
    cm_cnm_group_info_t *info = CM_MALLOC(sizeof(cm_cnm_group_info_t));
    sint8 name[CM_NAME_LEN] = {0};
    sint32 iRet = CM_OK;

    if(info == NULL)
    {
        CM_LOG_ERR(CM_MOD_CNM,"malloc fail");
        return CM_FAIL;
    }
 
    iRet = cm_system("%s isexist %llu 1",cm_cnm_user_sh,data_id);
    if(iRet != CM_OK)
    {
        *pdata = NULL;
        *plen = 0;
        return CM_OK;
    }
    
    iRet = cm_exec_tmout(info->name,sizeof(info->name),CM_CMT_REQ_TMOUT,
        "cat /etc/group | awk -F':' '$3==%llu {printf $1}'",data_id);
    if(iRet != CM_OK)
    {
        CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]\n%s",iRet,info->name);
        return CM_FAIL;      
    }
    
    info->id = data_id;
    *pdata = info;
    *plen = sizeof(cm_cnm_group_info_t);
    return CM_OK;
}

sint32 cm_cnm_group_sync_delete(uint64 id)
{
    sint8 name[CM_NAME_LEN] = {0};
    sint32 iRet = CM_OK;

    iRet = cm_system("%s isexist %llu 1",cm_cnm_user_sh,id);
    if(iRet != CM_OK)
    {
        return CM_OK;
    }

    cm_exec_tmout(name,sizeof(name),CM_CMT_REQ_TMOUT,
        "cat /etc/group | awk -F':' '$3==%llu {printf $1}'",id);
    cm_system("groupdel %s",name);
    return CM_OK;
}


/********************************************************
                  explorer(资源管理器)
*********************************************************/

static sint8* cm_cnm_explorer_find_dir = "/tmp/explorer/";

sint32 cm_cnm_explorer_init(void)
{
    cm_system("%s explorer_init",cm_cnm_user_sh);
    return CM_OK;
}

static sint32 cm_cnm_explorer_decode_ext(const cm_omi_obj_t ObjParam, void* data, cm_omi_field_flag_t *set)
{
    sint32 iRet = CM_OK;
    cm_cnm_explorer_info_t* info = data;
  
    cm_cnm_decode_param_t param_str[] =
    {
        {CM_OMI_FIELD_EXPLORER_DIR, sizeof(info->dir), info->dir, NULL},
        {CM_OMI_FIELD_EXPLORER_FIND, sizeof(info->find), info->find, NULL},
        {CM_OMI_FIELD_EXPLORER_FLAG, sizeof(info->flag), info->flag, NULL},
    };
 
    cm_cnm_decode_param_t param_num[] =
    {
        {CM_OMI_FIELD_EXPLORER_NID, sizeof(info->nid), &info->nid, NULL},
    };
    iRet = cm_cnm_decode_str(ObjParam, param_str,
        sizeof(param_str) / sizeof(cm_cnm_decode_param_t), set);
    if(CM_OK != iRet)
    {
        return iRet;
    }
 
    iRet = cm_cnm_decode_num(ObjParam, param_num,
        sizeof(param_num) / sizeof(cm_cnm_decode_param_t), set);
 
    return CM_OK;
}


sint32 cm_cnm_explorer_decode(const cm_omi_obj_t ObjParam,void **ppDecodeParam)
{
    return cm_cnm_decode_comm(ObjParam, sizeof(cm_cnm_explorer_info_t), cm_cnm_explorer_decode_ext, ppDecodeParam);
}

static void cm_cnm_explorer_encode_each(cm_omi_obj_t item, void* eachdata, void* arg)
{
    cm_omi_field_flag_t *field = arg;
    cm_cnm_explorer_info_t *info = eachdata;
    cm_cnm_map_value_str_t cols_str[] =
    {
        {CM_OMI_FIELD_EXPLORER_NAME, info->name},
        {CM_OMI_FIELD_EXPLORER_USER, info->user},
        {CM_OMI_FIELD_EXPLORER_GROUP, info->group},
    };

    cm_cnm_map_value_num_t cols_num[] = 
    {
        {CM_OMI_FIELD_EXPLORER_PERMISSION, info->permission},
        {CM_OMI_FIELD_EXPLORER_TYPE, info->type},
        //{CM_OMI_FIELD_EXPLORER_SIZE, info->size},
        {CM_OMI_FIELD_EXPLORER_ATIME, info->atime},
        {CM_OMI_FIELD_EXPLORER_MTIME, info->mtime},
        {CM_OMI_FIELD_EXPLORER_CTIME, info->ctime},
    };
    
 
    cm_cnm_encode_str(item,field,cols_str,sizeof(cols_str)/sizeof(cm_cnm_map_value_str_t));
    cm_cnm_encode_num(item,field,cols_num,sizeof(cols_num)/sizeof(cm_cnm_map_value_str_t));
    if(CM_OMI_FIELDS_FLAG_ISSET(field,CM_OMI_FIELD_EXPLORER_SIZE))
    {
        (void)cm_omi_obj_key_set_u64_ex(item,CM_OMI_FIELD_EXPLORER_SIZE,info->size);
    }
    return;
}
 
cm_omi_obj_t cm_cnm_explorer_encode(const void *pDecodeParam, void *pAckData, uint32 AckLen)
{
    return cm_cnm_encode_comm_ext(pDecodeParam, pAckData, AckLen,
        sizeof(cm_cnm_explorer_info_t), cm_cnm_explorer_encode_each);
}

sint32 cm_cnm_explorer_getbatch(const void *pDecodeParam, void **ppAckData, uint32 *pAckLen)
{
    return cm_cnm_request_comm(CM_OMI_OBJECT_EXPLORER,CM_OMI_CMD_GET_BATCH,sizeof(cm_cnm_explorer_info_t),
        pDecodeParam, ppAckData, pAckLen);
}

sint32 cm_cnm_explorer_count(const void *pDecodeParam, void **ppAckData, uint32 *pAckLen)
{
    return cm_cnm_request_comm(CM_OMI_OBJECT_EXPLORER,CM_OMI_CMD_COUNT,sizeof(cm_cnm_explorer_info_t),
        pDecodeParam, ppAckData, pAckLen);
}

sint32 cm_cnm_explorer_create(const void *pDecodeParam, void **ppAckData, uint32 *pAckLen)
{
    return cm_cnm_request_comm(CM_OMI_OBJECT_EXPLORER,CM_OMI_CMD_ADD,sizeof(cm_cnm_explorer_info_t),
        pDecodeParam, ppAckData, pAckLen);
}


static uint32 cm_cnm_explorer_get_permission(uint32 mode)
{
    uint32 cut = 0;
    
    if(S_IRUSR&mode)
    {
        cut += 400; 
    }
    if(S_IWUSR&mode)
    {
        cut += 200; 
    }
    if(S_IXUSR&mode)
    {
        cut += 100; 
    }
    if(S_IRGRP&mode)
    {
        cut += 40; 
    }
    if(S_IWGRP&mode)
    {
        cut += 20; 
    }
    if(S_IXGRP&mode)
    {
        cut += 10; 
    }
    if(S_IROTH&mode)
    {
        cut += 4; 
    }
    if(S_IWOTH&mode)
    {
        cut += 2; 
    }
    if(S_IXOTH&mode)
    {
        cut += 1; 
    }

    return cut;
}

static uint32 cm_cnm_explorer_get_type(uint32 mode)
{
    if(S_ISREG(mode))/*判断是否是文件*/
    {
        return 0;
    }else if(S_ISDIR(mode))/*判断是否是目录*/
    {
        return 1;
    }else if(S_ISCHR(mode))
    {
        return 2;
    }else if(S_ISBLK(mode))
    {
        return 3;
    }else if(S_ISFIFO(mode))
    {
        return 4;
    }else if(S_ISLNK(mode))
    {
        return 5;
    }else
    {
        return 6;
    }

    return CM_OK;
}

static void cm_cnm_explorer_get_name(sint8* buf, sint32 len, const sint8* name)
{
    sint32 lenth=len-5; /* 预留5个字节 */
    if (strlen(name) <= lenth)
    {
        CM_VSPRINTF(buf,len,name);
        return;
    }
    while((*name != '\0') && (lenth > 0))
    {
        /* 判断是否为汉字 汉字为8bit组成，最高位为1*/
        if ((*name & 0x80) && (*(name+1) & 0x80))
        {
            *(buf++) = *(name++);
            *(buf++) = *(name++);
            lenth -=2;
        }
        else
        {
            *(buf++) = *(name++);
            lenth--;
        }
    }
    return;    
}

static sint32 cm_cnm_explorer_get_each(void* ptemp,const sint8* dir,const sint8* name)
{
    struct stat buf;
    cm_cnm_explorer_info_t *info = (cm_cnm_explorer_info_t*)ptemp;

    stat(dir,&buf);

    if(NULL != name)
    {
        cm_cnm_explorer_get_name(info->name,sizeof(info->name),name);
    }
    
    info->permission = cm_cnm_explorer_get_permission(buf.st_mode);
    info->type = cm_cnm_explorer_get_type(buf.st_mode);
    info->size = buf.st_size;
    (void)cm_exec_tmout(info->user,sizeof(info->user),CM_CMT_REQ_TMOUT,
        "getent passwd %u|awk -F':' '{printf $1}'",buf.st_uid);
    if(0 == strlen(info->user))
    {
        CM_VSPRINTF(info->user,sizeof(info->user),"%u",buf.st_uid);
    }
    (void)cm_exec_tmout(info->group,sizeof(info->group),CM_CMT_REQ_TMOUT,
        "getent group %u|awk -F':' '{printf $1}'",buf.st_gid);
    if(0 == strlen(info->group))
    {
        CM_VSPRINTF(info->group,sizeof(info->group),"%u",buf.st_gid);
    }
    info->atime = buf.st_atime;
    info->mtime = buf.st_mtime;
    info->ctime = buf.st_ctime;

    return CM_OK;
}

static sint32 cm_cnm_explorer_get_list(
    const sint8* dir,uint64 offset,
    uint32 each_size,void **ppAck,uint32 *total)
{
    sint8* ptemp = NULL;
    DIR* pdir = NULL;
    struct dirent *entry;
    sint8 dir_file[CM_STRING_512] = {0};
    uint32 cut = 0;

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
        if(entry->d_name[0] == '.')
        {
            continue;
        }
            
        if(cut < offset)
        {
            cut++;
            continue;
        }
        
        CM_VSPRINTF(dir_file,sizeof(dir_file),"%s/%s",dir,entry->d_name);
        cm_cnm_explorer_get_each((void*)ptemp,dir_file,entry->d_name);
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
        CM_FREE(ptemp);
    }
    
    if(cut - offset < *total)
    {
        *total = cut - offset;
    }
    
    return CM_OK;
}

static void cm_cnm_explorer_get_tmpname(sint8 *buff, sint32 len, 
    const sint8* dir,const sint8* find)
{
    if(*dir == '/')
    {
        dir++;
    }
    while((len > 0) && (*dir != '\0'))
    {
        if((*dir == '/') || (*dir == ' ') || (*dir == '|'))
        {
            *buff = '_';
        }
        else
        {
            *buff = *dir;
        }
        buff++;
        dir++; 
        len--;
    }
    if (*(buff-1) != '_')
    {
        *(buff++) = '_';
        len--;
    }
    while((len > 0) && (*find != '\0'))
    {
        if((*find == '/') || (*find == ' ') || (*find == '|'))
        {
            *buff = '_';
        }
        else
        {
            *buff = *find;
        }
        buff++;
        find++; 
        len--;
    }
    return;
}

#if 0
static sint8* cm_cnm_explorer_get_tmpname(const sint8* dir,const sint8* find)
{
    uint32 i = 0;
    uint32 len = 0;
    sint8* tmpname = NULL;
    sint8* p = NULL;

    tmpname = CM_MALLOC(CM_STRING_256);
    CM_MEM_ZERO(tmpname,CM_STRING_256);

    p = (sint8* )dir;
    p++;
    len = strlen(dir);
    while(*p != '\0')
    {
        if(i >= CM_STRING_256-strlen(find)-1)
        {    
            break;
        }

        if(i == len-2 && *p == '/')
        {
            break;
        }

        if(*p == '/')
        {
            tmpname[i] = '_';
        }else
        {
            tmpname[i] = *p;
        }

        p++;
        i++;
    }
    tmpname[i] = '\0'; 
    CM_SNPRINTF_ADD(tmpname,CM_STRING_256,"_%s",find);

    return tmpname;
}
#endif

static sint32 cm_cnm_explorer_local_get_each(void *arg, sint8 **cols, uint32 col_num)
{
    cm_cnm_explorer_info_t *info=arg;
    if(col_num > 0)
    {
        CM_VSPRINTF(info->name,sizeof(info->name),*cols);
        return CM_OK;
    }
    return CM_FAIL;
}
sint32 cm_cnm_explorer_local_getbatch(
    void * param,uint32 len,
    uint64 offset,uint32 total,
    void **ppAck,uint32 *pAckLen)
{
    cm_cnm_decode_info_t *decode = param;
    cm_cnm_explorer_info_t *info = decode->data;
    sint32 iRet = CM_OK;

    if(CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_EXPLORER_FIND))
    {
        int size = sizeof(cm_cnm_explorer_info_t);
        uint32 cut = 0;
        sint8 tmpname[CM_STRING_256] = {0};
        sint8 dir_file[CM_STRING_256] = {0};
        cm_cnm_explorer_info_t *ptmp =NULL;

        cm_cnm_explorer_get_tmpname(tmpname,CM_STRING_256,info->dir,info->find);        
        iRet = cm_exec_get_list(cm_cnm_explorer_local_get_each,
            (uint32)offset,size,ppAck,&total,
            "cat %s%s",cm_cnm_explorer_find_dir,tmpname);
        if(iRet != CM_OK)
        {
            CM_LOG_WARNING(CM_MOD_NONE,"[%d]",iRet);
            return CM_OK;
        }
        for(cut = total,ptmp= *ppAck;
            cut>0;
            ptmp++,cut--)
        {
            CM_VSPRINTF(dir_file,sizeof(dir_file),"%s/%s",info->dir,ptmp->name);
            cm_cnm_explorer_get_each(ptmp,dir_file,NULL);
        }
        *pAckLen = total * size;
        return CM_OK;
    }

    iRet = cm_cnm_explorer_get_list(info->dir,offset,sizeof(cm_cnm_explorer_info_t),ppAck,&total);
    if(CM_OK != iRet)
    {
        *pAckLen = 0;
        return iRet;
    }

    *pAckLen = total * sizeof(cm_cnm_explorer_info_t);
    return CM_OK;
}

sint32 cm_cnm_explorer_local_count(
    void * param,uint32 len,
    uint64 offset,uint32 total,
    void **ppAck,uint32 *pAckLen)
{
    cm_cnm_decode_info_t *decode = param;
    cm_cnm_explorer_info_t *info = decode->data;
    uint32 cut = 0;

    if(CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_EXPLORER_FIND)
        &&CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_EXPLORER_FLAG))
    {
        return CM_FAIL;
    }

    if(CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_EXPLORER_FLAG))
    {
        sint8 tmpname[CM_STRING_256] = {0};
        cm_cnm_explorer_get_tmpname(tmpname,sizeof(tmpname),info->dir,info->flag);
        cut = (uint32)cm_exec_int("%s explorer_count_flag %s",cm_cnm_user_sh,tmpname);
        return cm_cnm_ack_uint64(cut, ppAck, pAckLen);
    }

    if(CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_EXPLORER_FIND))
    {
        sint8 tmpname[CM_STRING_256] = {0};
        cm_cnm_explorer_get_tmpname(tmpname,sizeof(tmpname),info->dir,info->find);
        cut = (uint32)cm_exec_int("%s explorer_count %s",cm_cnm_user_sh,tmpname);
        return cm_cnm_ack_uint64(cut, ppAck, pAckLen);
    }
    cut = (uint32)cm_exec_int("ls -1f %s|egrep -v '^\\.'|wc -l",info->dir);

    return cm_cnm_ack_uint64(cut, ppAck, pAckLen);
}

sint32 cm_cnm_explorer_local_create(
    void * param,uint32 len,
    uint64 offset,uint32 total,
    void **ppAck,uint32 *pAckLen)
{
    cm_cnm_decode_info_t *decode = param;
    cm_cnm_explorer_info_t *info = decode->data;
    sint32 iRet = CM_OK;

    sint8 tmpname[CM_STRING_256] = {0};
    cm_cnm_explorer_get_tmpname(tmpname,sizeof(tmpname),info->dir,info->find);
    iRet = cm_system("%s explorer_create %s %s %s",cm_cnm_user_sh,info->dir,tmpname,info->find);
    if(iRet != CM_OK)
    {
        CM_LOG_ERR(CM_MOD_CNM,"create fail");
        return CM_FAIL;
    }
    return CM_OK;
}



/*************domain   user*****************/

const sint8* cm_cnm_domain_user_sh = "/var/cm/script/cm_cnm_domain_user.sh";

sint32 cm_cnm_domain_user_init(void)
{
    return CM_OK;
}

static sint32 cm_cnm_domain_user_decode_ext(const cm_omi_obj_t ObjParam,void* data,cm_omi_field_flag_t* set)
{
    sint32 iRet = CM_OK;
    cm_cnm_domain_user_info_t *info = data;
    cm_cnm_decode_param_t param_str[] = 
    {
        {CM_OMI_FIELD_DOMAIN_USER,sizeof(info->domain_user),info->domain_user,NULL},
        {CM_OMI_FIELD_DOMAIN_LOCAL_USER,sizeof(info->local_user),info->local_user,NULL},
    };

    iRet = cm_cnm_decode_str(ObjParam,param_str,
        sizeof(param_str)/sizeof(cm_cnm_decode_param_t),set);
    if(iRet != CM_OK)
    {
        return iRet;
    }

    return CM_OK;
}

sint32 cm_cnm_domain_user_decode(const cm_omi_obj_t  ObjParam,void** ppDecodeParam)
{
    return cm_cnm_decode_comm(ObjParam,sizeof(cm_cnm_domain_user_info_t),cm_cnm_domain_user_decode_ext,ppDecodeParam);
}

static void cm_cnm_domain_user_encode_each(cm_omi_obj_t item,void* eachdata,void* arg)
{
    cm_omi_field_flag_t *field = arg;
    cm_cnm_domain_user_info_t *info = eachdata;
    cm_cnm_map_value_str_t cols_str[] = 
    {
        {CM_OMI_FIELD_DOMAIN_USER,info->domain_user},
        {CM_OMI_FIELD_DOMAIN_LOCAL_USER,info->local_user},
    };

    cm_cnm_encode_str(item,field,cols_str,sizeof(cols_str)/sizeof(cm_cnm_map_value_str_t));
    return;
}

cm_omi_obj_t cm_cnm_domain_user_encode(const void *pDecodeParam,void *pAckData,uint32 AckLen)
{
    return cm_cnm_encode_comm_ext(pDecodeParam,pAckData,AckLen,
        sizeof(cm_cnm_domain_user_info_t),cm_cnm_domain_user_encode_each);
}

static sint32 cm_cnm_domain_user_get_each(void *arg, sint8 **cols, uint32 col_num)
{
    cm_cnm_domain_user_info_t *info = arg;
    if(col_num != 2)
    {
        CM_LOG_ERR(CM_MOD_CNM,"col_num %u",col_num);
        return CM_FAIL;
    }

    CM_VSPRINTF(info->domain_user,sizeof(info->domain_user),"%s",cols[0]);
    CM_VSPRINTF(info->local_user,sizeof(info->local_user),"%s",cols[1]);
    return CM_OK;
}

sint32 cm_cnm_domain_user_getbatch(const void *pDecodeParam,void **ppAckData,uint32 *pAckLen)
{
    const cm_cnm_decode_info_t *decode = pDecodeParam;
    uint32 offset = 0;
    uint32 total = CM_CNM_MAX_RECORD;
    sint8 cmd[CM_STRING_128]= {0};
    sint32 iRet = CM_OK;
    if(decode != NULL)
    {
        offset = decode->offset;
        total = decode->total;
    }
    
    CM_VSPRINTF(cmd,sizeof(cmd),"%s getbatch",cm_cnm_domain_user_sh);
    iRet = cm_cnm_exec_get_list(cmd,cm_cnm_domain_user_get_each,
        offset,sizeof(cm_cnm_domain_user_info_t),ppAckData,&total);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]",iRet);
        return iRet;
    }
    *pAckLen = total * sizeof(cm_cnm_domain_user_info_t);
    return CM_OK;
}

sint32 cm_cnm_domain_user_count(const void *pDecodeParam,void **ppAckData,uint32 *pAckLen)
{
    uint64 cut = 0;

    cut = (uint64)cm_exec_int("%s count",cm_cnm_domain_user_sh);

    return cm_cnm_ack_uint64(cut,ppAckData,pAckLen);
}



sint32 cm_cnm_domain_user_create(const void* pDecodeParam,void** ppAckData,uint32* pAckLen)
{
    cm_cnm_decode_info_t *decode = pDecodeParam;
    cm_cnm_domain_user_info_t *info = decode->data;
    uint32 id = 0;

    if(!CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_DOMAIN_LOCAL_USER)||
        !CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_DOMAIN_USER))
    {
        CM_LOG_ERR(CM_MOD_CNM,"data null");
        return CM_PARAM_ERR;
    }

    id = cm_exec_int("%s count",cm_cnm_domain_user_sh)+1;

    return cm_sync_request(CM_SYNC_OBJ_DOMAIN_USER,id,(void*)info,sizeof(cm_cnm_domain_user_info_t));
}

sint32 cm_cnm_domain_user_delete(const void* pDecodeParam,void** ppAckData,uint32* pAckLen)
{
    cm_cnm_decode_info_t *decode = pDecodeParam;
    cm_cnm_domain_user_info_t *info = decode->data;
    uint32 id = 0;

    if(!CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_DOMAIN_LOCAL_USER)||
        !CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_DOMAIN_USER))
    {
        CM_LOG_ERR(CM_MOD_CNM,"data null");
        return CM_PARAM_ERR;
    }

    id = cm_exec_int("%s get_id %s %s",cm_cnm_domain_user_sh,info->domain_user,info->local_user);
    if(id == 0)
    {
        return CM_PARAM_ERR;
    }

    return cm_sync_delete(CM_SYNC_OBJ_DOMAIN_USER,id);
}

sint32 cm_cnm_domain_user_sync_request(uint64 data_id,void *pdata,uint32 len)
{
    const cm_cnm_domain_user_info_t *info = pdata;
    if(info == NULL)
    {
        return CM_PARAM_ERR;
    }
     
    return cm_system("%s insert %s %s",cm_cnm_domain_user_sh,info->domain_user,info->local_user);
}

sint32 cm_cnm_domain_user_sync_delete(uint64 id)
{
    return cm_system("%s delete %llu",cm_cnm_domain_user_sh,id);
}

static void cm_cnm_domain_user_oplog_report(
    const sint8* sessionid, const void *pDecodeParam, uint32 alarmid)
{
    const cm_cnm_decode_info_t *req = pDecodeParam;
    const uint32 cnt = 2;

    if(NULL == pDecodeParam)
    {   
        cm_cnm_oplog_report(sessionid,alarmid,NULL,cnt,NULL);
    }    
    else    
    {       
        const cm_cnm_domain_user_info_t *info = (const cm_cnm_domain_user_info_t*)req->data;
        
        cm_cnm_oplog_param_t params[2] = {
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_DOMAIN_USER,strlen(info->domain_user),info->domain_user},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_DOMAIN_LOCAL_USER,strlen(info->local_user),info->local_user},
        };
        
        cm_cnm_oplog_report(sessionid,alarmid,params,cnt,&req->set);
    }    
    
    return;
}    

void cm_cnm_domain_user_oplog_delete(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK)? CM_DOMIAN_USER_DELETE_OK : CM_DOMIAN_USER_DELETE_FAIL;
    
    cm_cnm_domain_user_oplog_report(sessionid,pDecodeParam,alarmid);
    return;
}
void cm_cnm_domain_user_oplog_create(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK)? CM_DOMIAN_USER_CREATE_OK : CM_DOMIAN_USER_CREATE_FAIL;
    
    cm_cnm_domain_user_oplog_report(sessionid,pDecodeParam,alarmid);
    return;
}


