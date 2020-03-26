/******************************************************************************
 *          Copyright (c) 2017 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_ini.c
 * author     : wbn
 * create date: 2017Äê11ÔÂ7ÈÕ
 * description: TODO:
 *
 *****************************************************************************/
#include "cm_ini.h"
#include "iniparser.h"
#include "cm_log.h"

cm_ini_handle_t cm_ini_open(const sint8 *filename)
{
    return (cm_ini_handle_t)iniparser_load(filename);
}

cm_ini_handle_t cm_ini_new(void)
{
    return (cm_ini_handle_t)dictionary_new(0);
}

sint32 cm_ini_save(cm_ini_handle_t handle, const sint8 *filename)
{
    FILE *fp = fopen(filename,"w");
    
    if(NULL == fp)
    {
        CM_LOG_ERR(CM_MOD_NONE,"open:%s fail", filename);
        return CM_FAIL;
    }
    
    iniparser_dump_ini((const dictionary*)handle,fp);
    fclose(fp);
    return CM_OK;
}

void cm_ini_free(cm_ini_handle_t handle)
{
    iniparser_freedict((dictionary*)handle);
}

sint32 cm_ini_set(cm_ini_handle_t handle, 
    const sint8 *section, const sint8 *key, const sint8 *val)
{
    sint32 iRet = CM_FAIL;
    dictionary* pdict = (dictionary*)handle;
    sint8 seckey[CM_STRING_512] = {0};

    if((NULL == handle) || (NULL == section) 
        || (NULL == key) || (NULL == val))
    {
        return CM_PARAM_ERR;
    }
    
    if(!iniparser_find_entry(pdict,section))
    {
        iRet = iniparser_set(pdict,section,NULL);
        if(CM_OK != iRet)
        {
            CM_LOG_ERR(CM_MOD_NONE,"new section[%s] fail[%d]", section,iRet);
            return CM_FAIL;
        }        
    }
    CM_VSPRINTF(seckey,sizeof(seckey)-1,"%s:%s",section,key);
    return iniparser_set(pdict,seckey,val);
    
}

const sint8* cm_ini_get(cm_ini_handle_t handle, 
    const sint8 *section, const sint8 *key)
{
    dictionary* pdict = (dictionary*)handle;
    sint8 seckey[CM_STRING_512] = {0};
    
    if((NULL == handle) || (NULL == section) || (NULL == key))
    {
        return NULL;
    }
    
    CM_VSPRINTF(seckey,sizeof(seckey)-1,"%s:%s",section,key);
    return iniparser_getstring(pdict,seckey, NULL);    
}

void cm_ini_delete_section(cm_ini_handle_t handle, 
    const sint8 *section)
{
    iniparser_unset((dictionary*)handle,section);
}

void cm_ini_delete_key(cm_ini_handle_t handle, 
    const sint8 *section, const sint8 *key)
{
    sint8 seckey[CM_STRING_512] = {0};
    
    CM_VSPRINTF(seckey,sizeof(seckey)-1,"%s:%s",section,key);
    iniparser_unset((dictionary*)handle,seckey);
}

sint32 cm_ini_set_ext(const sint8 *filename,
    const sint8 *section, const sint8 *key, const sint8 *val)
{
    sint32 iRet = CM_FAIL;
    cm_ini_handle_t handle = cm_ini_open(filename);

    if(NULL == handle)
    {
        handle = cm_ini_new();
        if(NULL == handle)
        {
            CM_LOG_ERR(CM_MOD_NONE,"new ini fail");
            return CM_FAIL;
        }
    }

    iRet = cm_ini_set(handle,section,key,val);
    if(CM_OK != iRet)
    {
        cm_ini_free(handle);
        return iRet;
    }

    iRet = cm_ini_save(handle,filename);
    cm_ini_free(handle);
    return iRet;    
}

sint32 cm_ini_get_ext(const sint8 *filename, 
    const sint8 *section, const sint8 *key, sint8 *val, sint32 len)
{
    const sint8* pvalue = NULL; 
    cm_ini_handle_t handle = cm_ini_open(filename);

    if(NULL == handle)
    {
        CM_LOG_ERR(CM_MOD_NONE,"open %s fail",filename);
        return CM_FAIL;
    }

    pvalue = cm_ini_get(handle,section,key);
    if(NULL == pvalue)
    {
        cm_ini_free(handle);
        CM_LOG_ERR(CM_MOD_NONE,"get %s:%s fail",section,key);
        return CM_FAIL;
    }

    if(len <= strlen(pvalue))
    {
        cm_ini_free(handle);
        CM_LOG_ERR(CM_MOD_NONE,"get %s:%s size[%d]",section,key,strlen(pvalue));
        return CM_FAIL;
    }
    
    CM_VSPRINTF(val,len,"%s",pvalue);
    cm_ini_free(handle);
    return CM_OK;    
}

sint32 cm_ini_set_ext_uint32(const sint8 *filename,
    const sint8 *section, const sint8 *key, uint32 val)
{
    sint8 str[CM_STRING_32] = {0};
    
    CM_VSPRINTF(str,sizeof(str),"%u",val);
    return cm_ini_set_ext(filename,section,key,str);
}

sint32 cm_ini_get_ext_uint32(const sint8 *filename, 
    const sint8 *section, const sint8 *key, uint32 *val)
{
    sint32 iRet = CM_FAIL;
    sint8 str[CM_STRING_32] = {0};

    iRet = cm_ini_get_ext(filename,section,key,str,sizeof(str));
    if(CM_OK != iRet)
    {
        return iRet;
    }
    *val = (uint32)atoi(str);
    return CM_OK;
}

sint32 cm_ini_delete_key_ext(const sint8 *filename, 
    const sint8 *section, const sint8 *key)
{
    sint32 iRet = CM_FAIL;
    cm_ini_handle_t handle = cm_ini_open(filename);

    if(NULL == handle)
    {
        handle = cm_ini_new();
        if(NULL == handle)
        {
            CM_LOG_ERR(CM_MOD_NONE,"new ini fail");
            return CM_FAIL;
        }
    }

    if(NULL == key)
    {
        cm_ini_delete_section(handle,section);
    }
    else
    {
        cm_ini_delete_key(handle,section,key);
    }

    iRet = cm_ini_save(handle,filename);
    cm_ini_free(handle);
    return iRet; 
}


