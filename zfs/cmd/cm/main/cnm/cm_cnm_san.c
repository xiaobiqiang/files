/******************************************************************************
 *          Copyright (c) 2018 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cnm_san.c
 * author     : wbn
 * create date: 2018.05.17
 * description: TODO:
 *
 *****************************************************************************/
#include "cm_cnm_san.h"
#include "cm_log.h"
#include "cm_xml.h"
#include "cm_node.h"
#include "cm_omi.h"
#include "cm_db.h"
#include "cm_cfg_global.h"
/*===========================================================================*/
#define CM_CNM_LUN_QOS_OFF 0
#define CM_CNM_LUN_QOS_IOPS 1
#define CM_CNM_LUN_QOS_BW 2

extern const cm_omi_map_enum_t CmOmiMapLunSyncTypeCfg;
extern const cm_omi_map_enum_t CmOmiMapLunStateTypeCfg;
extern const cm_omi_map_enum_t CmOmiMapEnumPoolStatusType;
extern const cm_omi_map_enum_t CmOmiMapEnumSwitchType;

extern const sint8* CmOmiCheckSizeRegex;
static cm_db_handle_t g_cm_lunmap_handle = NULL;

#define CM_CNM_CHECK_ALL_NODE_ONLINE() \
    do{\
        if(CM_OK != cm_node_check_all_online()) \
        { \
            CM_LOG_WARNING(CM_MOD_CNM,"some node offline"); \
            return CM_ERR_SOME_NODE_OFFLINE; \
        } \
    }while(0)

static const sint8 *g_cm_cnm_lun_get_cmd = NULL;

static const sint8 *g_cm_cnm_lun_stmfid_map = CM_SHELL_EXEC" stmfadm_list_all_luns";

sint32 cm_cnm_lun_init(void)
{
    switch(g_cm_sys_ver)
    {
        case CM_SYS_VER_SOLARIS_V7R16:
            g_cm_cnm_lun_get_cmd = "zfs list -H -t volume -o "
                "name,referenced,avail,volsize,volblocksize,"
                "refreservation,compression,sync,appmeta,dedup";
            break;
        default:
            g_cm_cnm_lun_get_cmd = "zfs list -H -t volume -o "
                "name,referenced,avail,volsize,volblocksize,"
                "refreservation,compression,sync,appmeta,dedup,"
                "zfs:single_data,thold";
            break;
    }
    return CM_OK;
}

sint32 cm_cnm_lun_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam)
{
    uint32 iloop = 0;
    uint32 value = 0;
    uint32 cols_u8[] = {
        CM_OMI_FIELD_LUN_WRITE_POLICY,
        CM_OMI_FIELD_LUN_IS_DOUBLE,
        CM_OMI_FIELD_LUN_IS_COMPRESS,
        CM_OMI_FIELD_LUN_IS_HOT,
        CM_OMI_FIELD_LUN_IS_SINGLE,
        CM_OMI_FIELD_LUN_ALARM_THRESHOLD,
        CM_OMI_FIELD_LUN_QOS,
        CM_OMI_FIELD_LUN_DEDUP,
        };
    uint8* val_u8[8] = {NULL};
    cm_cnm_lun_info_t *info = NULL;
    cm_cnm_lun_param_t *param = CM_MALLOC(sizeof(cm_cnm_lun_param_t));
    
    if(NULL == param)
    {
        CM_LOG_ERR(CM_MOD_CNM,"malloc fail");
        return CM_FAIL;
    }
    CM_MEM_ZERO(param,sizeof(cm_cnm_lun_param_t));
    cm_omi_decode_fields_flag(ObjParam,&param->field);
    
    info = &param->info;
    param->total = CM_CNM_MAX_RECORD;
    val_u8[0] = &info->write_policy;
    val_u8[1] = &info->is_double;
    val_u8[2] = &info->is_compress;
    val_u8[3] = &info->is_hot;
    val_u8[4] = &info->is_single;
    val_u8[5] = &info->alarm_threshold;
    val_u8[6] = &info->qos;
    val_u8[7] = &info->dedup;
    
    if(CM_OK == cm_omi_obj_key_get_u32_ex(ObjParam,CM_OMI_FIELD_LUN_NID,&param->nid))
    {
        CM_OMI_FIELDS_FLAG_SET(&param->set,CM_OMI_FIELD_LUN_NID);
    }
    (void)cm_omi_obj_key_get_u32_ex(ObjParam,CM_OMI_FIELD_FROM,&param->offset);
    (void)cm_omi_obj_key_get_u32_ex(ObjParam,CM_OMI_FIELD_TOTAL,&param->total);

    while(iloop < 8)
    {
        value = 0;
        if(CM_OK == cm_omi_obj_key_get_u32_ex(ObjParam,cols_u8[iloop],&value))
        {
            CM_OMI_FIELDS_FLAG_SET(&param->set,cols_u8[iloop]);
            *(val_u8[iloop]) = (uint8)value;
        }
        iloop++;
    }

    if(100 <= info->alarm_threshold)
    {
        CM_LOG_ERR(CM_MOD_CNM,"alarm_threshold[%u]",info->alarm_threshold);
        CM_FREE(param);
        return CM_PARAM_ERR;
    }
    
    if(CM_OK == cm_omi_obj_key_get_u32_ex(ObjParam,CM_OMI_FIELD_LUN_BLOCKSIZE,&value))
    {
        if((4 > value) /* 下限 */
            || (512 < value) /* 上限 */
            || (value & (value-1))) /* 2的N次方 */
        {
            CM_LOG_ERR(CM_MOD_CNM,"blocksize[%u]",value);
            CM_FREE(param);
            return CM_PARAM_ERR;
        }
        CM_OMI_FIELDS_FLAG_SET(&param->set,CM_OMI_FIELD_LUN_BLOCKSIZE);
        info->blocksize = value;
    }
    
    if(CM_OK == cm_omi_obj_key_get_str_ex(ObjParam,CM_OMI_FIELD_LUN_POOL,info->pool,sizeof(info->pool)))
    {
        CM_OMI_FIELDS_FLAG_SET(&param->set,CM_OMI_FIELD_LUN_POOL);
    }
    if(CM_OK == cm_omi_obj_key_get_str_ex(ObjParam,CM_OMI_FIELD_LUN_NAME,info->name,sizeof(info->name)))
    {
        CM_OMI_FIELDS_FLAG_SET(&param->set,CM_OMI_FIELD_LUN_NAME);
    }
    if(CM_OK == cm_omi_obj_key_get_str_ex(ObjParam,CM_OMI_FIELD_LUN_TOTAL,info->space_total,sizeof(info->space_total)))
    {
        CM_OMI_FIELDS_FLAG_SET(&param->set,CM_OMI_FIELD_LUN_TOTAL);
    }
    if(CM_OMI_FIELDS_FLAG_ISSET(&param->set,CM_OMI_FIELD_LUN_QOS)
        && (CM_CNM_LUN_QOS_OFF != info->qos))
    {
        sint32 iRet = CM_FAIL;
        if(CM_OK == cm_omi_obj_key_get_str_ex(ObjParam,CM_OMI_FIELD_LUN_QOS_VAL,info->qos_val,sizeof(info->qos_val)))
        {
            CM_OMI_FIELDS_FLAG_SET(&param->set,CM_OMI_FIELD_LUN_QOS_VAL);
        }
        if(CM_CNM_LUN_QOS_IOPS == info->qos)
        {
            iRet = cm_regex_check_num(info->qos_val,4);
        }
        else if(CM_CNM_LUN_QOS_BW == info->qos)
        {
            iRet = cm_regex_check(info->qos_val,CmOmiCheckSizeRegex);
        }
        else
        {
            ;
        }
        if(CM_OK != iRet)
        {
            CM_LOG_ERR(CM_MOD_CNM,"qos[%u] qos_val[%s]",info->qos,info->qos_val);
            CM_FREE(param);
            return CM_PARAM_ERR;
        }
    }
    
    *ppDecodeParam = param;
    param->total = CM_MIN(param->total,CM_CNM_MAX_RECORD);
    return CM_OK;
}

static void cm_cnm_lun_encode_each(cm_omi_obj_t item,void *eachdata,void *arg)
{
    cm_cnm_lun_info_t *info = eachdata;
    cm_omi_field_flag_t *pfield = arg;
    uint32 iloop = 0;
    uint32 cnt = 0;
    cm_cnm_map_value_str_t cols_str[] = {
            {CM_OMI_FIELD_LUN_NAME,     info->name},
            {CM_OMI_FIELD_LUN_POOL,     info->pool},
            {CM_OMI_FIELD_LUN_TOTAL,    info->space_total},
            {CM_OMI_FIELD_LUN_USED,     info->space_used},
            {CM_OMI_FIELD_LUN_AVAIL,    info->pool_avail},
            {CM_OMI_FIELD_LUN_QOS_VAL,  info->qos_val},
        };
    cm_cnm_map_value_num_t cols_num[] = {
            {CM_OMI_FIELD_LUN_NID,          info->nid},
            {CM_OMI_FIELD_LUN_BLOCKSIZE,    (uint32)info->blocksize},
            {CM_OMI_FIELD_LUN_WRITE_POLICY, (uint32)info->write_policy},
            {CM_OMI_FIELD_LUN_AC_STATE,     (uint32)info->access_state},
            {CM_OMI_FIELD_LUN_STATE,        (uint32)info->state},
            {CM_OMI_FIELD_LUN_IS_DOUBLE,    (uint32)info->is_double},
            {CM_OMI_FIELD_LUN_IS_COMPRESS,  (uint32)info->is_compress},
            {CM_OMI_FIELD_LUN_IS_HOT,       (uint32)info->is_hot},
            {CM_OMI_FIELD_LUN_IS_SINGLE,    (uint32)info->is_single},
            {CM_OMI_FIELD_LUN_ALARM_THRESHOLD,  (uint32)info->alarm_threshold},
            {CM_OMI_FIELD_LUN_QOS,          (uint32)info->qos},
            {CM_OMI_FIELD_LUN_DEDUP,          (uint32)info->dedup},
        };

    for(iloop=0, cnt=sizeof(cols_str)/sizeof(cm_cnm_map_value_str_t); iloop<cnt; iloop++)
    {
        if(CM_OMI_FIELDS_FLAG_ISSET(pfield,cols_str[iloop].id))
        {
            (void)cm_omi_obj_key_set_str_ex(item,cols_str[iloop].id,cols_str[iloop].value);
        }
    }
    
    for(iloop=0, cnt=sizeof(cols_num)/sizeof(cm_cnm_map_value_num_t); iloop<cnt; iloop++)
    {
        if(CM_OMI_FIELDS_FLAG_ISSET(pfield,cols_num[iloop].id))
        {
            (void)cm_omi_obj_key_set_u32_ex(item,cols_num[iloop].id,cols_num[iloop].value);
        }
    }

    return;
}    

cm_omi_obj_t cm_cnm_lun_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen)
{
    cm_omi_field_flag_t field;
    const cm_cnm_lun_param_t *param = pDecodeParam;
    if(pDecodeParam == NULL)
    {
        CM_OMI_FIELDS_FLAG_SET_ALL(&field);
    }
    else
    {
        CM_MEM_CPY(&field,sizeof(field),&param->field,sizeof(field));
    }
    return cm_cnm_encode_comm(pAckData,AckLen,
        sizeof(cm_cnm_lun_info_t),cm_cnm_lun_encode_each,&field);
}

static sint32 cm_cnm_lun_requst(uint32 cmd,const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    const cm_cnm_lun_param_t *pDecode = pDecodeParam;
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
        param.param_len = sizeof(cm_cnm_lun_param_t);
    }
    param.obj = CM_OMI_OBJECT_LUN;
    param.cmd = cmd;
    param.ppack = ppAckData;
    param.acklen = pAckLen;
    return cm_cnm_request(&param);
}

sint32 cm_cnm_lun_getbatch(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_cnm_lun_requst(CM_OMI_CMD_GET_BATCH,pDecodeParam,ppAckData,pAckLen);
}

sint32 cm_cnm_lun_get(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    const cm_cnm_lun_param_t *req = pDecodeParam;

    if(NULL == req)
    {
        CM_LOG_ERR(CM_MOD_CNM,"param null");
        return CM_PARAM_ERR;
    }
    if(!CM_OMI_FIELDS_FLAG_ISSET(&req->set,CM_OMI_FIELD_LUN_NID))
    {
        CM_LOG_ERR(CM_MOD_CNM,"pool null");
        return CM_PARAM_ERR;
    }
    if(!CM_OMI_FIELDS_FLAG_ISSET(&req->set,CM_OMI_FIELD_LUN_POOL))
    {
        CM_LOG_ERR(CM_MOD_CNM,"pool null");
        return CM_PARAM_ERR;
    }
    if(!CM_OMI_FIELDS_FLAG_ISSET(&req->set,CM_OMI_FIELD_LUN_NAME))
    {
        CM_LOG_ERR(CM_MOD_CNM,"lunname null");
        return CM_PARAM_ERR;
    }
    
    return cm_cnm_lun_requst(CM_OMI_CMD_GET,pDecodeParam,ppAckData,pAckLen);
}      
sint32 cm_cnm_lun_create(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    const cm_cnm_lun_param_t *req = pDecodeParam;
    const cm_cnm_lun_info_t *info = NULL;
    if(NULL == req)
    {
        CM_LOG_ERR(CM_MOD_CNM,"param null");
        return CM_PARAM_ERR;
    }
    info = &req->info;
    if(!CM_OMI_FIELDS_FLAG_ISSET(&req->set,CM_OMI_FIELD_LUN_NID))
    {
        CM_LOG_ERR(CM_MOD_CNM,"nid null");
        return CM_PARAM_ERR;
    }
    if(!CM_OMI_FIELDS_FLAG_ISSET(&req->set,CM_OMI_FIELD_LUN_POOL))
    {
        CM_LOG_ERR(CM_MOD_CNM,"pool null");
        return CM_PARAM_ERR;
    }
    if(!CM_OMI_FIELDS_FLAG_ISSET(&req->set,CM_OMI_FIELD_LUN_NAME))
    {
        CM_LOG_ERR(CM_MOD_CNM,"lunname null");
        return CM_PARAM_ERR;
    }
    if(!CM_OMI_FIELDS_FLAG_ISSET(&req->set,CM_OMI_FIELD_LUN_TOTAL))
    {
        CM_LOG_ERR(CM_MOD_CNM,"valsize null");
        return CM_PARAM_ERR;
    }
    CM_LOG_WARNING(CM_MOD_CNM,"%s/%s",info->pool,info->name);
    return cm_cnm_lun_requst(CM_OMI_CMD_ADD,pDecodeParam,ppAckData,pAckLen);
}    
    
sint32 cm_cnm_lun_update(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    const cm_cnm_lun_param_t *req = pDecodeParam;
    const cm_cnm_lun_info_t *info = NULL;
    
    if(NULL == req)
    {
        CM_LOG_ERR(CM_MOD_CNM,"param null");
        return CM_PARAM_ERR;
    }
    info = &req->info;
    
    if(!CM_OMI_FIELDS_FLAG_ISSET(&req->set,CM_OMI_FIELD_LUN_NID))
    {
        CM_LOG_ERR(CM_MOD_CNM,"nid null");
        return CM_PARAM_ERR;
    }
    if(!CM_OMI_FIELDS_FLAG_ISSET(&req->set,CM_OMI_FIELD_LUN_POOL))
    {
        CM_LOG_ERR(CM_MOD_CNM,"pool null");
        return CM_PARAM_ERR;
    }
    if(!CM_OMI_FIELDS_FLAG_ISSET(&req->set,CM_OMI_FIELD_LUN_NAME))
    {
        CM_LOG_ERR(CM_MOD_CNM,"lunname null");
        return CM_PARAM_ERR;
    }
    CM_LOG_WARNING(CM_MOD_CNM,"%s/%s",info->pool,info->name);
    return cm_cnm_lun_requst(CM_OMI_CMD_MODIFY,pDecodeParam,ppAckData,pAckLen);
}    
    
sint32 cm_cnm_lun_count(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_cnm_lun_requst(CM_OMI_CMD_COUNT,pDecodeParam,ppAckData,pAckLen);
}    
    
sint32 cm_cnm_lun_delete(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    const cm_cnm_lun_param_t *req = pDecodeParam;
    const cm_cnm_lun_info_t *info = NULL;
    
    if(NULL == req)
    {
        CM_LOG_ERR(CM_MOD_CNM,"param null");
        return CM_PARAM_ERR;
    }
    info = &req->info;
    if(!CM_OMI_FIELDS_FLAG_ISSET(&req->set,CM_OMI_FIELD_LUN_NID))
    {
        CM_LOG_ERR(CM_MOD_CNM,"pool null");
        return CM_PARAM_ERR;
    }
    if(!CM_OMI_FIELDS_FLAG_ISSET(&req->set,CM_OMI_FIELD_LUN_POOL))
    {
        CM_LOG_ERR(CM_MOD_CNM,"pool null");
        return CM_PARAM_ERR;
    }
    if(!CM_OMI_FIELDS_FLAG_ISSET(&req->set,CM_OMI_FIELD_LUN_NAME))
    {
        CM_LOG_ERR(CM_MOD_CNM,"lunname null");
        return CM_PARAM_ERR;
    }
    CM_LOG_WARNING(CM_MOD_CNM,"%s/%s",info->pool,info->name);
    return cm_cnm_lun_requst(CM_OMI_CMD_DELETE,pDecodeParam,ppAckData,pAckLen);
}    

