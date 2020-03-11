/******************************************************************************
 *          Copyright (c) 2018 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cnm_snapshot.c
 * author     : wbn
 * create date: 2018.05.24
 * description: TODO:
 *
 *****************************************************************************/
#include "cm_cnm_snapshot.h"
#include "cm_node.h"
#include "cm_log.h"
#include "cm_omi.h"
#include "cm_sche.h"

extern sint32 cm_cnm_lun_local_get_stmfid(const sint8* pool, const sint8* lun, sint8 *buf, uint32 
len);
sint32 cm_cnm_snapshot_init(void)
{
    return CM_OK;
}

static sint32 cm_cnm_snapshot_decode_ext(const cm_omi_obj_t ObjParam,void* data,cm_omi_field_flag_t *set)
{
    cm_cnm_snapshot_info_t *info = data;
    
    if(CM_OK == cm_omi_obj_key_get_str_ex(ObjParam,CM_OMI_FIELD_SNAPSHOT_NAME,info->name,sizeof(info->name)))
    {
        CM_OMI_FIELDS_FLAG_SET(set,CM_OMI_FIELD_SNAPSHOT_NAME);
    }
    if(CM_OK == cm_omi_obj_key_get_u32_ex(ObjParam,CM_OMI_FIELD_SNAPSHOT_NID,&info->nid))
    {
        CM_OMI_FIELDS_FLAG_SET(set,CM_OMI_FIELD_SNAPSHOT_NID);
    }
    if(CM_OK == cm_omi_obj_key_get_str_ex(ObjParam,CM_OMI_FIELD_SNAPSHOT_DIR,info->dir,sizeof(info->dir)))
    {
        CM_OMI_FIELDS_FLAG_SET(set,CM_OMI_FIELD_SNAPSHOT_DIR);
    }
    return CM_OK;
}

static void cm_cnm_snapshot_encode_each(cm_omi_obj_t item,void *eachdata,void *arg)
{
    cm_cnm_snapshot_info_t *info = eachdata;
    cm_omi_field_flag_t *field = arg;

    cm_cnm_map_value_str_t cols_str[] = {
            {CM_OMI_FIELD_SNAPSHOT_NAME,    info->name},
            {CM_OMI_FIELD_SNAPSHOT_DIR,     info->dir},
            {CM_OMI_FIELD_SNAPSHOT_USED,    info->used},
        };
    cm_cnm_map_value_num_t cols_num[] = {
            {CM_OMI_FIELD_SNAPSHOT_NID,     info->nid},
            {CM_OMI_FIELD_SNAPSHOT_DATE,    (uint32)info->tm},
            {CM_OMI_FIELD_SNAPSHOT_TYPE, info->type},
        };
    
    cm_cnm_encode_str(item,field,cols_str,sizeof(cols_str)/sizeof(cm_cnm_map_value_str_t));
    cm_cnm_encode_num(item,field,cols_num,sizeof(cols_num)/sizeof(cm_cnm_map_value_num_t));  
    return;
}

sint32 cm_cnm_snapshot_decode(
    const cm_omi_obj_t ObjParam, void **ppDecodeParam)
{
    return cm_cnm_decode_comm(ObjParam,sizeof(cm_cnm_snapshot_info_t),
        cm_cnm_snapshot_decode_ext,ppDecodeParam);
}    
    
cm_omi_obj_t cm_cnm_snapshot_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen)
{
    return cm_cnm_encode_comm_ext(pDecodeParam,pAckData,AckLen,
        sizeof(cm_cnm_snapshot_info_t),cm_cnm_snapshot_encode_each);
}    

#define cm_cnm_snapshot_req(cmd,param,ppAck,plen) \
    cm_cnm_request_comm(CM_OMI_OBJECT_SNAPSHOT,cmd,sizeof(cm_cnm_snapshot_info_t),param,ppAck,plen)
    
sint32 cm_cnm_snapshot_getbatch(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_cnm_snapshot_req(CM_OMI_CMD_GET_BATCH,pDecodeParam,ppAckData,pAckLen);
}    

static sint32 cm_cnm_snapshot_req_check(const void *pDecodeParam)
{
    const cm_cnm_decode_info_t *req = pDecodeParam;
    const cm_cnm_snapshot_info_t *info = NULL;

    if(NULL == req)
    {
        CM_LOG_ERR(CM_MOD_CNM,"param null");
        return CM_PARAM_ERR; 
    }
    info = (const cm_cnm_snapshot_info_t*)req->data;
    if(('\0' == info->name[0]) || ('\0' == info->dir[0]))
    {
        CM_LOG_ERR(CM_MOD_CNM,"%s@%s",info->dir,info->name);
        return CM_PARAM_ERR; 
    }
    return CM_OK;
}

