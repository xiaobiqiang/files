/******************************************************************************
 *          Copyright (c) 2017 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cnm_demo.c
 * author     : wbn
 * create date: 2017年10月26日
 * description: TODO:
 *
 *****************************************************************************/

#include "cm_cnm_demo.h"
#include "cm_queue.h"
#include "cm_log.h"
#include "cm_omi.h"
#include "cm_sync.h"
#include "cm_cnm_admin.h"
#include "cm_node.h"
#include "cm_alarm.h"
#include "cm_cnm_common.h"

/*============================================================================*/
#define CM_CNM_SESSION_COUNT_MAX 32
#define CM_CNM_SESSIONT_TMOUT_MAX 1200

#define CM_CNM_SESSION_LOCK_CNT 3
#define CM_CNM_SESSION_LOCK_TMOUT 600

typedef struct
{
    sint8 id[CM_ID_LEN];
    sint8 user[CM_NAME_LEN];
    sint8 pwd[CM_PASSWORD_LEN];
    sint8 ip[CM_IP_LEN];
    uint32 level;
    cm_time_t tmout;
}cm_cnm_session_info_t;

typedef struct
{
    sint8 ip[CM_IP_LEN];
    uint32 cnt;
    cm_time_t tmout;
}cm_cnm_session_lockip_t;

static cm_list_t *pSessions = NULL;
static cm_list_t *g_cm_session_lock = NULL;

extern sint32 cm_cnm_admin_login(const sint8* name, const sint8* pwd, uint32 
*level);
static sint32 cm_cnm_session_find_node(cm_cnm_session_info_t *pNode, sint8 *id);

static sint32 cm_cnm_session_lock_find(void *pNode, sint8 *ip)
{
    cm_cnm_session_lockip_t* info = pNode;

    if(0 != strcmp(info->ip, ip))
    {
        return CM_FAIL;
    }
    return CM_OK;
}


static sint32 cm_cnm_session_lock_check_each(void *pNode, sint8 *ip)
{
    cm_cnm_session_lockip_t* info = pNode;

    if(0 != strcmp(info->ip, ip))
    {
        return CM_FAIL;
    }
    if(info->cnt < CM_CNM_SESSION_LOCK_CNT)
    {
        return CM_FAIL;
    }
    return CM_OK;
}

static sint32 cm_cnm_session_lock_check_tmout(void *pNode, void *arg)
{
    cm_time_t *tmnow = (cm_time_t*)arg;
	cm_cnm_session_lockip_t *pinfo = (cm_cnm_session_lockip_t*)pNode;

	if(pinfo->tmout > *tmnow)
	{
	    /* 不用删除 */
	    return CM_FAIL;
	}
	CM_LOG_WARNING(CM_MOD_CNM, "ip:%s", pinfo->ip);
	CM_FREE(pinfo);
	return CM_OK;
}

static sint32 cm_cnm_session_lock_get(void *pNode, sint8 *ip)
{
    cm_cnm_session_lockip_t* info = pNode;

    if(0 != strcmp(info->ip, ip))
    {
        return CM_FAIL;
    }
    info->cnt++;
    info->tmout = cm_get_time() + CM_CNM_SESSION_LOCK_TMOUT;
    return CM_OK;
}

static sint32 cm_cnm_session_lock_check(const sint8* ip)
{
    void* pNode = NULL;
    
    return cm_list_find(g_cm_session_lock, 
        (cm_list_cbk_func_t)cm_cnm_session_lock_check_each, 
        (void *)ip, (void **)&pNode);
}

static sint32 cm_cnm_session_lock_add(const sint8* ip)
{
    void* pNode = NULL;
    cm_cnm_session_lockip_t *pinfo = NULL;
    sint32 iRet = cm_list_find(g_cm_session_lock, 
        (cm_list_cbk_func_t)cm_cnm_session_lock_get, 
        (void *)ip, (void **)&pNode);

    if(CM_OK == iRet)
    {
        return CM_OK;
    }

    pinfo = CM_MALLOC(sizeof(cm_cnm_session_lockip_t));
    if(NULL == pinfo)
    {
        CM_LOG_ERR(CM_MOD_CNM, "malloc fail");
        return CM_FAIL;
    }
    CM_VSPRINTF(pinfo->ip,sizeof(pinfo->ip),"%s",ip);
    pinfo->cnt = 1;
    pinfo->tmout = cm_get_time() + CM_CNM_SESSION_LOCK_TMOUT;

    iRet = cm_list_add(g_cm_session_lock,pinfo);
    if(CM_OK != iRet)
    {
        CM_FREE(pinfo); 
        return CM_FAIL;
    }
    CM_LOG_WARNING(CM_MOD_CNM, "ip:%s",ip);
    return CM_OK;    
}

