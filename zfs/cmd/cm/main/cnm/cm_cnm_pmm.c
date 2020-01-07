/******************************************************************************
 *          Copyright (c) 2017 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cnm_pmm.c
 * author     : wbn
 * create date: 2017Äê12ÔÂ21ÈÕ
 * description: TODO:
 *
 *****************************************************************************/
#include "cm_omi_types.h"
#include "cm_db.h"
#include "cm_cmt.h"
#include "cm_omi.h"
#include "cm_log.h"
#include "cm_pmm_types.h"
#include "cm_pmm.h"
#include "cm_cnm_pmm.h"
#include <sys/statvfs.h>

sint32 cm_cnm_pmm_init(void)
{
    return CM_OK;
}

sint32 cm_cnm_pmm_cluster_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam)
{
    return cm_pmm_decode(CM_OMI_OBJECT_PMM_CLUSTER,ObjParam,ppDecodeParam);
}

cm_omi_obj_t cm_cnm_pmm_cluster_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen)
{
    return cm_pmm_encode(CM_OMI_OBJECT_PMM_CLUSTER,pDecodeParam,pAckData,AckLen);
}

sint32 cm_cnm_pmm_cluster_current(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_pmm_current(CM_OMI_OBJECT_PMM_CLUSTER,pDecodeParam,ppAckData,pAckLen);
}
sint32 cm_cnm_pmm_cluster_history(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_pmm_history(CM_OMI_OBJECT_PMM_CLUSTER,pDecodeParam,ppAckData,pAckLen);
}

sint32 cm_cnm_pmm_node_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam)
{
    return cm_pmm_decode(CM_OMI_OBJECT_PMM_NODE,ObjParam,ppDecodeParam);
}
cm_omi_obj_t cm_cnm_pmm_node_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen)
{
    return cm_pmm_encode(CM_OMI_OBJECT_PMM_NODE,pDecodeParam,pAckData,AckLen);
}

sint32 cm_cnm_pmm_node_current(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_pmm_current(CM_OMI_OBJECT_PMM_NODE,pDecodeParam,ppAckData,pAckLen);
}
sint32 cm_cnm_pmm_node_history(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_pmm_history(CM_OMI_OBJECT_PMM_NODE,pDecodeParam,ppAckData,pAckLen);
}

typedef struct
{
    cm_pmm_cfg_t cfg;
    cm_omi_field_flag_t field;
}cm_cnm_pmm_cfg_decode_t;

sint32 cm_cnm_pmm_cfg_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam)
{
    cm_cnm_pmm_cfg_decode_t *pDecode = CM_MALLOC(sizeof(cm_cnm_pmm_cfg_decode_t));

    if(NULL == pDecode)
    {
        CM_LOG_ERR(CM_MOD_CNM,"malloc fail");
        return CM_FAIL;
    }
    CM_MEM_ZERO(pDecode,sizeof(cm_cnm_pmm_cfg_decode_t));
    if(CM_OK == cm_omi_obj_key_get_u32_ex(ObjParam,CM_OMI_FIELD_PMM_CFG_SAVE_HOURS,
        &pDecode->cfg.save_hours))
    {
        CM_OMI_FIELDS_FLAG_SET(&pDecode->field,CM_OMI_FIELD_PMM_CFG_SAVE_HOURS);
        pDecode->cfg.save_hours *= CM_HOURS_OF_DAY; /* hours of a day*/
    }
    *ppDecodeParam = pDecode;
    return CM_OK;
}
cm_omi_obj_t cm_cnm_pmm_cfg_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen)
{
    cm_pmm_cfg_t *cfg = pAckData;
    cm_omi_obj_t items = NULL;
    cm_omi_obj_t item = NULL;
    
    if(NULL == cfg)
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
    
    (void)cm_omi_obj_key_set_u32_ex(item,CM_OMI_FIELD_PMM_CFG_SAVE_HOURS,
        cfg->save_hours / CM_HOURS_OF_DAY);

    if(CM_OK != cm_omi_obj_array_add(items,item))
    {
        CM_LOG_ERR(CM_MOD_CNM,"add item fail");
        cm_omi_obj_delete(item);
    }
    return items;
}