sint32 cm_cnm_snapshot_get(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    sint32 iRet = cm_cnm_snapshot_req_check(pDecodeParam);
    
    if(CM_OK != iRet)
    {
        return iRet;
    }
    return cm_cnm_snapshot_req(CM_OMI_CMD_GET,pDecodeParam,ppAckData,pAckLen);
}    

sint32 cm_cnm_snapshot_create(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    sint32 iRet = cm_cnm_snapshot_req_check(pDecodeParam);
    
    if(CM_OK != iRet)
    {
        return iRet;
    }
    return cm_cnm_snapshot_req(CM_OMI_CMD_ADD,pDecodeParam,ppAckData,pAckLen);
}    
    
sint32 cm_cnm_snapshot_update(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    sint32 iRet = cm_cnm_snapshot_req_check(pDecodeParam);
    
    if(CM_OK != iRet)
    {
        return iRet;
    }
    return cm_cnm_snapshot_req(CM_OMI_CMD_MODIFY,pDecodeParam,ppAckData,pAckLen);
}    
    
sint32 cm_cnm_snapshot_count(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_cnm_snapshot_req(CM_OMI_CMD_COUNT,pDecodeParam,ppAckData,pAckLen);
}    
    
sint32 cm_cnm_snapshot_delete(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    sint32 iRet = cm_cnm_snapshot_req_check(pDecodeParam);
    
    if(CM_OK != iRet)
    {
        return iRet;
    }
    return cm_cnm_snapshot_req(CM_OMI_CMD_DELETE,pDecodeParam,ppAckData,pAckLen);
}    

/*============================================================================*/

static const sint8* g_cm_cnm_snapshot_get = "zfs list -H -r -t snapshot -o name,used,creation";
static const sint8* g_cm_cnm_snapshot_cnt = "zfs list -H -r -t snapshot -o name";

static sint32 cm_cnm_snapshot_local_get_each(void *arg, sint8 **cols, uint32 col_num)
{
    cm_cnm_snapshot_info_t *info = arg;
    const uint32 cols_def = 7;
    const sint8* month = NULL;
    const sint8* year = NULL;
    const sint8* hour = NULL;
    const sint8* day = NULL;
    const sint8* monthcfg[] = {
        "Jan","Feb","Mar","Apr","May","Jun",
        "Jul","Aug","Sep","Oct","Nov","Dec",
        "January","February","March","April","May","June",
        "July","August","September","October","November","December"};
    struct tm datetime;
    /*    
    Prodigy-root#zfs list -H -t snapshot -o name,used,creation -r test |egrep '^test'
    test/lun888@DGFDDYD     36K     五  4月 20 16:33 2018
    test/luna1@aaaaaaa      0       四  5月 24 10:41 2018
    test/luna1@1231asdasd   0       四  5月 24 19:54 2018
    */
    if((cols_def != col_num) && ((cols_def-1) != col_num))
    {
        /* 中文时，周几会被忽略掉 */
        CM_LOG_WARNING(CM_MOD_CNM,"num[%u] def[%u]",col_num,cols_def);
        return CM_FAIL;
    }
    col_num--;
    year = cols[col_num];
    hour = cols[col_num-1];
    day = cols[col_num-2];
    month = cols[col_num-3];
    
    col_num = sscanf(cols[0],"%[0-9a-zA-Z_-./]@%[0-9a-zA-Z_-.]",info->dir,info->name);
    if(2 != col_num)
    {
        CM_LOG_WARNING(CM_MOD_CNM,"%s",cols[0]);
        return CM_FAIL;
    }
    /* 1 lun, else nas */
    info->type = cm_exec_int("zfs get type %s |grep \" volume \" |wc -l",info->dir);
    CM_VSPRINTF(info->used,sizeof(info->used),"%s",cols[1]);
    info->nid = cm_node_get_id();
    
    cm_get_datetime(&datetime);
    
    /*接下来获取时间，这里比较恶心 strptime不能格式化出来，
    必要的时候将下面的代码单独成一个函数，暂时留着*/
    datetime.tm_year = atoi(year);
    if(datetime.tm_year > 1900)
    {  
        datetime.tm_year -= 1900;
    }
    datetime.tm_mday = atoi(day);
    sscanf(hour,"%02d:%02d",&datetime.tm_hour,&datetime.tm_min);
    datetime.tm_sec = 0;
    if(('1'<=month[0]) && ('9' >= month[0]))
    {
        /* 应该是中文格式的 , 直接转*/
        datetime.tm_mon = atoi(month)-1;
    }
    else if(('A'<=month[0]) && ('Z' >= month[0]))
    {
        /* 英文格式 */
        col_num = 0;
        while(col_num < 12)
        {
            if((0 == strcmp(month,monthcfg[col_num]))
                || (0 == strcmp(month,monthcfg[col_num+12])))
            {
                datetime.tm_mon = col_num; 
                break;
            }
            col_num++;
        }
    }
    else
    {
        /* 这样的情况应该不会出现 */
        CM_LOG_WARNING(CM_MOD_CNM,"month:%s",month);
    }
    info->tm = mktime(&datetime);
    return CM_OK;
}

