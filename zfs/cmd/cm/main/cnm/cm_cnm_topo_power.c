/******************************************************************************
 * description  : cm_cnm_topo_power.c
 * author       : zjd
 * create date  : 2018.8.13
 *****************************************************************************/
#include "cm_cnm_topo_power.h"
#include "cm_sync.h"
#include "cm_node.h"
const uint8* g_cm_cnm_topo_power_cmd = "cat /tmp/sensor.txt |grep +12V|awk -F'|' '{print $1 $2 $4}'";/*add  $6 $9*/

const uint8* g_cm_cnm_select_nid_cmd = "sqlite3 /var/cm/data/cm_node.db " 
    "\"select id from record_t WHERE sbbid=%u LIMIT 1\"";

static uint32 cm_cnm_select_nid_form_enid(uint32 enid);

static sint32 cm_cnm_topo_power_decode_ext(
        const cm_omi_obj_t ObjParam,void* data,cm_omi_field_flag_t *set)
{
    sint32 iRet = CM_OK;
    cm_cnm_topo_power_info_t* topo_power_info = data;

    cm_cnm_decode_param_t param_num[] = 
    {   
        {CM_OMI_FIRLD_TOPO_POWER_ENID,sizeof(topo_power_info->enid),&topo_power_info->enid,NULL},
    };
    
    iRet = cm_cnm_decode_num(ObjParam,param_num,
       sizeof(param_num)/sizeof(cm_cnm_decode_param_t),set);
    if(iRet != CM_OK)
    {
        return iRet;
    }
    return CM_OK;
}

sint32 cm_cnm_topo_power_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam)
{
    sint32 iRet = cm_cnm_decode_comm(ObjParam,sizeof(cm_cnm_topo_power_info_t),
        cm_cnm_topo_power_decode_ext,ppDecodeParam);
    if(NULL != *ppDecodeParam)
    {
        cm_cnm_decode_info_t *pdecode = *ppDecodeParam;
        cm_cnm_topo_power_info_t *info = (cm_cnm_topo_power_info_t*)pdecode->data;
        pdecode->nid = cm_cnm_select_nid_form_enid(info->enid);
    }
    return iRet;
}

static void cm_cnm_topo_power_encode_each(cm_omi_obj_t item,void *eachdata,void *arg)
{
    cm_omi_field_flag_t *field = arg;
    cm_cnm_topo_power_info_t * topo_power_info = eachdata;
    
    cm_cnm_map_value_str_t cols_str[] = 
    {
        {CM_OMI_FIRLD_TOPO_POWER,topo_power_info->power},
    };

    cm_cnm_map_value_num_t cols_num[] = 
    {
        {CM_OMI_FIRLD_TOPO_POWER_ENID,topo_power_info->enid}, 
        {CM_OMI_FIRLD_TOPO_POWER_STATUS,topo_power_info->status},
    };
    cm_cnm_map_value_double_t cols_double[] =
    {
        {CM_OMI_FIRLD_TOPO_POWER_VOLT,topo_power_info->volts},
    };
    cm_cnm_encode_str(item,field,cols_str,sizeof(cols_str)/sizeof(cm_cnm_map_value_str_t));
    cm_cnm_encode_num(item,field,cols_num,sizeof(cols_num)/sizeof(cm_cnm_map_value_num_t));
    cm_cnm_encode_double(item,field,cols_double,sizeof(cols_double)/sizeof(cm_cnm_map_value_double_t));
    return;
}

cm_omi_obj_t cm_cnm_topo_power_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen)
{
    return cm_cnm_encode_comm_ext(pDecodeParam,pAckData,AckLen,
        sizeof(cm_cnm_topo_power_info_t),cm_cnm_topo_power_encode_each);
}

static sint32 cm_cnm_each_sbb_request_comm(uint32 obj, uint32 cmd,
    uint32 eachsize, const void* pDecodeParam, void **ppAck, uint32 *pAckLen)
{
    const cm_cnm_decode_info_t *pDecode = pDecodeParam;
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
        param.param_len = sizeof(cm_cnm_decode_info_t) + eachsize;
    }

    param.obj = obj;
    param.cmd = cmd;
    param.ppack = ppAck;
    param.acklen = pAckLen;
    param.sbb_only = CM_TRUE;
    return cm_cnm_request(&param);
}


