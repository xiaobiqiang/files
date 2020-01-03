/******************************************************************************
 *          Copyright (c) 2018 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cnm_disk.c
 * author     : wbn
 * create date: 2018.04.25
 * description: TODO:
 *
 *****************************************************************************/

#include "cm_cnm_common.h"
#include "cm_log.h"
#include "cm_omi.h"
#include "cm_db.h"
#include "cm_node.h"
#include "cm_cnm_disk.h"
#include "cm_cnm_pool.h"

extern const cm_omi_map_enum_t CmOmiMapEnumDiskStatusType;
extern const cm_omi_map_enum_t CmOmiMapEnumDiskLedType;

const sint8* g_cm_disk_fields[] = {
    "id as f0",
    NULL, /* f1 NID不记录数据库 */
    "sn as f2", 
    "vendor as f3",
    "gsize as f4, dim as f100",
    "rpm as f5",
    "enid as f6",
    "slotid as f7",
    "status as f8",
    NULL, /* f9 POOL不记录数据库 */
    "led as f10",
};

#define CM_CNM_DISK_LED_STATE_NORMAL 0
#define CM_CNM_DISK_LED_STATE_FAULT 1
#define CM_CNM_DISK_LED_STATE_LOCATE 2
#define CM_CNM_DISK_LED_STATE_UNKNOWN 3

extern uint32 cm_cnm_pool_getbydisk(const sint8* disk, sint8* pool, uint32 len);
extern void* cm_cnm_disk_update_thread(void* arg);

sint32 cm_cnm_disk_get_sn(const sint8* id,sint8* sn, uint32 len)
{
    sint32 iRet = cm_exec_tmout(sn,len,2,"sqlite3 "CM_CNM_DISK_FILE
        " \"SELECT sn FROM record_t WHERE id='%s' LIMIT 1\"",id);
    if(CM_OK != iRet)
    {
        return iRet;
    }
    iRet = strlen(sn);    
    if(0 == iRet)
    {
        return CM_ERR_NOT_EXISTS;
    }
    iRet--;
    if('\n' == sn[iRet])
    {
        sn[iRet] = '\0';
    }
    return CM_OK;
}

sint32 cm_cnm_disk_get_id(const sint8* sn,sint8* id, uint32 len)
{
    sint32 iRet = cm_exec_tmout(id,len,2,"sqlite3 "CM_CNM_DISK_FILE
        " \"SELECT id FROM record_t WHERE sn='%s' LIMIT 1\"",sn);
    if(CM_OK != iRet)
    {
        return iRet;
    }

    iRet = strlen(id);    
    if(0 == iRet)
    {
        return CM_ERR_NOT_EXISTS;
    }
    iRet--;
    if('\n' == id[iRet])
    {
        id[iRet] = '\0';
    }
    return CM_OK;
}


void cm_cnm_disk_set_busy(sint8 *diskids)
{
    bool_t first=CM_TRUE;
    sint8 cmd[CM_STRING_1K] = {0};
    sint32 buff[CM_STRING_128] = {0};
    sint8* disk = strtok(diskids,",");

    CM_VSPRINTF(cmd,sizeof(cmd),"sqlite3 "CM_CNM_DISK_FILE" \"UPDATE record_t SET status='busy' WHERE id IN(");
    while(NULL != disk)
    {
        if(CM_FALSE == first)
        {
            CM_SNPRINTF_ADD(cmd,sizeof(cmd),",'%s'",disk);
        }
        else
        {
            first=CM_FALSE;
            CM_SNPRINTF_ADD(cmd,sizeof(cmd),"'%s'",disk);
        }
        disk = strtok(NULL,",");
    }
    CM_SNPRINTF_ADD(cmd,sizeof(cmd),")\"");
    (void)cm_cnm_exec_remote(CM_NODE_ID_NONE,CM_FALSE,buff,sizeof(buff),cmd);
    return;
}

uint32 cm_cnm_disk_enid_exchange(uint32 enid)
{
    uint32 sbbid = enid/1000;
    uint32 localid = enid % 1000;
    
    if(0 == sbbid)
    {
        sbbid = cm_node_get_sbbid();
    }

    if(localid <= 22)
    {
        localid = 0;
    }
    else
    {
        localid -= 22;
    }
    return (sbbid*1000 + localid);
}

static uint32 cm_cnm_disk_enid_exchange_tolocal(uint32 enid)
{
    uint32 sbbid = enid / 1000;
    uint32 localid = enid % 1000;

    if(sbbid == cm_node_get_sbbid())
    {
        sbbid = 0;
    }
    localid += 22;
    return (sbbid*1000 + localid);
}

