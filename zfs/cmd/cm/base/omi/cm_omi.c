/******************************************************************************
 *          Copyright (c) 2017 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_omi.c
 * author     : wbn
 * create date: 2017年10月23日
 * description: TODO:
 *
 *****************************************************************************/

#include "cm_omi.h"
#include "cm_log.h"
#define CM_OMI_MAX_NUM_LEN 32
extern const cm_omi_map_object_t** g_CmOmiObjCfg;

sint32 cm_omi_obj_key_get_s32(cm_omi_obj_t obj, const sint8* key, sint32 *val)
{
    sint8 buf[CM_OMI_MAX_NUM_LEN] = {0};
    sint32 iRet = cm_omi_obj_key_get_str(obj, key, buf, sizeof(buf));

    if(CM_OK != iRet)
    {
        return iRet;
    }

    /* TODO:校验是否纯数字 */

    *val = atoi(buf);
    return CM_OK;
}

sint32 cm_omi_obj_key_set_s32(cm_omi_obj_t obj, const sint8* key, sint32 val)
{
    sint8 buf[CM_OMI_MAX_NUM_LEN] = {0};

    CM_VSPRINTF(buf, sizeof(buf) - 1, "%d", val);
    return cm_omi_obj_key_set_str(obj, key, buf);
}

sint32 cm_omi_obj_key_get_u32(cm_omi_obj_t obj, const sint8* key, uint32 *val)
{
    sint8 buf[CM_OMI_MAX_NUM_LEN] = {0};
    sint32 iRet = cm_omi_obj_key_get_str(obj, key, buf, sizeof(buf));

    if(CM_OK != iRet)
    {
        return iRet;
    }

    /* TODO:校验是否纯数字 */

    *val = atol(buf);
    return CM_OK;
}

sint32 cm_omi_obj_key_set_u32(cm_omi_obj_t obj, const sint8* key, uint32 val)
{
    sint8 buf[CM_OMI_MAX_NUM_LEN] = {0};

    CM_VSPRINTF(buf, sizeof(buf) - 1, "%u", val);
    return cm_omi_obj_key_set_str(obj, key, buf);
}

sint32 cm_omi_obj_key_get_u64(cm_omi_obj_t obj, const sint8* key, uint64 *val)
{
    sint8 buf[CM_OMI_MAX_NUM_LEN] = {0};
    sint32 iRet = cm_omi_obj_key_get_str(obj, key, buf, sizeof(buf));

    if(CM_OK != iRet)
    {
        return iRet;
    }

    /* TODO:校验是否纯数字 */

    *val = (uint64)atoll(buf);
    return CM_OK;
}

sint32 cm_omi_obj_key_set_u64(cm_omi_obj_t obj, const sint8* key, uint64 val)
{
    sint8 buf[CM_OMI_MAX_NUM_LEN] = {0};

    CM_VSPRINTF(buf, sizeof(buf) - 1, "%llu", val);
    return cm_omi_obj_key_set_str(obj, key, buf);
}

sint32 cm_omi_obj_key_set_str_ex(cm_omi_obj_t obj, uint32 key, const sint8* val)
{
    sint8 buf[CM_OMI_MAX_NUM_LEN] = {0};

    CM_VSPRINTF(buf, sizeof(buf) - 1, "%u", key);
    return cm_omi_obj_key_set_str(obj, buf, val);
}

sint32 cm_omi_obj_key_get_str_ex(cm_omi_obj_t obj, uint32 key, sint8* val, uint32 len)
{
    sint8 buf[CM_OMI_MAX_NUM_LEN] = {0};

    CM_VSPRINTF(buf, sizeof(buf) - 1, "%u", key);
    return cm_omi_obj_key_get_str(obj, buf, val, len);
}