sint32 cm_cnm_snapshot_local_getbatch(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    sint32 iRet = CM_OK;
    cm_cnm_decode_info_t *req = param;
    cm_cnm_snapshot_info_t *info = NULL;
    sint8 cmd[CM_STRING_1K] = {0};
    uint32 cmd_len = sizeof(cmd);
    
    CM_SNPRINTF_ADD(cmd,cmd_len,"%s",g_cm_cnm_snapshot_get);
    if(NULL != req)
    {
        info = (cm_cnm_snapshot_info_t*)req->data;
        if('\0' != info->dir[0])
        {
            /* 假如路径名过长，这里还得通过修改长度 */
            CM_SNPRINTF_ADD(cmd,cmd_len," %s",info->dir);
            if('\0' != info->name[0])
            {
                CM_SNPRINTF_ADD(cmd,cmd_len,"@%s",info->name);
            }
            else
            {
                CM_SNPRINTF_ADD(cmd,cmd_len," 2>/dev/null |egrep '^%s@'",info->dir);
            }
        }
    }
    
    CM_LOG_INFO(CM_MOD_CNM,"%s",cmd);
    iRet = cm_cnm_exec_get_list(cmd,
        cm_cnm_snapshot_local_get_each,(uint32)offset,sizeof(cm_cnm_snapshot_info_t),
        ppAck,&total);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]",iRet);
        return iRet;
    }
    *pAckLen = total * sizeof(cm_cnm_snapshot_info_t);
    return CM_OK;
}    

sint32 cm_cnm_snapshot_local_get(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    sint32 iRet = CM_OK;
    cm_cnm_decode_info_t *req = param;
    cm_cnm_snapshot_info_t *info = NULL;
    cm_cnm_snapshot_info_t *data = NULL;

    if(NULL == req)
    {
        CM_LOG_ERR(CM_MOD_CNM,"param null");
        return CM_PARAM_ERR; 
    }
    info = (cm_cnm_snapshot_info_t*)req->data;
    if(('\0' == info->name[0]) || ('\0' == info->dir[0]))
    {
        CM_LOG_ERR(CM_MOD_CNM,"%s@%s",info->dir,info->name);
        return CM_PARAM_ERR; 
    }
    data = CM_MALLOC(sizeof(cm_cnm_snapshot_info_t));
    if(NULL == data)
    {
        CM_LOG_ERR(CM_MOD_CNM,"malloc fail");
        return CM_FAIL;
    }
    
    CM_LOG_INFO(CM_MOD_CNM,"%s@%s",info->dir,info->name);
    
    iRet = cm_cnm_exec_get_col(cm_cnm_snapshot_local_get_each,data,
        "%s %s@%s", g_cm_cnm_snapshot_get,info->dir,info->name);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]",iRet);
        CM_FREE(data);
        return CM_OK;
    }
    *ppAck = data;
    *pAckLen = sizeof(cm_cnm_snapshot_info_t );
    return CM_OK;
}    

sint32 cm_cnm_snapshot_local_count(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    cm_cnm_decode_info_t *req = param;
    cm_cnm_snapshot_info_t *info = NULL;
    sint8 cmd[CM_STRING_1K] = {0};
    uint32 cmd_len = sizeof(cmd);
    uint64 cnt = 0;
    
    CM_SNPRINTF_ADD(cmd,cmd_len,"%s",g_cm_cnm_snapshot_cnt);
    if(NULL != req)
    {
        info = (cm_cnm_snapshot_info_t*)req->data;
        if('\0' != info->dir[0])
        {
            CM_SNPRINTF_ADD(cmd,cmd_len," %s",info->dir);
            if('\0' != info->name[0])
            {
                CM_SNPRINTF_ADD(cmd,cmd_len,"@%s 2>/dev/null",info->name);
            }
            else
            {
                CM_SNPRINTF_ADD(cmd,cmd_len," 2>/dev/null |egrep '^%s@'",info->dir);
            }
        }
    }
    else
    {
        CM_SNPRINTF_ADD(cmd,cmd_len," 2>/dev/null");
    }
    CM_SNPRINTF_ADD(cmd,cmd_len," |wc -l");
    cnt = (uint64)cm_exec_double(cmd);
    return cm_cnm_ack_uint64(cnt,ppAck,pAckLen);
}    

static sint32 cm_cnm_snapshot_local_req(void *param, const sint8* cmd)
{
    sint32 iRet = CM_OK;
    cm_cnm_decode_info_t *req = param;
    cm_cnm_snapshot_info_t *info = (cm_cnm_snapshot_info_t*)req->data;
    sint8 buf[CM_STRING_1K] = {0};
    
    iRet = cm_exec_tmout(buf,sizeof(buf),CM_CMT_REQ_TMOUT,
        "zfs %s %s@%s 2>&1",cmd,info->dir,info->name);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]\n%s",iRet,buf);
        return cm_cnm_get_errcode(&CmCnmMapCommErrCfg,buf,iRet);
    }
    return CM_OK;
}