static sint32 cm_cnm_session_lock_delete(const sint8* ip)
{
    return cm_list_check_delete(g_cm_session_lock, (cm_list_cbk_func_t)cm_cnm_session_lock_find,(void*)ip);
}
static sint32 cm_cnm_session_update(const sint8* id,uint32 *level)
{
	cm_cnm_session_info_t *pNode = NULL;
	sint32 iRet = CM_FAIL;
#ifndef CM_OMI_LOCAL	
    uint32 masterid = cm_node_get_master();
    uint32 myid = cm_node_get_id();

    if((CM_NODE_ID_NONE != masterid)
        && (myid != masterid))
    {
        CM_LOG_ERR(CM_MOD_CNM, "masterid[%u] myid[%u]",masterid,myid);
        return CM_ERR_REDIRECT_MASTER;
    }
#endif
	iRet = cm_list_find(pSessions, (cm_list_cbk_func_t)cm_cnm_session_find_node, (void *)id, (void **)&pNode);
	if(CM_OK != iRet)
	{
		CM_LOG_INFO(CM_MOD_CNM, "find node[%s] fail", id);
		return CM_NEED_LOGIN;
	}
	pNode->tmout = cm_get_time() + CM_CNM_SESSIONT_TMOUT_MAX;
    *level = pNode->level;
	return iRet;
}

sint32 cm_session_new(const sint8* name, const sint8* ip, sint8* id, uint32 
len, uint32 level)
{
    /* 桩函数 */
    sint8 buff[CM_STRING_256] = {0};
	sint32 iRet = CM_FAIL;
	cm_cnm_session_info_t *s_node = NULL;
    cm_time_t tm = cm_get_time();
    
    CM_VSPRINTF(buff,sizeof(buff),"%lu,%s,%s,%u",
        tm,name,ip,(uint32)CM_THREAD_MYID());

    iRet = cm_get_md5sum(buff,id,len);
	if(CM_OK != iRet){
		CM_LOG_ERR(CM_MOD_CNM, "Make session id fail.");
		return iRet;
	}
	s_node = CM_MALLOC(sizeof(cm_cnm_session_info_t));
	if(NULL == s_node){
		CM_LOG_ERR(CM_MOD_CNM, "Malloc fail.");
		return CM_FAIL;
	}

    CM_LOG_INFO(CM_MOD_CNM, "ip:%s,name:%s,p:%p", ip, name,s_node); 
	CM_MEM_ZERO(s_node, sizeof(cm_cnm_session_info_t));
	s_node->tmout = tm + CM_CNM_SESSIONT_TMOUT_MAX;
	CM_MEM_CPY(s_node->id, CM_ID_LEN, id, CM_ID_LEN);
	CM_MEM_CPY(s_node->user, CM_NAME_LEN, name, CM_NAME_LEN);
	CM_MEM_CPY(s_node->ip, sizeof(s_node->ip), ip, strlen(ip)+1);
	s_node->level = level;
	iRet = cm_list_add(pSessions, (void *)s_node);
	if(CM_OK != iRet){
		CM_LOG_ERR(CM_MOD_CNM, "Add fail.");
		CM_FREE(s_node);
		return CM_FAIL;
	}
	return CM_OK;
}

static sint32 cm_cnm_session_handle_node(void *pNode, void *arg)
{
	cm_time_t *tmnow = (cm_time_t*)arg;
	cm_cnm_session_info_t *pinfo = (cm_cnm_session_info_t*)pNode;

	if(pinfo->tmout > *tmnow)
	{
	    /* 不用删除 */
	    return CM_FAIL;
	}
	CM_LOG_INFO(CM_MOD_CNM, "ip:%s,name:%s,p:%p", pinfo->ip,pinfo->user,pinfo);
	CM_FREE(pinfo);
	return CM_OK;
}

static void* cm_cnm_session_thread(void *arg)
{
    cm_time_t tmnow;
    
	while(1)
	{
	    tmnow = cm_get_time();
		cm_list_check_delete(pSessions, (cm_list_cbk_func_t)cm_cnm_session_handle_node,&tmnow);
		cm_list_check_delete(g_cm_session_lock, (cm_list_cbk_func_t)cm_cnm_session_lock_check_tmout,&tmnow);
		CM_SLEEP_S(5);
	}
	return NULL;
}

