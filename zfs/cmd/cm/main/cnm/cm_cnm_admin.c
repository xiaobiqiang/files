/******************************************************************************
 *          Copyright (c) 2018 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cnm_admin.c
 * author     : wbn
 * create date: 2018.07.07
 * description: TODO:
 *
 *****************************************************************************/
#include "cm_cnm_admin.h"
#include "cm_log.h"
#include "cm_sync.h"
#include "cm_node.h"
#include "cm_omi.h"
#include "cm_db.h"
#include "cm_xml.h"

#define CM_CNM_ADMIN_DB_FILE CM_DATA_DIR"cm_admin.db"
static cm_db_handle_t g_cm_admin_handle = NULL;

static sint32 cm_cnm_permission_check_cmd(uint32 obj, uint32 cmd, uint32 level);

sint32 cm_cnm_admin_init(void)
{
    sint32 iRet = CM_OK;
    uint64 cnt = 2;
    sint8 buff[CM_STRING_256] = {0};
    const sint8* table[] = {
        "CREATE TABLE IF NOT EXISTS record_t ("
            "id INT,"
            "level INT,"
            "name VARCHAR(64),"
            "pwd VARCHAR(64)"
            ")",
        "CREATE TABLE IF NOT EXISTS perm_t ("
            "obj INT,"
            "cmd INT,"
            "mask BIGINT"
            ")",    
         };

    iRet = cm_db_open_ext(CM_CNM_ADMIN_DB_FILE,&g_cm_admin_handle);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"open %s fail",CM_CNM_ADMIN_DB_FILE);
        return iRet;
    }
    while(cnt > 0)
    {
        cnt--;
        iRet = cm_db_exec_ext(g_cm_admin_handle,table[cnt]);
        if(CM_OK != iRet)
        {
            CM_LOG_ERR(CM_MOD_CNM,"create table fail[%d]",iRet);
            return iRet;
        }
    }
   
    (void)cm_db_exec_get_count(g_cm_admin_handle,&cnt,
        "SELECT COUNT(id) FROM record_t");
    if(0 == cnt)
    {
        /* 增加默认管理员 */
        (void)cm_get_md5sum("admin",buff,sizeof(buff));
        (void)cm_db_exec_ext(g_cm_admin_handle,"INSERT INTO record_t"
            " (id,level,name,pwd) VALUES (0,0,'admin','%s')",buff);
    }
    cm_omi_level_cbk_reg(cm_cnm_permission_check_cmd);
    cnt = 0;
    (void)cm_db_exec_get_count(g_cm_admin_handle,&cnt,
        "SELECT COUNT(obj) FROM perm_t");
    if(0 == cnt)
    {
        return cm_cnm_permission_init();
    }
    return CM_OK;
}

static sint32 cm_cnm_admin_decode_check_name(void *val)
{
    sint8* str = val;
    const sint8* match = "[a-z0-9A-Z_]{3,16}";
    if((NULL == str) || (0 == strlen(str)))
    {
        CM_LOG_ERR(CM_MOD_CNM,"str null");
        return CM_PARAM_ERR;
    }
    if(CM_OK != cm_regex_check(str,match))
    {
        CM_LOG_ERR(CM_MOD_CNM,"data[%s] match %s fail",str,match);
        return CM_PARAM_ERR;
    }
    return CM_OK;
}
static sint32 cm_cnm_admin_decode_check_level(void *val)
{
    uint32 level = *((uint32*)val);
    if(0 == level)
    {
        CM_LOG_ERR(CM_MOD_CNM,"level 0");
        return CM_PARAM_ERR;
    }
    return CM_OK;
}

static sint32 cm_cnm_admin_decode_ext(
    const cm_omi_obj_t ObjParam,void* data,cm_omi_field_flag_t *set)
{
    sint32 iRet = CM_OK;
    cm_cnm_admin_info_t *info = data;
    cm_cnm_decode_param_t param_str[] = 
    {
        {CM_OMI_FIELD_ADMIN_NAME,sizeof(info->name),info->name,cm_cnm_admin_decode_check_name},
        {CM_OMI_FIELD_ADMIN_PWD,sizeof(info->pwd),info->pwd,NULL},
    };

    cm_cnm_decode_param_t param_num[] = 
    {
        {CM_OMI_FIELD_ADMIN_ID,sizeof(info->id),&info->id,NULL},
        {CM_OMI_FIELD_ADMIN_LEVEL,sizeof(info->level),&info->level,cm_cnm_admin_decode_check_level},
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

sint32 cm_cnm_admin_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam)
{
    return cm_cnm_decode_comm(ObjParam,sizeof(cm_cnm_admin_info_t),
        cm_cnm_admin_decode_ext,ppDecodeParam);
}
    
cm_omi_obj_t cm_cnm_admin_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen)
{
    cm_omi_obj_t items = NULL;
    sint8 *sql = pAckData;

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

    (void)cm_db_exec(g_cm_admin_handle,cm_omi_encode_db_record_each,items,"%s",sql);
    return items;
}