sint32 cm_cnm_snapshot_local_create(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    return cm_cnm_snapshot_local_req(param,"snapshot");
}    

sint32 cm_cnm_snapshot_local_update(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    /* 快照还原 复用 */
    cm_cnm_decode_info_t *req = param;
    cm_cnm_snapshot_info_t *info = (cm_cnm_snapshot_info_t*)req->data;

    return cm_system(CM_SCRIPT_DIR"cm_cnm.sh snapshot_rollback '%s' '%s'",
        info->dir,info->name);
}    

sint32 cm_cnm_snapshot_local_delete(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    return cm_cnm_snapshot_local_req(param,"destroy");
}    

/*============================================================================*/
static void cm_cnm_snapshot_oplog_get_report(
    const sint8* sessionid, const void *pDecodeParam, uint32 alarmid)
{
    const cm_cnm_decode_info_t *req = pDecodeParam;
    const uint32 cnt = 3;
    
    /*<nid> <dir> <name>*/
    if(NULL == pDecodeParam)
    {   
        cm_cnm_oplog_report(sessionid,alarmid,NULL,cnt,NULL);
    }    
    else    
    {       
        const cm_cnm_snapshot_info_t*info = (const cm_cnm_snapshot_info_t*)req->data;
        const sint8* nodename = cm_node_get_name(info->nid);
        cm_cnm_oplog_param_t params[3] = {
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_SNAPSHOT_NID,strlen(nodename),nodename},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_SNAPSHOT_DIR,strlen(info->dir),info->dir},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_SNAPSHOT_NAME,strlen(info->name),info->name},
        };
        
        cm_cnm_oplog_report(sessionid,alarmid,params,cnt,&req->set);
    }   
    return;
}

void cm_cnm_snapshot_oplog_create(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK)? CM_ALARM_LOG_SNAPSHAT_CREATR_OK : CM_ALARM_LOG_SNAPSHAT_CREATR_FAIL;
    cm_cnm_snapshot_oplog_get_report(sessionid,pDecodeParam,alarmid);
}    

void cm_cnm_snapshot_oplog_update(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK)? CM_ALARM_LOG_SNAPSHAT_UPDATE_OK : CM_ALARM_LOG_SNAPSHAT_UPDATE_FAIL;
    cm_cnm_snapshot_oplog_get_report(sessionid,pDecodeParam,alarmid);
}    

void cm_cnm_snapshot_oplog_delete(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK)? CM_ALARM_LOG_SNAPSHAT_DELETE_OK : CM_ALARM_LOG_SNAPSHAT_DELETE_FAIL;
    cm_cnm_snapshot_oplog_get_report(sessionid,pDecodeParam,alarmid);
}    

/*============================================================================*/
sint32 cm_cnm_snapshot_sche_cbk(const sint8* name, const sint8* param)
{
    sint32 iRet = CM_OK;
    uint32 len = sizeof(cm_cnm_decode_info_t)+sizeof(cm_cnm_snapshot_info_t);
    cm_cnm_decode_info_t *decode = NULL;
    cm_cnm_snapshot_info_t *info = NULL;
    uint32 nid = cm_cnm_exec_exited_nid("zfs list -H -o name %s "
                    "2>/dev/null |wc -l", param);

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
    info = (cm_cnm_snapshot_info_t*)decode->data;
    
    CM_VSPRINTF(info->name,sizeof(info->name),"%s_%u",name,(uint32)cm_get_time());
    CM_OMI_FIELDS_FLAG_SET(&decode->set,CM_OMI_FIELD_SNAPSHOT_NAME);
    
    CM_VSPRINTF(info->dir,sizeof(info->dir),"%s",param);
    CM_OMI_FIELDS_FLAG_SET(&decode->set,CM_OMI_FIELD_SNAPSHOT_DIR);

    iRet = cm_cnm_snapshot_req(CM_OMI_CMD_ADD,decode,NULL,NULL);
    CM_FREE(decode);

    return iRet;
}

#define CM_CNM_SCHE_BIT_HOUR 0
#define CM_CNM_SCHE_BIT_WEEK_DAY 1
#define CM_CNM_SCHE_BIT_MONTH_DAY 2