sint32 cm_cnm_session_init(void)
{
	sint32 iRet = CM_FAIL;
	cm_thread_t Handle;

	iRet = cm_list_init(&pSessions, CM_CNM_SESSION_COUNT_MAX);
	if(CM_OK != iRet){
		CM_FREE(pSessions);
		CM_LOG_ERR(CM_MOD_CNM, "Init list fail!");
		return CM_FAIL;
	}

	iRet = cm_list_init(&g_cm_session_lock, 256);
	if(CM_OK != iRet){
		(void)cm_list_destory(pSessions);
		CM_LOG_ERR(CM_MOD_CNM, "Init list fail!");
		return CM_FAIL;
	}

	iRet = CM_THREAD_CREATE(&Handle, cm_cnm_session_thread, NULL);
	if(CM_OK != iRet)
	{
		(void)cm_list_destory(pSessions);
		(void)cm_list_destory(g_cm_session_lock);
		CM_LOG_ERR(CM_MOD_CNM, "create thread fail!");
		return CM_FAIL;
	}
	CM_THREAD_DETACH(Handle);

    cm_omi_session_cbk_reg(cm_cnm_session_update);
    return CM_OK;
}

sint32 cm_cnm_session_decode(const cm_omi_obj_t ObjParam, void
**ppDecodeParam)
{
    cm_cnm_session_info_t *pSession = CM_MALLOC(sizeof(cm_cnm_session_info_t));

    if(NULL == pSession)
    {
        CM_LOG_ERR(CM_MOD_CNM,"malloc fail");
        return CM_FAIL;
    }
    CM_MEM_ZERO(pSession,sizeof(cm_cnm_session_info_t));
    cm_omi_obj_key_get_str_ex(ObjParam,CM_OMI_FIELD_SESSION_ID,
        pSession->id, sizeof(pSession->id));
    cm_omi_obj_key_get_str_ex(ObjParam,CM_OMI_FIELD_SESSION_USER,
        pSession->user, sizeof(pSession->user));
    cm_omi_obj_key_get_str_ex(ObjParam,CM_OMI_FIELD_SESSION_PWD,
        pSession->pwd, sizeof(pSession->pwd));
    cm_omi_obj_key_get_str_ex(ObjParam,CM_OMI_FIELD_SESSION_IP,
        pSession->ip, sizeof(pSession->ip));
    *ppDecodeParam = pSession;
    return CM_OK;
}

cm_omi_obj_t cm_cnm_session_encode(const void *pDecodeParam, void *pAckData, uint32 AckLen)
{
    cm_cnm_session_info_t *pSession = pAckData;
    cm_omi_obj_t items = NULL;
    cm_omi_obj_t item = NULL;

    if(NULL == pSession)
    {
        return NULL;
    }
    items = cm_omi_obj_new_array();
    if(NULL == items)
    {
        CM_LOG_ERR(CM_MOD_CNM,"new items fail");
        return NULL;
    }
    item = cm_omi_obj_new();
    if(NULL == item)
    {
        CM_LOG_ERR(CM_MOD_CNM,"new item fail");
        return items;
    }
    (void)cm_omi_obj_key_set_str_ex(item,CM_OMI_FIELD_SESSION_ID,pSession->id);
    (void)cm_omi_obj_key_set_u32_ex(item,CM_OMI_FIELD_SESSION_LEVEL,pSession->level);
    if(CM_OK != cm_omi_obj_array_add(items,item))
    {
        CM_LOG_ERR(CM_MOD_CNM,"add item fail");
        cm_omi_obj_delete(item);
    }
    return items;
}