#define cm_cnm_topo_power_req(cmd,param,ppAck,plen) \
    cm_cnm_each_sbb_request_comm(CM_OMI_OBJECT_TOPO_POWER,cmd,sizeof(cm_cnm_topo_power_info_t),param,ppAck,plen)

static uint32 cm_cnm_select_nid_form_enid(uint32 enid)
{
    if(enid % 1000)
    {
        return CM_NODE_ID_NONE;
    }
    return cm_exec_int(g_cm_cnm_select_nid_cmd,enid/1000);
}

sint32 cm_cnm_topo_power_getbatch(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    const cm_cnm_decode_info_t * decode = (cm_cnm_decode_info_t *)pDecodeParam;
    const cm_cnm_topo_power_info_t * info = NULL;
    uint32 nid = 0;
    if(decode)
    {
        info = decode->data;
        if(CM_OMI_FIELDS_FLAG_ISSET(&decode->set, CM_OMI_FIRLD_TOPO_POWER_ENID))
        {
            if(info->enid < 1000)
            {
                return CM_OK;
            }
        }
    }
    return cm_cnm_topo_power_req(CM_OMI_CMD_GET_BATCH,pDecodeParam,ppAckData,pAckLen);
}

sint32 cm_cnm_topo_power_count(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    const cm_cnm_decode_info_t * decode = (cm_cnm_decode_info_t *)pDecodeParam;
    const cm_cnm_topo_power_info_t * info = decode->data;
    
    if((NULL != decode)&&(CM_OMI_FIELDS_FLAG_ISSET(&decode->set, CM_OMI_FIRLD_TOPO_POWER_ENID)))
    {
        if(info->enid < 1000)
        {
            return cm_cnm_ack_uint64(0,ppAckData,pAckLen);
        }        
    }
    return cm_cnm_topo_power_req(CM_OMI_CMD_COUNT,pDecodeParam,ppAckData,pAckLen);
}

static sint32 cm_cnm_topo_power_local_get_each(void *arg, sint8 **cols, uint32 col_num)
{
    cm_cnm_topo_power_info_t *topo_power_info = arg;
    const uint32 min_num = 3;
    const uint32 var = col_num - min_num;
    if(col_num < min_num)
    {
        CM_LOG_WARNING(CM_MOD_CNM,"col_num[%u]",col_num);
        return CM_FAIL;
    }
    /*get ret*/
    
    CM_MEM_CPY(topo_power_info->power,sizeof(topo_power_info->power),cols[0],strlen(cols[0])+1);
    topo_power_info->volts = atof(cols[1 + var]);
    topo_power_info->status= (CM_OK == strcmp(cols[2 + var],"ok")) ? CM_OK : CM_FAIL;
    topo_power_info->enid = (uint32)(cm_node_get_sbbid() * 1000);

    return CM_OK;
}

sint32 cm_cnm_topo_power_local_getbatch(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    sint32 iRet = CM_OK;
    iRet = cm_cnm_exec_get_list(g_cm_cnm_topo_power_cmd,cm_cnm_topo_power_local_get_each,
        (uint32)offset,sizeof(cm_cnm_topo_power_info_t),ppAck,&total);

    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]",iRet);
        return iRet;
    }
    *pAckLen = total * sizeof(cm_cnm_topo_power_info_t);

    return CM_OK;

}

sint32 cm_cnm_topo_power_local_count(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    uint64 cnt = 0;
    
    cnt = (uint64)cm_exec_int("%s|wc -l",g_cm_cnm_topo_power_cmd);
    
    return cm_cnm_ack_uint64(cnt,ppAck,pAckLen);
}

/*  fan   */

const sint8* g_cm_cnm_topo_fan_cmd = "cat /tmp/sensor.txt |grep RPM|awk -F'|' '{print $1 $2 $4}'";/*add  $6 $9*/

const sint8* g_cm_cnm_topo_fan_sh = "/var/cm/script/cm_cnm_topo_fan.sh %u";

const sint8* g_cm_cnm_topo_fan_get_sbbid = "sqlite3 /var/cm/data/cm_node.db " 
    "\"SELECT sbbid FROM record_t WHERE id=%u\"";