#if 0
/* 预留，采用XML格式查询使用 */
static sint32 cm_cnm_lun_info_get(cm_xml_node_ptr *root)
{
    const sint8* xmlfile = "/tmp/lu.xml";
    sint32 iRet = CM_OK;
    sint8 cmd[CM_STRING_1K] = {0};
    cm_xml_node_ptr xmlinfo = NULL;
    
    CM_VSPRINTF(cmd,sizeof(cmd),"fmadm genxml -u 2>&1");
    CM_MUTEX_LOCK(&g_cm_cnm_lun_mutex);
    iRet = cm_exec_cmd_for_str(cmd,cmd,sizeof(cmd),CM_CMT_REQ_TMOUT);
    if(CM_OK == iRet)
    {
        xmlinfo = cm_xml_parse_file(xmlfile);
        if(NULL == xmlinfo)
        {
            CM_LOG_ERR(CM_MOD_CNM,"load %s fail",xmlfile);
            iRet = CM_ERR_POOL_DISK_XML;
        }
    }
    else
    {
        CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]\n%s",iRet,cmd);
        iRet = cm_cnm_get_errcode(&CmCnmMapLunErrCfg,cmd,iRet);
    }
    CM_MUTEX_UNLOCK(&g_cm_cnm_lun_mutex);

    *root = xmlinfo;
    return iRet;
}

/*
<lu hostname="dfb" type="volume" name="poolb/lun01" guid="
600144f00200000000005aec1ce40001" status="Online" datafile="lun01" state="
Active" activeid="" available="1.78T" encryption="off" isworm="-" quota="-" 
recordsize="-" refreservation="102G" sharenfs="-" sharesmb="-" sync="poweroff
" used="102G" volblocksize="16K" volsize="100G" vscan="-" wormreliance="-"/>
  <lu hostname="dfb" type="volume" name="poolb/lun01_bit0" guid="
600144f00200000000005aec1d3b0002" status="Online" datafile="/dev/zvol/rdsk/
poolb/lun01_bit0" state="Active-&gt;Standby" activeid="" available="1.68T" 
encryption="off" isworm="-" quota="-" recordsize="-" refreservation="414M" 
sharenfs="-" sharesmb="-" sync="standard" used="414M" volblocksize="8K" 
volsize="401M" vscan="-" wormreliance="-"/>
  <lu hostname="dfb" type="volume" name="tank/wbnlun" guid="
600144f00200000000005afcf35e0001" status="Online" datafile="wbnlun" state="
Active" activeid="" available="509G" encryption="off" isworm="-" quota="-" 
recordsize="-" refreservation="none" sharenfs="-" sharesmb="-" sync="disk" 
used="36K" volblocksize="64K" volsize="80G" vscan="-" wormreliance="-"/>

*/
static sint32 cm_cnm_lun_local_check(cm_xml_node_ptr curnode,void *arg)
{
    const sint8* valstr = cm_xml_get_attr(curnode,"name");
    
    if(NULL == valstr)
    {
        CM_LOG_WARNING(CM_MOD_CNM,"get name fail"); 
        return CM_FAIL;
    }

    valstr = strstr(valstr,"_bit");
    if(NULL == valstr)
    {
        return CM_FAIL;
    }
    return CM_OK;
}

static sint32 cm_cnm_lun_local_geteach(cm_xml_node_ptr curnode,void *arg)
{
    cm_cnm_lun_info_t *info = arg;
    const sint8* attrs[] = {"sync","state","status"};
    uint32* vals[] = {&info->sync, &info->access_state, &info->state};
    const cm_omi_map_enum_t* Maps[]={
        &CmOmiMapLunSyncTypeCfg,
        &CmOmiMapLunStateTypeCfg,
        &CmOmiMapEnumPoolStatusType};

    const sint8* attrsize[] = {"volsize","used","available"};
    sint8* bufs[] = {info->space_total,info->space_used,info->pool_avail};
    
    const sint8* valstr = cm_xml_get_attr(curnode,"name");
    sint32 num = 0;

    if(NULL == valstr)
    {
        CM_LOG_WARNING(CM_MOD_CNM,"get name fail"); 
        return CM_FAIL;
    }
    /* 这里使用非安全函数，异常情况会出问题，当前版本不支持安全函数 */
    num = sscanf(valstr,"%[a-zA-Z0-9]/%[a-zA-Z0-9]",info->pool,info->name);
    if(num < 2)
    {
        CM_LOG_WARNING(CM_MOD_CNM,"name[%s]",valstr); 
        return CM_FAIL;
    }    
    info->nid = cm_node_get_id();
    for(num=0;num<3;num++)
    {
        valstr = cm_xml_get_attr(curnode,attrs[num]);
        if(NULL == valstr)
        {
            CM_LOG_WARNING(CM_MOD_CNM,"get %s fail", attrs[num]); 
            return CM_FAIL;
        }
        *(vals[num]) = cm_cnm_get_enum(Maps[num],valstr,0);

        valstr = cm_xml_get_attr(curnode,attrsize[num]);
        if(NULL == valstr)
        {
            CM_LOG_WARNING(CM_MOD_CNM,"get %s fail", attrsize[num]); 
            return CM_FAIL;
        }
        CM_VSPRINTF(bufs[num],sizeof(info->space_total),"%s",valstr);
    }
    
    valstr = cm_xml_get_attr(curnode,"refreservation");
    if(NULL == valstr)
    {
        CM_LOG_WARNING(CM_MOD_CNM,"get refreservation fail"); 
        return CM_FAIL;
    }
    info->is_single = (0 == strcmp(valstr,"none")) ? CM_TRUE : CM_FALSE;

    valstr = cm_xml_get_attr(curnode,"volblocksize");
    if(NULL == valstr)
    {
        CM_LOG_WARNING(CM_MOD_CNM,"get volblocksize fail"); 
        return CM_FAIL;
    }
    info->blocksize = (uint32)atoi(valstr);

    return CM_OK;
}

sint32 cm_cnm_lun_local_getbatch(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    cm_xml_node_ptr xmlroot = NULL;
    sint32 iRet = CM_OK;
    cm_xml_request_param_t xmlparam;
    
    CM_LOG_INFO(CM_MOD_CNM,"offset[%llu] total[%u]",offset,total); 
    iRet = cm_cnm_lun_info_get(&xmlroot);
    if(CM_OK != iRet)
    {
        return iRet;
    }
    CM_MEM_ZERO(&xmlparam,sizeof(xmlparam));
    xmlparam.root = xmlroot;
    xmlparam.nodename = "lu";
    xmlparam.param = param;
    xmlparam.offset = (uint32)offset;
    xmlparam.total = total;
    xmlparam.eachsize = sizeof(cm_cnm_lun_info_t);
    xmlparam.check = cm_cnm_lun_local_check;
    xmlparam.geteach = cm_cnm_lun_local_geteach;
    xmlparam.ppAck = ppAck;
    xmlparam.pAckLen = pAckLen;
    iRet = cm_xml_request(&xmlparam);
    cm_xml_free(xmlroot);
    return iRet;
}    

sint32 cm_cnm_lun_local_count(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    cm_xml_node_ptr xmlroot = NULL;
    sint32 iRet = CM_OK;
    cm_xml_request_param_t xmlparam;
    
    iRet = cm_cnm_lun_info_get(&xmlroot);
    if(CM_OK != iRet)
    {
        return iRet;
    }
    CM_MEM_ZERO(&xmlparam,sizeof(xmlparam));
    xmlparam.root = xmlroot;
    xmlparam.nodename = "lu";
    xmlparam.param = param;
    xmlparam.check = cm_cnm_lun_local_check;
    xmlparam.ppAck = ppAck;
    xmlparam.pAckLen = pAckLen;
    iRet = cm_xml_request_count(&xmlparam);
    cm_xml_free(xmlroot);
    return CM_OK;
}    
#else

sint32 cm_cnm_lun_local_get_stmfid(const sint8* pool, const sint8* lun, sint8 *buf, uint32 len)
{
    sint32 iRet = CM_OK;
    
    buf[0] = '\0';

    switch(g_cm_sys_ver)
    {
        case CM_SYS_VER_SOLARIS_V7R16:
            if(NULL == lun)
            {
                iRet = cm_exec_tmout(buf,len,CM_CMT_REQ_TMOUT,
                    "%s |awk '$2==\"%s\"{printf $1}'",g_cm_cnm_lun_stmfid_map,pool);
            }
            else
            {
                iRet = cm_exec_tmout(buf,len,CM_CMT_REQ_TMOUT,
                    "%s |awk '$2==\"%s/%s\"{printf $1}'",g_cm_cnm_lun_stmfid_map,pool,lun);
            }
            break;
        default:
            if(NULL == lun)
            {
                iRet = cm_exec_tmout(buf,len,CM_CMT_REQ_TMOUT,
                    "stmfadm list-all-luns 2>/dev/null |awk '$2==\"%s\"{printf $1}'",pool);
            }
            else
            {
                iRet = cm_exec_tmout(buf,len,CM_CMT_REQ_TMOUT,
                    "stmfadm list-all-luns 2>/dev/null |awk '$2==\"%s/%s\"{printf $1}'",pool,lun);
            }
            break;
    }
    

    if(CM_OK != iRet)
    {
        return iRet;
    }

    if('\0' == buf[0])
    {
        return CM_ERR_NOT_EXISTS;
    }
    return CM_OK;
}

static sint32 cm_cnm_lun_local_get_qos(cm_cnm_lun_info_t *info, const sint8* id)
{ 
    (void)cm_exec_tmout(info->qos_val,sizeof(info->qos_val),CM_CMT_REQ_TMOUT,
        "stmfadm get-iops-info %s 2>/dev/null |awk '{printf $6}'",id);
    if(0 != atoi(info->qos_val))
    {
        info->qos = CM_CNM_LUN_QOS_IOPS;
        return CM_OK;
    }

    (void)cm_exec_tmout(info->qos_val,sizeof(info->qos_val),CM_CMT_REQ_TMOUT,
        "stmfadm get-kbps %s 2>/dev/null |awk '{printf $6}'",id);
    if(0 != atoi(info->qos_val))
    {
        info->qos = CM_CNM_LUN_QOS_BW;
        return CM_OK;
    }
    info->qos = CM_CNM_LUN_QOS_OFF;
    return CM_OK;
}

/* TODO: 告警阈值，thold */

static sint32 cm_cnm_lun_local_get_each(void *arg, sint8 **cols, uint32 col_num)
{
    sint32 iRet = CM_OK;
    sint8 buf[CM_STRING_1K] = {0};
    sint8 sync[CM_STRING_64] = {0};
    sint8 refreservation[CM_STRING_64] = {0};
    cm_cnm_lun_info_t *info = arg;
    const uint32 def_num = (g_cm_sys_ver == CM_SYS_VER_SOLARIS_V7R16)? 10:12;    
    
    if(def_num != col_num)
    {
        CM_LOG_WARNING(CM_MOD_CNM,"col_num[%u] def_num[%u]",col_num,def_num);
        return CM_FAIL;
    }
    CM_MEM_ZERO(info,sizeof(cm_cnm_lun_info_t));
    /* 这里使用非安全函数，异常情况会出问题，当前版本不支持安全函数 */
    col_num = sscanf(cols[0],"%[a-zA-Z0-9_.-]/%[a-zA-Z0-9_.-]",info->pool,info->name);
    if(col_num < 2)
    {
        CM_LOG_WARNING(CM_MOD_CNM,"name[%s]",cols[0]); 
        return CM_FAIL;
    }
    info->nid = cm_node_get_id();
    CM_VSPRINTF(info->space_used,sizeof(info->space_used),"%s",cols[1]);
    CM_VSPRINTF(info->pool_avail,sizeof(info->pool_avail),"%s",cols[2]);
    CM_VSPRINTF(info->space_total,sizeof(info->space_total),"%s",cols[3]);
    info->blocksize = (uint16)atoi(cols[4]);
    info->is_single = (0 == strcmp(cols[5],"none")) ? CM_TRUE : CM_FALSE;
    info->is_compress = (0 == strcmp(cols[6],"on")) ? CM_TRUE : CM_FALSE;
    info->write_policy = (uint8)cm_cnm_get_enum(&CmOmiMapLunSyncTypeCfg,cols[7],0);
    info->is_hot = (0 == strcmp(cols[8],"on")) ? CM_TRUE : CM_FALSE;
    info->dedup = (0 == strcmp(cols[9],"on")) ? CM_TRUE : CM_FALSE;

    if(col_num == 12)
    {
        info->is_double = (0 != strcmp(cols[10],"1")) ? CM_TRUE : CM_FALSE;
        info->alarm_threshold = (uint8)atoi(cols[11]);
    }
    
    /* 复用一下sync 作为stmfid */
    iRet = cm_cnm_lun_local_get_stmfid(info->pool,info->name,sync,sizeof(sync));
    if(CM_OK != iRet)
    {
        CM_LOG_WARNING(CM_MOD_CNM,"get %s stmfid fail",cols[0]); 
        return CM_OK;
    }
    if(g_cm_sys_ver != CM_SYS_VER_SOLARIS_V7R16)
    {
        (void)cm_cnm_lun_local_get_qos(info,sync);
    }
    
    //CM_LOG_WARNING(CM_MOD_CNM,"%s",sync); 
    CM_VSPRINTF(buf,sizeof(buf),
        "stmfadm list-lu -v %s 2>/dev/null |egrep 'Operational|Access' "
        "|awk -F':' '{print $2}' |"CM_CNM_TRIM_LEFG,sync);
    iRet = cm_exec_cmd_for_str(buf,buf,sizeof(buf),CM_CMT_REQ_TMOUT);
    if(CM_OK != iRet)
    {
        CM_LOG_WARNING(CM_MOD_CNM,"get %s state fail",cols[0]); 
        return CM_OK;
    }
    col_num = sscanf(buf,"%[a-zA-Z->]\n%[a-zA-Z->]",sync,refreservation);
    if(col_num != 2)
    {
        CM_LOG_WARNING(CM_MOD_CNM,"get %s state col_num[%u]",cols[0],col_num); 
        return CM_OK;
    }
    //CM_LOG_WARNING(CM_MOD_CNM,"%s %s",sync,refreservation); 
    info->state = (uint8)cm_cnm_get_enum(&CmOmiMapEnumPoolStatusType,sync,0);
    info->access_state = (uint8)cm_cnm_get_enum(&CmOmiMapLunStateTypeCfg,refreservation,0);
    
    return CM_OK;
}
 
sint32 cm_cnm_lun_local_getbatch(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    sint32 iRet = CM_OK;
    sint8 cmd[CM_STRING_512] = {0};
    CM_LOG_INFO(CM_MOD_CNM,"offset[%llu] total[%u]",offset,total);

    CM_VSPRINTF(cmd,sizeof(cmd),"%s |grep -v _bit0",g_cm_cnm_lun_get_cmd);
    iRet = cm_cnm_exec_get_list(cmd,cm_cnm_lun_local_get_each,
        (uint32)offset,sizeof(cm_cnm_lun_info_t),ppAck,&total);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]",iRet);
        return iRet;
    }
    *pAckLen = total * sizeof(cm_cnm_lun_info_t);
    return CM_OK;
}

sint32 cm_cnm_lun_local_get(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    sint32 iRet = CM_OK;
    cm_cnm_lun_param_t *req = param;
    cm_cnm_lun_info_t *info = NULL;

    if(NULL == req)
    {
        CM_LOG_ERR(CM_MOD_CNM,"param null");
        return CM_PARAM_ERR; 
    }
    info = CM_MALLOC(sizeof(cm_cnm_lun_info_t));
    if(NULL == info)
    {
        CM_LOG_ERR(CM_MOD_CNM,"malloc fail");
        return CM_FAIL;
    }
    
    CM_LOG_INFO(CM_MOD_CNM,"%s/%s",req->info.pool,req->info.name);
    
    iRet = cm_cnm_exec_get_col(cm_cnm_lun_local_get_each,info,
        "%s %s/%s",g_cm_cnm_lun_get_cmd,req->info.pool,req->info.name);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]",iRet);
        CM_FREE(info);
        return CM_OK;
    }
    *ppAck = info;
    *pAckLen = sizeof(cm_cnm_lun_info_t);
    return CM_OK;
}

sint32 cm_cnm_lun_local_count(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    uint64 cnt = 0;
    const sint8 *cmd = "zfs list -H -t volume -o name |grep -v _bit0";
    
    cnt = (uint64)cm_exec_double("%s 2>/dev/null |wc -l",cmd);
    
    return cm_cnm_ack_uint64(cnt,ppAck,pAckLen);
}

#endif