sint32 cm_cnm_pmm_cfg_get(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_pmm_cfg_sync_get(0, ppAckData,pAckLen);
}
sint32 cm_cnm_pmm_cfg_update(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    const cm_cnm_pmm_cfg_decode_t *pDecode = pDecodeParam;
    cm_pmm_cfg_t *cfg = NULL;
    uint32 len = 0;
    sint32 iRet = CM_OK;
    
    if(NULL == pDecode)
    {
        CM_LOG_ERR(CM_MOD_CNM,"pDecodeParam NULL");
        return CM_PARAM_ERR;
    }
    cm_pmm_cfg_sync_get(0, (void**)&cfg,&len);
    if(NULL == cfg)
    {
        CM_LOG_ERR(CM_MOD_CNM,"get info fail");
        return CM_FAIL;
    }
    do
    {
        if(!CM_OMI_FIELDS_FLAG_ISSET(&pDecode->field,CM_OMI_FIELD_PMM_CFG_SAVE_HOURS)
            || (cfg->save_hours == pDecode->cfg.save_hours))
        {
            break;
        }
        cfg->save_hours = pDecode->cfg.save_hours;
        iRet = cm_pmm_cfg_update(cfg);
        if(CM_OK != iRet)
        {
            CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]",iRet);
        }
    }while(0);
    
    CM_FREE(cfg);
    return iRet;
}

void cm_cnm_pmm_cfg_oplog_update(
    const sint8* sessionid,const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK) ? CM_ALARM_LOG_PMM_CFG_MODIFY_OK : 
        CM_ALARM_LOG_PMM_CFG_MODIFY_FAIL;
    const cm_cnm_pmm_cfg_decode_t *req = pDecodeParam;
    const uint32 cnt = 1;
    uint32 save_days=0;

    if(NULL == pDecodeParam)
    {   
        cm_cnm_oplog_report(sessionid,alarmid,NULL,cnt,NULL);
    }    
    else    
    {   
        cm_pmm_cfg_t *info = &req->cfg;
        save_days = info->save_hours/CM_HOURS_OF_DAY;
        cm_cnm_oplog_param_t params[1] = {
            {CM_OMI_DATA_INT,CM_OMI_FIELD_PMM_CFG_SAVE_HOURS,sizeof(save_days),&save_days},
        };
        
        cm_cnm_oplog_report(sessionid,alarmid,params,cnt,&req->field);
    }   
    return;
}

/*
*nas
*/
static sint32 cm_pmm_nas_get_data_each(void *arg, sint8 **cols, uint32 col_num)
{
    cm_pmm_nas_data_t* info = arg;
    double ncreate = 0.0;
    double nmkdir = 0.0;
    double nread = 0.0;
    double nreaddir = 0.0;
    double nremove = 0.0;
    double nrmdir = 0.0;
    double nsymlink = 0.0;
    double nwrite = 0.0;
    double read_bytes = 0.0;
    double rlentime = 0.0;
    double write_bytes = 0.0;
    
    CM_MEM_ZERO(info,sizeof(cm_pmm_nas_data_t));
    if(11 != col_num)
    {
        CM_LOG_ERR(CM_MOD_CNM,"col_num err[col_num = %d]",col_num);
        return CM_FAIL;
    }

    ncreate = atof(cols[0]);                
    nmkdir = atof(cols[1]);                        
    nread = atof(cols[2]);                         
    nreaddir = atof(cols[3]);                      
    nremove = atof(cols[4]);                
    nrmdir = atof(cols[5]);                        
    nsymlink = atof(cols[6]);                       
    nwrite = atof(cols[7]);                     
    read_bytes = atof(cols[8]);                   
    rlentime = atof(cols[9]);                     
    write_bytes = atof(cols[10]); 

    /*
    CM_LOG_ERR(CM_MOD_CNM,"[%s %s %s %s %s %s %s %s %s %s %s]",
        cols[0],cols[1],cols[2],cols[3],cols[4],
        cols[5],cols[6],cols[7],cols[8],
        cols[9],cols[10]);
    CM_LOG_ERR(CM_MOD_CNM,"[%lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf]",
        ncreate,nmkdir,nread,nreaddir,
        nremove,nrmdir,nsymlink,nwrite,
        read_bytes,rlentime,write_bytes);
    */ 
    
    info->newfile = ncreate+nmkdir+nsymlink;
    info->rmfile = nremove+nrmdir;
    info->readdir = nreaddir;
    info->iops = nread+nwrite;
    info->reads = read_bytes;
    info->writes = write_bytes;
    info->delay = rlentime;
    /*
    CM_LOG_ERR(CM_MOD_CNM,"[%lf %lf %lf %lf %lf %lf %lf]",
        info->newfile,info->rmfile,info->readdir,info->iops,
        info->reads,info->writes,info->delay);
    */ 
    return CM_OK;
}