sint32 cm_cnm_admin_getbatch(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    const cm_cnm_decode_info_t *decode = pDecodeParam;
    sint8* sql = CM_MALLOC(CM_STRING_512);

    if(NULL == sql)
    {
        CM_LOG_ERR(CM_MOD_CNM,"malloc fail");
        return CM_FAIL;
    }

    CM_VSPRINTF(sql,CM_STRING_512,"SELECT id as f0,level as f2,name as f3 FROM record_t");
    if(NULL == decode)
    {
        CM_SNPRINTF_ADD(sql,CM_STRING_512," LIMIT %u",CM_CNM_MAX_RECORD);
    }
    else
    {
        CM_SNPRINTF_ADD(sql,CM_STRING_512," LIMIT %llu,%u",decode->offset,decode->total);
    }
    *ppAckData = sql;
    *pAckLen = CM_STRING_512;
    return CM_OK;
}

sint32 cm_cnm_admin_get(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    const cm_cnm_decode_info_t *decode = pDecodeParam;
    sint8* sql = NULL;
    const cm_cnm_admin_info_t *info = NULL;
    
    if((NULL == decode) || 
        (!CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_ADMIN_ID)))
    {
        return CM_PARAM_ERR;
    }

    sql = CM_MALLOC(CM_STRING_512);

    if(NULL == sql)
    {
        CM_LOG_ERR(CM_MOD_CNM,"malloc fail");
        return CM_FAIL;
    }
    
    info = (const cm_cnm_admin_info_t *)decode->data;
    
    CM_VSPRINTF(sql,CM_STRING_512,"SELECT id as f0,level as f2,name as f3 "
        "FROM record_t WHERE id=%u LIMIT 1",info->id);
    *ppAckData = sql;
    *pAckLen = CM_STRING_512;
    return CM_OK;;
}

sint32 cm_cnm_admin_create(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    const cm_cnm_decode_info_t *decode = pDecodeParam;
    const cm_cnm_admin_info_t *info = NULL;
    cm_cnm_admin_info_t infoset;
    uint64 cnt = 0;
    
    if(NULL == decode)
    {
        return CM_PARAM_ERR;
    }    
    
    info = (const cm_cnm_admin_info_t *)decode->data;
    CM_MEM_CPY(&infoset,sizeof(infoset),info,sizeof(cm_cnm_admin_info_t));
    if(!CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_ADMIN_NAME))
    {
        CM_LOG_ERR(CM_MOD_CNM,"name null");
        return CM_PARAM_ERR;
    }
    if(!CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_ADMIN_LEVEL))
    {
        CM_LOG_ERR(CM_MOD_CNM,"level null");
        return CM_PARAM_ERR;
    }

    (void)cm_db_exec_get_count(g_cm_admin_handle,&cnt,
        "SELECT COUNT(id) FROM record_t WHERE name='%s'",info->name);
    if(0 != cnt)
    {
        CM_LOG_ERR(CM_MOD_CNM,"name[%s] exits",info->name);
        return CM_ERR_ALREADY_EXISTS;
    }
    (void)cm_db_exec_get_count(g_cm_admin_handle,&cnt,
        "SELECT MAX(id) FROM record_t");

    if(!CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_ADMIN_PWD))
    {
        /* 设置默认密码 */
        cm_get_md5sum(info->name,infoset.pwd,sizeof(infoset.pwd));
    }
    return cm_sync_request(CM_SYNC_OBJ_ADMIN,(cnt+1),(void*)&infoset,sizeof(cm_cnm_admin_info_t));
}    
    
sint32 cm_cnm_admin_update(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    const cm_cnm_decode_info_t *decode = pDecodeParam;
    const cm_cnm_admin_info_t *info = NULL;
    cm_cnm_admin_info_t *oldinfo=NULL;
    uint32 len = 0;
    sint32 iRet = CM_OK;
    if(NULL == decode)
    {
        return CM_PARAM_ERR;
    }    
    
    info = (const cm_cnm_admin_info_t *)decode->data;
    if(!CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_ADMIN_ID))
    {
        CM_LOG_ERR(CM_MOD_CNM,"id null");
        return CM_PARAM_ERR;
    }

    iRet = cm_cnm_admin_sync_get(info->id,(void**)&oldinfo,&len);
    if((CM_OK != iRet) || (NULL == oldinfo))
    {
        CM_LOG_ERR(CM_MOD_CNM,"id %u",info->id);
        return CM_ERR_NOT_EXISTS;
    }
    len = 0;
    if(CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_ADMIN_LEVEL))
    {
        oldinfo->level = info->level;
        len++;
    }
    if(CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_ADMIN_PWD))
    {
        strcpy(oldinfo->pwd,info->pwd);
        len++;
    }
    if(len > 0)
    {
        iRet = cm_sync_request(CM_SYNC_OBJ_ADMIN,info->id,oldinfo,sizeof(cm_cnm_admin_info_t));
    }
    
    CM_FREE(oldinfo);
    return iRet;
}    
    