static sint32 cm_cnm_lun_local_qos_update(cm_cnm_lun_param_t *req, const sint8* stmfid)
{
    sint32 iRet = CM_OK;
    cm_cnm_lun_info_t *info = &req->info;
    const sint8 *pFormat = "stmfadm %s -c -l %s -n %s 1>/dev/null 2>/dev/null";
    const sint8 *subcmd = NULL;
    if(!CM_OMI_FIELDS_FLAG_ISSET(&req->set,CM_OMI_FIELD_LUN_QOS))
    {
        return CM_OK;
    }
    CM_LOG_INFO(CM_MOD_CNM,"id[%s] qos[%u] val[%s]",stmfid,info->qos,info->qos_val);
    /* 只设置一个有效 
    设置kbps 0时也会同时设置iops为0
    */
    if(CM_CNM_LUN_QOS_IOPS == info->qos)
    {
        subcmd = "set-iops-limit";
    }
    else
    {
        subcmd = "set-kbps";
    }
    
    if('\0' != info->qos_val[0])
    {
        iRet = cm_system_no_tmout(pFormat,subcmd,stmfid,info->qos_val);
    }
    else
    {
        iRet = cm_system_no_tmout(pFormat,subcmd,stmfid,"0");
    }

    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"set %s %s fail[%d]",stmfid,subcmd,iRet);
        return iRet;
    }
    return CM_OK;
}

static uint64 cm_cnm_lun_volsize_tokb(const sint8* volsize,uint32 blocksize)
{
    double dval=atof(volsize);
    sint32 len=strlen(volsize);
    sint8 flag=0;
    uint64 res=0;
    if(len < 2)
    {
        return 0;
    }
    len--;
    if((volsize[len]=='b') || (volsize[len]=='B'))
    {
        flag = volsize[len-1];
    }
    else
    {
        flag = volsize[len];
    }
    
    switch(flag)
    {
        case 'p':
        case 'P':
            dval *=1024;
        case 't':
        case 'T':
            dval *=1024;
        case 'g':
        case 'G':
            dval *=1024;
        case 'm':
        case 'M':
            dval *=1024;
        case 'k':
        case 'K':
            break;
        case 'b':
        case 'B':
            dval /=1024;
            break;
        default:
            dval=0;
            break;
    }
    res = (uint64)dval;
    return res-(res%blocksize);
}

#if 0
sint32 cm_cnm_lun_local_create(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    sint32 iRet = CM_FAIL;
    sint8 buf[CM_STRING_1K] = {0};
    sint8 id[CM_STRING_64] = {0};
    cm_cnm_lun_param_t *req = param;
    cm_cnm_lun_info_t *info = &req->info;
    uint32 blocksize=128;
    uint64 volsize = 0;
    
    CM_LOG_WARNING(CM_MOD_CNM,"create %s/%s",info->pool,info->name);
    CM_SNPRINTF_ADD(buf,sizeof(buf),"zfs create");
    if(CM_OMI_FIELDS_FLAG_ISSET(&req->set,CM_OMI_FIELD_LUN_IS_SINGLE)
        && (CM_TRUE == info->is_single))
    {
        CM_SNPRINTF_ADD(buf,sizeof(buf)," -s");
    }
    
    if(CM_OMI_FIELDS_FLAG_ISSET(&req->set,CM_OMI_FIELD_LUN_BLOCKSIZE))
    {
        CM_SNPRINTF_ADD(buf,sizeof(buf)," -b %uK",info->blocksize);
        blocksize = info->blocksize;
    }    
    
    if(CM_OMI_FIELDS_FLAG_ISSET(&req->set,CM_OMI_FIELD_LUN_IS_COMPRESS))
    {
        if(CM_TRUE == info->is_compress)
        {
            CM_SNPRINTF_ADD(buf,sizeof(buf)," -o compression=on");
        }
        else
        {
            CM_SNPRINTF_ADD(buf,sizeof(buf)," -o compression=off");
        }
    }
    
    if(CM_OMI_FIELDS_FLAG_ISSET(&req->set,CM_OMI_FIELD_LUN_IS_HOT))
    {
        if(CM_TRUE == info->is_compress)
        {
            CM_SNPRINTF_ADD(buf,sizeof(buf)," -o appmeta=on");
        }
        else
        {
            CM_SNPRINTF_ADD(buf,sizeof(buf)," -o appmeta=off");
        }
    }
    
    if(CM_OMI_FIELDS_FLAG_ISSET(&req->set,CM_OMI_FIELD_LUN_WRITE_POLICY))
    {
        const sint8* policy = cm_cnm_get_enum_str(&CmOmiMapLunSyncTypeCfg,info->write_policy);
        if(NULL == policy)
        {
            CM_LOG_ERR(CM_MOD_CNM,"policy[%u]",info->write_policy);
            return CM_PARAM_ERR;
        }
        CM_SNPRINTF_ADD(buf,sizeof(buf)," -o origin:sync=%s",policy);
    }

    /* begin for 8219 */
    volsize = cm_cnm_lun_volsize_tokb(info->space_total, blocksize);
    CM_LOG_WARNING(CM_MOD_CNM,"blocksize:%u volsize:%s = %lluK",
        blocksize,info->space_total,volsize);
    if(volsize == 0)
    {
        return CM_PARAM_ERR;
    }
    CM_SNPRINTF_ADD(buf,sizeof(buf)," -V %lluK %s/%s 2>&1", volsize,info->pool,info->name);
    /* end for 8219 */
    iRet = cm_exec_cmd_for_str(buf,buf,sizeof(buf),CM_CMT_REQ_TMOUT_NEVER);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]\n%s",iRet,buf);
        return cm_cnm_get_errcode(&CmCnmMapCommErrCfg,buf,iRet);
    }    

    if(CM_OMI_FIELDS_FLAG_ISSET(&req->set,CM_OMI_FIELD_LUN_DEDUP))
    {
        const sint8* str = (CM_TRUE == info->dedup)? "on" : "off";
        iRet = cm_exec_tmout(buf,sizeof(buf),CM_CMT_REQ_TMOUT_NEVER,
            "zfs set dedup=%s %s/%s 2>&1",
            str,info->pool,info->name);
        if(CM_OK != iRet)
        {
            CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]\n%s",iRet,buf);
        }
    }
    
    if(CM_OMI_FIELDS_FLAG_ISSET(&req->set,CM_OMI_FIELD_LUN_IS_DOUBLE))
    {
        if(CM_TRUE == info->is_double)
        {
            cm_system_no_tmout("zfs set zfs:single_data=0 %s/%s 2>/dev/null",info->pool,info->name);
        }
        else
        {
            cm_system_no_tmout("zfs set zfs:single_data=1 %s/%s 2>/dev/null",info->pool,info->name);
        }
    }

    if(g_cm_sys_ver == CM_SYS_VER_SOLARIS_V7R16)
    {
        return iRet;
    }
    
    if(CM_OMI_FIELDS_FLAG_ISSET(&req->set,CM_OMI_FIELD_LUN_ALARM_THRESHOLD))
    {
        cm_system_no_tmout("zfs set thold=%u %s/%s 2>/dev/null",info->alarm_threshold,info->pool,info->name);
    }
    /* 设置失败也不用返回错误了 */
    iRet = cm_cnm_lun_local_get_stmfid(info->pool,info->name,id,sizeof(id));
    if(CM_OK == iRet)
    {
        (void)cm_cnm_lun_local_qos_update(req,id);
    }
    
    return CM_OK;
}   

sint32 cm_cnm_lun_local_update(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    sint32 iRet = CM_OK;
    cm_cnm_lun_param_t *req = param;
    cm_cnm_lun_info_t *info = &req->info;
    const sint8* str = NULL;
    sint8 buf[CM_STRING_1K] = {0};
    sint8 id[CM_STRING_64] = {0};

    iRet = cm_cnm_lun_local_get_stmfid(info->pool,info->name,id,sizeof(id));
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"get %s/%s stmfid fail[%d]",info->pool,info->name,iRet);
        return iRet;
    }
    /* 这里比较麻烦，底层不支持多个属性的修改，只有一个个修改 */
    if(CM_OMI_FIELDS_FLAG_ISSET(&req->set,CM_OMI_FIELD_LUN_IS_COMPRESS))
    {
        str = (CM_TRUE == info->is_compress)? "on" : "off";
        iRet = cm_exec_tmout(buf,sizeof(buf),CM_CMT_REQ_TMOUT_NEVER,
            "zfs set compression=%s %s/%s 2>&1",str,info->pool,info->name);
        if(CM_OK != iRet)
        {
            CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]\n%s",iRet,buf);
            return cm_cnm_get_errcode(&CmCnmMapCommErrCfg,buf,iRet);
        }
    }

    if(CM_OMI_FIELDS_FLAG_ISSET(&req->set,CM_OMI_FIELD_LUN_IS_HOT))
    {
        str = (CM_TRUE == info->is_hot)? "on" : "off";
        iRet = cm_exec_tmout(buf,sizeof(buf),CM_CMT_REQ_TMOUT_NEVER,
            "zfs set appmeta=%s %s/%s 2>&1",str,info->pool,info->name);
        if(CM_OK != iRet)
        {
            CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]\n%s",iRet,buf);
            return cm_cnm_get_errcode(&CmCnmMapCommErrCfg,buf,iRet);
        }
    }
    
    if(CM_OMI_FIELDS_FLAG_ISSET(&req->set,CM_OMI_FIELD_LUN_WRITE_POLICY))
    {
        str = cm_cnm_get_enum_str(&CmOmiMapLunSyncTypeCfg,info->write_policy);
        if(NULL == str)
        {
            CM_LOG_ERR(CM_MOD_CNM,"policy[%u]",info->write_policy);
            return CM_PARAM_ERR;
        }
        iRet = cm_exec_tmout(buf,sizeof(buf),CM_CMT_REQ_TMOUT_NEVER,
            "zfs set origin:sync=%s %s/%s 2>&1",str,info->pool,info->name);
        if(CM_OK != iRet)
        {
            CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]\n%s",iRet,buf);
            return cm_cnm_get_errcode(&CmCnmMapCommErrCfg,buf,iRet);
        }
    }
    
    if(CM_OMI_FIELDS_FLAG_ISSET(&req->set,CM_OMI_FIELD_LUN_TOTAL))
    {
        /* begin for 8219 */
        uint32 blocksize=cm_exec_int("zfs get volblocksize %s/%s |sed -n 2p |awk '{print $3}'",
            info->pool,info->name);
        uint64 volsize = 0;
        if(blocksize == 0)
        {
            blocksize=512;
            CM_LOG_ERR(CM_MOD_CNM,"get %s/%s blocksize 0",info->pool,info->name);
        }
        volsize = cm_cnm_lun_volsize_tokb(info->space_total,blocksize);
        if(volsize == 0)
        {
            CM_LOG_ERR(CM_MOD_CNM,"get %s/%s volsize 0",info->pool,info->name);
            return CM_PARAM_ERR;
        }
        /* end for 8219 */
        iRet = cm_exec_tmout(buf,sizeof(buf),CM_CMT_REQ_TMOUT_NEVER,
            "stmfadm modify-lu -c -s %lluK %s 2>&1",volsize,id);
        if(CM_OK != iRet)
        {
            CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]\n%s",iRet,buf);
            return cm_cnm_get_errcode(&CmCnmMapCommErrCfg,buf,iRet);
        }
        
        iRet = cm_exec_tmout(buf,sizeof(buf),CM_CMT_REQ_TMOUT_NEVER,
            "zfs set volsize=%lluK %s/%s 2>&1",volsize,info->pool,info->name);
        if(CM_OK != iRet)
        {
            CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]\n%s",iRet,buf);
            return cm_cnm_get_errcode(&CmCnmMapCommErrCfg,buf,iRet);
        }
    }    

    if(CM_OMI_FIELDS_FLAG_ISSET(&req->set,CM_OMI_FIELD_LUN_DEDUP))
    {
        str = (CM_TRUE == info->dedup)? "on" : "off";
        iRet = cm_exec_tmout(buf,sizeof(buf),CM_CMT_REQ_TMOUT_NEVER,
            "zfs set dedup=%s %s/%s 2>&1",
            str,info->pool,info->name);
        if(CM_OK != iRet)
        {
            CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]\n%s",iRet,buf);
            return cm_cnm_get_errcode(&CmCnmMapCommErrCfg,buf,iRet);;
        }
    }

    if(CM_OMI_FIELDS_FLAG_ISSET(&req->set,CM_OMI_FIELD_LUN_IS_DOUBLE))
    {     
        if(CM_TRUE == info->is_double)
        {
            iRet = cm_exec_tmout(buf,sizeof(buf),CM_CMT_REQ_TMOUT_NEVER,
                "zfs set zfs:single_data=0 %s/%s 2>/dev/null",info->pool,info->name);
        }
        else
        {
            iRet = cm_exec_tmout(buf,sizeof(buf),CM_CMT_REQ_TMOUT_NEVER,
                "zfs set zfs:single_data=1 %s/%s 2>/dev/null",info->pool,info->name);
        }
        if(CM_OK != iRet)
        {
            CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]\n%s",iRet,buf);
            return cm_cnm_get_errcode(&CmCnmMapCommErrCfg,buf,iRet);
        }
    }

    if(g_cm_sys_ver == CM_SYS_VER_SOLARIS_V7R16)
    {
        return iRet;
    }
    
    if(CM_OMI_FIELDS_FLAG_ISSET(&req->set,CM_OMI_FIELD_LUN_ALARM_THRESHOLD))
    {
        iRet = cm_exec_tmout(buf,sizeof(buf),CM_CMT_REQ_TMOUT_NEVER,
            "zfs set thold=%u %s/%s 2>&1",info->alarm_threshold,info->pool,info->name);
        if(CM_OK != iRet)
        {
            CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]\n%s",iRet,buf);
            return cm_cnm_get_errcode(&CmCnmMapCommErrCfg,buf,iRet);
        }
    }  
    
    return cm_cnm_lun_local_qos_update(req,id);
}    

sint32 cm_cnm_lun_local_delete(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    sint32 iRet = CM_FAIL;
    sint8 buf[CM_STRING_1K] = {0};
    sint8 id[CM_STRING_64] = {0};    
    cm_cnm_lun_param_t *req = param;
    cm_cnm_lun_info_t *info = NULL;
    uint32 cut = 0;
    
    info = &req->info;

    CM_LOG_WARNING(CM_MOD_CNM,"%s/%s",info->pool,info->name); 
    iRet = cm_exec_tmout(buf,sizeof(buf),CM_CMT_REQ_TMOUT_NEVER,
        CM_SHELL_EXEC" cm_lun_delete %s %s 2>&1",info->pool,info->name);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]\n%s",iRet,buf);
        return cm_cnm_get_errcode(&CmCnmMapCommErrCfg,buf,iRet);
    }
    return CM_OK;
}    


#else
static void cm_cnm_lun_local_params(void* params, sint8 *buf, sint32 len)
{
    uint32 blocksize=0;
    uint64 volsize = 0;
    cm_cnm_lun_param_t *req = params;
    cm_cnm_lun_info_t *info = &req->info;

    CM_SNPRINTF_ADD(buf,len,"%s/%s",info->pool,info->name);
    
    if(CM_OMI_FIELDS_FLAG_ISSET(&req->set,CM_OMI_FIELD_LUN_TOTAL))
    {
        if(CM_OMI_FIELDS_FLAG_ISSET(&req->set,CM_OMI_FIELD_LUN_BLOCKSIZE))
        {
            blocksize = info->blocksize;
        }
        else
        {
            blocksize = cm_exec_int("zfs get volblocksize %s/%s |sed -n 2p |awk '{print $3}'",
                info->pool,info->name);
        }
        
        if(0 == blocksize)
        {
            blocksize = 512;
        }
        volsize = cm_cnm_lun_volsize_tokb(info->space_total, blocksize);
        CM_LOG_WARNING(CM_MOD_CNM,"blocksize:%u volsize:%s = %lluK",
            blocksize,info->space_total,volsize);
        if(volsize == 0)
        {
            return CM_PARAM_ERR;
        }
        CM_SNPRINTF_ADD(buf,len,"|%lluK",volsize);
    }
    else
    {
        CM_SNPRINTF_ADD(buf,len,"|-");
    }
    
    if(CM_OMI_FIELDS_FLAG_ISSET(&req->set,CM_OMI_FIELD_LUN_IS_SINGLE))
    {
        CM_SNPRINTF_ADD(buf,len,"|%u",info->is_single);
    }
    else
    {
        CM_SNPRINTF_ADD(buf,len,"|-");
    }

    if(CM_OMI_FIELDS_FLAG_ISSET(&req->set,CM_OMI_FIELD_LUN_BLOCKSIZE))
    {
        CM_SNPRINTF_ADD(buf,len,"|%u",info->blocksize);
    }
    else
    {
        CM_SNPRINTF_ADD(buf,len,"|-");
    }
    
    if(CM_OMI_FIELDS_FLAG_ISSET(&req->set,CM_OMI_FIELD_LUN_IS_COMPRESS))
    {
        CM_SNPRINTF_ADD(buf,len,"|%u",info->is_compress);
    }
    else
    {
        CM_SNPRINTF_ADD(buf,len,"|-");
    }

    if(CM_OMI_FIELDS_FLAG_ISSET(&req->set,CM_OMI_FIELD_LUN_IS_HOT))
    {
        CM_SNPRINTF_ADD(buf,len,"|%u",info->is_hot);
    }
    else
    {
        CM_SNPRINTF_ADD(buf,len,"|-");
    }
    
    if(CM_OMI_FIELDS_FLAG_ISSET(&req->set,CM_OMI_FIELD_LUN_WRITE_POLICY))
    {
        const sint8* policy = cm_cnm_get_enum_str(&CmOmiMapLunSyncTypeCfg,info->write_policy);
        if(NULL == policy)
        {
            CM_LOG_ERR(CM_MOD_CNM,"policy[%u]",info->write_policy);
            return CM_PARAM_ERR;
        }
        CM_SNPRINTF_ADD(buf,len,"|%s",policy);
    }
    else
    {
        CM_SNPRINTF_ADD(buf,len,"|-");
    }

    if(CM_OMI_FIELDS_FLAG_ISSET(&req->set,CM_OMI_FIELD_LUN_DEDUP))
    {
        CM_SNPRINTF_ADD(buf,len,"|%u",info->dedup);
    }
    else
    {
        CM_SNPRINTF_ADD(buf,len,"|-");
    }
    
    if(CM_OMI_FIELDS_FLAG_ISSET(&req->set,CM_OMI_FIELD_LUN_IS_DOUBLE))
    {
        CM_SNPRINTF_ADD(buf,len,"|%u",info->is_double);
    }
    else
    {
        CM_SNPRINTF_ADD(buf,len,"|-");
    }

    if(CM_OMI_FIELDS_FLAG_ISSET(&req->set,CM_OMI_FIELD_LUN_ALARM_THRESHOLD))
    {
        CM_SNPRINTF_ADD(buf,len,"|%u",info->alarm_threshold);
    }
    else
    {
        CM_SNPRINTF_ADD(buf,len,"|-");
    }

    if(CM_OMI_FIELDS_FLAG_ISSET(&req->set,CM_OMI_FIELD_LUN_QOS))
    {
        CM_SNPRINTF_ADD(buf,len,"|%u",info->qos);
    }
    else
    {
        CM_SNPRINTF_ADD(buf,len,"|-");
    }

    if(CM_OMI_FIELDS_FLAG_ISSET(&req->set,CM_OMI_FIELD_LUN_QOS_VAL))
    {
        CM_SNPRINTF_ADD(buf,len,"|%s",info->qos_val);
    }
    else
    {
        CM_SNPRINTF_ADD(buf,len,"|-");
    }
    return;
}