void cm_pmm_nas_get_data(const sint8 *name,void* data)
{
    
    cm_pmm_nas_data_t* info = data;
    uint32 ret = CM_OK; 
    
    ret = cm_cnm_exec_get_col(cm_pmm_nas_get_data_each,info,
        CM_SHELL_EXEC" cm_pmm_nas %s",name);
    /*
    CM_LOG_ERR(CM_MOD_CNM,"kstat -m unix -c misc -n %s|egrep \
        \'ncreate |nmkdir |nread |nreaddir |nremove |nrmdir |nsymlink |nwrite |read_bytes |rlentime |write_bytes \' \
        |awk '{printf $2\" \"}'",name);
    */
    if(CM_OK != ret)
    {
        CM_LOG_ERR(CM_MOD_CNM,"exec fail [%d]",ret);
    }
    
    return;
}

void cm_pmm_nas_cal_data(void* olddata,void* newdata,void* data)
{
    cm_pmm_nas_data_t *oldinfo = (cm_pmm_nas_data_t *)olddata;
    cm_pmm_nas_data_t *newinfo = (cm_pmm_nas_data_t *)newdata;
    cm_pmm_nas_data_t *info = data;

    CM_PMM_NODE_DATA_CAL(info->newfile,oldinfo->newfile,newinfo->newfile,CM_PMM_TIME_INTERVAL);
    CM_PMM_NODE_DATA_CAL(info->rmfile,oldinfo->rmfile,newinfo->rmfile,CM_PMM_TIME_INTERVAL);
    CM_PMM_NODE_DATA_CAL(info->readdir,oldinfo->readdir,newinfo->readdir,CM_PMM_TIME_INTERVAL);
    CM_PMM_NODE_DATA_CAL(info->iops,oldinfo->iops,newinfo->iops,CM_PMM_TIME_INTERVAL);
    CM_PMM_NODE_DATA_CAL(info->reads,oldinfo->reads,newinfo->reads,CM_PMM_TIME_INTERVAL);
    CM_PMM_NODE_DATA_CAL(info->writes,oldinfo->writes,newinfo->writes,CM_PMM_TIME_INTERVAL);
    CM_PMM_NODE_DATA_CAL(info->delay,oldinfo->delay,newinfo->delay,CM_PMM_TIME_INTERVAL);
    info->reads /= CM_PMM_UNITS;
    info->writes /= CM_PMM_UNITS; 
    if(info->iops >= 0.01)
    {
        info->delay /= info->iops*1000000; /*ms*/
    }
    else
    {
        info->delay = 0.0;
    }
  
    /*CM_LOG_ERR(CM_MOD_CNM,"[%lf %lf %lf %lf %lf %lf %lf]",
        info->newfile,info->rmfile,info->readdir,info->iops,
        info->reads,info->writes,info->delay);
    CM_LOG_ERR(CM_MOD_CNM,"[%lf %lf %lf %lf %lf %lf %lf]",
        newinfo->newfile,newinfo->rmfile,newinfo->readdir,newinfo->iops,
        newinfo->reads,newinfo->writes,newinfo->delay);
    CM_LOG_ERR(CM_MOD_CNM,"[%lf %lf %lf %lf %lf %lf %lf]",
        oldinfo->newfile,oldinfo->rmfile,oldinfo->readdir,oldinfo->iops,
        oldinfo->reads,oldinfo->writes,oldinfo->delay);
    */
    return;
}

