/******************************************************************************
 *          Copyright (c) 2017 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_omi_json.c
 * author     : wbn
 * create date: 2017Äê10ÔÂ23ÈÕ
 * description: TODO:
 *
 *****************************************************************************/
#include "cm_omi.h"
#include "cm_log.h"
#include "json.h"

cm_omi_obj_t cm_omi_obj_parse(const sint8 *pStrData)
{
    return json_tokener_parse(pStrData);
}

cm_omi_obj_t cm_omi_obj_new(void)
{
    return json_object_new_object();
}

cm_omi_obj_t cm_omi_obj_new_array(void)
{
    return json_object_new_array();
}

uint32 cm_omi_obj_array_size(cm_omi_obj_t obj)
{
    if(TRUE != json_object_is_type(obj,json_type_array))
    {
        return 0;
    }
    return json_object_array_length(obj);
}

sint32 cm_omi_obj_array_add(cm_omi_obj_t obj, cm_omi_obj_t val)
{
    return json_object_array_add(obj,val);
}

cm_omi_obj_t cm_omi_obj_array_idx(cm_omi_obj_t obj, uint32 index)
{
    return json_object_array_get_idx(obj, index);
}


void cm_omi_obj_delete(cm_omi_obj_t obj)
{
    (void)json_object_put(obj);
}

const sint8* cm_omi_obj_tostr(cm_omi_obj_t obj)
{
    return json_object_to_json_string_ext(obj, JSON_C_TO_STRING_PLAIN);
}

const sint8* cm_omi_obj_value(cm_omi_obj_t obj)
{
    return json_object_get_string(obj);
}

cm_omi_obj_t cm_omi_obj_key_find(cm_omi_obj_t obj, const sint8* key)
{
    return json_object_object_get(obj, key);
}

sint32 cm_omi_obj_key_add(cm_omi_obj_t obj, const sint8* key, cm_omi_obj_t subobj)
{
    return json_object_object_add(obj, key, subobj);
}

void cm_omi_obj_key_delete(cm_omi_obj_t obj, const sint8* key)
{
    json_object_object_del(obj, key);
}

sint32 cm_omi_obj_key_set_str(cm_omi_obj_t obj, const sint8* key, const sint8* val)
{
    sint32 iRet = CM_FAIL;
    struct json_object* pTmp = NULL;

    pTmp = cm_omi_obj_key_find(obj,key);
    if(NULL != pTmp)
    {
        CM_LOG_ERR(CM_MOD_OMI,"key[%s] exited!", key);
        return CM_FAIL;
    }

    pTmp = json_object_new_string(val);
    if(NULL == pTmp)
    {
        CM_LOG_ERR(CM_MOD_OMI,"new key[%s] fail!", key);
        return CM_FAIL;
    }
    iRet = json_object_object_add(obj,key,pTmp);
    if(CM_OK != iRet)
    {
        json_object_put(pTmp);
        CM_LOG_ERR(CM_MOD_OMI,"add key[%s] fail[%d]!", key,iRet);
        return CM_FAIL;
    }
    return CM_OK;
}

sint32 cm_omi_obj_key_get_str(cm_omi_obj_t obj, const sint8* key, sint8* val, uint32 len)
{
    const sint8* pData = NULL;
    uint32 DataLen = 0;
    struct json_object* pTmp = json_object_object_get(obj,key);

    if(NULL == obj)
    {
        //CM_LOG_DEBUG(CM_MOD_OMI,"get key[%s] fail!", key);
        return CM_FAIL;
    }

    pData = json_object_get_string(pTmp);
    if(NULL == pData)
    {
        //CM_LOG_DEBUG(CM_MOD_OMI,"get key[%s] null!", key);
        return CM_FAIL;
    }
    DataLen = strlen(pData);
    if(DataLen >= len)
    {
        CM_LOG_ERR(CM_MOD_OMI,"get key[%s] strlen[%u] >= buflen[%u]!",
            key,DataLen,len);
        return CM_FAIL;
    }
    strcpy(val,pData);
    return CM_OK;
}