sint32 cm_omi_obj_key_get_s32_ex(cm_omi_obj_t obj, uint32 key, sint32 *val)
{
    sint8 buf[CM_OMI_MAX_NUM_LEN] = {0};

    CM_VSPRINTF(buf, sizeof(buf) - 1, "%u", key);
    return cm_omi_obj_key_get_s32(obj, buf, val);
}
sint32 cm_omi_obj_key_set_s32_ex(cm_omi_obj_t obj, uint32 key, sint32 val)
{
    sint8 buf[CM_OMI_MAX_NUM_LEN] = {0};

    CM_VSPRINTF(buf, sizeof(buf) - 1, "%u", key);
    return cm_omi_obj_key_set_s32(obj, buf, val);
}
sint32 cm_omi_obj_key_get_u32_ex(cm_omi_obj_t obj, uint32 key, uint32 *val)
{
    sint8 buf[CM_OMI_MAX_NUM_LEN] = {0};

    CM_VSPRINTF(buf, sizeof(buf) - 1, "%u", key);
    return cm_omi_obj_key_get_u32(obj, buf, val);
}
sint32 cm_omi_obj_key_set_u32_ex(cm_omi_obj_t obj, uint32 key, uint32 val)
{
    sint8 buf[CM_OMI_MAX_NUM_LEN] = {0};

    CM_VSPRINTF(buf, sizeof(buf) - 1, "%u", key);
    return cm_omi_obj_key_set_u32(obj, buf, val);
}
sint32 cm_omi_obj_key_get_u64_ex(cm_omi_obj_t obj, uint32 key, uint64 *val)
{
    sint8 buf[CM_OMI_MAX_NUM_LEN] = {0};

    CM_VSPRINTF(buf, sizeof(buf) - 1, "%u", key);
    return cm_omi_obj_key_get_u64(obj, buf, val);
}
sint32 cm_omi_obj_key_set_u64_ex(cm_omi_obj_t obj, uint32 key, uint64 val)
{
    sint8 buf[CM_OMI_MAX_NUM_LEN] = {0};

    CM_VSPRINTF(buf, sizeof(buf) - 1, "%u", key);
    return cm_omi_obj_key_set_u64(obj, buf, val);
}

cm_omi_obj_t cm_omi_obj_key_find_ex(cm_omi_obj_t obj, uint32 key)
{
    sint8 buf[CM_OMI_MAX_NUM_LEN] = {0};

    CM_VSPRINTF(buf, sizeof(buf) - 1, "%u", key);
    return cm_omi_obj_key_find(obj, buf);
}

sint32 cm_omi_obj_key_set_double_ex(cm_omi_obj_t obj, uint32 key, double val)
{
    sint8 buf_key[CM_OMI_MAX_NUM_LEN] = {0};
    sint8 buf_val[CM_OMI_MAX_NUM_LEN] = {0};

    CM_VSPRINTF(buf_key, sizeof(buf_key) - 1, "%u", key);
    CM_VSPRINTF(buf_val, sizeof(buf_val) - 1, "%.02lf", val);
    return cm_omi_obj_key_set_str(obj, buf_key, buf_val);
}


cm_omi_obj_t cm_omi_encode_num(
    uint32 key,const void *pDecodeParam, void *pAckData, uint32 AckLen)
{
    cm_omi_obj_t items = NULL;
    cm_omi_obj_t item = NULL;
    uint64 *pData = (uint64*)pAckData;
    uint32 count = AckLen / sizeof(uint64);

    if(0 == count)
    {
        return NULL;
    }

    items = cm_omi_obj_new_array();

    if(NULL == items)
    {
        CM_LOG_ERR(CM_MOD_CNM, "new items fail");
        return NULL;
    }

    item = cm_omi_obj_new();

    if(NULL == item)
    {
        CM_LOG_ERR(CM_MOD_CNM, "new item fail");
        return items;
    }
    /* CM_OMI_FIELD_COUNT 259 */
    (void)cm_omi_obj_key_set_u64_ex(item, key, *pData);

    if(CM_OK != cm_omi_obj_array_add(items, item))
    {
        CM_LOG_ERR(CM_MOD_CNM, "add item fail");
        cm_omi_obj_delete(item);
    }

    return items;
}