sint32 cm_pmm_nas_current_mem(const void *pDecode,void **ppAckData, uint32 *pAckLen)
{
    cm_pmm_decode_t *pmm_nas_data = (cm_pmm_decode_t *)pDecode;
    #ifdef __linux__
    struct statvfs fs_info;
    #else
    struct statvfs64 fs_info;
    #endif
    sint8 name[256] = {0};
    uint32 iRet = CM_OK;
    if( NULL == pmm_nas_data)
    {
        CM_LOG_ERR(CM_MOD_CNM,"pDecode = NULL");
        return CM_FAIL; 
    }
    cm_pmm_nas_data_t *pmyack = CM_MALLOC(sizeof(cm_pmm_nas_data_t));
    if(NULL == pmyack)
    {
        CM_LOG_ERR(CM_MOD_CNM,"CM_MALLOC fail");
        return CM_FAIL;
    }
    CM_MEM_ZERO(pmyack,sizeof(cm_pmm_nas_data_t));
    CM_VSPRINTF(name,sizeof(name),"/%s",pmm_nas_data->param);
    if(statvfs64(name,&fs_info))
    {   
        CM_LOG_ERR(CM_MOD_CNM,"nas not exist");
		CM_FREE(pmyack);
        return CM_OK;
    }
    CM_MEM_ZERO(name,sizeof(name));
    CM_VSPRINTF(name,sizeof(name),"vopstats_%lx",fs_info.f_fsid);
    CM_LOG_INFO(CM_MOD_PMM,"nas [%s]",name);
    iRet = cm_pmm_common_get_data(CM_PMM_TYPE_NAS,name,pmyack);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"cm_pmm_common_get_data fail");
        CM_FREE(pmyack);
        return CM_OK;
    }
    pmyack->id = 0;

    *ppAckData = pmyack;
    *pAckLen = sizeof(cm_pmm_nas_data_t);
    return CM_OK;
}

sint32 cm_cnm_pmm_nas_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam)
{
    return cm_pmm_decode(CM_OMI_OBJECT_PMM_NAS,ObjParam,ppDecodeParam);
}

cm_omi_obj_t cm_cnm_pmm_nas_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen)
{
    return cm_pmm_encode(CM_OMI_OBJECT_PMM_NAS,pDecodeParam,pAckData,AckLen);
}

sint32 cm_cnm_pmm_nas_current(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_pmm_current(CM_OMI_OBJECT_PMM_NAS,pDecodeParam,ppAckData,pAckLen);
}
/*
*sas
*/

void cm_pmm_sas_get_data(const sint8 *name,void* data)
{
    cm_pmm_sas_data_t* info = data;
    sint8 ret[256] = {0};
    if(CM_OK != cm_exec_tmout(ret,sizeof(ret),3,"/var/cm/script/cm_cnm_pmm_sas.sh get %s",name))
    {
         CM_LOG_ERR(CM_MOD_CNM,"/var/cm/script/cm_cnm_pmm_sas.sh get %s fail",name);
         info->writes = 0.0;
         info->reads = 0.0;
         info->wiops = 0.0;
         info->riops = 0.0;
    }
    else
    {
        sscanf(ret,"%lf %lf %lf %lf",&info->writes,&info->reads,&info->wiops,&info->riops);
        /*
        CM_LOG_ERR(CM_MOD_CNM,"[%lf %lf %lf %lf]",
            info->writes,info->reads,info->wiops,info->riops);
        */ 
    }
    return;
}