static sint32 cm_cnm_topo_fan_decode_ext(
        const cm_omi_obj_t ObjParam,void* data,cm_omi_field_flag_t *set)
{
    sint32 iRet = CM_OK;
    cm_cnm_topo_fan_info_t* topo_fan_info = data;

    cm_cnm_decode_param_t param_num[] = 
    {
        {CM_OMI_FIRLD_TOPO_FAN_ENID,sizeof(topo_fan_info->enid),&topo_fan_info->enid,NULL},
        {CM_OMI_FIRLD_TOPO_FAN_NID,sizeof(topo_fan_info->nid),&topo_fan_info->nid,NULL},
    };
    
    iRet = cm_cnm_decode_num(ObjParam,param_num,
       sizeof(param_num)/sizeof(cm_cnm_decode_param_t),set);
    if(iRet != CM_OK)
    {
        return iRet;
    }
    return CM_OK;
}

sint32 cm_cnm_topo_fan_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam)
{
    return cm_cnm_decode_comm(ObjParam,sizeof(cm_cnm_topo_fan_info_t),
        cm_cnm_topo_fan_decode_ext,ppDecodeParam);
}

static void cm_cnm_topo_fan_encode_each(cm_omi_obj_t item,void *eachdata,void *arg)
{
    cm_omi_field_flag_t *field = arg;
    cm_cnm_topo_fan_info_t * topo_fan_info = eachdata;
    
    cm_cnm_map_value_str_t cols_str[] = 
    {
        {CM_OMI_FIRLD_TOPO_FAN,topo_fan_info->fan},
    };

    cm_cnm_map_value_num_t cols_num[] = 
    {
        {CM_OMI_FIRLD_TOPO_FAN_ENID,topo_fan_info->enid},
        {CM_OMI_FIRLD_TOPO_FAN_NID,topo_fan_info->nid},
        {CM_OMI_FIRLD_TOPO_FAN_ROTATE,topo_fan_info->rotate},
        {CM_OMI_FIRLD_TOPO_FAN_STATUS,topo_fan_info->status},
    };
    cm_cnm_encode_str(item,field,cols_str,sizeof(cols_str)/sizeof(cm_cnm_map_value_str_t));
    cm_cnm_encode_num(item,field,cols_num,sizeof(cols_num)/sizeof(cm_cnm_map_value_num_t));
    return;
}

cm_omi_obj_t cm_cnm_topo_fan_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen)
{
    return cm_cnm_encode_comm_ext(pDecodeParam,pAckData,AckLen,
        sizeof(cm_cnm_topo_fan_info_t),cm_cnm_topo_fan_encode_each);
}

#define cm_cnm_topo_fan_req(cmd,param,ppAck,plen) \
    cm_cnm_request_comm(CM_OMI_OBJECT_TOPO_FAN,cmd,sizeof(cm_cnm_topo_fan_info_t),param,ppAck,plen)
    
static sint32 cm_cnm_topo_fan_get_each(void *arg, sint8 **cols, uint32 col_num)
{
    cm_cnm_topo_fan_info_t *topo_fan_info = arg;
    const uint32 min_num = 4;
    const uint32 var = col_num - min_num;
    if(col_num < min_num)
    {
        CM_LOG_ERR(CM_MOD_CNM,"col_num[%u]",col_num);
        return CM_FAIL;
    }

    /*get ret*/
    CM_MEM_CPY(topo_fan_info->fan,sizeof(topo_fan_info->fan),cols[0],strlen(cols[0])+1);
    topo_fan_info->rotate= atoi(cols[1 + var]);
    topo_fan_info->status= (CM_OK == strcmp(cols[2 + var],"ok")) ? CM_OK : CM_FAIL;
    topo_fan_info->nid = atoi(cols[3 + var]);
    topo_fan_info->enid = cm_exec_int(g_cm_cnm_topo_fan_get_sbbid,topo_fan_info->nid)*1000;

    return CM_OK;
}