static sint32 cm_cnm_disk_db_cbk(
    void *arg, sint32 col_cnt, sint8 **col_vals, sint8 **col_names)
{
    cm_cnm_disk_info_t *pdisk = arg;
    sint8* gsize = NULL;
    sint8* dim = NULL;

    CM_MEM_ZERO(pdisk,sizeof(cm_cnm_disk_info_t));
    pdisk->nid = cm_node_get_id();
    while(col_cnt > 0)
    {
        col_cnt--;
        switch(atoi((*col_names)+1))
        {
            case CM_OMI_FIELD_DISK_ID:
                CM_MEM_CPY(pdisk->id,sizeof(pdisk->id),*col_vals,strlen(*col_vals)+1);                
                break;
            case CM_OMI_FIELD_DISK_SN:
                CM_MEM_CPY(pdisk->sn,sizeof(pdisk->sn),*col_vals,strlen(*col_vals)+1);
                break;
            case CM_OMI_FIELD_DISK_VENDOR:
                CM_MEM_CPY(pdisk->vendor,sizeof(pdisk->vendor),*col_vals,strlen(*col_vals)+1);
                break;
            case CM_OMI_FIELD_DISK_SIZE:
                gsize = *col_vals;
                break;
            case CM_OMI_FIELD_DISK_RPM:
                pdisk->rpm = (uint32)atoi(*col_vals);
                break;
            case CM_OMI_FIELD_DISK_ENID:
                pdisk->enid = cm_cnm_disk_enid_exchange((uint32)atoi(*col_vals));
                break;
            case CM_OMI_FIELD_DISK_SLOT:
                pdisk->slotid = (uint32)atoi(*col_vals);
                break;
            case CM_OMI_FIELD_DISK_STATUS:
                pdisk->status = cm_cnm_get_enum(&CmOmiMapEnumDiskStatusType,*col_vals,0);
                break;
            case 100:
                dim = *col_vals;
                break;
            default:
                break;
        }
        col_names++;
        col_vals++;
    }
    
    if(NULL != gsize)
    {
        CM_VSPRINTF(pdisk->size,sizeof(pdisk->size),"%s%s",gsize,dim);
    }    
    return CM_OK;
}
sint32 cm_cnm_disk_init(void)
{
    sint32 iRet = CM_FAIL;
    cm_db_handle_t handle = NULL;    
    const sint8* initdb="CREATE TABLE IF NOT EXISTS record_t ("
            "id VARCHAR(64),"
            "sn VARCHAR(64),"
            "vendor VARCHAR(64),"
            "nblocks VARCHAR(64),"
            "blksize VARCHAR(64),"
            "gsize DOUBLE,"
            "dim VARCHAR(64),"
            "enid INT,"
            "slotid INT,"
            "rpm INT,"
            "status VARCHAR(64),"
            "led VARCHAR(32))";
    iRet = cm_db_open_ext(CM_CNM_DISK_FILE,&handle);
    if(CM_OK != iRet)
    {
        return iRet;
    }
    iRet = cm_db_exec_ext(handle,initdb);
    cm_db_close(handle);
    return CM_OK;
}

sint32 cm_cnm_disk_decode(const cm_omi_obj_t ObjParam, void
**ppDecodeParam)
{
    cm_cnm_disk_req_param_t *pParam = CM_MALLOC(sizeof(cm_cnm_disk_req_param_t));
    cm_cnm_disk_info_t *pdisk = NULL;
    if(NULL == pParam)
    {
        CM_LOG_ERR(CM_MOD_CNM,"malloc fail");
        return CM_FAIL;
    }
    pdisk = &pParam->disk;
    CM_MEM_ZERO(pParam,sizeof(cm_cnm_disk_req_param_t));

    pParam->total = CM_CNM_MAX_RECORD;
    cm_omi_decode_fields_flag(ObjParam,&pParam->field);
    if(CM_OK == cm_omi_obj_key_get_str_ex(ObjParam,CM_OMI_FIELD_DISK_ID,pdisk->id,sizeof(pdisk->id)))
    {
        CM_OMI_FIELDS_FLAG_SET(&pParam->set,CM_OMI_FIELD_DISK_ID);
    }
    if(CM_OK == cm_omi_obj_key_get_u32_ex(ObjParam,CM_OMI_FIELD_DISK_NID,&pdisk->nid))
    {
        CM_OMI_FIELDS_FLAG_SET(&pParam->set,CM_OMI_FIELD_DISK_NID);
    }
    if(CM_OK == cm_omi_obj_key_get_u32_ex(ObjParam,CM_OMI_FIELD_DISK_RPM,&pdisk->rpm))
    {
        CM_OMI_FIELDS_FLAG_SET(&pParam->set,CM_OMI_FIELD_DISK_RPM);
    }
    if(CM_OK == cm_omi_obj_key_get_u32_ex(ObjParam,CM_OMI_FIELD_DISK_ENID,&pdisk->enid))
    {
        CM_OMI_FIELDS_FLAG_SET(&pParam->set,CM_OMI_FIELD_DISK_ENID);
    }
    if(CM_OK == cm_omi_obj_key_get_u32_ex(ObjParam,CM_OMI_FIELD_DISK_STATUS,&pdisk->status))
    {
        CM_OMI_FIELDS_FLAG_SET(&pParam->set,CM_OMI_FIELD_DISK_STATUS);
    }
    if(CM_OK == cm_omi_obj_key_get_u32_ex(ObjParam,CM_OMI_FIELD_DISK_ISLOCAL,&pdisk->islocal))
    {
        CM_OMI_FIELDS_FLAG_SET(&pParam->set,CM_OMI_FIELD_DISK_ISLOCAL);
    }
    (void)cm_omi_obj_key_get_u32_ex(ObjParam,CM_OMI_FIELD_FROM,(uint32*)&pParam->offset);
    (void)cm_omi_obj_key_get_u32_ex(ObjParam,CM_OMI_FIELD_TOTAL,(uint32*)&pParam->total);

    pParam->total = CM_MIN(CM_CNM_MAX_RECORD,pParam->total);
    *ppDecodeParam = pParam;
    return CM_OK;
}