sint32 cm_cnm_session_get(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    const cm_cnm_session_info_t *pSession = pDecodeParam;
    cm_cnm_session_info_t *pAck = NULL;
    sint32 iRet = CM_FAIL;
    
    if(NULL == pSession)
    {
        return CM_PARAM_ERR;
    }
    if('\0' == *(pSession->user))
    {
        CM_LOG_ERR(CM_MOD_CNM,"user null");
        return CM_PARAM_ERR;
    }
    if('\0' == *(pSession->pwd))
    {
        CM_LOG_ERR(CM_MOD_CNM,"pwd null");
        return CM_PARAM_ERR;
    }
    if('\0' == *(pSession->ip))
    {
        CM_LOG_ERR(CM_MOD_CNM,"ip null");
        return CM_PARAM_ERR;
    }
    
    CM_LOG_INFO(CM_MOD_CNM,"u[%s] i[%s]",pSession->user,pSession->ip);
    if(CM_OK == cm_cnm_session_lock_check(pSession->ip))
    {
        CM_LOG_INFO(CM_MOD_CNM, "ip:%s lock",pSession->ip);
        return CM_ERR_LOGIN_LOCK;
    }
    
    pAck = CM_MALLOC(sizeof(cm_cnm_session_info_t));
    if(NULL == pAck)
    {
        CM_LOG_ERR(CM_MOD_CNM,"malloc fail");
        return CM_FAIL;
    }
    
	iRet = cm_cnm_admin_login(pSession->user, pSession->pwd,&pAck->level);
	if(CM_OK != iRet)
	{
		CM_FREE(pAck);
		CM_LOG_ERR(CM_MOD_CNM, "login fail[%d]", iRet);
		(void)cm_cnm_session_lock_add(pSession->ip);
		return iRet;
	}
	else
	{
	    (void)cm_cnm_session_lock_delete(pSession->ip);
	}

    iRet = cm_session_new(pSession->user,pSession->ip,pAck->id,sizeof(pAck->id),pAck->level);
    if(CM_OK != iRet)
    {
        CM_FREE(pAck);
        CM_LOG_ERR(CM_MOD_CNM,"new fail[%d]",iRet);
        return CM_FAIL;
    }

    *ppAckData = pAck;
    *pAckLen = sizeof(cm_cnm_session_info_t);
    return CM_OK;
}


static sint32 cm_cnm_session_find_node(cm_cnm_session_info_t *pNode, sint8 *id)
{
	return strcmp(pNode->id, id) ? CM_FAIL : CM_OK;
}

sint32 cm_cnm_session_delete(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    const cm_cnm_session_info_t *pSession = pDecodeParam;
	cm_cnm_session_info_t *s_node = NULL;
    sint32 iRet = CM_FAIL;
    
    if(NULL == pSession)
    {
        return CM_PARAM_ERR;
    }
    if('\0' == *(pSession->id))
    {
        CM_LOG_ERR(CM_MOD_CNM,"id null");
        return CM_PARAM_ERR;
    }
    //CM_LOG_INFO(CM_MOD_CNM,"id[%s]",pSession->id);
	iRet = cm_list_find_delete(pSessions, (cm_list_cbk_func_t)cm_cnm_session_find_node, (void *)pSession->id, (void **)&s_node);
	if(CM_OK != iRet)
	{
		CM_LOG_INFO(CM_MOD_CNM, "delete id[%s] fail",pSession->id);
		return CM_OK;
	}
	CM_LOG_WARNING(CM_MOD_CNM, "ip:%s,name:%s,p:%p", s_node->ip, s_node->user ,s_node); 
	CM_FREE(s_node);
    return CM_OK;
}

typedef struct
{
    const sint8 *id;
    sint8 *buff;
    uint32 len;
}cm_cnm_session_log_data_t;

static sint32 cm_cnm_session_find_log(void *pNodeData, void *pArg)
{
    cm_cnm_session_log_data_t *data = pArg;
    cm_cnm_session_info_t *info = pNodeData;

    if(0 != strcmp(info->id, data->id))
    {
        return CM_FAIL;
    }
    CM_VSPRINTF(data->buff,data->len,"%s,%s",info->user,info->ip);
    return CM_OK;
}

static void cm_cnm_session_log(const sint8 *id, sint8 *buff, uint32 len)
{
    cm_cnm_session_log_data_t data = {id, buff,len};
    void *pFindData = NULL;
    sint32 iRet = CM_OK;

    if(NULL == id)
    {
        CM_VSPRINTF(data.buff,data.len,"-,-");
        return;
    }

    iRet = cm_list_find(pSessions,cm_cnm_session_find_log,&data,&pFindData);
    if(CM_OK != iRet)
    {
        CM_VSPRINTF(data.buff,data.len,"-,-");
    }
    return;
}
/*函数描述:将字符串中的一个字符替换为另一个字符，
           并限制长度，如果长度超过裁剪值，则将显示裁剪值范围内的值，
           并后缀...*/