sint32 cm_cnm_topo_fan_getbatch(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{ 
    cm_cnm_decode_info_t * decode = (cm_cnm_decode_info_t *)pDecodeParam;
    cm_cnm_topo_power_info_t * info = decode->data;
    sint8 cmd[128] = {0};
    sint32 iRet = CM_OK;
    uint32 total = 100;
    if((NULL != decode)&&(CM_OMI_FIELDS_FLAG_ISSET(&decode->set, CM_OMI_FIRLD_TOPO_POWER_ENID)))
    {
        if((info->enid < 1000)||(info->enid % 1000))
        {
            return CM_OK;
        }
        CM_VSPRINTF(cmd,sizeof(cmd),g_cm_cnm_topo_fan_sh,(info->enid / 1000));
        iRet = cm_cnm_exec_get_list(cmd,cm_cnm_topo_fan_get_each,
            0,sizeof(cm_cnm_topo_fan_info_t),ppAckData,&total);

        if(CM_OK != iRet)
        {
            CM_LOG_ERR(CM_MOD_CNM,"iRet[%d] cmd[%s]",iRet,cmd);
            return CM_OK;
        }
        *pAckLen = total * sizeof(cm_cnm_topo_fan_info_t);
        return CM_OK;
    }
    else
    {
        return cm_cnm_topo_fan_req(CM_OMI_CMD_GET_BATCH,pDecodeParam,ppAckData,pAckLen);
    }
}

sint32 cm_cnm_topo_fan_count(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    const cm_cnm_decode_info_t * decode = (cm_cnm_decode_info_t *)pDecodeParam;
    const cm_cnm_topo_power_info_t * info = decode->data;
    sint8 cmd[128] = {0};
    uint64 cnt = 0;
    if((NULL != decode)&&(CM_OMI_FIELDS_FLAG_ISSET(&decode->set, CM_OMI_FIRLD_TOPO_POWER_ENID)))
    {
        if((info->enid < 1000)||(info->enid % 1000))
        {
            return cm_cnm_ack_uint64(0,ppAckData,pAckLen);
        }
        CM_VSPRINTF(cmd,sizeof(cmd),g_cm_cnm_topo_fan_sh,(info->enid / 1000));

        cnt = (uint64)cm_exec_int("%s|wc -l",cmd);
    
        return cm_cnm_ack_uint64(cnt,ppAckData,pAckLen);
    }
    else
    {
        return cm_cnm_topo_fan_req(CM_OMI_CMD_COUNT,pDecodeParam,ppAckData,pAckLen);
    }
}

static sint32 cm_cnm_topo_fan_local_get_each(void *arg, sint8 **cols, uint32 col_num)
{
    cm_cnm_topo_fan_info_t *topo_fan_info = arg;
    const uint32 min_num = 3;
    const uint32 var = col_num - min_num;
    if(col_num < min_num)
    {
        CM_LOG_WARNING(CM_MOD_CNM,"col_num[%u]",col_num);
        return CM_FAIL;
    }
    /*get ret*/
    CM_MEM_CPY(topo_fan_info->fan,sizeof(topo_fan_info->fan),cols[0],strlen(cols[0])+1);
    topo_fan_info->rotate= atoi(cols[1 + var]);
    topo_fan_info->status= (CM_OK == strcmp(cols[2 + var],"ok")) ? CM_OK : CM_FAIL;
    topo_fan_info->nid = cm_node_get_id();
    topo_fan_info->enid = cm_node_get_sbbid()*1000;

    return CM_OK;
}

sint32 cm_cnm_topo_fan_local_getbatch(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    sint32 iRet = CM_OK;
    iRet = cm_cnm_exec_get_list(g_cm_cnm_topo_fan_cmd,cm_cnm_topo_fan_local_get_each,
        (uint32)offset,sizeof(cm_cnm_topo_fan_info_t),ppAck,&total);

    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]",iRet);
        return iRet;
    }
    *pAckLen = total * sizeof(cm_cnm_topo_fan_info_t);

    return CM_OK;
}

sint32 cm_cnm_topo_fan_local_count(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    uint64 cnt = 0;

    cnt = (uint64)cm_exec_int("%s|wc -l",g_cm_cnm_topo_fan_cmd);
    
    return cm_cnm_ack_uint64(cnt,ppAck,pAckLen);
}

#define CM_TOPO_DB_DIR "/var/cm/data/cm_topo.db"
extern const cm_omi_map_enum_t CmOmiMapEnumDevType;

static cm_db_handle_t topo_db_handle = NULL;

#define cm_cnm_topo_table_request(cmd,param,ppAck,plen) \
    cm_cnm_request_comm(CM_OMI_OBJECT_TOPO_TABLE,cmd,sizeof(cm_cnm_topo_table_info_t),param,ppAck,plen)

sint32 cm_cnm_topo_table_init(void)
{
    sint32 iRet = CM_OK;
    const sint8* table = "CREATE TABLE IF NOT EXISTS topo_t ("
                         "enid INT,"
                         "name VARCHAR(127),"
                         "setx INT,"
                         "typex INT,"
                         "Unum INT,"
                         "slot INT,"
                         "sn VARCHAR(127))";
    
    iRet = cm_db_open_ext(CM_TOPO_DB_DIR,&topo_db_handle);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"open %s fail",CM_TOPO_DB_DIR);
        return iRet;
    }
    iRet = cm_db_exec_ext(topo_db_handle, table);
    if(iRet != CM_OK)
    {
        CM_LOG_ERR(CM_MOD_CNM, "create topo_t fail[%d]", iRet);
        return iRet;
    }
    return CM_OK;
}