static uint32 cm_cnm_disk_get_led_state(uint32 enid, const sint8* diskid)
{
    uint32 sbbid = enid/1000;
    sint8 buff[CM_STRING_64] = {0};
    sint32 iRet = CM_OK;
    uint32 nids[2] = {0};
    uint32 states[2] = {CM_CNM_DISK_LED_STATE_NORMAL,CM_CNM_DISK_LED_STATE_NORMAL};
    uint32 iloop = 2;
    if(0 == sbbid)
    {
        CM_LOG_INFO(CM_MOD_CNM,"enid[%u]",enid);
        return CM_CNM_DISK_LED_STATE_UNKNOWN;/* 和CmOmiMapDiskLedType统一 */
    }

    iRet = cm_exec_tmout(buff,sizeof(buff),2,
        "sqlite3 /var/cm/data/cm_node.db "
        "'select id from record_t where sbbid=%u limit 2'",sbbid);
    if((CM_OK != iRet) || ('\0' == buff[0]))
    {
        CM_LOG_INFO(CM_MOD_CNM,"iRet[%d] buff:%s",iRet,buff);
        return CM_CNM_DISK_LED_STATE_UNKNOWN;
    }
    
    iRet = sscanf(buff,"%u\n%u",&nids[0],&nids[1]);
    if(0 == iRet)
    {
        return CM_CNM_DISK_LED_STATE_UNKNOWN;
    }
    
    for(iloop=0;iloop<2;iloop++)
    {
        if(CM_NODE_ID_NONE == nids[iloop])
        {
            continue;
        }
        buff[0] = '\0';
        iRet = cm_cnm_exec_remote(nids[iloop],CM_TRUE,buff,sizeof(buff),
            "sqlite3 "CM_CNM_DISK_FILE" \"SELECT led FROM record_t WHERE id='%s'\"",diskid);
        if('\0' == buff[0])
        {
            continue;
        }
        buff[strlen(buff)-1] = '\0';
        states[iloop] = cm_cnm_get_enum(&CmOmiMapEnumDiskLedType,buff,CM_CNM_DISK_LED_STATE_UNKNOWN);
    }
    
    CM_LOG_INFO(CM_MOD_CNM,"diskid[%s] %u=%u %u=%u",diskid,nids[0],states[0],nids[1],states[1]);
    
    if((CM_CNM_DISK_LED_STATE_NORMAL == states[0])
        && (CM_CNM_DISK_LED_STATE_NORMAL == states[1]))
    {
        return CM_CNM_DISK_LED_STATE_NORMAL;
    }

    if((CM_CNM_DISK_LED_STATE_FAULT == states[0])
        || (CM_CNM_DISK_LED_STATE_FAULT == states[1]))
    {
        return CM_CNM_DISK_LED_STATE_FAULT;
    }

    if((CM_CNM_DISK_LED_STATE_LOCATE == states[0])
        || (CM_CNM_DISK_LED_STATE_LOCATE == states[1]))
    {
        return CM_CNM_DISK_LED_STATE_LOCATE;
    }
    
    CM_LOG_INFO(CM_MOD_CNM,"state0[%u] state1[%u]",states[0],states[1]);
    return CM_CNM_DISK_LED_STATE_UNKNOWN;
}

cm_omi_obj_t cm_cnm_disk_encode(const void *pDecodeParam, void *pAckData, uint32 AckLen)
{   
    cm_omi_field_flag_t field;
    const cm_omi_field_flag_t *pfield = NULL;
    const cm_cnm_disk_req_param_t *pParam = pDecodeParam;
    cm_cnm_disk_info_t *pData = pAckData;
    cm_omi_obj_t items = NULL;
    cm_omi_obj_t item = NULL;
    uint32 nid = 0;
    
    if(NULL == pAckData)
    {
        CM_LOG_ERR(CM_MOD_CNM,"pAckData null");
        return NULL;
    }    
    if(NULL == pParam)
    {
        CM_OMI_FIELDS_FLAG_SET_ALL(&field);
        pfield = &field;
    }
    else
    {
        pfield = &pParam->field;
    }
    items = cm_omi_obj_new_array();
    if(NULL == items)
    {
        CM_LOG_ERR(CM_MOD_CNM,"new items fail");
        return NULL;
    }
    AckLen = AckLen/sizeof(cm_cnm_disk_info_t);
    while(AckLen > 0)
    {
        AckLen--;
        item = cm_omi_obj_new();
        if(NULL == item)
        {
            CM_LOG_ERR(CM_MOD_CNM,"new item fail");
            return items;
        }
        nid = pData->nid;
        
        if(CM_OMI_FIELDS_FLAG_ISSET(pfield,CM_OMI_FIELD_DISK_ID))
        {
            (void)cm_omi_obj_key_set_str_ex(item,CM_OMI_FIELD_DISK_ID,pData->id);
        }        
        if(CM_OMI_FIELDS_FLAG_ISSET(pfield,CM_OMI_FIELD_DISK_SN))
        {
            (void)cm_omi_obj_key_set_str_ex(item,CM_OMI_FIELD_DISK_SN,pData->sn);
        }
        if(CM_OMI_FIELDS_FLAG_ISSET(pfield,CM_OMI_FIELD_DISK_VENDOR))
        {
            (void)cm_omi_obj_key_set_str_ex(item,CM_OMI_FIELD_DISK_VENDOR,pData->vendor);
        }
        if(CM_OMI_FIELDS_FLAG_ISSET(pfield,CM_OMI_FIELD_DISK_SIZE))
        {
            (void)cm_omi_obj_key_set_str_ex(item,CM_OMI_FIELD_DISK_SIZE,pData->size);
        }
        if(CM_OMI_FIELDS_FLAG_ISSET(pfield,CM_OMI_FIELD_DISK_RPM))
        {
            (void)cm_omi_obj_key_set_u32_ex(item,CM_OMI_FIELD_DISK_RPM,pData->rpm);
        }
        if(CM_OMI_FIELDS_FLAG_ISSET(pfield,CM_OMI_FIELD_DISK_ENID))
        {
            (void)cm_omi_obj_key_set_u32_ex(item,CM_OMI_FIELD_DISK_ENID,pData->enid);
        }
        if(CM_OMI_FIELDS_FLAG_ISSET(pfield,CM_OMI_FIELD_DISK_SLOT))
        {
            (void)cm_omi_obj_key_set_u32_ex(item,CM_OMI_FIELD_DISK_SLOT,pData->slotid);
        }
        if(CM_OMI_FIELDS_FLAG_ISSET(pfield,CM_OMI_FIELD_DISK_STATUS))
        {
            (void)cm_omi_obj_key_set_u32_ex(item,CM_OMI_FIELD_DISK_STATUS,pData->status);
        }
        if(CM_OMI_FIELDS_FLAG_ISSET(pfield,CM_OMI_FIELD_DISK_POOL))
        {
            if((2 == pData->status) && ('\0' != pData->sn[0])) /* busy */
            {
                nid = cm_cnm_pool_getbydisk(pData->sn,pData->pool,sizeof(pData->pool));
                if(CM_NODE_ID_NONE == nid)
                {
                    nid = pData->nid;
                }
            }          
            
            (void)cm_omi_obj_key_set_str_ex(item,CM_OMI_FIELD_DISK_POOL,pData->pool);
        }
        if(CM_OMI_FIELDS_FLAG_ISSET(pfield,CM_OMI_FIELD_DISK_NID))
        {
            (void)cm_omi_obj_key_set_u32_ex(item,CM_OMI_FIELD_DISK_NID,nid);
        }
        if(CM_OMI_FIELDS_FLAG_ISSET(pfield,CM_OMI_FIELD_DISK_LED))
        {
            (void)cm_omi_obj_key_set_u32_ex(item,CM_OMI_FIELD_DISK_LED,
                cm_cnm_disk_get_led_state(pData->enid,pData->id));
        }
        if(CM_OMI_FIELDS_FLAG_ISSET(pfield,CM_OMI_FIELD_DISK_ISLOCAL))
        {
            uint32 islocal = (pData->slotid <= 200)? CM_TRUE : CM_FALSE;
            (void)cm_omi_obj_key_set_u32_ex(item,CM_OMI_FIELD_DISK_ISLOCAL,islocal);
        }
        if(CM_OK != cm_omi_obj_array_add(items,item))
        {
            CM_LOG_ERR(CM_MOD_CNM,"add item fail");
            cm_omi_obj_delete(item);
            break;
        }
        pData++;
    }
 
    return items;
}