static uint32 cm_cnm_snapshot_sche_decode_bits(uint32 type,
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

static sint32 cm_cnm_snapshot_sche_decode_ext(
    const cm_omi_obj_t ObjParam,void* data,cm_omi_field_flag_t *set)
{
    sint32 iRet = CM_OK;
    uint32 temp = 0;    
    cm_sche_info_t *info = data;

    info->obj = CM_OMI_OBJECT_SNAPSHOT_SCHE;

    iRet = cm_omi_obj_key_get_u64_ex(ObjParam,CM_OMI_FIELD_SNAPSHOT_SCHE_ID,&info->id);
    if(CM_OK == iRet)
    {
        CM_OMI_FIELDS_FLAG_SET(set,CM_OMI_FIELD_SNAPSHOT_SCHE_ID);
    }
    
    iRet = cm_omi_obj_key_get_str_ex(ObjParam,CM_OMI_FIELD_SNAPSHOT_SCHE_NAME,info->name,sizeof(info->name));
    if(CM_OK == iRet)
    {
        CM_OMI_FIELDS_FLAG_SET(set,CM_OMI_FIELD_SNAPSHOT_SCHE_NAME);
    }
    
    iRet = cm_omi_obj_key_get_str_ex(ObjParam,CM_OMI_FIELD_SNAPSHOT_SCHE_DIR,info->param,sizeof(info->param));
    if(CM_OK == iRet)
    {
        CM_OMI_FIELDS_FLAG_SET(set,CM_OMI_FIELD_SNAPSHOT_SCHE_DIR);
    }

    iRet = cm_omi_obj_key_get_u32_ex(ObjParam,CM_OMI_FIELD_SNAPSHOT_SCHE_EXPIRED,&temp);
    if(CM_OK == iRet)
    {
        CM_OMI_FIELDS_FLAG_SET(set,CM_OMI_FIELD_SNAPSHOT_SCHE_EXPIRED);
        info->expired = (cm_time_t)temp;
    }

    iRet = cm_omi_obj_key_get_u32_ex(ObjParam,CM_OMI_FIELD_SNAPSHOT_SCHE_TYPE,&temp);
    if(CM_OK == iRet)
    {
        CM_OMI_FIELDS_FLAG_SET(set,CM_OMI_FIELD_SNAPSHOT_SCHE_TYPE);
        info->type = (uint8)temp;
    }
    iRet = cm_omi_obj_key_get_u32_ex(ObjParam,CM_OMI_FIELD_SNAPSHOT_SCHE_DAYTYPE,&temp);
    if(CM_OK == iRet)
    {
        CM_OMI_FIELDS_FLAG_SET(set,CM_OMI_FIELD_SNAPSHOT_SCHE_DAYTYPE);
        info->day_type = (uint8)temp;
    }
    iRet = cm_omi_obj_key_get_u32_ex(ObjParam,CM_OMI_FIELD_SNAPSHOT_SCHE_MINUTE,&temp);
    if(CM_OK == iRet)
    {
        CM_OMI_FIELDS_FLAG_SET(set,CM_OMI_FIELD_SNAPSHOT_SCHE_MINUTE);
        info->minute = (uint8)temp;
    }
    info->hours = cm_cnm_snapshot_sche_decode_bits(
        CM_CNM_SCHE_BIT_HOUR,ObjParam,CM_OMI_FIELD_SNAPSHOT_SCHE_HOURS,set);
    
    switch(info->day_type)
    {
        case CM_SCHE_DAY_MONTH:
            info->days = cm_cnm_snapshot_sche_decode_bits(CM_CNM_SCHE_BIT_MONTH_DAY,
                ObjParam,CM_OMI_FIELD_SNAPSHOT_SCHE_DAYS,set);
            break;
        case CM_SCHE_DAY_WEEK:
            info->days = cm_cnm_snapshot_sche_decode_bits(CM_CNM_SCHE_BIT_WEEK_DAY,
                ObjParam,CM_OMI_FIELD_SNAPSHOT_SCHE_DAYS,set);
            break;
        default:
            break;
    }
    CM_LOG_INFO(CM_MOD_CNM,"t[%u],d[%u],m[%u],hs[%u],ds[%u]",
        info->type,info->day_type,info->minute,info->hours,info->days);
    return CM_OK;
}

sint32 cm_cnm_snapshot_sche_decode(
    const cm_omi_obj_t ObjParam, void** ppDecodeParam)
{
    return cm_cnm_decode_comm(ObjParam,sizeof(cm_sche_info_t),
        cm_cnm_snapshot_sche_decode_ext,ppDecodeParam);
}

static void cm_cnm_snapshot_sche_encode_bits(uint32 type,uint32 val, sint8 *buf,uint32 len)
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

static void cm_cnm_snapshot_sche_encode_each(cm_omi_obj_t item,void *eachdata,void *arg)
{
    cm_sche_info_t *info = eachdata;
    cm_omi_field_flag_t *field = arg;
    sint8 hours[CM_STRING_256] = {0};
    sint8 days[CM_STRING_256] = {0};
    
    cm_cnm_map_value_str_t cols_str[] = {
        {CM_OMI_FIELD_SNAPSHOT_SCHE_NAME,    info->name},
        {CM_OMI_FIELD_SNAPSHOT_SCHE_DIR,     info->param},
        {CM_OMI_FIELD_SNAPSHOT_SCHE_HOURS,   &hours[1]},
        {CM_OMI_FIELD_SNAPSHOT_SCHE_DAYS,    &days[1]},
    };
    cm_cnm_map_value_num_t cols_num[] = {
        {CM_OMI_FIELD_SNAPSHOT_SCHE_EXPIRED,     (uint32)info->expired},
        {CM_OMI_FIELD_SNAPSHOT_SCHE_TYPE,    (uint32)info->type},
        {CM_OMI_FIELD_SNAPSHOT_SCHE_DAYTYPE,    (uint32)info->day_type},
        {CM_OMI_FIELD_SNAPSHOT_SCHE_MINUTE,    (uint32)info->minute},
    };

    cm_cnm_snapshot_sche_encode_bits(CM_CNM_SCHE_BIT_HOUR,info->hours,hours,sizeof(hours));
    switch(info->day_type)
    {
        case CM_SCHE_DAY_MONTH:
            cm_cnm_snapshot_sche_encode_bits(CM_CNM_SCHE_BIT_MONTH_DAY,info->days,days,sizeof(days));
            break;
        case CM_SCHE_DAY_WEEK:
            cm_cnm_snapshot_sche_encode_bits(CM_CNM_SCHE_BIT_WEEK_DAY,info->days,days,sizeof(days));
            break;
        default:
            break;
    }
    
    if(CM_OMI_FIELDS_FLAG_ISSET(field,CM_OMI_FIELD_SNAPSHOT_SCHE_ID))
    {
        (void)cm_omi_obj_key_set_u64_ex(item,CM_OMI_FIELD_SNAPSHOT_SCHE_ID,info->id);
    }
    cm_cnm_encode_str(item,field,cols_str,sizeof(cols_str)/sizeof(cm_cnm_map_value_str_t));
    cm_cnm_encode_num(item,field,cols_num,sizeof(cols_num)/sizeof(cm_cnm_map_value_num_t)); 
    return;
}

cm_omi_obj_t cm_cnm_snapshot_sche_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen)
{
    return cm_cnm_encode_comm_ext(pDecodeParam,pAckData,AckLen,
        sizeof(cm_sche_info_t),cm_cnm_snapshot_sche_encode_each);
}