sint32 cm_cnm_lun_local_exec(const sint8* act,
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    sint32 iRet = CM_FAIL;
    sint8 buf[CM_STRING_128] = {0};
    sint32 opts[CM_STRING_1K] = {0};
    cm_cnm_lun_param_t *req = param;
    cm_cnm_lun_info_t *info = &req->info;
    uint32 cnt=0;
    
    /*lunname|lunsize|is_single|blocksize|is_compress|is_hot|write_policy|dedup|is_double|thold|qos|qos_val*/
    CM_LOG_WARNING(CM_MOD_CNM,"%s %s/%s",act,info->pool,info->name);
    cm_cnm_lun_local_params(param,opts,sizeof(opts));

    iRet = cm_exec_out(ppAck,pAckLen,CM_CMT_REQ_TMOUT_NEVER,
        CM_SCRIPT_DIR"cm_cnm.sh lun_%s '%s' 2>&1",act,opts);
    if(CM_OK != iRet)
    {
        return iRet;
    }
    if((*ppAck == NULL) || (*pAckLen == 0))
    {
        return iRet;
    }
    cnt = atoi((sint8*)*ppAck);
    CM_FREE(*ppAck);
    *ppAck = NULL;
    *pAckLen = 0;
    if(cnt > 0)
    {
        return cm_cnm_ack_uint64(cnt,ppAck,pAckLen);
    }
    return CM_OK;
}

sint32 cm_cnm_lun_local_create(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    return cm_cnm_lun_local_exec("create",param,len,offset,total,ppAck,pAckLen);
}

sint32 cm_cnm_lun_local_update(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    return cm_cnm_lun_local_exec("update",param,len,offset,total,ppAck,pAckLen);
}

sint32 cm_cnm_lun_local_delete(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    return cm_cnm_lun_local_exec("delete",param,len,offset,total,ppAck,pAckLen);
}


#endif
static void cm_cnm_lun_oplog_report(
    const sint8* sessionid, const void *pDecodeParam, uint32 alarmid)
{
    const cm_cnm_lun_param_t *req = pDecodeParam;
    const cm_cnm_lun_info_t *info = NULL;
    uint32 cnt = 12;
    if((alarmid == CM_ALARM_LOG_LUN_CREATR_OK) 
        || (alarmid == CM_ALARM_LOG_LUN_CREATR_FAIL))
    {
        cnt = 14;
    }
    /*node,pool,lun,space,write_policy,is_double,is_compress,is_hot,alarm_threshold,qos,qos_val,dedup,blocksize,is_single*/
    if(NULL == pDecodeParam)
    {   
        cm_cnm_oplog_report(sessionid,alarmid,NULL,cnt,NULL);
    }    
    else    
    {       
        info = (const cm_cnm_lun_info_t*)&req->info;
        const sint8* nodename = cm_node_get_name(info->nid);
        cm_cnm_oplog_param_t params[14] = {
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_LUN_NID,strlen(nodename),nodename},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_LUN_POOL,strlen(info->pool),info->pool},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_LUN_NAME,strlen(info->name),info->name},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_LUN_TOTAL,strlen(info->space_total),info->space_total},
            {CM_OMI_DATA_INT,CM_OMI_FIELD_LUN_WRITE_POLICY,sizeof(info->write_policy),&info->write_policy},

            {CM_OMI_DATA_INT,CM_OMI_FIELD_LUN_IS_DOUBLE,sizeof(info->is_double),&info->is_double},
            {CM_OMI_DATA_INT,CM_OMI_FIELD_LUN_IS_COMPRESS,sizeof(info->is_compress),&info->is_compress},
            {CM_OMI_DATA_INT,CM_OMI_FIELD_LUN_IS_HOT,sizeof(info->is_hot),&info->is_hot},
            {CM_OMI_DATA_INT,CM_OMI_FIELD_LUN_ALARM_THRESHOLD,sizeof(info->alarm_threshold),&info->alarm_threshold},
            {CM_OMI_DATA_INT,CM_OMI_FIELD_LUN_QOS,sizeof(info->qos),&info->qos},

            {CM_OMI_DATA_STRING,CM_OMI_FIELD_LUN_QOS_VAL,strlen(info->qos_val),info->qos_val},
            {CM_OMI_DATA_INT,CM_OMI_FIELD_LUN_DEDUP,sizeof(info->dedup),&info->dedup},
            
            {CM_OMI_DATA_INT,CM_OMI_FIELD_LUN_BLOCKSIZE,sizeof(info->blocksize),&info->blocksize},
            {CM_OMI_DATA_INT,CM_OMI_FIELD_LUN_IS_SINGLE,sizeof(info->is_single),&info->is_single},
        };
        
        cm_cnm_oplog_report(sessionid,alarmid,params,cnt,&req->set);
    }    
    
    return;
} 


void cm_cnm_lun_oplog_create(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{
    const cm_cnm_lun_param_t *req = pDecodeParam;
    const cm_cnm_lun_info_t *info = NULL;
    uint32 alarmid = (Result == CM_OK)? CM_ALARM_LOG_LUN_CREATR_OK : CM_ALARM_LOG_LUN_CREATR_FAIL;

    cm_cnm_lun_oplog_report(sessionid,pDecodeParam,alarmid);
    if(NULL == req)
    {
        return;
    }
    info = &req->info;
    CM_LOG_WARNING(CM_MOD_CNM,"%s/%s iRet[%d]",info->pool,info->name,Result);
    if(CM_OK != Result)
    {
        return;
    }   
    
    (void)cm_db_exec_ext(g_cm_lunmap_handle,
            "INSERT INTO record_t VALUES('%s/%s','','',0)",info->pool,info->name);
    return;
}    

void cm_cnm_lun_oplog_update(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{
    const cm_cnm_lun_param_t *req = pDecodeParam;
    const cm_cnm_lun_info_t *info = NULL;
    uint32 alarmid = (Result == CM_OK)? CM_ALARM_LOG_LUN_UPDATE_OK : CM_ALARM_LOG_LUN_UPDATE_FAIL;

    cm_cnm_lun_oplog_report(sessionid,pDecodeParam,alarmid);
    
    if(NULL == req)
    {
        return;
    }
    info = &req->info;
    CM_LOG_WARNING(CM_MOD_CNM,"%s/%s iRet[%d]",info->pool,info->name,Result);
    if(CM_OK != Result)
    {
        return;
    } 
    return;
}    

void cm_cnm_lun_oplog_delete(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{
    const cm_cnm_lun_param_t *req = pDecodeParam;
    const cm_cnm_lun_info_t *info = NULL;
    uint32 alarmid = (Result == CM_OK)? CM_ALARM_LOG_LUN_DELETE_OK : CM_ALARM_LOG_LUN_DELETE_FAIL;
    uint32 cnt = 3;
    
    if(NULL == pDecodeParam)
    {   
        cm_cnm_oplog_report(sessionid,alarmid,NULL,cnt,NULL);
        return;
    }    
    else    
    {       
        info = (const cm_cnm_lun_info_t*)&req->info;
        const sint8* nodename = cm_node_get_name(info->nid);
        cm_cnm_oplog_param_t params[3] = {
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_LUN_NID,strlen(nodename),nodename},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_LUN_POOL,strlen(info->pool),info->pool},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_LUN_NAME,strlen(info->name),info->name},
        };
        
        cm_cnm_oplog_report(sessionid,alarmid,params,cnt,&req->set);
    } 
    CM_LOG_WARNING(CM_MOD_CNM,"%s/%s iRet[%d]",info->pool,info->name,Result);
    if(CM_OK != Result)
    {
        return;
    } 
    
    (void)cm_db_exec_ext(g_cm_lunmap_handle,
            "DELETE FROM record_t WHERE lun='%s/%s'",
            info->pool,info->name);
    return;
}    

/*===========================================================================*/

typedef struct
{
    uint32 nid; /* 预留 */
    sint8 name[CM_NAME_LEN_HOSTGROUP];
    sint8 members[CM_STRING_1K];
    uint8 action; /* 修改下发使用 */
    uint8 type; /* 0:host group, 1: target group */
}cm_cnm_hg_info_t;

#define CM_CNM_GROUP_TYPE_HOST 0
#define CM_CNM_GROUP_TYPE_TARGET 1

static const sint8* g_cm_cnm_str_type[2] = {"host group","target group"};

const sint8* g_cm_cnm_group_types[] = {"hg","tg"};
sint32 cm_cnm_hg_init(void)
{
    return CM_OK;
}

static sint32 cm_cnm_hg_decode_ext(const cm_omi_obj_t ObjParam,void* data,cm_omi_field_flag_t *set)
{
    uint32 value = 0;
    cm_cnm_hg_info_t *info = data;
    
    if(CM_OK == cm_omi_obj_key_get_str_ex(ObjParam,CM_OMI_FIELD_HG_NAME,info->name,sizeof(info->name)))
    {
        CM_OMI_FIELDS_FLAG_SET(set,CM_OMI_FIELD_HG_NAME);
    }
    if(CM_OK == cm_omi_obj_key_get_str_ex(ObjParam,CM_OMI_FIELD_HG_MEMBER,info->members,sizeof(info->members)))
    {
        CM_OMI_FIELDS_FLAG_SET(set,CM_OMI_FIELD_HG_MEMBER);
    }
    if(CM_OK == cm_omi_obj_key_get_u32_ex(ObjParam,CM_OMI_FIELD_HG_ACT,&value))
    {
        CM_OMI_FIELDS_FLAG_SET(set,CM_OMI_FIELD_HG_ACT);
        info->action = (uint8)value;
    }
    if(CM_OK == cm_omi_obj_key_get_u32_ex(ObjParam,CM_OMI_FIELD_HG_TYPE,&value))
    {
        info->type= (uint8)value;
        if(info->type >= 2)
        {
            CM_LOG_ERR(CM_MOD_CNM,"type[%u]",info->type);
            return CM_PARAM_ERR;
        }
        CM_OMI_FIELDS_FLAG_SET(set,CM_OMI_FIELD_HG_TYPE);
        
    }
    return CM_OK;
}

sint32 cm_cnm_hg_decode(
    const cm_omi_obj_t ObjParam, void** ppDecodeParam)
{
    cm_cnm_decode_info_t *decode = NULL;
    cm_cnm_hg_info_t *info = NULL;
    sint32 iRet = cm_cnm_decode_comm(ObjParam,sizeof(cm_cnm_hg_info_t),
        cm_cnm_hg_decode_ext,ppDecodeParam);
    if(CM_OK != iRet)
    {
        return iRet;
    }
    decode = *ppDecodeParam;
    info = (cm_cnm_hg_info_t*)decode->data;
    if(CM_CNM_GROUP_TYPE_HOST == info->type)
    {
        /* 复用该标记为，设置之后表示Host Group,否则Target Group */
        CM_OMI_FIELDS_FLAG_SET(&decode->field,CM_OMI_FIELD_HG_TYPE);
    }
    else
    {
        CM_OMI_FIELDS_FLAG_CLR(&decode->field,CM_OMI_FIELD_HG_TYPE);
    }
    return CM_OK;
}    

static void cm_cnm_hg_encode_each(cm_omi_obj_t item,void *eachdata,void *arg)
{
    sint32 iRet = CM_OK;
    sint8 *hg = eachdata;
    uint8 type = CM_CNM_GROUP_TYPE_HOST;
    sint8 members[CM_STRING_32] = {0};
    cm_omi_field_flag_t *field = arg;

    cm_cnm_map_value_str_t cols_str[] = {
            {CM_OMI_FIELD_HG_NAME,    hg},
            {CM_OMI_FIELD_HG_MEMBER,  members},
        };
    
    if(CM_OMI_FIELDS_FLAG_ISSET(field,CM_OMI_FIELD_HG_MEMBER))
    {
        if(!CM_OMI_FIELDS_FLAG_ISSET(field,CM_OMI_FIELD_HG_TYPE))
        {
            type = CM_CNM_GROUP_TYPE_TARGET;
        }
        iRet = cm_exec_int(
        CM_SCRIPT_DIR"cm_cnm_port.sh group_member_count '%s' '%s'",
            g_cm_cnm_group_types[type],hg);
        CM_VSPRINTF(members,sizeof(members),"%d",iRet);
    }
    cm_cnm_encode_str(item,field,cols_str,sizeof(cols_str)/sizeof(cm_cnm_map_value_str_t));
    return;
}  

cm_omi_obj_t cm_cnm_hg_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen)
{
    /* 这里只传主机组名字，如果需要获取member,在里面进行 */
    return cm_cnm_encode_comm_ext(pDecodeParam,pAckData,AckLen,
        CM_NAME_LEN_HOSTGROUP,cm_cnm_hg_encode_each);
}    

static sint32 cm_cnm_hg_get_each(void *arg, sint8 **cols, uint32 col_num)
{
    sint8 *hg = arg;
    if(col_num != 1)
    {
        CM_LOG_ERR(CM_MOD_CNM,"col_num[%d]",col_num);
        return CM_FAIL;
    }
    CM_VSPRINTF(hg,CM_NAME_LEN_HOSTGROUP,"%s",cols[0]);
    return CM_OK;
}

sint32 cm_cnm_hg_getbatch(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    sint32 iRet = CM_OK;
    uint32 offset = 0;
    uint32 total = CM_CNM_MAX_RECORD;
    uint8 type = CM_CNM_GROUP_TYPE_HOST;
    const sint8 *hg = NULL;
    const cm_cnm_decode_info_t *decode = pDecodeParam;
    const cm_cnm_hg_info_t *info = NULL;
    sint8 cmd[CM_STRING_512] = {0};
    
    if(NULL != decode)
    {
        offset = (uint32)decode->offset;
        total = decode->total;
        info = (const cm_cnm_hg_info_t *)decode->data;
        if('\0' != info->name[0])
        {
            hg = info->name;
        }
        type= info->type;
    }
    
    /*
    Prodigy-root#stmfadm list-hg 
    Host Group: hg_win45
    Host Group: win_qgao
    Host Group: linux_qgao
    Host Group: wbnaaa
    Host Group: aaaaaaaaaaa
    Host Group: aaa1
    */

    if(NULL == hg)
    {
        CM_VSPRINTF(cmd,sizeof(cmd),CM_SCRIPT_DIR"cm_cnm_port.sh group_getbatch '%s'",
            g_cm_cnm_group_types[type]);
    }
    else
    {
        CM_VSPRINTF(cmd,sizeof(cmd),CM_SCRIPT_DIR"cm_cnm_port.sh group_member_getbatch '%s' '%s'",
            g_cm_cnm_group_types[type],hg);
    }
    iRet = cm_cnm_exec_get_list(cmd,cm_cnm_hg_get_each,
        offset,CM_NAME_LEN_HOSTGROUP,ppAckData,&total);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]",iRet);
        return iRet;
    }
    *pAckLen = CM_NAME_LEN_HOSTGROUP * total;
    return iRet;
}    

sint32 cm_cnm_hg_get(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return CM_OK;
}    