static void cm_cnm_disk_param_init(const void *in,cm_cnm_disk_req_param_t *out)
{
    if(NULL != in)
    {
        CM_MEM_CPY(out,sizeof(cm_cnm_disk_req_param_t),in,sizeof(cm_cnm_disk_req_param_t));
    }
    else
    {
        CM_MEM_ZERO(out,sizeof(cm_cnm_disk_req_param_t));
        out->total = CM_CNM_MAX_RECORD;
        CM_OMI_FIELDS_FLAG_SET_ALL(&out->field);        
    }
    /* 硬盘部分，各节点都能查询其他的硬盘，不用单独转发 */
    if(CM_NODE_ID_NONE == out->disk.nid)
    {
        out->disk.nid = cm_node_get_id();
    }
    
    return;
}

static sint32 cm_cnm_disk_request(uint32 cmd,const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    cm_cnm_disk_req_param_t decode;
    cm_cnm_req_param_t param;
    
    CM_MEM_ZERO(&param,sizeof(cm_cnm_req_param_t));
    cm_cnm_disk_param_init(pDecodeParam,&decode);

    if((CM_OMI_CMD_GET == cmd) || (CM_OMI_CMD_MODIFY == cmd))
    {
        cm_cnm_disk_info_t *info = &decode.disk;
        if((CM_NODE_ID_NONE == info->nid) || ('\0' == info->id[0]))
        {
            CM_LOG_ERR(CM_MOD_CNM,"nid[%u] id[%s]",info->nid,info->id);
            return CM_PARAM_ERR;
        }
    }
    param.nid = decode.disk.nid;
    param.offset = decode.offset;
    param.total = decode.total;
    param.obj = CM_OMI_OBJECT_DISK;
    param.cmd = cmd;
    param.ppack = ppAckData;
    param.acklen = pAckLen;
    param.param = &decode;
    param.param_len = sizeof(decode);
    
    return cm_cnm_request(&param);
}

sint32 cm_cnm_disk_getbatch(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_cnm_disk_request(CM_OMI_CMD_GET_BATCH,pDecodeParam,ppAckData,pAckLen);
}

sint32 cm_cnm_disk_get(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_cnm_disk_request(CM_OMI_CMD_GET,pDecodeParam,ppAckData,pAckLen);
}

sint32 cm_cnm_disk_count(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_cnm_disk_request(CM_OMI_CMD_COUNT,pDecodeParam,ppAckData,pAckLen);
}

sint32 cm_cnm_disk_clear(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    if(NULL == pDecodeParam)
    {
        return CM_PARAM_ERR;
    }
    return cm_cnm_disk_request(CM_OMI_CMD_MODIFY,pDecodeParam,ppAckData,pAckLen);
}