sint32 cm_cnm_snapshot_sche_getbatch(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    sint32 iRet = CM_OK;
    const cm_cnm_decode_info_t *decode = pDecodeParam;
    const cm_sche_info_t *info = NULL;

    *pAckLen = 0;
    if(NULL == decode)
    {
        iRet = cm_sche_getbatch(CM_OMI_OBJECT_SNAPSHOT_SCHE,NULL,0,CM_CNM_MAX_RECORD,
            (cm_sche_info_t**)ppAckData,pAckLen);
    }
    else
    {
        info = (const cm_sche_info_t*)decode->data;
        iRet = cm_sche_getbatch(CM_OMI_OBJECT_SNAPSHOT_SCHE,info->param,
            (uint32)decode->offset,decode->total,
            (cm_sche_info_t**)ppAckData,pAckLen);
    }
    *pAckLen *= sizeof(cm_sche_info_t);
    return iRet;
}

sint32 cm_cnm_snapshot_sche_get(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    const cm_cnm_decode_info_t *decode = pDecodeParam;
    const cm_sche_info_t *info = NULL;
    sint32 iRet = CM_OK;
    if(NULL == decode)
    {
        return CM_PARAM_ERR;
    }

    if(!CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_SNAPSHOT_SCHE_ID))
    {
        CM_LOG_ERR(CM_MOD_CNM,"id null");
        return CM_PARAM_ERR;
    }
    info = (const cm_sche_info_t*)decode->data;
    iRet = cm_sche_cbk_sync_get(info->id,ppAckData,pAckLen);
    if(iRet == CM_ERR_NOT_EXISTS)
    {
        iRet = CM_OK;
    }
    return iRet;
}
   