void cm_pmm_sas_cal_data(void* olddata,void* newdata,void* data)
{
    cm_pmm_sas_data_t *oldinfo = (cm_pmm_sas_data_t *)olddata;
    cm_pmm_sas_data_t *newinfo = (cm_pmm_sas_data_t *)newdata;
    cm_pmm_sas_data_t *info = data;

    CM_PMM_NODE_DATA_CAL(info->reads,oldinfo->reads,newinfo->reads,CM_PMM_TIME_INTERVAL);
    CM_PMM_NODE_DATA_CAL(info->writes,oldinfo->writes,newinfo->writes,CM_PMM_TIME_INTERVAL);
    CM_PMM_NODE_DATA_CAL(info->riops,oldinfo->riops,newinfo->riops,CM_PMM_TIME_INTERVAL);
    CM_PMM_NODE_DATA_CAL(info->wiops,oldinfo->wiops,newinfo->wiops,CM_PMM_TIME_INTERVAL);
    
    info->reads /= CM_PMM_UNITS;
    info->writes /= CM_PMM_UNITS;
    return;
}

sint32 cm_pmm_sas_check_data(const sint8* name)
{
    return cm_exec_int("/var/cm/script/cm_cnm_pmm_sas.sh check %s",name);
}

sint32 cm_pmm_sas_current_mem(const void *pDecode,void **ppAckData, uint32 *pAckLen)
{
    cm_pmm_decode_t *pmm_sas_data = (cm_pmm_decode_t *)pDecode;
    uint32 iRet = CM_OK;
    if( NULL == pmm_sas_data)
    {
        CM_LOG_ERR(CM_MOD_CNM,"pDecodeParam = NULL");
        return CM_FAIL; 
    }
    cm_pmm_sas_data_t *pmyack = CM_MALLOC(sizeof(cm_pmm_sas_data_t));
    if(NULL == pmyack)
    {
        CM_LOG_ERR(CM_MOD_CNM,"CM_MALLOC fail");
        return CM_FAIL;
    }
    CM_MEM_ZERO(pmyack,sizeof(cm_pmm_sas_data_t));
    iRet = cm_pmm_common_get_data(CM_PMM_TYPE_SAS,pmm_sas_data->param,pmyack);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"cm_pmm_common_get_data fail");
        CM_FREE(pmyack);
        return CM_OK;
    }
    pmyack->id = 0;
    
    *ppAckData = pmyack;
    *pAckLen = sizeof(cm_pmm_sas_data_t);
    return CM_OK;
}  

sint32 cm_cnm_pmm_sas_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam)
{
    return cm_pmm_decode(CM_OMI_OBJECT_PMM_SAS,ObjParam,ppDecodeParam);
}

cm_omi_obj_t cm_cnm_pmm_sas_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen)
{
    return cm_pmm_encode(CM_OMI_OBJECT_PMM_SAS,pDecodeParam,pAckData,AckLen);
}

sint32 cm_cnm_pmm_sas_current(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_pmm_current(CM_OMI_OBJECT_PMM_SAS,pDecodeParam,ppAckData,pAckLen);
}