sint32 cm_cnm_topo_table_local_get_each(
    void *arg, sint32 col_cnt, sint8 **col_vals, sint8 **col_names)
{
    cm_cnm_topo_table_info_t *info = arg;

    if(col_cnt != 7)
    {
        CM_LOG_ERR(CM_MOD_CNM,"col_cnt[%d]",col_cnt);
        return CM_FAIL;
    }
    CM_MEM_ZERO(info,sizeof(cm_cnm_topo_table_info_t));
    info->enid = (uint32)atoi(col_vals[0]);
    CM_VSPRINTF(info->name,sizeof(info->name),"%s",col_vals[1]);
    info->set = (uint32)atoi(col_vals[2]);
    info->type = (uint8)atoi(col_vals[3]);
    info->U_num = atoi(col_vals[4]);
    info->slot_num = atoi(col_vals[5]);
    CM_VSPRINTF(info->sn,sizeof(info->sn),"%s",col_vals[6]);
    return CM_OK;
}

sint32  cm_cnm_topo_table_insert(void *pDecodeParam,void **ppAckData,uint32 * pAckLen)
{
    sint32 iRet = CM_OK;
    uint32 cnt = 0;
    cm_cnm_decode_info_t *decode = pDecodeParam;
    cm_cnm_topo_table_info_t *info = NULL;
    if(decode == NULL)
    {
        CM_LOG_ERR(CM_MOD_CNM,"decode param null");
        return CM_PARAM_ERR;
    }
    info = (const cm_cnm_topo_table_info_t *)decode->data;
    iRet = cm_db_exec_get_count(topo_db_handle, &cnt,
        "SELECT COUNT(enid) FROM topo_t WHERE enid=%u",(uint32)info->enid);                                 
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM, "get enid count fail[%d]", iRet);
        return CM_FAIL;
    }
    if(cnt != 0)
    {
        CM_LOG_ERR(CM_MOD_CNM,"this enid %u is already exits",info->enid);
        return CM_ERR_ALREADY_EXISTS;
    }
    iRet = cm_db_exec_get_count(topo_db_handle, &cnt,
        "SELECT COUNT(sn) FROM topo_t WHERE sn='%s'",info->sn);                                 
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM, "get sn count fail[%d]", iRet);
        return CM_FAIL;
    }
    if(cnt != 0)
    {
        CM_LOG_ERR(CM_MOD_CNM,"this sn %u is already exits",info->sn);
        return CM_ERR_ALREADY_EXISTS;
    }
    if((info->enid % 1000) != 0)    /*ENC*/
    {
        info->type = 2;
    }
    else
    {
        if(info->slot_num == 24)
        {
            info->type = 1;     /*AIC*/
        }
        else
        {
            info->type = 0;     /*SBB*/
        }
    }
    return cm_sync_request(CM_SYNC_OBJ_TOPO_TABLE,(uint64)info->enid,info,sizeof(cm_cnm_topo_table_info_t));
}    
sint32  cm_cnm_topo_table_update(const void *pDecodeParam,void **ppAckData,uint32 * pAckLen)
{
    uint32 cnt = 0;
    const cm_cnm_decode_info_t *decode = pDecodeParam;
    cm_cnm_topo_table_info_t *data = CM_MALLOC(sizeof(cm_cnm_topo_table_info_t));
    const cm_cnm_topo_table_info_t *info = NULL;
    if(data == NULL)
    {
        CM_LOG_ERR(CM_MOD_CNM,"malloc fail");
        return CM_FAIL;
    }
    if(decode == NULL)
    {
        CM_LOG_ERR(CM_MOD_CNM,"decode param null");
        CM_FREE(data);
        return CM_PARAM_ERR;
    }
    if(!(CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_TOPO_TABLE_ENID)))
    {
        CM_LOG_ERR(CM_MOD_CNM,"enid can't be null");
        CM_FREE(data);
        return CM_PARAM_ERR;
    }
    info = (const cm_cnm_topo_table_info_t *)decode->data;
    CM_MEM_ZERO(data,sizeof(cm_cnm_topo_table_info_t));

    cnt = cm_db_exec_get(topo_db_handle,cm_cnm_topo_table_local_get_each,data,1,
        sizeof(cm_cnm_topo_table_info_t),"SELECT * FROM topo_t WHERE enid=%u",info->enid);
    if(cnt == 0)
    {
        CM_LOG_ERR(CM_MOD_CNM,"enid dosen't exit");
        CM_FREE(data);
        return CM_ERR_NOT_EXISTS;
    }
    if(CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_TOPO_TABLE_SN))
    {
        strcpy(data->sn,info->sn);
    }
    if(CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_TOPO_TABLE_NAME))
    {
        strcpy(data->name,info->name);
    }
    if(CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_TOPO_TABLE_SET))
    {
        data->set = info->set;
    }
    return cm_sync_request(CM_SYNC_OBJ_TOPO_TABLE,(uint64)data->enid,data,sizeof(cm_cnm_topo_table_info_t));
}    