static void cm_cnm_disk_make_cond(const cm_cnm_disk_req_param_t *query, sint8* sql, uint32 buf_size)
{
    uint32 col_ids[] = {CM_OMI_FIELD_DISK_RPM,CM_OMI_FIELD_DISK_ENID};
    const sint8* col_names[] = {"rpm","enid",};
    uint32 col_vals[] = {query->disk.rpm,query->disk.enid};

    if(CM_OMI_FIELDS_FLAG_ISSET(&query->set,CM_OMI_FIELD_DISK_ENID))
    {
        col_vals[1] = cm_cnm_disk_enid_exchange_tolocal(query->disk.enid);
    }
    cm_omi_make_select_cond(col_ids,col_names,col_vals,sizeof(col_ids)/sizeof(uint32),
        &query->set,sql,buf_size);
    if(CM_OMI_FIELDS_FLAG_ISSET(&query->set,CM_OMI_FIELD_DISK_STATUS))
    {
        const sint8* val = cm_cnm_get_enum_str(&CmOmiMapEnumDiskStatusType,query->disk.status);
        if(NULL == val)
        {
            return;
        }
        if('\0' == sql[0])
        {
            CM_VSPRINTF(sql,buf_size," WHERE status='%s'",val);
        }
        else
        {
            CM_VSPRINTF(sql+strlen(sql),buf_size-strlen(sql)," AND status='%s'",val);
        }
    }
    if(CM_OMI_FIELDS_FLAG_ISSET(&query->set,CM_OMI_FIELD_DISK_ISLOCAL))
    {
        sint8 cond[CM_STRING_64] = {0};

        if(CM_TRUE != query->disk.islocal)
        {
            /* 第三方盘是255,如果有其他的时候再改 */
            CM_VSPRINTF(cond,sizeof(cond),"slotid>200");
        }
        else
        {
            CM_VSPRINTF(cond,sizeof(cond),"slotid<=200");
        }
        if('\0' == sql[0])
        {
            CM_VSPRINTF(sql,buf_size," WHERE %s",cond);
        }
        else
        {
            CM_VSPRINTF(sql+strlen(sql),buf_size-strlen(sql)," AND %s",cond);
        }
    }
    return;
}

#if 0
static sint32 
cm_cnm_disk_local_get_led_state
    (cm_cnm_disk_req_param_t *query, cm_cnm_disk_info_t *pAck, uint32 cnt)
{  
    sint32 iRet;
    sint32 i = 0;
    const cm_omi_field_flag_t *pfield = &query->field;
    if(!CM_OMI_FIELDS_FLAG_ISSET(pfield, CM_OMI_FIELD_DISK_LED))
        return CM_OK;

    for( ; i < cnt; ++i,++pAck)
    {
        iRet = cm_exec_tmout(pAck->led, sizeof(pAck->led), 2, 
            "disk led-state -d /dev/rdsk/%s | awk '{printf $3}'", pAck->id);
        if(iRet != CM_OK)
        {
            CM_LOG_ERR(CM_MOD_CNM, "get disk[%s] led-state fail[%d]", pAck->id, iRet);
            return CM_FAIL;
        }
    }
    return CM_OK;
}
#endif