sint32 cm_cnm_sas_getbatch(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{     
    return cm_cnm_request_comm(CM_OMI_OBJECT_CNM_SAS,CM_OMI_CMD_GET_BATCH,
        sizeof(cm_pmm_name_t),pDecodeParam,ppAckData,pAckLen);
}

sint32 cm_cnm_sas_count(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_cnm_request_comm(CM_OMI_OBJECT_CNM_SAS,CM_OMI_CMD_COUNT,
        sizeof(cm_pmm_name_t),pDecodeParam,ppAckData,pAckLen);
}

static sint32 cm_cnm_sas_get_each_name(void *arg, sint8 **cols, uint32 col_num)
{
    cm_pmm_name_t *sas = arg;
    const uint32 def_num = 1;     
    if(def_num != col_num)
    {
        CM_LOG_WARNING(CM_MOD_CNM,"col_num[%u] def_num[%u]",col_num,def_num);
        return CM_FAIL;
    }
    CM_MEM_ZERO(sas,sizeof(cm_pmm_name_t));
    sas->nid = cm_node_get_id();
    CM_VSPRINTF(sas->name,sizeof(sas->name),"%s",cols[0]);
    return CM_OK;
}

sint32 cm_cnm_sas_local_getbatch(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    uint32 iRet = CM_OK; 
    iRet = cm_cnm_exec_get_list("/var/cm/script/cm_cnm_pmm_sas.sh getbatch",cm_cnm_sas_get_each_name,
        0,sizeof(cm_pmm_name_t),ppAck,&total);
    if(iRet != CM_OK)
    {
        CM_LOG_ERR(CM_MOD_CNM,"script fail");
        return CM_OK;
    }
    *pAckLen = total * sizeof(cm_pmm_name_t);
    return CM_OK;
}

sint32 cm_cnm_sas_local_count(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    uint64 count = cm_exec_int("/var/cm/script/cm_cnm_pmm_sas.sh getbatch|wc -l");
    return cm_cnm_ack_uint64(count,ppAck,pAckLen);
}

/*
*protocol
*/
void cm_pmm_protocol_get_data(const sint8 *name,void* data)
{
    cm_pmm_protocol_data_t* info = data;
    info->iops=cm_exec_double("/var/cm/script/cm_cnm_pmm_protocol.sh get %s",name);
    return;
}

void cm_pmm_protocol_cal_data(void* olddata,void* newdata,void* data)
{
    cm_pmm_protocol_data_t *oldinfo = (cm_pmm_protocol_data_t *)olddata;
    cm_pmm_protocol_data_t *newinfo = (cm_pmm_protocol_data_t *)newdata;
    cm_pmm_protocol_data_t *info = data;
    CM_PMM_NODE_DATA_CAL(info->iops,oldinfo->iops,newinfo->iops,CM_PMM_TIME_INTERVAL);
    return;
}

void cm_pmm_protocol_cifs_cal_data(void* olddata,void* newdata,void* data)
{
    cm_pmm_protocol_data_t *oldinfo = (cm_pmm_protocol_data_t *)olddata;
    cm_pmm_protocol_data_t *newinfo = (cm_pmm_protocol_data_t *)newdata;
    cm_pmm_protocol_data_t *info = data;
    info->iops = newinfo->iops;
    return;
}

sint32 cm_pmm_protocol_check_data(const sint8* name)
{
    return cm_exec_int("/var/cm/script/cm_cnm_pmm_protocol.sh check %s",name);
}

sint32 cm_pmm_protocol_current_mem(const void *pDecode,void **ppAckData, uint32 *pAckLen)
{
    cm_pmm_decode_t *pmm_protocol_data = (cm_pmm_decode_t *)pDecode;
    uint32 iRet = CM_OK;
    if( NULL == pmm_protocol_data)
    {
        CM_LOG_ERR(CM_MOD_CNM,"pDecode = NULL");
        return CM_FAIL; 
    }
    cm_pmm_protocol_data_t *pmyack = CM_MALLOC(sizeof(cm_pmm_protocol_data_t));
    if(NULL == pmyack)
    {
        CM_LOG_ERR(CM_MOD_CNM,"CM_MALLOC fail");
        return CM_FAIL;
    }
    CM_MEM_ZERO(pmyack,sizeof(cm_pmm_protocol_data_t));

    if(!strcmp(pmm_protocol_data->param,"cifs"))
    {
        iRet = cm_pmm_common_get_data(CM_PMM_TYPE_CIFS,pmm_protocol_data->param,pmyack);
    }
    else
    {
        iRet = cm_pmm_common_get_data(CM_PMM_TYPE_PROTO,pmm_protocol_data->param,pmyack);
    }
    
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"cm_pmm_common_get_data fail");
        CM_FREE(pmyack);
        return CM_OK;
    }
    pmyack->id = 0;
    
    *ppAckData = pmyack;
    *pAckLen = sizeof(cm_pmm_protocol_data_t);
    return CM_OK;
}