static uint32 strrepchr(sint8* strSrc,sint8 chrSrc,sint8 chrDes,uint32 trunLen)
{
    sint8* tmpSrc = strSrc;
    const sint8* strtail = "...";
    sint8 * changePoint = NULL;
    uint32 tailLen = strlen(strtail);
    uint32 tmplen = 0;

    if(tmpSrc == NULL)
    {
        CM_LOG_ERR(CM_MOD_ALARM,"strSrc is NULL");
        return CM_FAIL;
    }
    for(;*tmpSrc && (tmplen + 1 ) < trunLen;tmpSrc++,tmplen++)
    {
        if(*tmpSrc == chrSrc )
        {
            *tmpSrc = chrDes;
        }
    }
    
    if(0 != *tmpSrc)
    {
        for(tmpSrc = tmpSrc - tailLen;tmpSrc > strSrc;tmpSrc--)
        {
            if(*tmpSrc == chrDes)
            {
                break;
            }
        }
        CM_VSPRINTF(tmpSrc,strlen(tmpSrc) + 1,strtail);
    }
    return CM_OK;
}


void cm_cnm_oplog_report(const sint8 *sessionid,uint32 alarmid,
    const cm_cnm_oplog_param_t* params, uint32 cnt,
    const cm_omi_field_flag_t* set)
{
    sint8 alarmparam[CM_STRING_512] = {0};
    uint32 len = sizeof(alarmparam);
    uint32 max_param_len = 72;    
    sint32 iRet = CM_OK;
    uint32 str_len = 0;
    
    if(NULL == set)
    {
        for(;cnt > 0; cnt--)
        {
            CM_SNPRINTF_ADD(alarmparam,len,",-");
        }
        iRet = CM_ALARM_REPORT(alarmid,alarmparam);
        CM_LOG_WARNING(CM_MOD_CNM, "%u %s %d",alarmid,alarmparam,iRet);
        return;
    }
    
    cm_cnm_session_log(sessionid,alarmparam,len);
    for(;cnt > 0; cnt--,params++)
    {
        if((!CM_OMI_FIELDS_FLAG_ISSET(set,params->id))
            ||(0 == params->size) 
            || (NULL == params->data))
        {
            CM_SNPRINTF_ADD(alarmparam,len,",-");
            continue;
        }
        switch(params->type)
        {
            case CM_OMI_DATA_STRING:
                str_len = len - strlen(alarmparam);
                str_len = CM_MIN(max_param_len,str_len);                
                str_len = CM_MIN(params->size,str_len);  
                str_len += 2;
                
                strrepchr((sint8*)params->data,',',' ',max_param_len);
                CM_VSPRINTF(alarmparam+strlen(alarmparam),str_len,",%s",(const sint8*)params->data);
                break;
            case CM_OMI_DATA_INT:
            case CM_OMI_DATA_ENUM:
                switch(params->size)
                {
                    case 1:
                        CM_SNPRINTF_ADD(alarmparam,len,",%u",*((const uint8*)params->data));
                        break;
                    case 2:
                        CM_SNPRINTF_ADD(alarmparam,len,",%u",*((const uint16*)params->data));
                        break;
                    case 4:
                        CM_SNPRINTF_ADD(alarmparam,len,",%u",*((const uint32*)params->data));
                        break;
                    case 8:
                        CM_SNPRINTF_ADD(alarmparam,len,",%llu",*((const uint64*)params->data));
                        break;
                    default:
                        CM_SNPRINTF_ADD(alarmparam,len,",0");
                        break;
                }            
                break;
            case CM_OMI_DATA_DOUBLE:
                CM_SNPRINTF_ADD(alarmparam,len,",%.02f",*((const double*)params->data));
                break;
            case CM_OMI_DATA_TIME:
                {
                    struct tm tin;
                    cm_get_datetime_ext(&tin,*(const cm_time_t*)params->data);
                    CM_SNPRINTF_ADD(alarmparam,len,",%04d-%02d-%02d %02d:%02d:%02d",
                        tin.tm_year,tin.tm_mon,tin.tm_mday,
                        tin.tm_hour,tin.tm_min,tin.tm_sec);
                }
                break;
            default:
                CM_SNPRINTF_ADD(alarmparam,len,",x");
                break;
        }
    }
    
    iRet = CM_ALARM_REPORT(alarmid,alarmparam);
    CM_LOG_WARNING(CM_MOD_CNM, "%u %s %d",alarmid,alarmparam,iRet);
    return;
}