sint32 cm_cnm_admin_count(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    uint64 cnt = 0;
    sint8 sql[CM_STRING_512] = {0};
    const cm_cnm_decode_info_t *decode = pDecodeParam;
    const cm_cnm_admin_info_t *info = NULL;

    CM_SNPRINTF_ADD(sql,sizeof(sql),"SELECT COUNT(id) FROM record_t");

    if((NULL != decode) && 
        CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_ADMIN_NAME))
    {
        info = (const cm_cnm_admin_info_t *)decode->data;
        CM_SNPRINTF_ADD(sql,sizeof(sql)," WHERE name='%s'",info->name);
        if(CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_ADMIN_PWD))
        {
            CM_SNPRINTF_ADD(sql,sizeof(sql)," AND name='%s'",info->pwd);
        }
    }
    (void)cm_db_exec_get_count(g_cm_admin_handle,&cnt,sql);
    return cm_cnm_ack_uint64(cnt,ppAckData,pAckLen);
}    
    
sint32 cm_cnm_admin_delete(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    const cm_cnm_decode_info_t *decode = pDecodeParam;
    const cm_cnm_admin_info_t *info = NULL;
    uint64 cnt = 0;
    if(NULL == decode)
    {
        return CM_PARAM_ERR;
    }    
    
    info = (const cm_cnm_admin_info_t *)decode->data;
    if(!CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_ADMIN_ID))
    {
        CM_LOG_ERR(CM_MOD_CNM,"id null");
        return CM_PARAM_ERR;
    }
    if(0 == info->id)
    {
        CM_LOG_ERR(CM_MOD_CNM,"id 0");
        return CM_FAIL;
    }
    (void)cm_db_exec_get_count(g_cm_admin_handle,&cnt,"SELECT COUNT(id) FROM record_t"
        " WHERE id=%u",info->id);
    if(0 == cnt)
    {
        return CM_OK;
    }
    return cm_sync_delete(CM_SYNC_OBJ_ADMIN,info->id);
}    

void cm_cnm_admin_oplog_create(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK) ? CM_ALARM_LOG_ADMIN_CREATE_OK : CM_ALARM_LOG_ADMIN_CREATE_FAIL;
    const cm_cnm_decode_info_t *req = pDecodeParam; 
    const uint32 cnt = 2;

    if(NULL == pDecodeParam)
    {   
        cm_cnm_oplog_report(sessionid,alarmid,NULL,cnt,NULL);
    }    
    else    
    {   
        cm_cnm_admin_info_t *info = (cm_cnm_admin_info_t *)req->data;
        cm_cnm_oplog_param_t params[2] = {
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_ADMIN_NAME,strlen(info->name),info->name},
            {CM_OMI_DATA_ENUM,CM_OMI_FIELD_ADMIN_LEVEL,sizeof(info->level),&info->level},
        };
        
        cm_cnm_oplog_report(sessionid,alarmid,params,cnt,&req->set);
    }   
    return;
}    

void cm_cnm_admin_oplog_update(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK) ? CM_ALARM_LOG_ADMIN_MODIFY_OK : CM_ALARM_LOG_ADMIN_MODIFY_FAIL;
    const cm_cnm_decode_info_t *req = pDecodeParam;
    sint8 pwd[CM_NAME_LEN] = {"******"};
    const uint32 cnt = 3;

    if(NULL == pDecodeParam)
    {   
        cm_cnm_oplog_report(sessionid,alarmid,NULL,cnt,NULL);
    }    
    else    
    {   
        cm_cnm_admin_info_t *info = (cm_cnm_admin_info_t *)req->data;
        cm_cnm_oplog_param_t params[3] = {
            {CM_OMI_DATA_INT,CM_OMI_FIELD_ADMIN_ID,sizeof(info->id),&info->id},
            {CM_OMI_DATA_ENUM,CM_OMI_FIELD_ADMIN_LEVEL,sizeof(info->level),&info->level},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_ADMIN_PWD,strlen(pwd),pwd},
        };
        cm_cnm_oplog_report(sessionid,alarmid,params,cnt,&req->set);
    }   
    return;
}    