static sint32 cm_cnm_hg_exec(const void *pDecodeParam, const sint8* cmd)
{
    sint32 iRet = CM_OK;
    sint8 buff[CM_STRING_1K] = {0};
    const cm_cnm_decode_info_t *decode = pDecodeParam;
    const cm_cnm_hg_info_t *info = NULL;
    
    if(NULL == decode)
    {
        CM_LOG_ERR(CM_MOD_CNM,"param null");
        return CM_PARAM_ERR;
    }
    info = (const cm_cnm_hg_info_t *)decode->data;
    if('\0' == info->name[0])
    {
        CM_LOG_ERR(CM_MOD_CNM,"name null");
        return CM_PARAM_ERR;
    }
    CM_CNM_CHECK_ALL_NODE_ONLINE();
    
    CM_LOG_WARNING(CM_MOD_CNM,"%s %s %s",cmd,g_cm_cnm_group_types[info->type],info->name);
    iRet = cm_exec_tmout(buff,sizeof(buff),CM_CMT_REQ_TMOUT,
        "stmfadm %s-%s -c %s 2>&1",cmd,g_cm_cnm_group_types[info->type],info->name);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]\n%s",iRet,buff);
        return cm_cnm_get_errcode(&CmCnmMapCommErrCfg,buff,iRet);
    }
    return CM_OK;
}

sint32 cm_cnm_hg_create(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_cnm_hg_exec(pDecodeParam, "create");
}    
    
sint32 cm_cnm_hg_update(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    sint32 iRet = CM_OK;
    const cm_cnm_hg_info_t *info = NULL;
    const cm_cnm_decode_info_t *decode = pDecodeParam;
    const sint8* action[] = {"add", "remove"};
    sint8 buff[CM_STRING_1K] = {0};
    
    if(NULL == decode)
    {
        CM_LOG_ERR(CM_MOD_CNM,"param null");
        return CM_PARAM_ERR;
    }
    info = (const cm_cnm_hg_info_t *)decode->data;
    if('\0' == info->name[0])
    {
        CM_LOG_ERR(CM_MOD_CNM,"name null");
        return CM_PARAM_ERR;
    }
    if('\0' == info->members[0])
    {
        CM_LOG_ERR(CM_MOD_CNM,"members null");
        return CM_PARAM_ERR;
    }
    if(!CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_HG_ACT)
        || (info->action >= 2))
    {
        CM_LOG_ERR(CM_MOD_CNM,"action [%u]",info->action);
        return CM_PARAM_ERR;
    }
    
    CM_CNM_CHECK_ALL_NODE_ONLINE();
    
    CM_MEM_CPY(buff,sizeof(buff),info->members,sizeof(info->members));
    cm_cnm_comma2blank(buff);
    CM_LOG_WARNING(CM_MOD_CNM,"%s[%s]%s",info->name,action[info->action],info->members);

    if(CM_CNM_GROUP_TYPE_TARGET == info->type)
    {
        /* 先offline */
        (void)cm_system("stmfadm offline-target -c %s 2>/dev/null",buff);
    }
    
    iRet = cm_exec_tmout(buff,sizeof(buff),CM_CMT_REQ_TMOUT,
        "stmfadm %s-%s-member -c -g %s %s 2>&1",
        action[info->action],g_cm_cnm_group_types[info->type],info->name,buff);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]\n%s",iRet,buff);
        return cm_cnm_get_errcode(&CmCnmMapCommErrCfg,buff,iRet);
    }

    if(CM_CNM_GROUP_TYPE_TARGET == info->type)
    {
        /* online */
        CM_MEM_CPY(buff,sizeof(buff),info->members,sizeof(info->members));
        cm_cnm_comma2blank(buff);
        (void)cm_system("stmfadm online-target -c %s 2>/dev/null",buff);
    }
    
    return CM_OK;
}    
    
sint32 cm_cnm_hg_count(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    uint64 cnt = 0;
    uint8 type = CM_CNM_GROUP_TYPE_HOST;
    const sint8 *hg = NULL;
    const cm_cnm_decode_info_t *decode = pDecodeParam;
    const cm_cnm_hg_info_t *info = NULL;
    
    if(NULL != decode)
    {
        info = (const cm_cnm_hg_info_t *)decode->data;
        if('\0' != info->name[0])
        {
            hg = info->name;
        }
        type = info->type;
    }

    if(NULL == hg)
    {
        cnt = (uint64)cm_exec_int(CM_SCRIPT_DIR"cm_cnm_port.sh group_count '%s'",
            g_cm_cnm_group_types[type]);
    }
    else
    {
        cnt = (uint64)cm_exec_int(
        CM_SCRIPT_DIR"cm_cnm_port.sh group_member_count '%s' '%s'",
            g_cm_cnm_group_types[type],hg);
    }
    return cm_cnm_ack_uint64(cnt,ppAckData,pAckLen);
}    
    
sint32 cm_cnm_hg_delete(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_cnm_hg_exec(pDecodeParam, "delete");
} 

void cm_cnm_hg_oplog_create(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK)? CM_ALARM_LOG_HOSTGROUP_CREATR_OK : CM_ALARM_LOG_HOSTGROUP_CREATR_FAIL;
    const cm_cnm_decode_info_t *req = pDecodeParam;
    const uint32 cnt = 2;
    
    /*<type> <name>*/
    if(NULL == pDecodeParam)
    {   
        cm_cnm_oplog_report(sessionid,alarmid,NULL,cnt,NULL);
    }    
    else    
    {    
        const cm_cnm_hg_info_t*info = (const cm_cnm_hg_info_t*)req->data;
        cm_cnm_oplog_param_t params[] = {
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_HG_TYPE,strlen(g_cm_cnm_str_type[info->type]),g_cm_cnm_str_type[info->type]},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_HG_NAME,strlen(info->name),info->name},
        };
        cm_cnm_oplog_report(sessionid,alarmid,params,cnt,&req->set);
    }   
    return;
}    

void cm_cnm_hg_oplog_update(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK)? CM_ALARM_LOG_HOSTGROUP_UPDATE_OK : CM_ALARM_LOG_HOSTGROUP_UPDATE_FAIL;
    const cm_cnm_decode_info_t *req = pDecodeParam;
    const uint32 cnt = 4;
    
    /*<type> <name> <action> <members>*/
    if(NULL == pDecodeParam)
    {   
        cm_cnm_oplog_report(sessionid,alarmid,NULL,cnt,NULL);
    }    
    else    
    {    
        const cm_cnm_hg_info_t*info = (const cm_cnm_hg_info_t*)req->data;
        cm_cnm_oplog_param_t params[] = {
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_HG_TYPE,strlen(g_cm_cnm_str_type[info->type]),g_cm_cnm_str_type[info->type]},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_HG_NAME,strlen(info->name),info->name},
            {CM_OMI_DATA_INT,CM_OMI_FIELD_HG_ACT,sizeof(info->action),&info->action},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_HG_MEMBER,strlen(info->members),info->members},
        };
        cm_cnm_oplog_report(sessionid,alarmid,params,cnt,&req->set);
    }   
    return;
}      

void cm_cnm_hg_oplog_delete(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK)? CM_ALARM_LOG_HOSTGROUP_DELETE_OK : CM_ALARM_LOG_HOSTGROUP_DELETE_FAIL;
    const cm_cnm_decode_info_t *req = pDecodeParam;
    const uint32 cnt = 2;
    
    /*<type> <name>*/
    if(NULL == pDecodeParam)
    {   
        cm_cnm_oplog_report(sessionid,alarmid,NULL,cnt,NULL);
    }    
    else    
    {    
        const cm_cnm_hg_info_t*info = (const cm_cnm_hg_info_t*)req->data;
        cm_cnm_oplog_param_t params[] = {
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_HG_TYPE,strlen(g_cm_cnm_str_type[info->type]),g_cm_cnm_str_type[info->type]},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_HG_NAME,strlen(info->name),info->name},
        };
        cm_cnm_oplog_report(sessionid,alarmid,params,cnt,&req->set);
    }   
    return;
}    

/*============================================================================*/

const sint8* cm_cnm_port_sh = "/var/cm/script/cm_cnm_port.sh";


typedef struct
{
    sint8 name[CM_NAME_LEN_TARGET];
    uint32 protocol;  
}cm_cnm_target_info_t;
typedef struct
{
    uint32 status;
    uint32 nid;
    uint32 protocol;
    sint8 name[CM_STRING_128];
    sint8 provider[CM_STRING_128];
    uint32 sessions;
}cm_cnm_port_info_t;
extern const cm_omi_map_enum_t CmOmiMapEnumProtocolInType;
sint32 cm_cnm_target_init(void)
{
    return CM_OK;
}

static sint32 cm_cnm_target_decode_ext(
    const cm_omi_obj_t ObjParam,void* data,cm_omi_field_flag_t *set)
{
    cm_cnm_port_info_t *info = data;
    
    if(CM_OK == cm_omi_obj_key_get_str_ex(ObjParam,CM_OMI_FIELD_TGT_NAME,info->name,sizeof(info->name)))
    {
        CM_OMI_FIELDS_FLAG_SET(set,CM_OMI_FIELD_TGT_NAME);
    }
    if(CM_OK == cm_omi_obj_key_get_u32_ex(ObjParam,CM_OMI_FIELD_TGT_PROTOCOL,&info->protocol))
    {
        CM_OMI_FIELDS_FLAG_SET(set,CM_OMI_FIELD_TGT_PROTOCOL);
    }
    if(CM_OK == cm_omi_obj_key_get_u32_ex(ObjParam,CM_OMI_FIELD_TGT_NID,&info->nid))
    {
        CM_OMI_FIELDS_FLAG_SET(set,CM_OMI_FIELD_TGT_NID);
    }
    return CM_OK;
}

sint32 cm_cnm_target_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam)
{
    return cm_cnm_decode_comm(ObjParam,sizeof(cm_cnm_port_info_t),
        cm_cnm_target_decode_ext,ppDecodeParam);
}    

static void cm_cnm_target_encode_each(cm_omi_obj_t item,void *eachdata,void *arg)
{
    cm_omi_field_flag_t *field = arg;
    cm_cnm_port_info_t *info = eachdata;

    cm_cnm_map_value_str_t cols_str[] = {
            {CM_OMI_FIELD_TGT_NAME,    info->name},
            {CM_OMI_FIELD_TGT_PROVIDER,  info->provider}, 
            {CM_OMI_FIELD_TGT_INITIATOR, ""},
        };
    cm_cnm_map_value_num_t cols_num[] = {
            {CM_OMI_FIELD_TGT_NID,      info->nid},
            {CM_OMI_FIELD_TGT_STATUS,      info->status},
            {CM_OMI_FIELD_TGT_PROTOCOL,    info->protocol},
            {CM_OMI_FIELD_TGT_SESSIONS,  info->sessions},
        };
    /* , Online, iscsit, iSCSI, 2 */

    cm_cnm_encode_str(item,field,cols_str,sizeof(cols_str)/sizeof(cm_cnm_map_value_str_t));
    cm_cnm_encode_num(item,field,cols_num,sizeof(cols_num)/sizeof(cm_cnm_map_value_num_t)); 
    return;
}

cm_omi_obj_t cm_cnm_target_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen)
{
    return cm_cnm_encode_comm_ext(pDecodeParam,pAckData,AckLen,
        sizeof(cm_cnm_port_info_t),cm_cnm_target_encode_each);
}    

static sint32 cm_cnm_port_get_each(void *arg, sint8 **cols, uint32 col_num)
{
    cm_cnm_port_info_t* info = arg;

    if(6 != col_num)
    {
        CM_LOG_WARNING(CM_MOD_CNM,"col_num[%u] def_num[6]",col_num);
        return CM_FAIL;
    }

    CM_VSPRINTF(info->name,sizeof(info->name),"%s",cols[0]);
    if(strcmp(cols[1],"Online") == 0)
    {
        info->status=1;
    }else
    {
        info->status=0;
    }
    CM_VSPRINTF(info->provider,sizeof(info->provider),"%s",cols[2]);

    if(0 == strcmp(cols[3],"Fibre"))
    {
        info->protocol = 2;
    }else if(strcmp(cols[3],"iSCSI"))
    {
        info->protocol = 3;
    }else if(strcmp(cols[3],"IB"))
    {
        info->protocol = 1;
    }else
    {
        info->protocol = 0;
    }
    info->sessions = atoi(cols[4]);

    info->nid = cm_node_get_id_by_inter(atoi(cols[5]));

    return CM_OK;
}
static sint32 cm_cnm_port_get_initiator(void *arg, sint8 **cols, uint32 col_num)
{
    cm_cnm_port_info_t* info = arg;
    if(1 != col_num)
    {
        CM_LOG_WARNING(CM_MOD_CNM,"col_num[%u] def_num[1]",col_num);
        return CM_FAIL;
    }
    CM_MEM_ZERO(info,sizeof(cm_cnm_port_info_t));
    CM_VSPRINTF(info->name,sizeof(info->name),"%s",cols[0]);
    return CM_OK;
}

sint32 cm_cnm_target_getbatch(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    sint32 iRet = CM_OK;
    uint32 offset = 0;
    uint32 total = CM_CNM_MAX_RECORD;
    const sint8 *hg = NULL;
    const cm_cnm_decode_info_t *decode = pDecodeParam;
    const cm_cnm_port_info_t *info = NULL;
    
    if(NULL != decode)
    {
        offset = (uint32)decode->offset;
        total = decode->total;
        info = (const cm_cnm_port_info_t *)decode->data;
        if('\0' != info->name[0])
        {
            hg = info->name;
        }
    }
    
    /*
    Prodigy-root#stmfadm list-target
    Target: iqn.1986-03.com.csd:02:b78b2295-be59-c67e-d4e1-b22175093a9b
    Target: iqn.1986-03.com.csd:02:c29804b7-6452-c74d-c1ab-c0d1f070ab23
    Target: iqn.1986-03.com.csd:02:4680b121-9a86-c712-9c26-cd13a4a10e4c
    Target: iqn.1986-03.com.csd:02:36142a5d-fac7-6584-affe-e67c632f6e23
    Target: iqn.1986-03.com.csd:02:b8f1f71f-bcd9-4d10-ec4d-c64a2ece99f0
    Target: wwn.21000024FF3E9AED
    Target: wwn.21000024FF3E9AEC
    
    */
    if(NULL == hg)
    {
        iRet = cm_exec_get_list(cm_cnm_port_get_each,
            offset,sizeof(cm_cnm_port_info_t),ppAckData,&total,
            CM_SCRIPT_DIR"cm_cnm_port.sh getbatch");
    }
    else
    {
        iRet = cm_exec_get_list(cm_cnm_port_get_initiator,
            offset,sizeof(cm_cnm_port_info_t),ppAckData,&total,
            CM_SCRIPT_DIR"cm_cnm_port.sh initiator_getbatch %u '%s'",
            info->nid,info->name);
    }
    
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]",iRet);
        return iRet;
    }
    *pAckLen = sizeof(cm_cnm_port_info_t) * total;
    return iRet;
}    

sint32 cm_cnm_target_get(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return CM_OK;
}    
   
sint32 cm_cnm_target_create(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    const cm_cnm_decode_info_t *decode = pDecodeParam;
    const cm_cnm_port_info_t *info = NULL;
    sint32 iRet = CM_OK;
    sint8 name[CM_STRING_256] = {0};
    
    if(NULL == decode)
    {
        CM_LOG_ERR(CM_MOD_CNM,"decode null");
        return CM_PARAM_ERR;
    }
    info = (const cm_cnm_port_info_t*)decode->data;

    if(!CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_TGT_PROTOCOL))
    {
        CM_LOG_ERR(CM_MOD_CNM,"protocol not set");
        return CM_PARAM_ERR;
    }

    if(1 != info->protocol)
    {
        /* 目前仅支持 ISCSI*/
        return CM_ERR_NOT_SUPPORT;
    }

    iRet = cm_cnm_exec_remote(0,CM_FALSE,name,sizeof(name),
        "itadm create-target 1>/dev/null 2>/dev/null");
    return iRet;
}    
    
sint32 cm_cnm_target_update(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return CM_OK;
}    
    
sint32 cm_cnm_target_count(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    uint64 cnt = 0;
    const sint8 *hg = NULL;
    const cm_cnm_decode_info_t *decode = pDecodeParam;
    const cm_cnm_port_info_t *info = NULL;
    
    if(NULL != decode)
    {
        info = (const cm_cnm_port_info_t *)decode->data;
        if('\0' != info->name[0])
        {
            hg = info->name;
        }
    }

    if(NULL == hg)
    {
        cnt = (uint64)cm_exec_int(CM_SCRIPT_DIR"cm_cnm_port.sh count");
    }
    else
    {
        cnt = (uint64)cm_exec_int(
            CM_SCRIPT_DIR"cm_cnm_port.sh initiator_count %u '%s'",
            info->nid,info->name);
    }
    return cm_cnm_ack_uint64(cnt,ppAckData,pAckLen);
}    
    
sint32 cm_cnm_target_delete(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    sint32 iRet = CM_OK;
    sint8 cmd[CM_STRING_256] = {0};
    const cm_cnm_decode_info_t *decode = pDecodeParam;
    const cm_cnm_port_info_t *info = decode->data;
    
    iRet = cm_cnm_exec_remote(info->nid,CM_FALSE,cmd,sizeof(cmd),
        "itadm delete-target -f %s 1>/dev/null 2>/dev/null",info->name);
    return iRet;
}    