sint32 cm_cnm_snapshot_sche_update(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    const cm_cnm_decode_info_t *decode = pDecodeParam;
    const cm_sche_info_t *info = NULL;
    uint32 set_flag = 0;
    cm_cnm_map_value_num_t mapflag[] = {
        {CM_OMI_FIELD_SNAPSHOT_SCHE_NAME, CM_SCHE_FLAG_NAME},
        {CM_OMI_FIELD_SNAPSHOT_SCHE_EXPIRED, CM_SCHE_FLAG_EXPIRED},
        {CM_OMI_FIELD_SNAPSHOT_SCHE_TYPE, CM_SCHE_FLAG_TYPE},
        {CM_OMI_FIELD_SNAPSHOT_SCHE_DAYTYPE, CM_SCHE_FLAG_DAYTYPE},
        {CM_OMI_FIELD_SNAPSHOT_SCHE_MINUTE, CM_SCHE_FLAG_MINUTE},
        {CM_OMI_FIELD_SNAPSHOT_SCHE_HOURS, CM_SCHE_FLAG_HOURS},
        {CM_OMI_FIELD_SNAPSHOT_SCHE_DAYS, CM_SCHE_FLAG_DAYS},
    };
    uint32 iloop = sizeof(mapflag)/sizeof(cm_cnm_map_value_num_t);
    
    if(NULL == decode)
    {
        return CM_PARAM_ERR;
    }
    
    if(!CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_SNAPSHOT_SCHE_ID))
    {
        CM_LOG_ERR(CM_MOD_CNM,"id null");
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
    return cm_sche_update(info->id,set_flag,(cm_sche_info_t*)info);
}
    
sint32 cm_cnm_snapshot_sche_create(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    const cm_cnm_decode_info_t *decode = pDecodeParam;
    const cm_sche_info_t *info = NULL;

    if(NULL == decode)
    {
        return CM_PARAM_ERR;
    }
    info = (const cm_sche_info_t*)decode->data;
    if((!CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_SNAPSHOT_SCHE_NAME))
        || (0 == strlen(info->name)))
    {
        CM_LOG_ERR(CM_MOD_CNM,"name null");
        return CM_PARAM_ERR;
    }
    if((!CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_SNAPSHOT_SCHE_DIR))
        || (0 == strlen(info->param)))
    {
        CM_LOG_ERR(CM_MOD_CNM,"dir null");
        return CM_PARAM_ERR;
    }
    /* 校验dir是否存在 */
    if(CM_NODE_ID_NONE == cm_cnm_exec_exited_nid(
        "zfs list -H -o name %s 2>/dev/null |wc -l",info->param))
    {
        CM_LOG_ERR(CM_MOD_CNM,"dir[%s] not exited!",info->param);
        return CM_ERR_NOT_EXISTS;
    }
    
    if(!CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_SNAPSHOT_SCHE_EXPIRED))
    {
        CM_LOG_ERR(CM_MOD_CNM,"expired null");
        return CM_PARAM_ERR;
    }
    
    return cm_sche_create((cm_sche_info_t*)info);
}
    
sint32 cm_cnm_snapshot_sche_count(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    uint64 cnt = 0;
    const cm_cnm_decode_info_t *decode = pDecodeParam;
    const cm_sche_info_t *info = NULL;

    if(NULL == decode)
    {
        cnt = cm_sche_count(CM_OMI_OBJECT_SNAPSHOT_SCHE,NULL);
    }
    else
    {
        info = (const cm_sche_info_t*)decode->data;
        cnt = cm_sche_count(CM_OMI_OBJECT_SNAPSHOT_SCHE,info->param);
    }
    return cm_cnm_ack_uint64(cnt,ppAckData,pAckLen);
}
    
sint32 cm_cnm_snapshot_sche_delete(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    const cm_cnm_decode_info_t *decode = pDecodeParam;
    const cm_sche_info_t *info = NULL;
    
    if(NULL == decode)
    {
        return CM_PARAM_ERR;
    }
    
    if(!CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_SNAPSHOT_SCHE_ID))
    {
        CM_LOG_ERR(CM_MOD_CNM,"id null");
        return CM_PARAM_ERR;
    }
    info = (const cm_sche_info_t*)decode->data;
    return cm_sche_delete(info->id);
}

static void cm_snapshot_sche_oplog_encode_bits(uint32 type,uint32 val, sint8 *buf,uint32 len)
{
    uint32 iloop = 0;

    if(CM_CNM_SCHE_BIT_MONTH_DAY == type)
    {
        while(val)
        {
            iloop++;
            if(val & 1)
            {
                CM_SNPRINTF_ADD(buf,len,"|%u",iloop);
            }
            val >>= 1;            
        }
        return;
    }
    
    while(val)
    {            
        if(val & 1)
        {
            CM_SNPRINTF_ADD(buf,len,"|%u",iloop);
        }
        val >>= 1; 
        iloop++;
    }
    return;
}