void cm_cnm_admin_oplog_delete(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK) ? CM_ALARM_LOG_ADMIN_DELETE_OK : CM_ALARM_LOG_ADMIN_DELETE_FAIL;
    const cm_cnm_decode_info_t *req = pDecodeParam;
    const uint32 cnt = 1;

    if(NULL == pDecodeParam)
    {   
        cm_cnm_oplog_report(sessionid,alarmid,NULL,cnt,NULL);
    }    
    else    
    {   
        cm_cnm_admin_info_t *info = (cm_cnm_admin_info_t *)req->data;
        cm_cnm_oplog_param_t params[1] = {
            {CM_OMI_DATA_INT,CM_OMI_FIELD_ADMIN_ID,sizeof(info->id),&info->id},
        };
        
        cm_cnm_oplog_report(sessionid,alarmid,params,cnt,&req->set);
    }   
    return;
}

sint32 cm_cnm_admin_sync_request(uint64 data_id, void *pdata, uint32 len)
{
    uint64 cnt = 0;
    cm_cnm_admin_info_t *info = pdata;

    (void)cm_db_exec_get_count(g_cm_admin_handle,&cnt,"SELECT COUNT(id) FROM record_t"
        " WHERE id=%llu",data_id);
    if(0 == cnt)
    {
        return cm_db_exec_ext(g_cm_admin_handle,"INSERT INTO record_t"
            " (id,level,name,pwd) VALUES (%llu,%u,'%s','%s')",
            data_id,info->level,info->name,info->pwd);
    }
    return cm_db_exec_ext(g_cm_admin_handle,"UPDATE record_t SET"
            " level=%u,pwd='%s' WHERE id=%llu",
            info->level,info->pwd,data_id);
}

static sint32 cm_cnm_admin_get_each(
    void *arg, sint32 col_cnt, sint8 **col_vals, sint8 **col_names)
{
    cm_cnm_admin_info_t *info = arg;

    if(4 != col_cnt)
    {
        CM_LOG_ERR(CM_MOD_CNM,"col_cnt %u",col_cnt);
        return CM_FAIL;
    }
    info->id = (uint32)atoi(col_vals[0]);
    info->level = (uint32)atoi(col_vals[1]);
    CM_VSPRINTF(info->name,sizeof(info->name),"%s",col_vals[2]);
    CM_VSPRINTF(info->pwd,sizeof(info->pwd),"%s",col_vals[3]);
    return CM_OK;
}

sint32 cm_cnm_admin_sync_get(uint64 data_id, void **pdata, uint32 *plen)
{
    uint32 cnt = 0;
    cm_cnm_admin_info_t *info = CM_MALLOC(sizeof(cm_cnm_admin_info_t));

    if(NULL == info)
    {
        CM_LOG_ERR(CM_MOD_CNM,"malloc fail");
        return CM_FAIL;
    }
    cnt = cm_db_exec_get(g_cm_admin_handle,cm_cnm_admin_get_each,
        info,1,sizeof(cm_cnm_admin_info_t),
        "SELECT * FROM record_t WHERE id=%llu",data_id);
    if(0 == cnt)
    {
        CM_LOG_ERR(CM_MOD_CNM,"id=%llu",data_id);
        CM_FREE(info);
        return CM_FAIL;
    }
    *pdata = info;
    *plen = sizeof(cm_cnm_admin_info_t);
    return CM_OK;
}
sint32 cm_cnm_admin_sync_delete(uint64 data_id)
{
    return cm_db_exec_ext(g_cm_admin_handle,"DELETE FROM record_t"
            " WHERE id=%llu",data_id);
}

sint32 cm_cnm_admin_login(const sint8* name, const sint8* pwd, uint32 *level)
{
    uint64 lvl = (uint64)-1;

    (void)cm_db_exec_get_count(g_cm_admin_handle,&lvl,
        "SELECT level FROM record_t"
        " WHERE name='%s' AND pwd='%s' LIMIT 1",name,pwd);
    if(lvl == (uint64)-1)  
    {
        return CM_ERR_PWD;
    }
    *level = (uint32)lvl;
    return CM_OK;
}

#define CM_CNM_PERMISSION_ALL_SET 0xFFFFFFFF
extern const cm_omi_object_cmd_cfg_t* cm_omi_get_obj_cmd_cfg(
    uint32 ObjId, uint32 CmdId);
    
static sint32 cm_cnm_permission_decode_check_obj(void *val)
{
    uint32 value = *((uint32*)val);
    if(value >= CM_OMI_OBJECT_BUTT)
    {
        CM_LOG_ERR(CM_MOD_CNM,"obj [%u]",value);
        return CM_PARAM_ERR;
    }
    return CM_OK;
}