void cm_cnm_target_oplog_create(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK)? CM_ALARM_LOG_TARGET_INSERT_OK : CM_ALARM_LOG_TARGET_INSERT_FAIL;
    const cm_cnm_decode_info_t *req = pDecodeParam;
    const uint32 cnt = 1;
    if(NULL == pDecodeParam)
    {   
        cm_cnm_oplog_report(sessionid,alarmid,NULL,cnt,NULL);
    }    
    else    
    {       
        const cm_cnm_target_info_t *info = (const cm_cnm_target_info_t*)req->data;
        cm_cnm_oplog_param_t params[1] = {
            {CM_OMI_DATA_INT,CM_OMI_FIELD_TGT_PROTOCOL,sizeof(info->protocol),&info->protocol},
        };
        cm_cnm_oplog_report(sessionid,alarmid,params,cnt,&req->set);
    }   
    return;
}    

void cm_cnm_target_oplog_update(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{
    return;
}    

void cm_cnm_target_oplog_delete(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{
    return;
}    

/*============================================================================*/
/* lunmap  */
#define CM_NAME_LEN_LUN_FULL (CM_NAME_LEN_POOL+CM_NAME_LEN_LUN+1)
#define CM_CNM_LUNMAP_DB_FILE CM_DATA_DIR"cm_lunmap.db"

typedef struct
{
    sint8 lun[CM_NAME_LEN_LUN_FULL];
    sint8 hg[CM_NAME_LEN_HOSTGROUP];
    sint8 tg[CM_NAME_LEN_HOSTGROUP];
}cm_cnm_lunmap_info_t;

static void cm_cnm_lunmap_update_each(const sint8* stmfid,
    const sint8* lun, cm_db_handle_t handle)
{
    sint8 buff[CM_STRING_1K] = {0};
    uint32 cnt = 0;
    sint8 *pblank = NULL;
    sint8 *pbuff = NULL;
    sint8 *hg = NULL;
    sint8 *tg = NULL;
    sint8 *lunid = NULL;
    
    /*
    stmfadm list-view -l 600144F00200000000005B0FB0EF0014 |egrep "Target group|
    Host group" |awk -F':' '{sum=sum$2;cnt+=1}END{cnt/=2;printf cnt""sum}'
    3 wbnaaa All win_qgao All wbn20180606 All
    */
    (void)cm_exec_tmout(buff,sizeof(buff),CM_CMT_REQ_TMOUT,
        "stmfadm list-view -l %s 2>/dev/null |egrep \"Target group|Host group|LUN\" "
        "|awk -F':' '{sum=sum$2;cnt+=1}END{cnt/=3;printf cnt\"\"sum}'",stmfid);
    cnt = (uint32)atoi(buff);
    if(0 == cnt)
    {
        (void)cm_db_exec_ext(handle,"INSERT INTO record_t (lun,hg,tg,lunid) VALUES "
            "('%s','','',0)",lun);
        return;
    }
    pbuff = &buff[2];
    for(;(cnt > 0) && (NULL != pbuff); cnt--)
    {
        /* 获取 Host group */
        hg = pbuff;

        /* 获取 Target group */
        pblank = strstr(hg," ");
        if(NULL == pblank)
        {
            break;
        }        
        tg = pblank+1;
        *pblank = '\0';

        /* 获取 LUNID */
        pblank = strstr(tg," ");
        if(NULL == pblank)
        {
            break;
        }
        lunid = pblank+1;
        *pblank = '\0';

        /* 继续下一条 */
        pblank = strstr(lunid," ");
        if(NULL == pblank)
        {
            pbuff = NULL;
        }
        else
        {
            pbuff = pblank+1;
            *pblank = '\0';
        }
        (void)cm_db_exec_ext(handle,"INSERT INTO record_t (lun,hg,tg,lunid) VALUES "
            "('%s','%s','%s',%u)",lun,hg,tg,(uint32)atoi(lunid));
    }
    return;
}

static void* cm_cnm_lunmap_thread(void* arg)
{
    sint32 iRet = CM_OK;
    uint32 row = 0;
    uint32 col_num = 0;
    sint8 buff[CM_STRING_512] = {0};
    sint8* cols[2] = {NULL};
    cm_db_handle_t handle = arg;
    const sint8* tmpfile="/tmp/cm_cnm_lunmap_thread.tmp";
    static bool_t isrun = CM_FALSE;
    
    if(CM_FALSE != isrun)
    {
        return NULL;
    }
    isrun = CM_TRUE;
    /* 清空缓存表 */
    (void)cm_db_exec_ext(handle,"DELETE FROM record_t");

    /* start  for 0006581  */
    if(CM_SYS_VER_SOLARIS_V7R16 == g_cm_sys_ver)
    {
        iRet = cm_system("%s |sed '/.*bit0$/d'> %s",g_cm_cnm_lun_stmfid_map,tmpfile);
    }
    else
    {
        iRet = cm_system("stmfadm list-all-luns 2>/dev/null |sed '/.*bit0$/d' > %s",tmpfile);
    }
    /* end  for 0006581  */
    
    if(CM_OK != iRet)
    {
        CM_LOG_INFO(CM_MOD_CNM,"stmfadm list-all-luns iRet[%d]",iRet);
        isrun = CM_FALSE;
        return NULL;
    }    
    
    row = (uint32)cm_exec_int("cat %s |wc -l",tmpfile);
    for(;row>0;row--)
    {
        iRet = cm_exec(buff,sizeof(buff),"sed -n %up %s",row,tmpfile);
        if(CM_OK != iRet)
        {
            CM_LOG_ERR(CM_MOD_CNM,"row[%u] iRet[%d]",row,iRet);
            continue;
        }
        /*
        600144F00200000000005B0FB0EF0014        test/wbnlun
        */
        col_num = cm_cnm_get_cols(buff,cols,2);
        if(2 != col_num)
        {
            CM_LOG_ERR(CM_MOD_CNM,"row[%u] col_num[%u]",row,col_num);
            continue;
        }
        cm_cnm_lunmap_update_each(cols[0],cols[1],handle);
    }
    //cm_system("rm -f %s",tmpfile);
    isrun = CM_FALSE;
    return NULL;
}
sint32 cm_cnm_lunmap_update_pre(void* arg)
{
    sint32 iRet = CM_OK;
    uint32 time_wait = 60;
    uint32 cut = 0;
    while(1)
    {
        sleep(1);
        cut++;
        if(cut >= time_wait)
        {
            cm_cnm_lunmap_thread((void*)g_cm_lunmap_handle);
            break;
        }
    }
    return CM_OK;
}    

sint32 cm_cnm_lunmap_init(void)
{
    cm_thread_t handle;
    sint32 iRet = CM_OK;
    const sint8* table = "CREATE TABLE IF NOT EXISTS record_t ("
            "lun VARCHAR(255),"
            "hg VARCHAR(255),"
            "tg VARCHAR(255),"
            "lunid INT)";

    //(void)cm_system("rm -f "CM_CNM_LUNMAP_DB_FILE" 2>/dev/null");
    iRet = cm_db_open_ext(CM_CNM_LUNMAP_DB_FILE,&g_cm_lunmap_handle);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"open %s fail",CM_CNM_LUNMAP_DB_FILE);
        return iRet;
    }
    iRet = cm_db_exec_ext(g_cm_lunmap_handle,table);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"create table fail[%d]",iRet);
        return iRet;
    }
    iRet = CM_THREAD_CREATE(&handle,cm_cnm_lunmap_update_pre,g_cm_lunmap_handle);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"create thread fail[%d]",iRet);
        return iRet;
    }
    CM_THREAD_DETACH(handle);
    return CM_OK;
}

static sint32 cm_cnm_lunmap_decode_ext(
    const cm_omi_obj_t ObjParam,void* data,cm_omi_field_flag_t *set)
{
    cm_cnm_lunmap_info_t *info = data;
    
    if(CM_OK == cm_omi_obj_key_get_str_ex(ObjParam,CM_OMI_FIELD_LUNMAP_LUN,info->lun,sizeof(info->lun)))
    {
        CM_OMI_FIELDS_FLAG_SET(set,CM_OMI_FIELD_LUNMAP_LUN);
    }
    if(CM_OK == cm_omi_obj_key_get_str_ex(ObjParam,CM_OMI_FIELD_LUNMAP_HG,info->hg,sizeof(info->hg)))
    {
        CM_OMI_FIELDS_FLAG_SET(set,CM_OMI_FIELD_LUNMAP_HG);
    }
    if(CM_OK == cm_omi_obj_key_get_str_ex(ObjParam,CM_OMI_FIELD_LUNMAP_TG,info->tg,sizeof(info->tg)))
    {
        CM_OMI_FIELDS_FLAG_SET(set,CM_OMI_FIELD_LUNMAP_TG);
    }
    return CM_OK;
}

sint32 cm_cnm_lunmap_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam)
{
    return cm_cnm_decode_comm(ObjParam,sizeof(cm_cnm_lunmap_info_t),
        cm_cnm_lunmap_decode_ext,ppDecodeParam);
}
    
cm_omi_obj_t cm_cnm_lunmap_encode(
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
    (void)cm_db_exec(g_cm_lunmap_handle,cm_omi_encode_db_record_each,items,"%s",cmd);
    return items;
}

static void cm_cnm_lunmap_where(sint8 *cmd, uint32 len,const cm_cnm_decode_info_t *decode)
{
    const cm_cnm_lunmap_info_t *lunmap = (const cm_cnm_lunmap_info_t *)decode->data;
    bool_t isfirst = CM_TRUE;
    const sint8* setname[3] = {"lun","hg","tg"};
    const sint8* setcols[3] = {NULL};
    uint32 setcolid[3] = {CM_OMI_FIELD_LUNMAP_LUN,CM_OMI_FIELD_LUNMAP_HG,CM_OMI_FIELD_LUNMAP_TG};
    uint32 offset = 0;
    
    setcols[0] = lunmap->lun;
    setcols[1] = lunmap->hg;
    setcols[2] = lunmap->tg;
    for(offset = 0;offset<3;offset++)
    {
        if(CM_OMI_FIELDS_FLAG_ISSET(&decode->set,setcolid[offset]))
        {
            if(isfirst == CM_FALSE)
            {
                CM_SNPRINTF_ADD(cmd,CM_STRING_512," AND %s='%s'",
                    setname[offset],setcols[offset]);
            }
            else
            {
                CM_SNPRINTF_ADD(cmd,CM_STRING_512," WHERE %s='%s'",
                    setname[offset],setcols[offset]);
                isfirst = CM_FALSE;    
            }
        }
    }
    return;
}
sint32 cm_cnm_lunmap_getbatch(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    const cm_cnm_decode_info_t *decode = pDecodeParam;
    const cm_cnm_lunmap_info_t *lunmap = NULL;
    const sint8* fields[] = {
        "lun as f0",
        NULL, /* f1 NID不记录数据库 */
        "hg as f2", 
        "tg as f3",
        "lunid as f4",
    };
    
    sint8 *cmd = CM_MALLOC(CM_STRING_512);

    if(NULL == cmd)
    {
        CM_LOG_ERR(CM_MOD_CNM,"malloc fail");
        return CM_FAIL;
    }
    *ppAckData = cmd;
    *pAckLen = CM_STRING_512;
    
    if(NULL == decode)
    {
        CM_VSPRINTF(cmd,CM_STRING_512,"SELECT lun as f0,hg as f2,tg as f3,lunid as f4 FROM record_t LIMIT %u",CM_CNM_MAX_RECORD);
        return CM_OK;
    }
    
    lunmap = (const cm_cnm_lunmap_info_t *)decode->data;
    cm_omi_make_select_field(fields,sizeof(fields)/sizeof(sint8*),
        &decode->field,cmd,CM_STRING_512);
    CM_SNPRINTF_ADD(cmd,CM_STRING_512," FROM record_t");
    
    cm_cnm_lunmap_where(cmd,CM_STRING_512,decode);
    CM_SNPRINTF_ADD(cmd,CM_STRING_512," LIMIT %llu,%u",decode->offset,decode->total);
    //CM_LOG_INFO(CM_MOD_CNM,"%s",cmd);
    (void)cm_cnm_lunmap_update(NULL,NULL,NULL);
    return CM_OK;
}    

sint32 cm_cnm_lunmap_get(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return CM_OK;
}    
   
sint32 cm_cnm_lunmap_create(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    const cm_cnm_decode_info_t *decode = pDecodeParam;
    const cm_cnm_lunmap_info_t *info = NULL;
    sint8 stmfid[CM_STRING_64] = {0};
    sint32 iRet = CM_OK;
    const sint8* group_def = "All";
    const sint8* hg = group_def;
    const sint8* tg = group_def;
    sint8 buf[CM_STRING_1K] = {0};
    
    if(NULL == decode)
    {
        return CM_PARAM_ERR;
    }
    info = (const cm_cnm_lunmap_info_t *)decode->data;
    if(!CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_LUNMAP_LUN))
    {
        CM_LOG_ERR(CM_MOD_CNM,"lun null");
        return CM_PARAM_ERR;
    }
    if(CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_LUNMAP_HG))
    {
        hg = info->hg;
    }
    if(CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_LUNMAP_TG))
    {
        tg = info->tg;
    }
    CM_CNM_CHECK_ALL_NODE_ONLINE();
    
    iRet = cm_cnm_lun_local_get_stmfid(info->lun,NULL,stmfid,sizeof(stmfid));
    if((CM_OK != iRet) || ('\0' == stmfid[0]))
    {
        CM_LOG_ERR(CM_MOD_CNM,"lun[%s] iRet[%d]",info->lun,iRet);
        return CM_ERR_NOT_EXISTS;
    }
    CM_SNPRINTF_ADD(buf,sizeof(buf),"stmfadm add-view -c");
    if(hg != group_def)
    {
        CM_SNPRINTF_ADD(buf,sizeof(buf)," -h %s",hg);
    }
    if(tg != group_def)
    {
        CM_SNPRINTF_ADD(buf,sizeof(buf)," -t %s",tg);
    }
    iRet = cm_exec_tmout(buf,sizeof(buf),CM_CMT_REQ_TMOUT,"%s %s 2>&1",buf,stmfid);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]\n%s",iRet,buf);
        return cm_cnm_get_errcode(&CmCnmMapCommErrCfg,buf,iRet);
    }
    (void)cm_cnm_lunmap_update(NULL,NULL,NULL);
    return CM_OK;
}    
    
sint32 cm_cnm_lunmap_update(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    sint32 iRet = CM_OK;
    cm_thread_t handle;
    
    iRet = CM_THREAD_CREATE(&handle,cm_cnm_lunmap_thread,g_cm_lunmap_handle);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"create thread fail[%d]",iRet);
        return iRet;
    }
    CM_THREAD_DETACH(handle);
    return CM_OK;
}    

void cm_cnm_lunmap_update_period(void)
{
#ifndef CM_OMI_LOCAL
    if(cm_node_get_master() != cm_node_get_id())
    {
        return;
    }
#endif    
    //(void)cm_cnm_lunmap_update(NULL,NULL,NULL);
}

sint32 cm_cnm_lunmap_count(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{   
    sint8 cmd[CM_STRING_512] = {0};
    uint64 cnt = 0;
    const cm_cnm_decode_info_t *decode = pDecodeParam;

    CM_SNPRINTF_ADD(cmd,CM_STRING_512,"SELECT COUNT(lun) FROM record_t");
    if(NULL != decode)
    {
        cm_cnm_lunmap_where(cmd,CM_STRING_512,decode);
    }
    (void)cm_db_exec_get_count(g_cm_lunmap_handle,&cnt,"%s",cmd);
    
    return cm_cnm_ack_uint64(cnt,ppAckData,pAckLen);
}    
    
sint32 cm_cnm_lunmap_delete(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    const cm_cnm_decode_info_t *decode = pDecodeParam;
    const cm_cnm_lunmap_info_t *info = NULL;
    sint8 stmfid[CM_STRING_64] = {0};
    sint32 iRet = CM_OK;
    const sint8* group_def = "All";
    const sint8* hg = group_def;
    const sint8* tg = group_def;
    sint8 buf[CM_STRING_1K] = {0};
    uint64 cnt = 0;
    
    if(NULL == decode)
    {
        return CM_PARAM_ERR;
    }
    info = (const cm_cnm_lunmap_info_t *)decode->data;
    if(!CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_LUNMAP_LUN))
    {
        CM_LOG_ERR(CM_MOD_CNM,"lun null");
        return CM_PARAM_ERR;
    }
    if(!CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_LUNMAP_HG))
    {
        CM_LOG_ERR(CM_MOD_CNM,"hg null");
        return CM_PARAM_ERR;
    }
    else
    {
        hg = info->hg;
    }
    if(CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_LUNMAP_TG))
    {
        tg = info->tg;
    }
    CM_CNM_CHECK_ALL_NODE_ONLINE();
    
    iRet = cm_cnm_lun_local_get_stmfid(info->lun,NULL,stmfid,sizeof(stmfid));
    if((CM_OK != iRet) || ('\0' == stmfid[0]))
    {
        CM_LOG_ERR(CM_MOD_CNM,"lun[%s] iRet[%d]",info->lun,iRet);
        return CM_ERR_NOT_EXISTS;
    }
    CM_SNPRINTF_ADD(buf,sizeof(buf),"stmfadm remove-view -c -l %s",stmfid);
    if(hg != group_def)
    {
        CM_SNPRINTF_ADD(buf,sizeof(buf)," -h %s",hg);
    }
    if(tg != group_def)
    {
        CM_SNPRINTF_ADD(buf,sizeof(buf)," -t %s",tg);
    }
    iRet = cm_exec_tmout(buf,sizeof(buf),CM_CMT_REQ_TMOUT,"%s 2>&1",buf);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]\n%s",iRet,buf);
        return cm_cnm_get_errcode(&CmCnmMapCommErrCfg,buf,iRet);
    }
    (void)cm_cnm_lunmap_update(NULL,NULL,NULL);
    return CM_OK;
}    