sint32 cm_cnm_pmm_protocol_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam)
{
    return cm_pmm_decode(CM_OMI_OBJECT_PMM_PROTO,ObjParam,ppDecodeParam);
}

cm_omi_obj_t cm_cnm_pmm_protocol_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen)
{
    return cm_pmm_encode(CM_OMI_OBJECT_PMM_PROTO,pDecodeParam,pAckData,AckLen);
}

sint32 cm_cnm_pmm_protocol_current(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_pmm_current(CM_OMI_OBJECT_PMM_PROTO,pDecodeParam,ppAckData,pAckLen);
}

sint32 cm_cnm_protocol_getbatch(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{     
    return cm_cnm_request_comm(CM_OMI_OBJECT_CNM_PROTO,CM_OMI_CMD_GET_BATCH,
        sizeof(cm_pmm_name_t),pDecodeParam,ppAckData,pAckLen);
}

sint32 cm_cnm_protocol_count(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_cnm_request_comm(CM_OMI_OBJECT_CNM_PROTO,CM_OMI_CMD_COUNT,
        sizeof(cm_pmm_name_t),pDecodeParam,ppAckData,pAckLen);
}

static sint32 cm_cnm_protocol_get_each_name(void *arg, sint8 **cols, uint32 col_num)
{
    cm_pmm_name_t *protocol = arg;
    const uint32 def_num = 1;     
    if(def_num != col_num)
    {
        CM_LOG_WARNING(CM_MOD_CNM,"col_num[%u] def_num[%u]",col_num,def_num);
        return CM_FAIL;
    }
    CM_MEM_ZERO(protocol,sizeof(cm_pmm_name_t));
    protocol->nid = cm_node_get_id();
    CM_VSPRINTF(protocol->name,sizeof(protocol->name),"%s",cols[0]);
    return CM_OK;
}

sint32 cm_cnm_protocol_local_getbatch(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    uint32 iRet = CM_OK;  
    iRet = cm_cnm_exec_get_list("/var/cm/script/cm_cnm_pmm_protocol.sh getbatch",cm_cnm_protocol_get_each_name,
        0,sizeof(cm_pmm_name_t),ppAck,&total);
    if(iRet != CM_OK)
    {
        CM_LOG_ERR(CM_MOD_CNM,"script fail");
        return CM_OK;
    }
    *pAckLen = total * sizeof(cm_pmm_name_t);
    return CM_OK;
}

sint32 cm_cnm_protocol_local_count(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    uint64 count = cm_exec_int("/var/cm/script/cm_cnm_pmm_protocol.sh getbatch|wc -l");
    return cm_cnm_ack_uint64(count,ppAck,pAckLen);
}

/*
*cache
*/  
void cm_pmm_cache_get_data(const sint8 *name,void* data)
{
    cm_pmm_cache_data_t* info = data;
    uint8 buff[256] = {0};
    uint32 ret = CM_OK;
	CM_MEM_ZERO(buff,sizeof(buff));
    ret = cm_exec_tmout(buff,sizeof(buff),3,CM_SHELL_EXEC" cm_pmm_cache");
	
    info->l1_hits = 0.0;
    info->l1_misses = 0.0;
    info->l2_hits = 0.0;
    info->l2_misses = 0.0;
	info->l1_size = 0.0;
	info->l2_size = 0.0;
    if(CM_OK != ret)
    {
        CM_LOG_ERR(CM_MOD_CNM,"exec fail [%d]",ret);
    }
    else
    {
        sscanf(buff,"%lf %lf %lf %lf %lf %lf",
			&info->l1_hits,&info->l2_hits,&info->l2_misses,&info->l1_misses,&info->l1_size,&info->l2_size);
        /*
        CM_LOG_ERR(CM_MOD_CNM,"kstat -m zfs -n arcstats|awk \
        '$1==\"hits\"{printf $2\" \"}$1==\"misses\"{printf $2\" \"}$1==\"l2_hits\"{printf \
        $2\" \"}$1==\"l2_misses\"{printf $2\" \"}'");
        
        CM_LOG_ERR(CM_MOD_CNM,"[%lf %lf %lf %lf] [%s]",
            info->l1_hits,info->l2_hits,info->l2_misses,info->l1_misses,buff);
        */
    }  
    return;
}

void cm_pmm_cache_cal_data(void* olddata,void* newdata,void* data)
{
    cm_pmm_cache_data_t *oldinfo = (cm_pmm_cache_data_t *)olddata;
    cm_pmm_cache_data_t *newinfo = (cm_pmm_cache_data_t *)newdata;
    cm_pmm_cache_data_t *info = data;

    CM_PMM_NODE_DATA_CAL(info->l1_hits,oldinfo->l1_hits,newinfo->l1_hits,CM_PMM_TIME_INTERVAL);
    CM_PMM_NODE_DATA_CAL(info->l1_misses,oldinfo->l1_misses,newinfo->l1_misses,CM_PMM_TIME_INTERVAL);
    CM_PMM_NODE_DATA_CAL(info->l2_hits,oldinfo->l2_hits,newinfo->l2_hits,CM_PMM_TIME_INTERVAL);
    CM_PMM_NODE_DATA_CAL(info->l2_misses,oldinfo->l2_misses,newinfo->l2_misses,CM_PMM_TIME_INTERVAL);
    info->l1_size = newinfo->l1_size/1024;
    info->l2_size = newinfo->l2_size/1024;
    return;
}



sint32 cm_pmm_cache_current_mem(const void *pDecode,void **ppAckData, uint32 *pAckLen)
{
    uint32 iRet = CM_OK;
    cm_pmm_cache_data_t *pmyack = CM_MALLOC(sizeof(cm_pmm_cache_data_t));
    if(NULL == pmyack)
    {
        CM_LOG_ERR(CM_MOD_CNM,"malloc fail");
        return CM_FAIL;
    }
    CM_MEM_ZERO(pmyack,sizeof(cm_pmm_cache_data_t));
    iRet = cm_pmm_common_get_data(CM_PMM_TYPE_CACHE,"name",pmyack);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"cm_pmm_common_get_data fail");
        CM_FREE(pmyack);
        return CM_OK;
    }
    pmyack->id = 0;
  
    *ppAckData = pmyack;
    *pAckLen = sizeof(cm_pmm_cache_data_t);
    return CM_OK;
}