static sint32 cm_cnm_permission_decode_check_cmd(void *val)
{
    uint32 value = *((uint32*)val);
    if(value >= CM_OMI_CMD_BUTT)
    {
        CM_LOG_ERR(CM_MOD_CNM,"cmd [%u]",value);
        return CM_PARAM_ERR;
    }
    return CM_OK;
}

static sint32 cm_cnm_permission_decode_ext(
    const cm_omi_obj_t ObjParam,void* data,cm_omi_field_flag_t *set)
{
    sint32 iRet = CM_OK;
    cm_cnm_permission_info_t *info = data;
    cm_cnm_decode_param_t param_num[] = 
    {
        {CM_OMI_FIELD_PERMISSION_OBJ,sizeof(info->obj),&info->obj,cm_cnm_permission_decode_check_obj},
        {CM_OMI_FIELD_PERMISSION_CMD,sizeof(info->cmd),&info->cmd,cm_cnm_permission_decode_check_cmd},
        {CM_OMI_FIELD_PERMISSION_MASK,sizeof(info->mask),&info->mask,NULL},
    };
    iRet = cm_cnm_decode_num(ObjParam,param_num,
        sizeof(param_num)/sizeof(cm_cnm_decode_param_t),set);
    if(iRet != CM_OK)
    {
        return iRet;
    }
    return CM_OK;
}

sint32 cm_cnm_permission_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam)

{
    return cm_cnm_decode_comm(ObjParam,sizeof(cm_cnm_permission_info_t),
        cm_cnm_permission_decode_ext,ppDecodeParam);
}   
    
cm_omi_obj_t cm_cnm_permission_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen)
{
    cm_omi_obj_t items;
    sint8 *cmd = pAckData;
    if(NULL == pAckData)
    {
        CM_LOG_ERR(CM_MOD_CNM,"pAckData null");
        return NULL;
    }    
    
    items = cm_omi_obj_new_array();
    if(NULL == items)
    {
        CM_LOG_ERR(CM_MOD_CNM,"new items fail");
        return NULL;
    }
    CM_LOG_INFO(CM_MOD_CNM,"%s",cmd);
    (void)cm_db_exec(g_cm_admin_handle,cm_omi_encode_db_record_each,items,"%s",cmd);
    return items;
}    

static void cm_cnm_permission_sql_where(const cm_cnm_decode_info_t 
*decode,sint8 *sql, uint32 len)
{
    const cm_cnm_permission_info_t *info = (const cm_cnm_permission_info_t *)decode->data;
    uint32 cnt = 0;
    
    if(CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_PERMISSION_OBJ))
    {
        CM_SNPRINTF_ADD(sql,len," WHERE obj=%u",info->obj);
        cnt++;
    }
    if(CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_PERMISSION_CMD))
    {
        if(0 == cnt)
        {
            CM_SNPRINTF_ADD(sql,len," WHERE cmd=%u",info->cmd);
        }
        else
        {
            CM_SNPRINTF_ADD(sql,len," AND cmd=%u",info->cmd);
        }
        cnt++;
    }
    if(CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_PERMISSION_MASK))
    {
        if(0 == cnt)
        {
            CM_SNPRINTF_ADD(sql,len," WHERE mask&%u=%u",info->mask,info->mask);
        }
        else
        {
            CM_SNPRINTF_ADD(sql,len," AND mask&%u=%u",info->mask,info->mask);
        }
        cnt++;
    }
    return;
}

sint32 cm_cnm_permission_getbatch(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    const cm_cnm_decode_info_t *decode = pDecodeParam;
    const sint8* fields[] = {
        "obj AS f0",
        "cmd AS f1",
        "mask AS f2",
        };
    cm_omi_field_flag_t field_flag; 
    const cm_omi_field_flag_t *pfield = NULL;
    uint32 len = CM_STRING_128;
    sint8 *sql = CM_MALLOC(len);

    if(NULL == sql)
    {
        CM_LOG_ERR(CM_MOD_CNM,"malloc fail");
        return CM_FAIL;
    }
    if(NULL == decode)
    {
        CM_OMI_FIELDS_FLAG_SET_ALL(&field_flag);
        pfield = &field_flag;
        
    }
    else
    {
        pfield = &decode->field;
    }
    cm_omi_make_select_field(fields,sizeof(fields)/sizeof(sint8*),pfield,sql,len);
    
    CM_SNPRINTF_ADD(sql,len," FROM perm_t");
    if(NULL == decode)
    {
        CM_SNPRINTF_ADD(sql,len," ORDER BY obj,cmd LIMIT %u", CM_CNM_MAX_RECORD);
    }
    else
    {
        cm_cnm_permission_sql_where(decode,sql,len);
        CM_SNPRINTF_ADD(sql,len," ORDER BY obj,cmd LIMIT %llu,%u", decode->offset,decode->total);
    }

    *ppAckData = sql;
    *pAckLen = len;
    return CM_OK;
}    