sint32 cm_cnm_disk_local_get(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{   
    /*sint32 iRet;*/
    cm_cnm_disk_req_param_t *query = param;
    sint8 sql[CM_STRING_512] = {0};
    uint32 cnt = 0;
    cm_omi_field_flag_t field;
    
    cm_cnm_disk_info_t *pAck = CM_MALLOC(sizeof(cm_cnm_disk_info_t));

    if(NULL == pAck)
    {
        CM_LOG_ERR(CM_MOD_CNM,"malloc fail");
        return CM_FAIL;
    }
    CM_OMI_FIELDS_FLAG_SET_ALL(&field);
    
    cm_omi_make_select_field(g_cm_disk_fields,sizeof(g_cm_disk_fields)/sizeof(sint8*),
        &field,sql,sizeof(sql));

    cnt = cm_db_exec_file_get(CM_CNM_DISK_FILE,cm_cnm_disk_db_cbk,pAck,1,
        sizeof(cm_cnm_disk_info_t),
        "%s FROM record_t WHERE id='%s'",sql,query->disk.id);
    if(0 == cnt)
    {
        CM_FREE(pAck);
        CM_LOG_ERR(CM_MOD_CNM,"id[%s] null", query->disk.id);
        return CM_OK;
    }
#if 0
    iRet = cm_cnm_disk_local_get_led_state(query, pAck, 1);
    if(iRet != CM_OK)
    {
        CM_LOG_ERR(CM_MOD_CNM, "get led-state fail");
        CM_FREE(pAck);
        return CM_FAIL;
    }
#endif    
    *ppAck = pAck;
    *pAckLen = sizeof(cm_cnm_disk_info_t);
    return CM_OK;
}

sint32 cm_cnm_disk_local_getbatch(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    /*sint32 iRet;*/
    cm_cnm_disk_req_param_t *query = param;
    sint8 sql[CM_STRING_512] = {0};
    uint32 buf_size = sizeof(sql);
    uint32 cnt = 0;
    cm_omi_field_flag_t field;
    void *pAck = CM_MALLOC(sizeof(cm_cnm_disk_info_t) * total);

    if(NULL == pAck)
    {
        CM_LOG_ERR(CM_MOD_CNM,"malloc fail");
        return CM_FAIL;
    }
    CM_OMI_FIELDS_FLAG_SET_ALL(&field);
    
    cm_omi_make_select_field(g_cm_disk_fields,sizeof(g_cm_disk_fields)/sizeof(sint8*),
        &field,sql,sizeof(sql));
    
    cnt = strlen(sql);
    cnt += CM_VSPRINTF(sql+cnt,buf_size-cnt, " FROM record_t");
    
    cm_cnm_disk_make_cond(query,sql+cnt,buf_size-cnt);
    total = CM_MIN(total,CM_CNM_MAX_RECORD);
    cnt = cm_db_exec_file_get(CM_CNM_DISK_FILE,cm_cnm_disk_db_cbk,pAck,total,
        sizeof(cm_cnm_disk_info_t),
        "%s LIMIT %llu,%u",sql,offset,total);
    CM_LOG_INFO(CM_MOD_CNM,"%s",sql);
    if(0 == cnt)
    {
        CM_FREE(pAck);
        *ppAck = NULL;
        *pAckLen = 0;
    }
    else
    {
#if 0    
        iRet = cm_cnm_disk_local_get_led_state(query, pAck, cnt);
        if(iRet != CM_OK)
        {
            CM_LOG_ERR(CM_MOD_CNM, "get led-state fail");
            CM_FREE(pAck);
            return CM_FAIL;
        }
#endif        
        *ppAck = pAck;
        *pAckLen = sizeof(cm_cnm_disk_info_t) * cnt;
    }
    return CM_OK;
}

sint32 cm_cnm_disk_local_count(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    cm_cnm_disk_req_param_t *query = param;
    sint8 sql[CM_STRING_512] = {0};
    uint32 buf_size = sizeof(sql);
    uint64 cnt = 0;
    
    cnt = CM_VSPRINTF(sql,buf_size, "SELECT COUNT(id) FROM record_t");
    
    cm_cnm_disk_make_cond(query,sql+cnt,buf_size-cnt);
    CM_LOG_INFO(CM_MOD_CNM,"%s",sql);
    (void)cm_db_exec_file_count(CM_CNM_DISK_FILE,&cnt,"%s",sql);
    
    return cm_cnm_ack_uint64(cnt,ppAck,pAckLen);
}

sint32 cm_cnm_disk_local_clear(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    cm_cnm_disk_req_param_t *query = param;
    cm_cnm_disk_info_t *info = &query->disk;
    sint8 buff[CM_STRING_1K] = {0};
    uint32 idlen = strlen(info->id);
    uint8* disk_name = info->id;
    sint32 iRet = CM_OK;
    uint64 uret = cm_cnm_exec_count(CM_NODE_ID_NONE,"cat /tmp/pool.xml |grep %s|wc -l",disk_name);
    
    if(0 == idlen)
    {
        return CM_PARAM_ERR;
    }
    else if(0 != uret)
    {
        return CM_ERR_POOL_DISK_USED;
    }
    else if((30 <= idlen) || (g_cm_sys_ver == CM_SYS_VER_SOLARIS_V7R16))
    {
        iRet = cm_exec_tmout(buff,sizeof(buff),CM_CMT_REQ_TMOUT,
            "echo y|disk initialize -d /dev/rdsk/%s 2>/dev/null",info->id);
    }
    else
    {
        iRet = cm_exec_tmout(buff,sizeof(buff),CM_CMT_REQ_TMOUT,
            "echo y|disk initialize -f -d /dev/rdsk/%s 2>/dev/null",info->id);
    }
    if(CM_OK == iRet)
    {
        (void)cm_system("sqlite3 "CM_CNM_DISK_FILE" \"UPDATE record_t SET "
            "status='free' WHERE id='%s'\"",info->id);
    }
    return iRet;
}

void cm_cnm_disk_oplog_clear(const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{
    uint32 alarmid = (Result == CM_OK) ? CM_ALARM_LOG_DISK_MODIFY_OK : CM_ALARM_LOG_DISK_MODIFY_FAIL;
    const cm_cnm_disk_req_param_t *req = pDecodeParam;
    const uint32 cnt = 2;

    if(NULL == pDecodeParam)
    {   
        cm_cnm_oplog_report(sessionid,alarmid,NULL,cnt,NULL);
    }    
    else    
    {   
        cm_cnm_disk_info_t *info = &req->disk;
        const sint8* nodename = cm_node_get_name(info->nid);
        cm_cnm_oplog_param_t params[2] = {
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_DISK_NID,strlen(nodename),nodename},
            {CM_OMI_DATA_STRING,CM_OMI_FIELD_DISK_ID,strlen(info->id),info->id},
        };
        
        cm_cnm_oplog_report(sessionid,alarmid,params,cnt,&req->set);
        if(CM_OK == Result)
        {
            (void)cm_thread_start(cm_cnm_disk_update_thread,nodename);
        }
    }   
    return;
}


void* cm_cnm_disk_update_thread(void* arg)
{
    static bool_t isrun = CM_FALSE;
    const sint8* nodename = (const sint8*)arg;
    sint8 buff[CM_STRING_256] = {0};
    if(CM_FALSE != isrun)
    {
        return NULL;
    }
    isrun = CM_TRUE;
    CM_LOG_WARNING(CM_MOD_CNM,"fmadm genxml -u");
    (void)cm_cnm_exec_remote(CM_NODE_ID_NONE,CM_FALSE,buff,sizeof(buff),
        "fmadm genxml -u &",nodename);
    isrun = CM_FALSE;
    return NULL;
}

/*============================================================================*/
#define CM_CNM_DISKSPARE_DB_FILE CM_DATA_DIR"cm_diskspare.db"
static cm_db_handle_t g_cm_diskspare_handle = NULL;
extern const cm_omi_map_enum_t CmOmiMapEnumPoolStatusType;

uint64 cm_cnm_diskspare_used_count(cm_cnm_pool_list_param_t *diskparam,cm_cnm_pooldisk_info_t *pDisk)
{
    uint32 type = diskparam->type;
    void *pDiskAck = NULL;
    uint32 DiskAckLen = 0;
    sint32 iRet = CM_OK;
    uint64 cnt = 0;
    
    diskparam->type = CM_POOL_DISK_DATA;
    CM_VSPRINTF(diskparam->disk_ids,sizeof(diskparam->disk_ids),"%s",pDisk->disk);
    iRet = cm_cnm_pooldisk_count(diskparam,&pDiskAck, &DiskAckLen);
    if((CM_OK == iRet) && (NULL != pDiskAck))
    {
        cnt = *((uint64*)pDiskAck);
        CM_FREE(pDiskAck);
    }
    else
    {
        CM_LOG_ERR(CM_MOD_CNM,"disk[%s] iRet[%d]",pDisk->disk,iRet);
    }
    CM_MEM_ZERO(diskparam->disk_ids,sizeof(diskparam->disk_ids));
    diskparam->type = type;
    return cnt;
}

static void cm_cnm_diskspare_thread_pool(
    const cm_cnm_pool_list_t *pPool, cm_db_handle_t handle)
{
    cm_cnm_pool_list_param_t diskparam;
    cm_cnm_pooldisk_info_t *pDisk = NULL;
    void *pDiskAck = NULL;
    uint32 DiskAckLen = 0;
    sint32 iRet = CM_OK;
    uint32 iloop = 0;
    bool_t isused = CM_FALSE;
    uint64 cnt = 0;

    CM_MEM_ZERO(&diskparam,sizeof(diskparam));
    diskparam.total = 20; /*默认一次取这么多*/     

    CM_VSPRINTF(diskparam.name,sizeof(diskparam.name),"%s",pPool->name);
    diskparam.nid = pPool->nid;
    diskparam.type = CM_POOL_DISK_SPARE;
    diskparam.raid = CM_RAID_BUTT;
    while(1)
    {
        pDiskAck = NULL;
        DiskAckLen = 0;        
        iRet = cm_cnm_pooldisk_getbatch(&diskparam,&pDiskAck,&DiskAckLen);
        if(CM_OK != iRet)
        {
            CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]",iRet);
            break;
        }
        if(NULL == pDiskAck)
        {
            break;
        }

        DiskAckLen = DiskAckLen/sizeof(cm_cnm_pooldisk_info_t);
        /* 依次获取热备盘信息 */        
        for(iloop=0,pDisk=pDiskAck;iloop<DiskAckLen;iloop++,pDisk++)
        {       
            isused = CM_FALSE;
            if(pDisk->status == cm_cnm_get_enum(&CmOmiMapEnumPoolStatusType,"INUSE",0))
            {
                if(0 != cm_cnm_diskspare_used_count(&diskparam,pDisk))
                {
                    isused = CM_TRUE;
                }                
            }
            (void)cm_db_exec_ext(handle,"INSERT INTO pool_t (pool,nid,diskid,used) VALUES "
                    "('%s',%u,'%s',%u)",pPool->name,pPool->nid,pDisk->disk,isused);
            cnt = 0;
            (void)cm_db_exec_get_count(handle, &cnt,"SELECT COUNT(diskid) FROM"
                    " disk_t WHERE diskid='%s'",pDisk->disk);
            if(0 == cnt)
            {
                (void)cm_db_exec_ext(handle,"INSERT INTO disk_t (diskid,enid,slotid,status,cnt) VALUES "
                    "('%s',%u,%u,%u,1)",pDisk->disk,pDisk->enid,pDisk->slotid,pDisk->status);
            }
            else
            {
                (void)cm_db_exec_ext(handle,"UPDATE disk_t SET cnt=cnt+1"
                    " WHERE diskid='%s'",pDisk->disk);
            }
        }
        
        CM_FREE(pDiskAck);
        if(DiskAckLen < diskparam.total)
        {
            CM_LOG_INFO(CM_MOD_CNM,"offset[%u] total[%u] get[%u]",
                diskparam.offset,diskparam.total,DiskAckLen);
            break;
        }
        /* 取下一页 */
        diskparam.offset += diskparam.total;
    }
    return;
}