cm_omi_obj_t cm_omi_encode_count(
    const void *pDecodeParam, void *pAckData, uint32 AckLen)
{
    return cm_omi_encode_num(259,pDecodeParam,pAckData,AckLen);
}

cm_omi_obj_t cm_omi_encode_taskid(
    const void *pDecodeParam, void *pAckData, uint32 AckLen)
{
    return cm_omi_encode_num(266,pDecodeParam,pAckData,AckLen);
}


uint32 cm_omi_get_fields(const sint8 *fields_str, sint8 **fields, uint32 max)
{
    const sint8 *ptemp = fields_str;
    uint32 cnt = 0;
    if(NULL == ptemp)
    {
        return 0;
    }

    
    while(cnt < max)
    {
        *fields = ptemp;
        if(('\0' != *ptemp) && (',' != *ptemp))
        {
            fields++;
            cnt++;
        }
        
        ptemp = strstr(ptemp,",");
        if(NULL == ptemp)
        {
            break;
        }
        ptemp++;        
    }
    return cnt;
}


const cm_omi_map_object_t* cm_omi_get_map_obj_cfg(uint32 obj)
{
    return g_CmOmiObjCfg[obj];
}

void cm_omi_decode_fields_flag(cm_omi_obj_t obj_param, cm_omi_field_flag_t *pflag)
{
    sint32 iRet = CM_OK;
    sint8 fields[CM_STRING_256] = {0};
    uint32 cnt = 0;
    sint8 *pfield[CM_OMI_FIELD_MAX_NUM] = {NULL};

    /* CM_OMI_FIELD_FIELDS 256 */
    iRet = cm_omi_obj_key_get_str_ex(obj_param,256,
        fields, sizeof(fields));
    if(CM_OK != iRet)
    {
        CM_OMI_FIELDS_FLAG_SET_ALL(pflag);
        return;
    }
    
    CM_OMI_FIELDS_FLAG_CLR_ALL(pflag);
    cnt = cm_omi_get_fields(fields,pfield,CM_OMI_FIELD_MAX_NUM);
    
    while(cnt > 0)
    {
        cnt--;
        iRet = atoi(pfield[cnt]);
        if(CM_OMI_FIELD_MAX_NUM > iRet)
        {
            CM_OMI_FIELDS_FLAG_SET(pflag,iRet);
        }        
    }
    return;
}

sint32 cm_omi_encode_db_record_each(
    void *arg, sint32 col_cnt, sint8 **col_vals, sint8 **col_names)
{
    cm_omi_obj_t items = arg;
    cm_omi_obj_t item = cm_omi_obj_new();
    sint32 iRet = CM_OK;
    
    if(NULL == item)
    {
        CM_LOG_ERR(CM_MOD_CNM,"new item fail");
        return CM_FAIL;
    }
    while(col_cnt > 0)
    {
        col_cnt--;
        iRet = cm_omi_obj_key_set_str(item,(*col_names + 1),*col_vals);
        if(CM_OK != iRet)
        {
            CM_LOG_ERR(CM_MOD_CNM,"set key[%s] val[%s] fail",(*col_names + 1),*col_vals);
            cm_omi_obj_delete(item);
            return CM_FAIL;
        }
        col_vals++;
        col_names++;
    }
    if(CM_OK != cm_omi_obj_array_add(items,item))
    {
        CM_LOG_ERR(CM_MOD_CNM,"add item fail");
        cm_omi_obj_delete(item);
        return CM_FAIL;
    }
    return CM_OK;
}

//之前这函数存在重大缺陷:当查询的字段一个都不是往数据库查的时候一定会出错
void cm_omi_make_select_field(const sint8** all_fields, uint32 cnt,
    const cm_omi_field_flag_t *set_flag,sint8 *sql, uint32 buf_size)
{
    bool_t is_first = CM_TRUE;
    uint32 iloop = 0;
    uint32 len = 0;
    
    for(iloop = 0;iloop<cnt;iloop++)
    {
        if(!CM_OMI_FIELDS_FLAG_ISSET(set_flag,iloop))
        {
            continue;
        }
        
        if(NULL == all_fields[iloop])
        {
            continue;
        }
        
        if(CM_FALSE == is_first)
        {
            len += CM_VSPRINTF(sql+len,buf_size-len,",%s",all_fields[iloop]);
        }
        else
        {
            len += CM_VSPRINTF(sql+len,buf_size-len,"SELECT %s",all_fields[iloop]);
            is_first = CM_FALSE;
        }
    }
    return;
}