void cm_cnm_lunmap_oplog_create(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK)? CM_ALARM_LOG_LUNMAP_CREATE_OK : CM_ALARM_LOG_LUNMAP_CREATE_FAIL;
    const cm_cnm_decode_info_t *req = pDecodeParam;
    const uint32 cnt = 3;
    if(NULL == pDecodeParam)
    {   
        cm_cnm_oplog_report(sessionid,alarmid,NULL,cnt,NULL);
    }    
    else    
    {       
        const cm_cnm_lunmap_info_t *info = (const cm_cnm_lunmap_info_t*)req->data;
        cm_cnm_oplog_param_t params[3] = {
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_LUNMAP_LUN,strlen(info->lun),info->lun},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_LUNMAP_HG,strlen(info->hg),info->hg},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_LUNMAP_TG,strlen(info->tg),info->tg},
        };
        cm_cnm_oplog_report(sessionid,alarmid,params,cnt,&req->set);

        if(Result == CM_OK)
        {
            (void)cm_thread_start(cm_cnm_lunmap_thread,g_cm_lunmap_handle);
        }
    }   
    return;
}    

void cm_cnm_lunmap_oplog_update(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{
    /* 不需要上报操作日志 */ 
    return;
}    

void cm_cnm_lunmap_oplog_delete(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK)? CM_ALARM_LOG_LUNMAP_DELETE_OK : CM_ALARM_LOG_LUNMAP_DELETE_FAIL;
    const cm_cnm_decode_info_t *req = pDecodeParam;
    const uint32 cnt = 2;
    if(NULL == pDecodeParam)
    {   
        cm_cnm_oplog_report(sessionid,alarmid,NULL,cnt,NULL);
    }    
    else    
    {       
        const cm_cnm_lunmap_info_t *info = (const cm_cnm_lunmap_info_t*)req->data;
        cm_cnm_oplog_param_t params[2] = {
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_LUNMAP_LUN,strlen(info->lun),info->lun},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_LUNMAP_HG,strlen(info->hg),info->hg},
        };
        cm_cnm_oplog_report(sessionid,alarmid,params,cnt,&req->set);
        if(Result == CM_OK)
        {
            (void)cm_thread_start(cm_cnm_lunmap_thread,g_cm_lunmap_handle);
        }
    }   
    return;
}    


/*============================================================================*/

const sint8* g_cm_cnm_lun_script=CM_SCRIPT_DIR"cm_cnm_lun.sh";

extern const cm_omi_map_enum_t CmOmiMapEnumLunInsActions;
extern const cm_omi_map_enum_t CmOmiMapEnumLunUpdActions;

static sint32 cm_cnm_lunmirror_decode_ext(
    const cm_omi_obj_t ObjParam,void* data,cm_omi_field_flag_t *set)
{
    sint32 iRet = CM_OK;
    cm_cnm_lunmirror_info_t *info = data;
    cm_cnm_decode_param_t param_str[] = 
    {
        {CM_OMI_FIELD_LUNMIRROR_PATH,sizeof(info->path),info->path,NULL},
        {CM_OMI_FIELD_LUNMIRROR_NIC,sizeof(info->nic),info->nic,NULL},
        {CM_OMI_FIELD_LUNMIRROR_RDC_IP_MASTER,sizeof(info->rdc_ip_master),info->rdc_ip_master,NULL},
        {CM_OMI_FIELD_LUNMIRROR_RDC_IP_SLAVE,sizeof(info->rdc_ip_slave),info->rdc_ip_slave,NULL},
        {CM_OMI_FIELD_LUNMIRROR_NETMASK_MASTER,sizeof(info->netmask_master),info->netmask_master,NULL},
        {CM_OMI_FIELD_LUNMIRROR_NETMASK_SLAVE,sizeof(info->netmask_slave),info->netmask_slave,NULL},
    };

    cm_cnm_decode_param_t param_num[] = 
    {
        {CM_OMI_FIELD_LUNMIRROR_INSERT_ACTION, sizeof(info->ins_act),&info->ins_act,NULL},
        {CM_OMI_FIELD_LUNMIRROR_NID_MASTER,sizeof(info->nid_master),&info->nid_master,NULL},
        {CM_OMI_FIELD_LUNMIRROR_NID_SLAVE,sizeof(info->nid_slave),&info->nid_slave,NULL},
        {CM_OMI_FIELD_LUNMIRROR_SYNC_ONCE,sizeof(info->sync),&info->sync,NULL},
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

sint32 cm_cnm_lunmirror_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam)
{
    return cm_cnm_decode_comm(ObjParam,sizeof(cm_cnm_lunmirror_info_t),
        cm_cnm_lunmirror_decode_ext,ppDecodeParam);
}

static void cm_cnm_lunmirror_encode_each(cm_omi_obj_t item,void *eachdata,void *arg)
{
    cm_omi_field_flag_t *field = arg;
    cm_cnm_lunmirror_info_t *info = eachdata;
    cm_cnm_map_value_str_t cols_str[] = 
    {
        {CM_OMI_FIELD_LUNMIRROR_PATH,     info->path},
        {CM_OMI_FIELD_LUNMIRROR_NIC,     info->nic},
        {CM_OMI_FIELD_LUNMIRROR_RDC_IP_MASTER,    info->rdc_ip_master},
        {CM_OMI_FIELD_LUNMIRROR_RDC_IP_SLAVE,     info->rdc_ip_slave},
        {CM_OMI_FIELD_LUNMIRROR_NETMASK_MASTER,     info->netmask_master},
        {CM_OMI_FIELD_LUNMIRROR_NETMASK_SLAVE,     info->netmask_slave},
    };
    cm_cnm_map_value_num_t cols_num[] = 
    {
        {CM_OMI_FIELD_LUNMIRROR_NID_MASTER, info->nid_master},
        {CM_OMI_FIELD_LUNMIRROR_NID_SLAVE,  info->nid_slave},
        {CM_OMI_FIELD_LUNMIRROR_INSERT_ACTION, info->ins_act},
    };
    cm_cnm_encode_str(item,field,cols_str,sizeof(cols_str)/sizeof(cm_cnm_map_value_str_t));
    cm_cnm_encode_num(item,field,cols_num,sizeof(cols_num)/sizeof(cm_cnm_map_value_num_t)); 
    return;
}

cm_omi_obj_t cm_cnm_lunmirror_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen)
{
    return cm_cnm_encode_comm_ext(pDecodeParam,pAckData,AckLen,
        sizeof(cm_cnm_lunmirror_info_t),cm_cnm_lunmirror_encode_each);
}


#define cm_cnm_lunmirror_req(cmd,param,ppAck,plen) \
    cm_cnm_request_comm(CM_OMI_OBJECT_LUN_MIRROR,cmd,sizeof(cm_cnm_lunmirror_info_t),param,ppAck,plen)

sint32 cm_cnm_lunmirror_getbatch(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_cnm_lunmirror_req(CM_OMI_CMD_GET_BATCH,pDecodeParam,ppAckData,pAckLen);
}

sint32 cm_cnm_lunmirror_create(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    if(NULL == pDecodeParam)
    {
        return CM_PARAM_ERR;
    }
    return cm_cnm_lunmirror_req(CM_OMI_CMD_ADD,pDecodeParam,ppAckData,pAckLen);
}
    
sint32 cm_cnm_lunmirror_count(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_cnm_lunmirror_req(CM_OMI_CMD_COUNT,pDecodeParam,ppAckData,pAckLen);
}
    
sint32 cm_cnm_lunmirror_delete(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    if(NULL == pDecodeParam)
    {
        return CM_PARAM_ERR;
    }
    return cm_cnm_lunmirror_req(CM_OMI_CMD_DELETE,pDecodeParam,ppAckData,pAckLen);
}

void cm_cnm_lunmirror_oplog_create(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK) ? CM_ALARM_LOG_LUNMIRROR_CREATE_OK : CM_ALARM_LOG_LUNMIRROR_CREATE_FAIL;
    const cm_cnm_decode_info_t *req = pDecodeParam;
    const uint32 cnt = 10;

    if(NULL == pDecodeParam)
    {   
        cm_cnm_oplog_report(sessionid,alarmid,NULL,cnt,NULL);
    }    
    else    
    {   
        cm_cnm_lunmirror_info_t *info = (cm_cnm_lunmirror_info_t *)req->data;
        const sint8 *mname = cm_node_get_name(info->nid_master);
        const sint8 *sname = cm_node_get_name(info->nid_slave);
        cm_cnm_oplog_param_t params[10] = {
            {CM_OMI_DATA_ENUM,CM_OMI_FIELD_LUNMIRROR_INSERT_ACTION,sizeof(info->ins_act), &info->ins_act},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_LUNMIRROR_NID_MASTER,strlen(mname),mname},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_LUNMIRROR_NID_SLAVE,strlen(sname),sname},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_LUNMIRROR_PATH,strlen(info->path),info->path},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_LUNMIRROR_NIC,strlen(info->nic),info->nic},
            
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_LUNMIRROR_RDC_IP_MASTER,strlen(info->rdc_ip_master),info->rdc_ip_master},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_LUNMIRROR_RDC_IP_SLAVE,strlen(info->rdc_ip_slave),info->rdc_ip_slave},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_LUNMIRROR_NETMASK_MASTER,strlen(info->netmask_master),info->netmask_master},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_LUNMIRROR_NETMASK_SLAVE,strlen(info->netmask_slave),info->netmask_slave},
            {CM_OMI_DATA_ENUM,CM_OMI_FIELD_LUNMIRROR_SYNC_ONCE,sizeof(info->sync),&info->sync},
        };
        
        cm_cnm_oplog_report(sessionid,alarmid,params,cnt,&req->set);
    }   
    return;
}

void cm_cnm_lunmirror_oplog_delete(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK) ? CM_ALARM_LOG_LUNMIRROR_DELETE_OK : CM_ALARM_LOG_LUNMIRROR_DELETE_FAIL;
    const cm_cnm_decode_info_t *req = pDecodeParam;
    const uint32 cnt = 2;

    if(NULL == pDecodeParam)
    {   
        cm_cnm_oplog_report(sessionid,alarmid,NULL,cnt,NULL);
    }    
    else    
    {   
        cm_cnm_lunmirror_info_t *info = (cm_cnm_lunmirror_info_t *)req->data;
        const sint8 *mname = cm_node_get_name(info->nid_master);
        cm_cnm_oplog_param_t params[2] = {
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_LUNMIRROR_NID_MASTER,strlen(mname),mname},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_LUNMIRROR_PATH,strlen(info->path),info->path},
        };
        
        cm_cnm_oplog_report(sessionid,alarmid,params,cnt,&req->set);
    }   
    return;
}

static sint32 cm_cnm_lunmirror_local_get_each(void *arg, sint8 **cols, uint32 col_num)
{
    cm_cnm_lunmirror_info_t *info = arg;
    const uint32 def_num = 8;  
    
    /* path nic snid rdc_ip_master rdc_ip_slave sync netmask_master netmask_slave*/
    if(def_num != col_num)
    {
        CM_LOG_WARNING(CM_MOD_CNM,"col_num[%u] def_num[%u]",col_num,def_num);
        return CM_FAIL;
    }
    info->nid_master = cm_node_get_id();
    info->nid_slave = (uint32)atoi(cols[2]);
    info->ins_act=(uint32)cm_cnm_get_enum(&CmOmiMapEnumLunInsActions, cols[7], 0);
    CM_VSPRINTF(info->path,sizeof(info->path),"%s",cols[0]);
    CM_VSPRINTF(info->nic,sizeof(info->nic),"%s",cols[1]);
    CM_VSPRINTF(info->rdc_ip_master,sizeof(info->rdc_ip_master),"%s",cols[3]);
    CM_VSPRINTF(info->rdc_ip_slave,sizeof(info->rdc_ip_slave),"%s",cols[4]);
    CM_VSPRINTF(info->netmask_master,sizeof(info->netmask_master),"%s",cols[5]);
    CM_VSPRINTF(info->netmask_slave,sizeof(info->netmask_slave),"%s",cols[6]);
    return CM_OK;
}

sint32 cm_cnm_lunmirror_local_getbatch(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    sint32 iRet = CM_OK;
    sint8 cmd[CM_STRING_512] = {0};
    
    CM_LOG_INFO(CM_MOD_CNM,"offset[%llu] total[%u]",offset,total);

    CM_VSPRINTF(cmd,sizeof(cmd),"%s mirror_getbatch %llu %u",
        g_cm_cnm_lun_script,offset,total);
    iRet = cm_cnm_exec_get_list(cmd,cm_cnm_lunmirror_local_get_each,
        (uint32)0,sizeof(cm_cnm_lunmirror_info_t),ppAck,&total);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]",iRet);
        return iRet;
    }
    *pAckLen = total * sizeof(cm_cnm_lunmirror_info_t);
    return CM_OK;;
}

sint32 cm_cnm_lunmirror_local_count(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    uint64 cnt = (uint64)cm_exec_int("%s mirror_count",g_cm_cnm_lun_script);
    return cm_cnm_ack_uint64(cnt,ppAck,pAckLen);
}

sint32 cm_cnm_lunmirror_local_create(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    sint8 *sync = NULL;
    sint8 *action = NULL;
    cm_cnm_decode_info_t *decode = param;
    cm_cnm_lunmirror_info_t *info = (cm_cnm_lunmirror_info_t*)decode->data;
    
    sync = cm_cnm_get_enum_str(&CmOmiMapEnumSwitchType,info->sync);
    action = cm_cnm_get_enum_str(&CmOmiMapEnumLunInsActions,info->ins_act);
    
    CM_LOG_WARNING(CM_MOD_CNM,"%u %u '%s' '%s' '%s' '%s' '%s' '%s' '%s' '%s'",
        info->nid_master,info->nid_slave,info->path,
        info->nic,info->rdc_ip_master,info->rdc_ip_slave,
        sync, info->netmask_master, info->netmask_slave,
        action);
        
    return cm_system("%s mirror_create %u '%s' '%s' '%s' '%s' '%s' '%s' '%s' '%s'",
        g_cm_cnm_lun_script,info->nid_slave,info->path,
        info->nic,info->rdc_ip_master,info->rdc_ip_slave,
        sync, info->netmask_master, info->netmask_slave,
        action);
}

sint32 cm_cnm_lunmirror_local_delete(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    cm_cnm_decode_info_t *decode = param;
    cm_cnm_lunmirror_info_t *info = (cm_cnm_lunmirror_info_t*)decode->data;

    CM_LOG_WARNING(CM_MOD_CNM,"'%s'",info->path);
    
    return cm_system("%s mirror_delete '%s'",
        g_cm_cnm_lun_script,info->path);
}