sint32 cm_cnm_topo_table_get(const void * pDecodeParam,void **ppAckData,uint32 * pAckLen)
{
    const cm_cnm_decode_info_t *decode = pDecodeParam;
    const cm_cnm_topo_table_info_t *req= NULL;
    
    if(decode == NULL)
    {
        CM_LOG_ERR(CM_MOD_CNM,"param null");
        return CM_PARAM_ERR;
    }
    req = (const cm_cnm_topo_table_info_t *)decode->data;
    if(!CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_TOPO_TABLE_ENID))
    {
        CM_LOG_ERR(CM_MOD_CNM,"enid id null");
        return CM_PARAM_ERR;
    }
    return cm_cnm_topo_table_sync_get(req->enid,ppAckData,pAckLen);
}

sint32 cm_cnm_topo_table_getbatch(const void * pDecodeParam,void **ppAckData,uint32 * pAckLen)
{
    const cm_cnm_decode_info_t *dec = pDecodeParam;
    cm_cnm_topo_table_info_t *info = NULL;
    uint32 offset = 0;
    uint32 total = CM_CNM_MAX_RECORD;
    if(dec != NULL && dec->offset != 0)
    {
        offset = (uint32)dec->offset;
    }
    if(dec != NULL && dec->total != 0)
    {
        total = dec->total;
    }
    info = CM_MALLOC(sizeof(cm_cnm_topo_table_info_t)*total);
    if(NULL == info)
    {
        CM_LOG_ERR(CM_MOD_CNM,"malloc fail");
        return CM_FAIL;
    }
    total = cm_db_exec_get(topo_db_handle,cm_cnm_topo_table_local_get_each, info,
                               total, sizeof(cm_cnm_topo_table_info_t),
                               "SELECT * FROM topo_t ORDER BY name,setx LIMIT %u,%u",offset,total);
    if(total == 0)
    {
        CM_LOG_ERR(CM_MOD_CNM,"topo_table getbatch fail");
        CM_FREE(info);
        return CM_OK;
    }
    *ppAckData = info;
    *pAckLen = total * sizeof(cm_cnm_topo_table_info_t);
    return CM_OK;
}

sint32 cm_cnm_topo_table_delete(const void * pDecodeParam,void **ppAckData,uint32 * pAckLen)
{
    sint32 iRet = CM_OK;
    uint64 count = 0;
    const cm_cnm_decode_info_t *decode = pDecodeParam;
    const cm_cnm_topo_table_info_t *info = NULL;
    
    if(decode==NULL)
    {
        return CM_PARAM_ERR;
    }
    info = (const cm_cnm_topo_table_info_t*)decode->data;
    iRet = cm_db_exec_get_count(topo_db_handle, &count,
            "SELECT COUNT(enid) FROM topo_t WHERE enid=%u",info->enid);                                 
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM, "get topo_t count fail[%d]", iRet);
        return CM_FAIL;
    }
    if(count == 1)
    {
        return cm_sync_delete(CM_SYNC_OBJ_TOPO_TABLE,info->enid);
    }
    return CM_ERR_NOT_EXISTS;
}

sint32  cm_cnm_topo_table_off(void *pDecodeParam,void **ppAckData,uint32 * pAckLen)
{
    const cm_cnm_decode_info_t *decode = pDecodeParam;
    const cm_cnm_topo_table_info_t *info = NULL;
    
    if(decode==NULL)
    {
        return CM_PARAM_ERR;
    }
    info = (const cm_cnm_topo_table_info_t*)decode->data;

    return cm_system("/var/cm/script/cm_topo.sh table_off %u",info->enid);
}