/*============================================================================*/
typedef struct
{
    sint8 ipaddr[CM_IP_LEN];
    uint8 type;
}cm_cnm_remote_decode_t;
typedef enum
{
    CM_CNM_REMOTE_TYPE_POOL=0,
    CM_CNM_REMOTE_TYPE_LUN,
    CM_CNM_REMOTE_TYPE_NAS,
    CM_CNM_REMOTE_TYPE_SNAPSHOT,
    CM_CNM_REMOTE_TYPE_BUTT
}cm_cnm_remote_type_e;

typedef struct
{
    sint8 name[CM_NAME_LEN];
    sint8 avail[CM_STRING_32];
    sint8 rdcip[CM_STRING_64];
    sint8 quota[CM_STRING_32];
}cm_cnm_remote_info_t;


sint32 cm_cnm_remote_init(void)
{
    return CM_OK;
}

static sint32 cm_cnm_remote_decode_check_type(void *val)
{
    uint8 type = *((uint8*)val);
    if(type >= CM_CNM_REMOTE_TYPE_BUTT)
    {
        CM_LOG_ERR(CM_MOD_CNM,"type[%u]",type);
        return CM_PARAM_ERR;
    }
    return CM_OK;
}

static sint32 cm_cnm_remote_decode_ext(
    const cm_omi_obj_t ObjParam,void* data,cm_omi_field_flag_t *set)
{
    sint32 iRet = CM_OK;
    cm_cnm_remote_decode_t *info = data;
    cm_cnm_decode_param_t param_str[] = 
    {
        {CM_OMI_FIELD_REMOTE_IPADDR,sizeof(info->ipaddr),info->ipaddr,NULL},
    };

    cm_cnm_decode_param_t param_num[] = 
    {
        {CM_OMI_FIELD_REMOTE_TYPE,sizeof(info->type),&info->type,cm_cnm_remote_decode_check_type},
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

    if(!CM_OMI_FIELDS_FLAG_ISSET(set,CM_OMI_FIELD_REMOTE_IPADDR))
    {
        CM_LOG_ERR(CM_MOD_CNM,"ipaddr null");
        return CM_PARAM_ERR;
    }
    if(!CM_OMI_FIELDS_FLAG_ISSET(set,CM_OMI_FIELD_REMOTE_TYPE))
    {
        CM_LOG_ERR(CM_MOD_CNM,"type null");
        return CM_PARAM_ERR;
    }
    return CM_OK;
}

sint32 cm_cnm_remote_decode(const cm_omi_obj_t ObjParam, void **ppDecodeParam)
{
    return cm_cnm_decode_comm(ObjParam,sizeof(cm_cnm_remote_decode_t),
        cm_cnm_remote_decode_ext,ppDecodeParam);
}

static void cm_cnm_remote_encode_each(cm_omi_obj_t item,void *eachdata,void *arg)
{
    cm_omi_field_flag_t *field = arg;
    cm_cnm_remote_info_t *info = eachdata;
    cm_cnm_map_value_str_t cols_str[] = 
    {
        {CM_OMI_FIELD_REMOTE_NAME,     info->name},
        {CM_OMI_FIELD_REMOTE_AVAIL,     info->avail},
        {CM_OMI_FIELD_REMOTE_RDCIP,     info->rdcip},
        {CM_OMI_FIELD_REMOTE_QUOTA,     info->quota},
    };
    cm_cnm_encode_str(item,field,cols_str,sizeof(cols_str)/sizeof(cm_cnm_map_value_str_t));
    return;
}

cm_omi_obj_t cm_cnm_remote_encode(const void *pDecodeParam, void *pAckData, uint32 AckLen)
{
    return cm_cnm_encode_comm_ext(pDecodeParam,pAckData,AckLen,
        sizeof(cm_cnm_remote_info_t),cm_cnm_remote_encode_each);
}

static sint32 cm_cnm_remote_get_each(void *arg, sint8 **cols, uint32 col_num)
{
    cm_cnm_remote_info_t* info=arg;
    if(col_num < 3)
    {
        CM_LOG_ERR(CM_MOD_CNM,"col_num %u",col_num);
        return CM_FAIL;
    }
    CM_MEM_ZERO(info,sizeof(cm_cnm_remote_info_t));
    CM_VSPRINTF(info->name,sizeof(info->name),"%s",cols[0]);
    CM_VSPRINTF(info->avail,sizeof(info->avail),"%s",cols[1]);
    CM_VSPRINTF(info->rdcip,sizeof(info->rdcip),"%s",cols[2]);
    if(col_num > 3)
    {
        CM_VSPRINTF(info->quota,sizeof(info->quota),"%s",cols[3]);
    }
    return CM_OK;
}

sint32 cm_cnm_remote_getbatch(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    const cm_cnm_decode_info_t *decode = pDecodeParam;
    const cm_cnm_remote_decode_t *info = NULL;
    sint8 cmd[CM_STRING_512] = {0};
    sint32 iRet = CM_OK;
    uint32 total = 0;
    
    if(NULL == pDecodeParam)
    {
        CM_LOG_ERR(CM_MOD_CNM,"param null");
        return CM_PARAM_ERR;
    }
    info = (const cm_cnm_remote_decode_t*)&decode->data;
    CM_VSPRINTF(cmd,sizeof(cmd),"ceres_exec '%s' \"zfs list -H -o name,avail,rdc:ip_owner",info->ipaddr);
    switch(info->type)
    {
        case CM_CNM_REMOTE_TYPE_POOL:
            CM_SNPRINTF_ADD(cmd,sizeof(cmd)," -t filesystem 2>/dev/null"
                " |grep -v \"/\"");
            break;
        case CM_CNM_REMOTE_TYPE_LUN:
            CM_SNPRINTF_ADD(cmd,sizeof(cmd),",volsize -t volume |grep -v '_bit0' 2>/dev/null");
            break;
        case CM_CNM_REMOTE_TYPE_NAS:
            CM_SNPRINTF_ADD(cmd,sizeof(cmd),",quota -t filesystem 2>/dev/null"
                " |grep \"/\"");
            break;
        case CM_CNM_REMOTE_TYPE_SNAPSHOT:
            CM_SNPRINTF_ADD(cmd,sizeof(cmd)," -t snapshot 2>/dev/null");
            break;
        default:
            return CM_ERR_NOT_SUPPORT;
    }
    CM_SNPRINTF_ADD(cmd,sizeof(cmd)," |sed -n '%llu,%llup'\"",
        decode->offset+1,decode->offset+decode->total);

    total = decode->total;
    iRet = cm_cnm_exec_get_list(cmd,cm_cnm_remote_get_each,
        (uint32)0,sizeof(cm_cnm_remote_info_t),ppAckData,&total);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]",iRet);
        return iRet;
    }
    *pAckLen = total * sizeof(cm_cnm_remote_info_t);    
    return CM_OK;
}