void cm_cnm_snapshot_sche_oplog_create(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK)? CM_ALARM_LOG_SNAPSHAT_SCHE_CREATR_OK : CM_ALARM_LOG_SNAPSHAT_SCHE_CREATR_FAIL;
    const cm_cnm_decode_info_t *req = pDecodeParam;
    const uint32 cnt = 8;
    uint8 hours[CM_STRING_256] = {0};
    uint8 days[CM_STRING_256] = {0};
    
    /*<dir> <name> <expired> [-type type] [-dtype daytype] [-minute minute] [-hours hours] [-days days]*/
    if(NULL == pDecodeParam)
    {   
        cm_cnm_oplog_report(sessionid,alarmid,NULL,cnt,NULL);
    }    
    else    
    {    
        const cm_sche_info_t*info = (const cm_sche_info_t*)req->data;
        cm_snapshot_sche_oplog_encode_bits(CM_CNM_SCHE_BIT_HOUR,info->hours,hours,sizeof(hours));
        cm_snapshot_sche_oplog_encode_bits(CM_CNM_SCHE_BIT_MONTH_DAY,info->days,days,sizeof(days));
        
        cm_cnm_oplog_param_t params[] = {
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_SNAPSHOT_SCHE_DIR,strlen(info->param),info->param},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_SNAPSHOT_SCHE_NAME,strlen(info->name),info->name},
            {CM_OMI_DATA_TIME,CM_OMI_FIELD_SNAPSHOT_SCHE_EXPIRED,sizeof(info->expired),&info->expired},
                
            {CM_OMI_DATA_INT,CM_OMI_FIELD_SNAPSHOT_SCHE_TYPE,sizeof(info->type),&info->type},
            {CM_OMI_DATA_INT,CM_OMI_FIELD_SNAPSHOT_SCHE_DAYTYPE,sizeof(info->day_type),&info->day_type},
            {CM_OMI_DATA_INT,CM_OMI_FIELD_SNAPSHOT_SCHE_MINUTE,sizeof(info->minute),&info->minute},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_SNAPSHOT_SCHE_HOURS,strlen(hours),hours},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_SNAPSHOT_SCHE_DAYS,strlen(days),days},
        };
        
        cm_cnm_oplog_report(sessionid,alarmid,params,cnt,&req->set);
    }   
    return;
}

void cm_cnm_snapshot_sche_oplog_update(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK)? CM_ALARM_LOG_SNAPSHAT_SCHE_UPDATE_OK : CM_ALARM_LOG_SNAPSHAT_SCHE_UPDATE_FAIL;
    const cm_cnm_decode_info_t *req = pDecodeParam;
    const uint32 cnt = 7;
    uint8 hours[CM_STRING_256] = {0};
    uint8 days[CM_STRING_256] = {0};
    /*<id> [-type type] [-dtype daytype] [-minute minute] [-hours hours] [-days days] [-expired expired]*/
    if(NULL == pDecodeParam)
    {   
        cm_cnm_oplog_report(sessionid,alarmid,NULL,cnt,NULL);
    }    
    else    
    {    
        const cm_sche_info_t*info = (const cm_sche_info_t*)req->data;
        cm_snapshot_sche_oplog_encode_bits(CM_CNM_SCHE_BIT_HOUR,info->hours,hours,sizeof(hours));
        cm_snapshot_sche_oplog_encode_bits(CM_CNM_SCHE_BIT_MONTH_DAY,info->days,days,sizeof(days));
        cm_cnm_oplog_param_t params[] = {
            {CM_OMI_DATA_INT,CM_OMI_FIELD_SNAPSHOT_SCHE_ID,sizeof(info->id),&info->id},
            {CM_OMI_DATA_INT,CM_OMI_FIELD_SNAPSHOT_SCHE_TYPE,sizeof(info->type),&info->type},
            {CM_OMI_DATA_INT,CM_OMI_FIELD_SNAPSHOT_SCHE_DAYTYPE,sizeof(info->day_type),&info->day_type},
            {CM_OMI_DATA_INT,CM_OMI_FIELD_SNAPSHOT_SCHE_MINUTE,sizeof(info->minute),&info->minute},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_SNAPSHOT_SCHE_HOURS,strlen(hours),hours},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_SNAPSHOT_SCHE_DAYS,strlen(days),days},
            {CM_OMI_DATA_TIME,CM_OMI_FIELD_SNAPSHOT_SCHE_EXPIRED,sizeof(info->expired),&info->expired},    
        };
        
        cm_cnm_oplog_report(sessionid,alarmid,params,cnt,&req->set);
    }   
    return;
}

void cm_cnm_snapshot_sche_oplog_delete(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK)? CM_ALARM_LOG_SNAPSHAT_SCHE_DELETE_OK : CM_ALARM_LOG_SNAPSHAT_SCHE_DELETE_FAIL;
    const cm_cnm_decode_info_t *req = pDecodeParam;
    const uint32 cnt = 1;
    
    /*<id>*/
    if(NULL == pDecodeParam)
    {   
        cm_cnm_oplog_report(sessionid,alarmid,NULL,cnt,NULL);
    }    
    else    
    {    
        const cm_sche_info_t*info = (const cm_sche_info_t*)req->data;
        cm_cnm_oplog_param_t params[] = {
            {CM_OMI_DATA_INT,CM_OMI_FIELD_SNAPSHOT_SCHE_ID,sizeof(info->id),&info->id},    
        };
        
        cm_cnm_oplog_report(sessionid,alarmid,params,cnt,&req->set);
    }   
    return;
}