static sint32 cm_cnm_topo_table_decode_check_type(void *val)
{
    uint8 type = *((uint8*)val);
    const sint8* str = cm_cnm_get_enum_str(&CmOmiMapEnumDevType,type);
    if(NULL == str)
    {
        CM_LOG_ERR(CM_MOD_CNM,"type[%u]",type);
        return CM_PARAM_ERR;
    }
    return CM_OK;
}
static sint32 cm_cnm_topo_table_decode_ext(
    const cm_omi_obj_t ObjParam,void* data,cm_omi_field_flag_t *set)
{
    sint32 iRet = CM_OK;
    cm_cnm_topo_table_info_t *info = data;
    cm_cnm_decode_param_t param_str[] = 
    {
        {CM_OMI_FIELD_TOPO_TABLE_NAME,sizeof(info->name),info->name,NULL},
        {CM_OMI_FIELD_TOPO_TABLE_SN,sizeof(info->sn),info->sn,NULL},
    };
    cm_cnm_decode_param_t param_num[] = 
    {
        {CM_OMI_FIELD_TOPO_TABLE_ENID,sizeof(info->enid),&info->enid,NULL},
        {CM_OMI_FIELD_TOPO_TABLE_SET,sizeof(info->set),&info->set,NULL},
        {CM_OMI_FIELD_TOPO_TABLE_TYPE,sizeof(info->type),&info->type,cm_cnm_topo_table_decode_check_type},
        {CM_OMI_FIELD_TOPO_TABLE_UNUM,sizeof(info->U_num),&info->U_num,NULL},
        {CM_OMI_FIELD_TOPO_TABLE_SLOT,sizeof(info->slot_num),&info->slot_num,NULL},
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
sint32 cm_cnm_topo_table_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam)
{
    return cm_cnm_decode_comm(ObjParam,sizeof(cm_cnm_topo_table_info_t),
        cm_cnm_topo_table_decode_ext,ppDecodeParam);
}
static void cm_cnm_topo_table_encode_each(cm_omi_obj_t item,void *eachdata,void *arg)
{
    cm_omi_field_flag_t *field = arg;
    cm_cnm_topo_table_info_t *info = eachdata;
    cm_cnm_map_value_str_t cols_str[] = 
    {
        {CM_OMI_FIELD_TOPO_TABLE_NAME,    info->name},
        {CM_OMI_FIELD_TOPO_TABLE_SN,    info->sn},
    };
    cm_cnm_map_value_num_t cols_num[] = 
    {
        {CM_OMI_FIELD_TOPO_TABLE_ENID,    info->enid},
        {CM_OMI_FIELD_TOPO_TABLE_SET,     info->set},
        {CM_OMI_FIELD_TOPO_TABLE_TYPE,    info->type},
        {CM_OMI_FIELD_TOPO_TABLE_UNUM,    info->U_num},
        {CM_OMI_FIELD_TOPO_TABLE_SLOT,    info->slot_num},
    };
    cm_cnm_encode_str(item,field,cols_str,sizeof(cols_str)/sizeof(cm_cnm_map_value_str_t));
    cm_cnm_encode_num(item,field,cols_num,sizeof(cols_num)/sizeof(cm_cnm_map_value_num_t)); 
    return;
}
cm_omi_obj_t cm_cnm_topo_table_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen)
{
    return cm_cnm_encode_comm_ext(pDecodeParam,pAckData,AckLen,
        sizeof(cm_cnm_topo_table_info_t),cm_cnm_topo_table_encode_each);
}

