#include "cm_cnm_expansion_cabinet.h"
#include "cm_sync.h"
#include "cm_log.h"
#include "cm_db.h"
#include "cm_node.h"

static sint32 cm_cnm_cabinet_get_eachnode(cm_node_info_t *pNode, void *parg)
{
    void * pAckData = NULL;
    uint32  AckLen= 0;
    cm_cnm_cabinet_info_t *pinfo = NULL;
    sint32 iRet = CM_OK;
    cm_cnm_decode_info_t* pdecode=parg;  
    uint32 cnt=0;
    pdecode->nid = pNode->id;
    pdecode->offset = 0;
    pdecode->total = CM_CNM_MAX_RECORD;

    while(1)
    {
        AckLen = 0;
        iRet = cm_cnm_request_comm(CM_OMI_OBJECT_TOPO_TABLE,CM_OMI_CMD_GET_BATCH,
            sizeof(cm_cnm_cabinet_info_t),pdecode,&pAckData,&AckLen);
        if((CM_OK != iRet) || (NULL == pAckData))
        {
            break;
        }
        cnt = AckLen/sizeof(cm_cnm_cabinet_info_t);
        CM_LOG_INFO(CM_MOD_CNM,"nid[%u] cnt[%u]",pdecode->nid,cnt);
        for(AckLen=cnt,pinfo = pAckData;
            AckLen > 0; AckLen--,pinfo++)
        {        
            iRet = cm_cnm_topo_table_checkadd(pinfo);
        }
        CM_FREE(pAckData);
        pAckData = 0;
        if(cnt < pdecode->total)
        {
            break;
        }
        pdecode->offset += pdecode->total;
    }
    
    return CM_OK;    
}

void cm_cnm_cabinet_thread(void)
{
    cm_cnm_decode_info_t* pdecode=NULL;    
    sint32 len = 0;
    sint32 iRet = CM_OK;
    static uint32 second_count = 0;
    
    if(cm_node_get_id() != cm_node_get_master())
    {
        return;
    }
    second_count = second_count + 1;
    /*大约30秒刷新一次*/
    if(second_count > 30)
    {
        second_count = 0;
    }
    if(second_count != 1)
    {
        return;
    }
    len = sizeof(cm_cnm_decode_info_t)+sizeof(cm_cnm_cabinet_info_t);
    pdecode = CM_MALLOC(len);
    if(NULL == pdecode)
    {
        return;
    }
    CM_MEM_ZERO(pdecode, len);
    iRet = cm_node_traversal_all(cm_cnm_cabinet_get_eachnode,pdecode, CM_FALSE);
    CM_FREE(pdecode);
    return;
} 

static sint32 cm_cnm_cabinet_local_get_each(void *arg, sint8 **cols, uint32 col_num)
{
    cm_cnm_cabinet_info_t *cabinet_info = arg;
    uint32 sbbid = 0;
    uint32 listid = 0;
    const uint32 col_cur = 5;
    uint32 row = 0;
    if(col_num != col_cur)
    {
        CM_LOG_WARNING(CM_MOD_CNM,"col_num[%u]",col_num);
        return CM_FAIL;
    }
    /*get ret*/    
    sbbid = atoi(cols[0]);
    listid = atoi(cols[1]); 
    cabinet_info->enid = sbbid * 1000 + listid; 
    cabinet_info->type = atoi(cols[2]);
    cabinet_info->U_num = atoi(cols[3]);
    cabinet_info->slot_num = atoi(cols[4]);
    return CM_OK;
}

sint32 cm_cnm_cabinet_local_getbatch(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    sint32 iRet = CM_OK;
    iRet = cm_cnm_exec_get_list("/var/cm/script/cm_cnm_expansion_cabinet.sh",cm_cnm_cabinet_local_get_each,
        (uint32)offset,sizeof(cm_cnm_cabinet_info_t),ppAck,&total);

    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]",iRet);
        return iRet;
    }
    *pAckLen = total * sizeof(cm_cnm_cabinet_info_t);

    return CM_OK;        
}

sint32 cm_cnm_cabinet_local_count(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen)
{
    sint32 cnt;

    cnt = cm_exec_int("/var/cm/script/cm_cnm_expansion_cabinet.sh count");

    return cm_cnm_ack_uint64((uint64)cnt,ppAck,pAckLen);       
}