sint32 cm_cnm_permission_get(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return CM_OK;
}

sint32 cm_cnm_permission_request(uint32 cmd,
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    const cm_cnm_decode_info_t *decode = pDecodeParam;
    const cm_cnm_permission_info_t *info = NULL;
    cm_cnm_permission_info_t infoset = {0,0,0};
    uint64 cnt = 0;
    
    if(NULL == decode)
    {
        return CM_PARAM_ERR;
    }    
    info = (const cm_cnm_permission_info_t *)decode->data;
    if(!CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_PERMISSION_OBJ))
    {
        CM_LOG_ERR(CM_MOD_CNM,"obj null");
        return CM_PARAM_ERR;
    }
    
    if(!CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_PERMISSION_CMD))
    {
        CM_LOG_ERR(CM_MOD_CNM,"cmd null");
        return CM_PARAM_ERR;
    }
    
    if(!CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_PERMISSION_MASK)
        || ((CM_OMI_CMD_MODIFY != cmd) && (0 == info->mask)))
    {
        CM_LOG_ERR(CM_MOD_CNM,"mask null");
        return CM_PARAM_ERR;
    }
    if(NULL == cm_omi_get_obj_cmd_cfg(info->obj,info->cmd))
    {
        CM_LOG_ERR(CM_MOD_CNM,"obj[%u] cmd[%u]",info->obj,info->cmd);
        return CM_ERR_NOT_SUPPORT;
    }
    infoset.obj = info->obj;
    infoset.cmd = info->cmd;
    cnt = 0;
    (void)cm_db_exec_get_count(g_cm_admin_handle,&cnt,
        "SELECT mask FROM perm_t WHERE obj=%u AND cmd=%u LIMIT 1",
        info->obj,info->cmd);
    infoset.mask = (uint32)cnt;

    switch(cmd)
    {
        case CM_OMI_CMD_ADD:
            if((infoset.mask & info->mask) == info->mask)
            {
                /* 已经添加 不需要再进行 */
                return CM_OK;
            }
            infoset.mask |= info->mask;
            break;
        case CM_OMI_CMD_MODIFY:
            if(infoset.mask == info->mask)
            {
                /* 一样的情况 */
                return CM_OK;
            }
            infoset.mask = info->mask;
            break;
        case CM_OMI_CMD_DELETE:
            if((infoset.mask & info->mask) == 0)
            {
                /* 没有要清除的 */
                return CM_OK;
            }
            infoset.mask &= ~info->mask;
            break;
        default:
            return CM_OK;
    }

    cnt = (uint64)(((uint64)info->obj) << 32) | ((uint64)info->cmd);
    return cm_sync_request(CM_SYNC_OBJ_PERMISSION,cnt,(void*)&infoset,sizeof(cm_cnm_permission_info_t));
}
sint32 cm_cnm_permission_add(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_cnm_permission_request(CM_OMI_CMD_ADD,pDecodeParam,ppAckData,pAckLen);
}    
    
sint32 cm_cnm_permission_update(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_cnm_permission_request(CM_OMI_CMD_MODIFY,pDecodeParam,ppAckData,pAckLen);
}
    
sint32 cm_cnm_permission_count(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    const cm_cnm_decode_info_t *decode = pDecodeParam;
    sint8 sql[CM_STRING_128] = {0};
    uint32 len = sizeof(sql);
    uint64 cnt = 0;
    
    CM_VSPRINTF(sql,len,"SELECT COUNT(obj) FROM perm_t");
    if(NULL != decode)
    {
        cm_cnm_permission_sql_where(decode,sql,len);
    }
    (void)cm_db_exec_get_count(g_cm_admin_handle,&cnt,sql);
    return cm_cnm_ack_uint64(cnt,ppAckData,pAckLen);
}
    
sint32 cm_cnm_permission_delete(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_cnm_permission_request(CM_OMI_CMD_DELETE,pDecodeParam,ppAckData,pAckLen);
}