sint32 cm_cnm_topo_table_sync_request(uint64 enid, void *pdata, uint32 len)
{
    sint32 iRet = CM_OK;
    uint64 count = 0;
    cm_cnm_topo_table_info_t *info = (cm_cnm_topo_table_info_t*)pdata;
    
    iRet = cm_db_exec_get_count(topo_db_handle, &count,
                                "SELECT COUNT(enid) FROM topo_t WHERE enid=%u",(uint32)enid);                                 
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM, "get enid count fail[%d]", iRet);
        return CM_FAIL;
    }
    /*****update*********/
    if(count == 1)
    {
        iRet = cm_db_exec_ext(topo_db_handle,
        "UPDATE topo_t SET setx=%u, name='%s', sn='%s' WHERE enid=%u",
            info->set,info->name,info->sn,(uint32)enid);
        if(CM_OK != iRet)
        {
            CM_LOG_ERR(CM_MOD_CNM,"update topo table fail[%d]",iRet);
            return iRet;
        }   
    }
    /*********get***********/
    if(count == 0)
    {
        iRet = cm_db_exec_ext(topo_db_handle,
            "INSERT INTO topo_t (enid,name,setx,typex,Unum,slot,sn)"
            "VALUES(%u,'%s',%u,%u,%u,%u,'%s')",(uint32)enid,info->name,info->set,info->type,info->U_num,info->slot_num,info->sn);
        if(CM_OK != iRet)
        {
            CM_LOG_ERR(CM_MOD_CNM,"insert to topo table fail[%d]",iRet);
            return iRet;
        }   
    }
    return CM_OK;
}

sint32 cm_cnm_topo_table_sync_get(uint64 enid, void **pdata, uint32 *plen)
{
    cm_cnm_topo_table_info_t *info = CM_MALLOC(sizeof(cm_cnm_topo_table_info_t));
    uint32 cnt = 0;
    if(info == NULL)
    {
        CM_LOG_ERR(CM_MOD_CNM,"malloc fail");
        return CM_FAIL;
    }
    CM_MEM_ZERO(info,sizeof(cm_cnm_topo_table_info_t));
    cnt = cm_db_exec_get(topo_db_handle,cm_cnm_topo_table_local_get_each,info,1,
        sizeof(cm_cnm_topo_table_info_t),"SELECT * FROM topo_t WHERE enid=%u",(uint32)enid);
    if(cnt == 0)
    {
        CM_LOG_ERR(CM_MOD_CNM,"enid dosen't exit");
        CM_FREE(info);
        return CM_ERR_NOT_EXISTS;
    }
    *pdata = info;
    *plen = sizeof(cm_cnm_topo_table_info_t);
    return CM_OK;
}

sint32 cm_cnm_topo_table_sync_delete(uint64 enid)
{   
    sint32 iRet;
    iRet = cm_db_exec_ext(topo_db_handle,
		"DELETE FROM topo_t WHERE enid=%u",(uint32)enid);
    return iRet;
}
sint32 cm_cnm_topo_table_count(const void * pDecodeParam,void **ppAckData,uint32 *pAckLen)
{
    uint64 cnt = 0;
    sint32 iRet = CM_OK;
    const cm_cnm_decode_info_t *decode = pDecodeParam;
    const cm_cnm_topo_table_info_t *info = NULL;
    info = (const cm_cnm_topo_table_info_t *)decode->data;
    if(decode != NULL
       &&CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_TOPO_TABLE_NAME))
    {
        iRet = cm_db_exec_get_count(topo_db_handle, &cnt,
            "SELECT COUNT(name) FROM topo_t WHERE name='%s'",info->name);                                 
        if(CM_OK != iRet)
        {
            CM_LOG_ERR(CM_MOD_CNM, "get count fail[%d]", iRet);
            return CM_FAIL;
        }
        return cm_cnm_ack_uint64(cnt,ppAckData,pAckLen);
    }
    iRet = cm_db_exec_get_count(topo_db_handle,&cnt,"SELECT COUNT(*) FROM topo_t");
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM, "get count fail[%d]", iRet);
        return CM_FAIL;
    }
    return cm_cnm_ack_uint64(cnt,ppAckData,pAckLen);
}

sint32 cm_cnm_topo_table_checkadd(cm_cnm_cabinet_info_t *data)
{
    uint64 cnt = 0;
    sint32 iRet = CM_OK;
    cm_cnm_topo_table_info_t info;
    iRet = cm_db_exec_get_count(topo_db_handle,&cnt,
            "SELECT COUNT(*) FROM topo_t WHERE enid=%u", data->enid);
    if((CM_OK != iRet) || (cnt > 0))
    {
        return CM_OK;
    }
    info.enid = data->enid;
    info.type = data->type;
    info.set = 0;
    info.slot_num = data->slot_num;
    info.U_num = data->U_num;
    CM_VSPRINTF(info.name,sizeof(info.name),"NONE");
    CM_VSPRINTF(info.sn,sizeof(info.sn),"unknown");
    return cm_sync_request(CM_SYNC_OBJ_TOPO_TABLE,(uint64)info.enid,&info,sizeof(cm_cnm_topo_table_info_t));
}