void cm_omi_make_select_cond(
    const uint32 *col_ids, 
    const sint8** col_names,
    const uint32 *col_vals,
    uint32 col_cnt,
    const cm_omi_field_flag_t *set_flag,sint8 *sql, uint32 buf_size)
{
    bool_t is_first = CM_TRUE;
    uint32 iloop = 0;
    uint32 len = 0;
    
    for(iloop = 0;iloop<col_cnt;iloop++,col_ids++,col_names++,col_vals++)
    {
        if(!CM_OMI_FIELDS_FLAG_ISSET(set_flag,*col_ids))
        {
            continue;
        }
        
        if(NULL == *col_names)
        {
            continue;
        }
        
        if(CM_FALSE == is_first)
        {
            len += CM_VSPRINTF(sql+len,buf_size-len," AND %s=%u",*col_names,*col_vals);
        }
        else
        {
            len += CM_VSPRINTF(sql+len,buf_size-len," WHERE %s=%u",*col_names,*col_vals);
            is_first = CM_FALSE;
        }
    }
    return;
}

extern const uint32* g_CmOmiNoCheck;

sint32 cm_omi_check_permission(uint32 obj, uint32 cmd)
{
    const uint32* pObj = g_CmOmiNoCheck;
    const uint32* pCmd = NULL;
    uint32 iloop = 0;
    /* CM_OMI_OBJECT_DEMO 0 */
    while(*pObj != 0)
    {
        iloop = *(pObj+1);
        if(obj != *pObj)
        {
            pObj += iloop + 2;
            continue;
        }
        for(pCmd = pObj + 2; iloop > 0; pCmd++,iloop--)
        {
            if(cmd == *pCmd)
            {
                return CM_OK;
            }
        }
        break;        
    }
    return CM_FAIL;
}

sint32 cm_omi_check_permission_obj(uint32 obj)
{
    const uint32* pObj = g_CmOmiNoCheck;
    uint32 iloop = 0;

    /* CM_OMI_OBJECT_DEMO 0 */
    while(*pObj != 0)
    {
        iloop = *(pObj+1);
        if(obj != *pObj)
        {
            pObj += iloop + 2;
            continue;
        }
        return CM_OK;        
    }
    return CM_ERR_NO_PERMISSION;
}

extern sint32 cm_omi_init_client(void);
extern sint32 cm_omi_init_server(void);

sint32 cm_omi_init(bool_t isclient)
{
    if(isclient == CM_TRUE)
    {
        return cm_omi_init_client();
    }

    return cm_omi_init_server();
}

cm_omi_obj_t cm_omi_encode_errormsg(
    const void *pDecodeParam, void *pAckData, uint32 AckLen)
{
    cm_omi_obj_t items = NULL;
    cm_omi_obj_t item = NULL;
    sint8 *pData = (sint8*)pAckData;
    uint32 count = AckLen;

    if(0 == count)
    {
        return NULL;
    }

    items = cm_omi_obj_new_array();

    if(NULL == items)
    {
        CM_LOG_ERR(CM_MOD_CNM, "new items fail");
        return NULL;
    }

    item = cm_omi_obj_new();

    if(NULL == item)
    {
        CM_LOG_ERR(CM_MOD_CNM, "new item fail");
        return items;
    }
    /* CM_OMI_FIELD_ERROR_MSG 265 */
    (void)cm_omi_obj_key_set_str_ex(item, 265, pData);

    if(CM_OK != cm_omi_obj_array_add(items, item))
    {
        CM_LOG_ERR(CM_MOD_CNM, "add item fail");
        cm_omi_obj_delete(item);
    }

    return items;
}