/*============================================================================*/
static sint32 cm_cnm_lunbackup_decode_ext(
    const cm_omi_obj_t ObjParam,void* data,cm_omi_field_flag_t *set)
{
    sint32 iRet = CM_OK;
    cm_cnm_lunbackup_info_t *info = data;
    cm_cnm_decode_param_t param_str[] = 
    {
        {CM_OMI_FIELD_LUNBACKUP_PATH,sizeof(info->path_master),info->path_master,NULL},
        {CM_OMI_FIELD_LUNBACKUP_NIC,sizeof(info->nic),info->nic,NULL},
        {CM_OMI_FIELD_LUNBACKUP_RDC_IP_MASTER,sizeof(info->rdc_ip_master),info->rdc_ip_master,NULL},
        {CM_OMI_FIELD_LUNBACKUP_RDC_IP_SLAVE,sizeof(info->rdc_ip_slave),info->rdc_ip_slave,NULL},
        {CM_OMI_FIELD_LUNBACKUP_IP_SLAVE,sizeof(info->ip_slave),info->ip_slave,NULL},
        {CM_OMI_FIELD_LUNBACKUP_PATH_SLAVE,sizeof(info->path_slave),info->path_slave,NULL},
        {CM_OMI_FIELD_LUNBACKUP_NETMASK_MASTER,sizeof(info->netmask_master),info->netmask_master,NULL},
        {CM_OMI_FIELD_LUNBACKUP_NETMASK_SLAVE,sizeof(info->netmask_slave),info->netmask_slave,NULL},
    };

    cm_cnm_decode_param_t param_num[] = 
    {
        {CM_OMI_FIELD_LUNBACKUP_INSERT_ACTION, sizeof(info->ins_act), &info->ins_act, NULL},
        {CM_OMI_FIELD_LUNBACKUP_UPDATE_ACTION, sizeof(info->upd_act), &info->upd_act, NULL},
        {CM_OMI_FIELD_LUNBACKUP_ASYNC_TIME, sizeof(info->interval), &info->interval, NULL},
        {CM_OMI_FIELD_LUNBACKUP_NID_MASTER,sizeof(info->nid_master),&info->nid_master,NULL},
        {CM_OMI_FIELD_LUNBACKUP_SYNC_ONCE,sizeof(info->sync),&info->sync,NULL},
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

sint32 cm_cnm_lunbackup_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam)
{
    return cm_cnm_decode_comm(ObjParam,sizeof(cm_cnm_lunbackup_info_t),
        cm_cnm_lunbackup_decode_ext,ppDecodeParam);
}

static void cm_cnm_lunbackup_encode_each(cm_omi_obj_t item,void *eachdata,void *arg)
{
    cm_omi_field_flag_t *field = arg;
    cm_cnm_lunbackup_info_t *info = eachdata;
    cm_cnm_map_value_str_t cols_str[] = 
    {
        {CM_OMI_FIELD_LUNBACKUP_PATH,     info->path_master},
        {CM_OMI_FIELD_LUNBACKUP_IP_SLAVE,     info->ip_slave},
        {CM_OMI_FIELD_LUNBACKUP_NIC,    info->nic},
        {CM_OMI_FIELD_LUNBACKUP_RDC_IP_MASTER,     info->rdc_ip_master},
        {CM_OMI_FIELD_LUNBACKUP_RDC_IP_SLAVE,     info->rdc_ip_slave},
        {CM_OMI_FIELD_LUNBACKUP_PATH_SLAVE,     info->path_slave},
        {CM_OMI_FIELD_LUNBACKUP_NETMASK_MASTER,     info->netmask_master},
        {CM_OMI_FIELD_LUNBACKUP_NETMASK_SLAVE,     info->netmask_slave},
    };
    cm_cnm_map_value_num_t cols_num[] = 
    {
        {CM_OMI_FIELD_LUNBACKUP_INSERT_ACTION, info->ins_act},
        {CM_OMI_FIELD_LUNBACKUP_NID_MASTER, info->nid_master},
        {CM_OMI_FIELD_LUNBACKUP_ASYNC_TIME, info->interval},
    };
    cm_cnm_encode_str(item,field,cols_str,sizeof(cols_str)/sizeof(cm_cnm_map_value_str_t));
    cm_cnm_encode_num(item,field,cols_num,sizeof(cols_num)/sizeof(cm_cnm_map_value_num_t)); 
    return;
}

cm_omi_obj_t cm_cnm_lunbackup_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen)
{
    return cm_cnm_encode_comm_ext(pDecodeParam,pAckData,AckLen,
        sizeof(cm_cnm_lunbackup_info_t),cm_cnm_lunbackup_encode_each);
}

// info->nid==0:send to all nodes
#define cm_cnm_lunbackup_req(cmd,param,ppAck,plen) \
    cm_cnm_request_comm(CM_OMI_OBJECT_LUN_BACKUP,cmd,sizeof(cm_cnm_lunbackup_info_t),param,ppAck,plen)

sint32 cm_cnm_lunbackup_getbatch(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_cnm_lunbackup_req(CM_OMI_CMD_GET_BATCH,pDecodeParam,ppAckData,pAckLen);
}

sint32 cm_cnm_lunbackup_create(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    if(NULL == pDecodeParam)
    {
        return CM_PARAM_ERR;
    }
    return cm_cnm_lunbackup_req(CM_OMI_CMD_ADD,pDecodeParam,ppAckData,pAckLen);
}
    
sint32 cm_cnm_lunbackup_count(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_cnm_lunbackup_req(CM_OMI_CMD_COUNT,pDecodeParam,ppAckData,pAckLen);
}

sint32 cm_cnm_lunbackup_modify(
    const void *pDecodeParam, void **ppAckData, uint32 *pAckLen)
{
    sint8 *action = NULL;
    sint8 command[CM_STRING_256] = {0};
    sint8 desc[CM_STRING_256] = {0};
    cm_cnm_decode_info_t *decode = pDecodeParam;
    cm_cnm_lunbackup_info_t *info = (cm_cnm_lunbackup_info_t*)decode->data;

    // 0:SET_TIME 1:FULL_SYNC 2:UPDATE_SYNC 3:REV_FULL_SYNC 4:REV_UPDATE_SYNC
    action = cm_cnm_get_enum_str(&CmOmiMapEnumLunUpdActions, info->upd_act);
    CM_LOG_WARNING(CM_MOD_CNM, "'%s' '%s' '%s' '%s' '%u'",
                   action, info->path_master, info->ip_slave,
                   info->path_slave, info->interval);

    //set interval but no time param
    if(info->upd_act == 0)
    {
        if(info->interval == 0)
        {
            CM_LOG_ERR(CM_MOD_CNM, "less param to set interval for async");
            return CM_PARAM_ERR;
        }
        else
        {
            return cm_cnm_lunbackup_req(CM_OMI_CMD_MODIFY, pDecodeParam, ppAckData, pAckLen);
        }
    }

    CM_VSPRINTF(command, sizeof(command), "%s backup_update %s %s %s %s %u",
                g_cm_cnm_lun_script, action, info->path_master,
                info->ip_slave, info->path_slave, info->interval);
    CM_VSPRINTF(desc, sizeof(desc), "%s|%s|%s|%u|%s", info->path_master,
                info->ip_slave, info->path_slave, info->nid_master, action);
    return cm_task_add(info->nid_master, CM_TASK_TYPE_SNAPSHOT_SAN_BACKUP,
                       command, desc);
}

    
sint32 cm_cnm_lunbackup_delete(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    if(NULL == pDecodeParam)
    {
        return CM_PARAM_ERR;
    }
    return cm_cnm_lunbackup_req(CM_OMI_CMD_DELETE,pDecodeParam,ppAckData,pAckLen);
}

void cm_cnm_lunbackup_oplog_create(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK) ? CM_ALARM_LOG_LUNBACKUP_CREATE_OK : CM_ALARM_LOG_LUNBACKUP_CREATE_FAIL;
    const cm_cnm_decode_info_t *req = pDecodeParam;
    const uint32 cnt = 11;

    if(NULL == pDecodeParam)
    {   
        cm_cnm_oplog_report(sessionid,alarmid,NULL,cnt,NULL);
    }    
    else    
    {   
        cm_cnm_lunbackup_info_t *info = (cm_cnm_lunbackup_info_t *)req->data;
        const sint8 *mname = cm_node_get_name(info->nid_master);
        cm_cnm_oplog_param_t params[11] = {
            {CM_OMI_DATA_ENUM,CM_OMI_FIELD_LUNBACKUP_INSERT_ACTION,sizeof(info->ins_act), &info->ins_act},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_LUNBACKUP_NID_MASTER,strlen(mname),mname},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_LUNBACKUP_IP_SLAVE,strlen(info->ip_slave),info->ip_slave},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_LUNBACKUP_PATH,strlen(info->path_master),info->path_master},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_LUNBACKUP_PATH_SLAVE,strlen(info->path_slave),info->path_slave},
            
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_LUNBACKUP_NIC,strlen(info->nic),info->nic},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_LUNBACKUP_RDC_IP_MASTER,strlen(info->rdc_ip_master),info->rdc_ip_master},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_LUNBACKUP_RDC_IP_SLAVE,strlen(info->rdc_ip_slave),info->rdc_ip_slave},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_LUNBACKUP_NETMASK_MASTER,strlen(info->netmask_master),info->netmask_master},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_LUNBACKUP_NETMASK_SLAVE,strlen(info->netmask_slave),info->netmask_slave},
            
            {CM_OMI_DATA_ENUM,CM_OMI_FIELD_LUNBACKUP_SYNC_ONCE,sizeof(info->sync),&info->sync},
        };
        
        cm_cnm_oplog_report(sessionid,alarmid,params,cnt,&req->set);
    }   
    return;
}

void cm_cnm_lunbackup_oplog_modify(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK) ? CM_ALARM_LOG_LUNBACKUP_MODIFY_OK : CM_ALARM_LOG_LUNBACKUP_MODIFY_FAIL;
    const cm_cnm_decode_info_t *req = pDecodeParam;
    const uint32 cnt = 6;

    if(NULL == pDecodeParam)
    {   
        cm_cnm_oplog_report(sessionid,alarmid,NULL,cnt,NULL);
    }    
    else    
    {   
        cm_cnm_lunbackup_info_t *info = (cm_cnm_lunbackup_info_t *)req->data;
        const sint8 *mname = cm_node_get_name(info->nid_master);
        cm_cnm_oplog_param_t params[6] = {
            {CM_OMI_DATA_ENUM,CM_OMI_FIELD_LUNBACKUP_UPDATE_ACTION,sizeof(info->upd_act), &info->upd_act},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_LUNBACKUP_NID_MASTER,strlen(mname),mname},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_LUNBACKUP_IP_SLAVE,strlen(info->ip_slave),info->ip_slave},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_LUNBACKUP_PATH,strlen(info->path_master),info->path_master},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_LUNBACKUP_PATH_SLAVE,strlen(info->path_slave),info->path_slave},
            {CM_OMI_DATA_INT,CM_OMI_FIELD_LUNBACKUP_ASYNC_TIME,sizeof(info->interval),&info->interval},
        };
        
        cm_cnm_oplog_report(sessionid,alarmid,params,cnt,&req->set);
    }   
    return;
}


void cm_cnm_lunbackup_oplog_delete(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK) ? CM_ALARM_LOG_LUNBACKUP_DELETE_OK : CM_ALARM_LOG_LUNBACKUP_DELETE_FAIL;
    const cm_cnm_decode_info_t *req = pDecodeParam;
    const uint32 cnt = 4;

    if(NULL == pDecodeParam)
    {   
        cm_cnm_oplog_report(sessionid,alarmid,NULL,cnt,NULL);
    }    
    else    
    {   
        cm_cnm_lunbackup_info_t *info = (cm_cnm_lunbackup_info_t *)req->data;
        const sint8 *mname = cm_node_get_name(info->nid_master);
        cm_cnm_oplog_param_t params[4] = {
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_LUNBACKUP_NID_MASTER,strlen(mname),mname},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_LUNBACKUP_IP_SLAVE,strlen(info->ip_slave),info->ip_slave},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_LUNBACKUP_PATH,strlen(info->path_master),info->path_master},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_LUNBACKUP_PATH_SLAVE,strlen(info->path_slave),info->path_slave},
        };
        
        cm_cnm_oplog_report(sessionid,alarmid,params,cnt,&req->set);
    }   
    return;
}

static sint32 cm_cnm_lunbackup_local_get_each(void *arg, sint8 **cols, uint32 col_num)
{
    cm_cnm_lunbackup_info_t *info = arg;
    const uint32 def_num = 10;  
    
    /* path nic slave_ip rdc_ip_master rdc_ip_slave path_slave netmask_master netmask_slave sync sync_gap*/
    if(def_num != col_num)
    {
        CM_LOG_WARNING(CM_MOD_CNM,"col_num[%u] def_num[%u]",col_num,def_num);
        return CM_FAIL;
    }
    info->nid_master = cm_node_get_id();
    info->ins_act=(uint32)cm_cnm_get_enum(&CmOmiMapEnumLunInsActions, cols[8], 0);
    CM_VSPRINTF(info->path_master,sizeof(info->path_master),"%s",cols[0]);
    CM_VSPRINTF(info->nic,sizeof(info->nic),"%s",cols[1]);
    CM_VSPRINTF(info->ip_slave,sizeof(info->ip_slave),"%s",cols[2]);
    CM_VSPRINTF(info->rdc_ip_master,sizeof(info->rdc_ip_master),"%s",cols[3]);
    CM_VSPRINTF(info->rdc_ip_slave,sizeof(info->rdc_ip_slave),"%s",cols[4]);
    CM_VSPRINTF(info->path_slave,sizeof(info->path_slave),"%s",cols[5]);
    CM_VSPRINTF(info->netmask_master,sizeof(info->netmask_master),"%s",cols[6]);
    CM_VSPRINTF(info->netmask_slave,sizeof(info->netmask_slave),"%s",cols[7]);
    info->interval = (uint32)atoi(cols[9]);
    return CM_OK;
}

sint32 cm_cnm_lunbackup_local_getbatch(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    sint32 iRet = CM_OK;
    sint8 cmd[CM_STRING_512] = {0};
    
    CM_LOG_INFO(CM_MOD_CNM,"offset[%llu] total[%u]",offset,total);

    CM_VSPRINTF(cmd,sizeof(cmd),"%s backup_getbatch %llu %u",
        g_cm_cnm_lun_script,offset,total);
    iRet = cm_cnm_exec_get_list(cmd,cm_cnm_lunbackup_local_get_each,
        (uint32)0,sizeof(cm_cnm_lunbackup_info_t),ppAck,&total);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]",iRet);
        return iRet;
    }
    *pAckLen = total * sizeof(cm_cnm_lunbackup_info_t);
    return CM_OK;;
}

sint32 cm_cnm_lunbackup_local_count(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    uint64 cnt = (uint64)cm_exec_int("%s backup_count",g_cm_cnm_lun_script);
    return cm_cnm_ack_uint64(cnt,ppAck,pAckLen);
}

sint32 cm_cnm_lunbackup_local_create(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    sint8 *sync = NULL;
    sint8 *action = NULL;
    sint8 cmd[CM_STRING_1K] = {0};
    cm_cnm_decode_info_t *decode = param;
    cm_cnm_lunbackup_info_t *info = (cm_cnm_lunbackup_info_t*)decode->data;

    sync = cm_cnm_get_enum_str(&CmOmiMapEnumSwitchType, info->sync);
    // 0:SYNC, 1:ASYNC
    action = cm_cnm_get_enum_str(&CmOmiMapEnumLunInsActions, info->ins_act);  
    CM_LOG_WARNING(CM_MOD_CNM,"'%s' '%s' '%s' '%s' '%s' '%s' '%s' '%s' '%s' '%s'",
        info->ip_slave,info->path_master,
        info->nic,info->rdc_ip_master,info->rdc_ip_slave,info->path_slave,
        sync, info->netmask_master, info->netmask_slave, action);
    CM_SNPRINTF_ADD(cmd, CM_STRING_1K, "%s backup_create '%s' '%s' '%s' '%s' '%s' '%s' '%s' '%s' '%s' '%s'",
        g_cm_cnm_lun_script,info->ip_slave,info->path_master,
        info->nic,info->rdc_ip_master,info->rdc_ip_slave,info->path_slave,
        sync, info->netmask_master, info->netmask_slave, action);
    CM_LOG_WARNING(CM_MOD_CNM, "cmd:%s", cmd);
    return cm_system(cmd);
}

sint32 cm_cnm_lunbackup_local_modify(
    void *param, uint32 len,
    uint64 offset, uint32 total,
    void **ppAck, uint32 *pAckLen)
{
    sint8 *action = NULL;
    cm_cnm_decode_info_t *decode = param;
    cm_cnm_lunbackup_info_t *info = (cm_cnm_lunbackup_info_t*)decode->data;

    if(info->upd_act == 0)
    {
        // 只可能是SET_TIME
        action = cm_cnm_get_enum_str(&CmOmiMapEnumLunUpdActions, info->upd_act);
        CM_LOG_WARNING(CM_MOD_CNM, "'%s' '%s' '%s' '%s' '%u'",
                       action, info->path_master, info->ip_slave,
                       info->path_slave, info->interval);
        return cm_system("%s backup_update '%s' '%s' '%s' '%s' '%u'",
                         g_cm_cnm_lun_script, action, info->path_master,
                         info->ip_slave, info->path_slave, info->interval);
    }

    return CM_FAIL;
}


sint32 cm_cnm_lunbackup_local_delete(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    cm_cnm_decode_info_t *decode = param;
    cm_cnm_lunbackup_info_t *info = (cm_cnm_lunbackup_info_t*)decode->data;

    CM_LOG_WARNING(CM_MOD_CNM,"'%s' '%s' '%s'",info->path_master, info->ip_slave,info->path_slave);
    
    return cm_system("%s backup_delete '%s' '%s' '%s'",
        g_cm_cnm_lun_script,info->path_master, info->ip_slave,info->path_slave);
}

sint32 cm_cnm_lunbackup_cbk_task_report
(cm_task_send_state_t *pSnd, cm_task_cmt_echo_t **pproc)
{
    cm_task_cmt_echo_t *pCmt = CM_MALLOC(sizeof(cm_task_cmt_echo_t));
    if(NULL == pCmt)
    {
        CM_LOG_ERR(CM_MOD_CNM, "malloc fail");
        return CM_FAIL;
    }

    pCmt->task_pid = pSnd->task_pid;
    pCmt->task_prog = cm_exec_int("/var/cm/script/cm_cnm_lun.sh backup_prog '%s'", pSnd->task_param);
    if(pCmt->task_prog > 100)
    {
        pCmt->task_prog = 100;
        pCmt->task_state = CM_TASK_STATE_WRONG;
        pCmt->task_end = cm_get_time();
    }
    else if(pCmt->task_prog == 100)
    {
        pCmt->task_state = CM_TASK_STATE_FINISHED;
        pCmt->task_end = cm_get_time();
    }
    else
    {
        pCmt->task_state = CM_TASK_STATE_RUNNING;
        pCmt->task_end = 0;
    }
    *pproc = pCmt;
    return CM_OK;
}