sint32 cm_cnm_remote_count(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    const cm_cnm_decode_info_t *decode = pDecodeParam;
    const cm_cnm_remote_decode_t *info = NULL;
    uint64 cnt = 0;
    sint8 cmd[CM_STRING_512] = {0};
    
    if(NULL == pDecodeParam)
    {
        CM_LOG_ERR(CM_MOD_CNM,"param null");
        return CM_PARAM_ERR;
    }
    info = (const cm_cnm_remote_decode_t*)&decode->data;
    CM_VSPRINTF(cmd,sizeof(cmd),"ceres_exec '%s' \"zfs list -H -o name",info->ipaddr);
    switch(info->type)
    {
        case CM_CNM_REMOTE_TYPE_POOL:
            CM_SNPRINTF_ADD(cmd,sizeof(cmd)," -t filesystem 2>/dev/null"
                " |grep -v \"/\"");
            break;
        case CM_CNM_REMOTE_TYPE_LUN:
            CM_SNPRINTF_ADD(cmd,sizeof(cmd)," -t volume |grep -v '_bit0' 2>/dev/null");
            break;
        case CM_CNM_REMOTE_TYPE_NAS:
            CM_SNPRINTF_ADD(cmd,sizeof(cmd)," -t filesystem 2>/dev/null"
                " |grep \"/\"");
            break;
        case CM_CNM_REMOTE_TYPE_SNAPSHOT:
            CM_SNPRINTF_ADD(cmd,sizeof(cmd)," -t snapshot 2>/dev/null");
            break;
        default:
            return CM_ERR_NOT_SUPPORT;
    }
    CM_SNPRINTF_ADD(cmd,sizeof(cmd)," |wc -l\"");
    cnt = (uint64)cm_exec_int("%s",cmd);
    return cm_cnm_ack_uint64(cnt,ppAckData,pAckLen);
}