sint32 cm_cnm_pmm_cache_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam)
{
    return cm_pmm_decode(CM_OMI_OBJECT_PMM_CACHE,ObjParam,ppDecodeParam);
}

cm_omi_obj_t cm_cnm_pmm_cache_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen)
{
    return cm_pmm_encode(CM_OMI_OBJECT_PMM_CACHE,pDecodeParam,pAckData,AckLen);
}

sint32 cm_cnm_pmm_cache_current(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_pmm_current(CM_OMI_OBJECT_PMM_CACHE,pDecodeParam,ppAckData,pAckLen);
}

static void cm_cnm_name_encode_each(cm_omi_obj_t item,void *eachdata,void *arg)
{
    cm_omi_field_flag_t *field = arg;
    cm_pmm_name_t* sas_info = eachdata;
    cm_cnm_map_value_str_t cols_str[] = 
    {
        {CM_OMI_FIELD_PMM_NAME,sas_info->name},   
    };

    cm_cnm_map_value_num_t cols_num[] = 
    {
        {CM_OMI_FIELD_PMM_NID,sas_info->nid},
    };
    
    cm_cnm_encode_str(item,field,cols_str,sizeof(cols_str)/sizeof(cm_cnm_map_value_str_t));
    cm_cnm_encode_num(item,field,cols_num,sizeof(cols_num)/sizeof(cm_cnm_map_value_num_t));
    return;
}

cm_omi_obj_t cm_cnm_name_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen)
{
    return cm_cnm_encode_comm_ext(pDecodeParam,pAckData,AckLen,
        sizeof(cm_pmm_name_t),cm_cnm_name_encode_each);
}