static void cm_cnm_permission_oplog_report(
    const sint8* sessionid, const void *pDecodeParam, uint32 alarmid)
{
    const cm_cnm_decode_info_t *req = pDecodeParam;
    const uint32 cnt = 3;
    if(NULL == pDecodeParam)
    {   
        cm_cnm_oplog_report(sessionid,alarmid,NULL,cnt,NULL);
    }    
    else    
    {       
        const cm_cnm_permission_info_t *info = (const cm_cnm_permission_info_t*)req->data;
        cm_cnm_oplog_param_t params[3] = {
            {CM_OMI_DATA_INT,CM_OMI_FIELD_PERMISSION_OBJ,sizeof(info->obj),&info->obj},
            {CM_OMI_DATA_INT,CM_OMI_FIELD_PERMISSION_CMD,sizeof(info->cmd),&info->cmd},
            {CM_OMI_DATA_INT,CM_OMI_FIELD_PERMISSION_MASK,sizeof(info->mask),&info->mask},
        };
        cm_cnm_oplog_report(sessionid,alarmid,params,cnt,&req->set);
    }    
    return;
}    
void cm_cnm_permission_oplog_add(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK)? CM_ALARM_LOG_PERMISSION_INSERT_OK : CM_ALARM_LOG_PERMISSION_INSERT_FAIL;
    cm_cnm_permission_oplog_report(sessionid,pDecodeParam,alarmid);
    return;
}

void cm_cnm_permission_oplog_update(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK)? CM_ALARM_LOG_PERMISSION_UPDATE_OK : CM_ALARM_LOG_PERMISSION_UPDATE_FAIL;
    cm_cnm_permission_oplog_report(sessionid,pDecodeParam,alarmid);
    return;
}

void cm_cnm_permission_oplog_delete(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK)? CM_ALARM_LOG_PERMISSION_DELETE_OK : CM_ALARM_LOG_PERMISSION_DELETE_FAIL;
    cm_cnm_permission_oplog_report(sessionid,pDecodeParam,alarmid);
    return;
}

sint32 cm_cnm_permission_sync_request(uint64 data_id, void *pdata, uint32 len)
{
    cm_cnm_permission_info_t *info = pdata;
    uint64 cnt = 0;

    (void)cm_db_exec_get_count(g_cm_admin_handle,&cnt,
        "SELECT count(mask) FROM perm_t WHERE obj=%u AND cmd=%u LIMIT 1",
        info->obj,info->cmd);
    if(0 != cnt)
    {
        return cm_db_exec_ext(g_cm_admin_handle,"UPDATE perm_t"
            " SET mask=%u WHERE obj=%u AND cmd=%u",info->mask,info->obj,info->cmd);
    }

    return cm_db_exec_ext(g_cm_admin_handle,"INSERT INTO perm_t"
            " VALUES (%u,%u,%u)",info->obj,info->cmd,info->mask);
}
sint32 cm_cnm_permission_sync_get(uint64 data_id, void **pdata, uint32 *plen)
{
    uint64 mask = 0;
    cm_cnm_permission_info_t *info = CM_MALLOC(sizeof(cm_cnm_permission_info_t));

    if(NULL == info)
    {
        CM_LOG_ERR(CM_MOD_CNM,"malloc fail");
        return CM_FAIL;
    }
    info->obj = (uint32)(data_id>>32);
    info->cmd = (uint32)data_id;

    (void)cm_db_exec_get_count(g_cm_admin_handle,&mask,
        "SELECT mask FROM perm_t WHERE obj=%u AND cmd=%u LIMIT 1",info->obj,info->cmd);

    info->mask = (uint32)mask;
    *pdata = info;
    *plen = sizeof(cm_cnm_permission_info_t);
    return CM_OK;
}

sint32 cm_cnm_permission_sync_delete(uint64 data_id)
{
    uint32 obj = (uint32)(data_id>>32);
    uint32 cmd = (uint32)data_id;
    
    return cm_db_exec_ext(g_cm_admin_handle,"UPDATE perm_t"
            " SET mask=0 WHERE obj=%u AND cmd=%u",obj,cmd);
}