static void* cm_cnm_diskspare_thread(void *arg)
{
    static bool_t isrun = CM_FALSE;
    cm_db_handle_t handle = arg;
    cm_cnm_pool_list_param_t param;
    cm_cnm_pool_list_t *pPool = NULL;
    void *pPoolAck = NULL;
    uint32 PoolAckLen = 0;
    sint32 iRet = CM_OK;
    uint32 iloop = 0;
    
    if((CM_TRUE == isrun) 
#ifndef CM_OMI_LOCAL
        || (cm_node_get_master() != cm_node_get_id())
#endif        
        )
    {
        return NULL;
    }
    isrun = CM_TRUE;
    CM_LOG_INFO(CM_MOD_CNM,"start");
    /*清空记录*/
    cm_db_exec_ext(handle,"DELETE FROM disk_t");
    cm_db_exec_ext(handle,"DELETE FROM pool_t");

    CM_MEM_ZERO(&param,sizeof(param));
    
    param.total = 20; /*默认一次取这么多*/
    
    while(1)
    {
        pPoolAck = NULL;
        PoolAckLen = 0;
        iRet = cm_cnm_pool_getbatch(&param,&pPoolAck,&PoolAckLen);
        if(CM_OK != iRet)
        {
            CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]",iRet);
            break;
        }
        if(NULL == pPoolAck)
        {
            break;
        }
        /* 计算获取的记录数 */
        PoolAckLen = PoolAckLen/sizeof(cm_cnm_pool_list_t);
        CM_LOG_INFO(CM_MOD_CNM,"poolcnt=%u",PoolAckLen);
        /* 依次获取热备盘信息 */        
        for(iloop=0,pPool=pPoolAck;iloop<PoolAckLen;iloop++,pPool++)
        {       
            cm_cnm_diskspare_thread_pool(pPool, handle);
        }
        CM_FREE(pPoolAck);
        if(PoolAckLen < param.total)
        {
            CM_LOG_INFO(CM_MOD_CNM,"offset[%u] total[%u] get[%u]",
                param.offset,param.total,PoolAckLen);
            break;
        }
        /* 取下一页 */
        param.offset += param.total;
    }
    CM_LOG_INFO(CM_MOD_CNM,"end");
    isrun = CM_FALSE;
    return NULL;
}