static sint32 cm_cnm_permission_init_cmd(cm_xml_node_ptr curnode,void *arg)
{
    cm_xml_node_ptr *Def = arg;
    cm_xml_node_ptr CommandsDef = Def[0];
    cm_xml_node_ptr Levels = Def[1];
    cm_xml_node_ptr Object = cm_xml_get_parent(curnode);
    const sint8 *ObjectId = NULL;
    const sint8 *Cmd = NULL;
    const sint8 *Admins = NULL;
    const sint8 *Level = NULL;
    sint8 *Admin = NULL;
    const uint32 perm_all = 0xFFFFFFFF;
    sint8 Adminlist[CM_STRING_256] = {0};
    uint32 perm_cur = 0;
    uint32 LevelId = 0;
    
    if(NULL == Object)
    {
        CM_LOG_ERR(CM_MOD_CNM,"get parent fail");
        return CM_FAIL;
    }
    /* 获取对象ID  */
    ObjectId = cm_xml_get_attr(Object,"id");
    if(NULL == ObjectId)
    {
        CM_LOG_ERR(CM_MOD_CNM,"get obj id fail");
        return CM_FAIL;
    }

    /* 获取操作ID */
    Cmd = cm_xml_get_attr(curnode, "name");
    if(NULL == Cmd)
    {
        CM_LOG_ERR(CM_MOD_CNM,"get cmd name fail");
        return CM_FAIL;
    }
    Object = cm_xml_get_node(CommandsDef,"Command","name",Cmd);
    if(NULL == Object)
    {
        CM_LOG_ERR(CM_MOD_CNM,"get cmd %s fail",Cmd);
        return CM_FAIL;
    }
    Cmd = cm_xml_get_attr(Object,"id");
    if(NULL == Object)
    {
        CM_LOG_ERR(CM_MOD_CNM,"get cmd id fail");
        return CM_FAIL;
    }

    /* 获取权限 */
    Admins = cm_xml_get_attr(curnode,"admins");
    if(NULL == Admins)
    {
        CM_LOG_ERR(CM_MOD_CNM,"get admins fail");
        return CM_FAIL;
    }
    CM_VSPRINTF(Adminlist,sizeof(Adminlist),"%s",Admins);

    Admin = strtok(Adminlist,"|");
    while(NULL != Admin)
    {
        if(0 == strcasecmp(Admin,"ALL"))
        {
            perm_cur |= perm_all;
        }
        else
        {
            Object = cm_xml_get_node(Levels,"Level","name",Admin);
            if(NULL != Object)
            {
                Level = cm_xml_get_attr(Object,"id");
                if(NULL != Level)
                {
                    LevelId = (uint32)atoi(Level);
                    if((0 < LevelId) && (32 >= LevelId))
                    {
                        perm_cur |= 1<<(LevelId-1);
                    }
                }
                else
                {
                    CM_LOG_ERR(CM_MOD_CNM,"get Level id %s fail",Admin);
                }
            }
            else
            {
                CM_LOG_ERR(CM_MOD_CNM,"get Level name %s fail",Admin);
            }
        }
        Admin = strtok(NULL,"|");
    }
    
    return cm_db_exec_ext(g_cm_admin_handle,"INSERT INTO perm_t"
            " VALUES (%s,%s,%u)",ObjectId,Cmd,perm_cur);
}

sint32 cm_cnm_permission_init(void)
{
    const sint8 *cfgxml = CM_OMI_PERMISSION_CFG_FILE;
    cm_xml_node_ptr cfg_xml_root = NULL;
    cm_xml_node_ptr Def[2] = {NULL};
    cm_xml_node_ptr Objects = NULL;
    
    if(!cm_file_exist(cfgxml))
    {
        return CM_OK;
    }

    cfg_xml_root = cm_xml_parse_file(cfgxml);
    if(NULL == cfg_xml_root)
    {
        CM_LOG_ERR(CM_MOD_CNM,"parse: %s fail",cfgxml);
        return CM_FAIL;
    }

    Def[0] = cm_xml_get_node(cfg_xml_root,"CommandsDef",NULL,NULL);
    if(NULL == Def[0])
    {
        CM_LOG_ERR(CM_MOD_CNM,"get:CommandsDef fail");
        cm_xml_free(cfg_xml_root);
        return CM_FAIL;
    }

    Def[1] = cm_xml_get_node(cfg_xml_root,"Levels",NULL,NULL);
    if(NULL == Def[1])
    {
        CM_LOG_ERR(CM_MOD_CNM,"get:Levels fail");
        cm_xml_free(cfg_xml_root);
        return CM_FAIL;
    }

    Objects = cm_xml_get_node(cfg_xml_root,"Objects",NULL,NULL);
    if(NULL == Objects)
    {
        CM_LOG_ERR(CM_MOD_CNM,"get:Objects fail");
        cm_xml_free(cfg_xml_root);
        return CM_FAIL;
    }
    
    (void)cm_xml_find_node(Objects,"Command",NULL,NULL,cm_cnm_permission_init_cmd,Def);

    cm_xml_free(cfg_xml_root);
    return CM_OK;
}


static sint32 cm_cnm_permission_check_cmd(uint32 obj, uint32 cmd, uint32 level)
{
    uint64 cnt = 0;    
       
    /* 转换为mask */
    level = 1<<(level - 1);
    cm_db_exec_get_count(g_cm_admin_handle,&cnt,
        "SELECT COUNT(obj) FROM perm_t "
        "WHERE obj=%u AND cmd=%u AND mask&%u=%u",obj,cmd,level,level);
    return (cnt>0)? CM_OK : CM_ERR_NO_PERMISSION; 
}