sint32 cm_cnm_diskspare_init(void)
{
    cm_thread_t handle;
    sint32 iRet = CM_OK;    
    const sint8* table[] = {
        "CREATE TABLE IF NOT EXISTS disk_t ("
            "diskid VARCHAR(64),"
            "enid INT,"
            "slotid INT,"
            "status INT,"
            "cnt INT)",
        "CREATE TABLE IF NOT EXISTS pool_t ("
            "pool VARCHAR(255),"
            "nid INT,"
            "diskid VARCHAR(64),"
            "used INT)",
        };
    uint32 iloop = sizeof(table)/sizeof(const sint8*);
    
    iRet = cm_db_open_ext(CM_CNM_DISKSPARE_DB_FILE,&g_cm_diskspare_handle);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"open %s fail",CM_CNM_DISKSPARE_DB_FILE);
        return iRet;
    }
    while(iloop > 0)
    {
        iloop--;
        iRet = cm_db_exec_ext(g_cm_diskspare_handle,table[iloop]);
        if(CM_OK != iRet)
        {
            CM_LOG_ERR(CM_MOD_CNM,"create table fail[%d]",iRet);
            return iRet;
        }
    }
    iRet = CM_THREAD_CREATE(&handle,cm_cnm_diskspare_thread,g_cm_diskspare_handle);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"create thread fail[%d]",iRet);
        return iRet;
    }
    CM_THREAD_DETACH(handle);
    return CM_OK;
}

static sint32 cm_cnm_diskspare_decode_ext(
    const cm_omi_obj_t ObjParam,void* data,cm_omi_field_flag_t *set)
{
    sint8 *disk = data;
    if(CM_OK == cm_omi_obj_key_get_str_ex(ObjParam,CM_OMI_FIELD_DISKSPARE_DISK,disk,CM_STRING_64))
    {
        CM_OMI_FIELDS_FLAG_SET(set,CM_OMI_FIELD_DISKSPARE_DISK);
    }
    return CM_OK;
}

sint32 cm_cnm_diskspare_decode(const cm_omi_obj_t ObjParam, void **ppDecodeParam)
{
    return cm_cnm_decode_comm(ObjParam,CM_STRING_64,
        cm_cnm_diskspare_decode_ext,ppDecodeParam);
}

cm_omi_obj_t cm_cnm_diskspare_encode(const void *pDecodeParam, void *pAckData, uint32 AckLen)
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

    (void)cm_db_exec(g_cm_diskspare_handle,cm_omi_encode_db_record_each,items,"%s",sql);
    return items;
}

sint32 cm_cnm_diskspare_getbatch(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    const cm_cnm_decode_info_t *decode = pDecodeParam;
    sint8* sql = CM_MALLOC(CM_STRING_512);

    if(NULL == sql)
    {
        CM_LOG_ERR(CM_MOD_CNM,"malloc fail");
        return CM_FAIL;
    }

    if((NULL != decode) 
        && (CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_DISKSPARE_DISK)))
    {
        CM_VSPRINTF(sql,CM_STRING_512,"SELECT pool as f5,nid as f1,"
            "used as f6 FROM pool_t WHERE diskid='%s' "
            "LIMIT %llu,%u", (const sint8*)decode->data,decode->offset,decode->total);
    }
    else
    {
         CM_VSPRINTF(sql,CM_STRING_512,"SELECT diskid as f0,enid as f2,"
            "slotid as f3,status as f4 FROM disk_t WHERE cnt>1");
        if(NULL == decode)
        {
            CM_SNPRINTF_ADD(sql,CM_STRING_512," LIMIT %u",CM_CNM_MAX_RECORD);
        }
        else
        {
            CM_SNPRINTF_ADD(sql,CM_STRING_512," LIMIT %llu,%u",decode->offset,decode->total);
        }
    }
    *ppAckData = sql;
    *pAckLen = CM_STRING_512;
    return CM_OK;
}

sint32 cm_cnm_diskspare_get(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return CM_OK;
}

sint32 cm_cnm_diskspare_count(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    const cm_cnm_decode_info_t *decode = pDecodeParam;
    uint64 cnt = 0;
    
    if((NULL != decode) 
        && (CM_OMI_FIELDS_FLAG_ISSET(&decode->set,CM_OMI_FIELD_DISKSPARE_DISK)))
    {
        (void)cm_db_exec_get_count(g_cm_diskspare_handle,&cnt,
            "SELECT COUNT(pool) FROM pool_t WHERE diskid='%s'",(const sint8*)decode->data);
    }
    else
    {
        (void)cm_db_exec_get_count(g_cm_diskspare_handle,&cnt,
            "SELECT COUNT(diskid) FROM disk_t WHERE cnt>1");
    }
    return cm_cnm_ack_uint64(cnt,ppAckData,pAckLen);
}

sint32 cm_cnm_diskspare_update(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    cm_thread_t handle;
    sint32 iRet = CM_OK;
    
    iRet = CM_THREAD_CREATE(&handle,cm_cnm_diskspare_thread,g_cm_diskspare_handle);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"create thread fail[%d]",iRet);
        return iRet;
    }
    CM_THREAD_DETACH(handle);
    return CM_OK;
}

void cm_cnm_diskspare_oplog_update(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result)
{
    return;
}    

