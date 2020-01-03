/******************************************************************************
 *          Copyright (c) 2018 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cnm_common.c
 * author     : wbn
 * create date: 2018年3月16日
 * description: TODO:
 *
 *****************************************************************************/
#include "cm_cnm_common.h"
#include "cm_log.h"
#include <stdarg.h>
#include "cm_node.h"
#include "cm_cmt.h"
#include "cm_omi.h"

uint32 cm_cnm_get_enum(const cm_omi_map_enum_t *pEnum, const sint8* 
    pName, uint32 default_val)
{
    uint32 iloop = pEnum->num;
    const cm_omi_map_cfg_t *pCfg = pEnum->value;
    while(iloop> 0)
    {
        if(0 == strcasecmp(pName, pCfg->pname))
        {
            return pCfg->code;
        }
        iloop--;
        pCfg++;
    }
    return default_val;
}

const sint8* cm_cnm_get_enum_str(const cm_omi_map_enum_t *pEnum, uint32 val)
{
    uint32 iloop = pEnum->num;
    const cm_omi_map_cfg_t *pCfg = pEnum->value;
    while(iloop> 0)
    {
        if(pCfg->code == val)
        {
            return pCfg->pname;
        }
        iloop--;
        pCfg++;
    }
    return NULL;
}


#define cm_check_char_1(c) (((c)>='A' && (c)<='Z') \
        || ((c)>='a' && (c)<='z') \
        || ((c)>='0' && (c)<='9') \
        || ((c)=='%') || ((c)=='.') \
        || ((c)=='-') ||((c)=='_') \
        || ((c)=='/'))
#define cm_check_cmd_char(c) (((c)>='A' && (c)<='Z') \
        || ((c)>='a' && (c)<='z') \
        || ((c)>='0' && (c)<='9') \
        || ((c)=='.') || ((c)=='_'))

static void cm_cnm_cmd_to_tmpfile(const sint8 *cmd, sint8 *tmpfile, uint32 len)
{
    len -= CM_VSPRINTF(tmpfile,len,"/tmp/cm_%u_%u_",
        (uint32)cm_get_time(),(uint32)CM_THREAD_MYID());
    tmpfile += strlen(tmpfile);
    len--;
    while(('\0' != *cmd) && (len > 0))
    {
        if('|' == *cmd)
        {
            break;
        }
        
        if(cm_check_cmd_char(*cmd))
        {
            *(tmpfile++) = *(cmd++);
            len--;
        }
        else if(' ' == *cmd)
        {
            *(tmpfile++) = '_';
            len--;
            cmd++;
        }
        else
        {
            cmd++;
        }
    }
    CM_VSPRINTF(tmpfile,len,".tmp");
    return;
}

#if 0
sint32 cm_cnm_exec_get_list_old(const sint8 *cmd, cm_cnm_exec_cbk_col_t cbk, 
    uint32 row_offset, uint32 each_size,void **ppbuff,uint32 *row_cnt)
{
    sint32 iRet = CM_OK;
    uint32 rows = 0;
    uint32 row_end = 0;
    uint32 cols_num = 0;
    sint8 tmpfile[CM_STRING_256] = {0};
    sint8 tmpbuff[CM_STRING_1K] = {0};
    sint8* cols[32] = {NULL};
    sint8 *ptemp = NULL;
    
    if((0 == *row_cnt) || (0 == each_size))
    {
        return CM_FAIL;
    }
    cm_cnm_cmd_to_tmpfile(cmd, tmpfile,sizeof(tmpfile));
    
    iRet = cm_system("%s 2>/dev/null |sed -n '%u,%up' > %s",cmd,row_offset+1, row_offset + *row_cnt, tmpfile);
    if(CM_OK != iRet)
    {
        CM_LOG_INFO(CM_MOD_CNM,"cmd[%s] iRet[%d]",cmd,iRet);
        cm_system("rm -f %s",tmpfile);
        return iRet;
    }

    do
    {
        row_end = (uint32)cm_exec_int("wc -l %s |awk '{print $1}'",tmpfile);
        row_end = CM_MIN(*row_cnt,row_end);
        if(0 == row_end)
        {
            break;
        }
        rows = 0;
        *ppbuff = CM_MALLOC(each_size * row_end);
        if(NULL == *ppbuff)
        {
            CM_LOG_ERR(CM_MOD_CNM,"cmd[%s] malloc fail",cmd);
            iRet = CM_FAIL;
            break;
        }
        if(CM_STRING_8K < (each_size * row_end))
        {
            CM_LOG_WARNING(CM_MOD_CNM,"each_size[%u],row_end[%u],row_cnt[%u]",
                each_size,row_end,*row_cnt);
            CM_LOG_WARNING(CM_MOD_CNM,"cmd[%s]",cmd);  
            cm_system("cp %s %s_bak",tmpfile,tmpfile);
        }
        row_offset = 0;
        ptemp = *ppbuff;
        while(row_offset < row_end)
        {
            row_offset++;
            iRet = cm_exec(tmpbuff,sizeof(tmpbuff),"sed -n %up %s",row_offset,tmpfile);
            if(CM_OK != iRet)
            {
                CM_LOG_ERR(CM_MOD_CNM,"cmd[%s] row_offset[%u]",cmd,rows);
                break;
            }
            cols_num = cm_cnm_get_cols(tmpbuff,cols,sizeof(cols)/sizeof(sint8*));
            if(0 == cols_num)
            {
                //CM_LOG_WARNING(CM_MOD_CNM,"cmd[%s] row_offset[%u] null",cmd,row_offset);
                continue;
            }
            iRet = cbk((void*)ptemp,cols,cols_num);
            if(CM_OK != iRet)
            {
                CM_LOG_INFO(CM_MOD_CNM,"cmd[%s] row_offset[%u] iRet[%d]",cmd,row_offset,iRet);
                continue;
            }
            ptemp += each_size;
            rows++;
        }
        iRet = CM_OK;
    }while(0);

    cm_system("rm -f %s",tmpfile);
    
    if((CM_OK == iRet) && (rows > 0))
    {
        *row_cnt = rows;
    }
    else
    {
        *row_cnt = 0;
        if(NULL != *ppbuff)
        {
            CM_FREE(*ppbuff);
            *ppbuff = NULL;
        }
    }
    
    return iRet;
}

#endif

sint32 cm_cnm_exec_get_row(
    const sint8 **row_names, uint32 row_cnt,
    cm_cnm_exec_cbk_row_t cbk, void *info,
    const sint8 *cmd_format, ...)
{
    sint32 iRet = CM_FAIL;
    uint32 index = 0;
    uint32 tmplen = 0;
    sint8 buf[CM_STRING_512] = {0};
    sint8 tmpfile[CM_STRING_256] = {0};
    
    CM_ARGS_TO_BUF(buf,CM_STRING_512,cmd_format);
    cm_cnm_cmd_to_tmpfile(buf, tmpfile,sizeof(tmpfile));

    iRet = cm_system("%s |"CM_CNM_TRIM_LEFG" > %s",buf,tmpfile);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"cmd[%s] iRet[%d]",buf,iRet);
        return iRet;
    }
    while(row_cnt > 0)
    {
        row_cnt--;
        buf[0] = '\0';
        (void)cm_exec(buf,sizeof(buf),"egrep '^%s' %s |awk -F':' '{print $2}'"
            " |"CM_CNM_TRIM_LEFG,*row_names,tmpfile);
        tmplen = strlen(buf);
        if(tmplen > 0)
        {
            /* 去掉末尾的换行符, 也可以通过{printf $2}去掉，
            但是去掉之后又不能去掉左边的空格 */
            buf[tmplen-1] = '\0';
        }
        iRet = cbk(info,buf,index);
        if(CM_OK != iRet)
        {
            CM_LOG_ERR(CM_MOD_CNM,"col_names[%s] iRet[%d]",*row_names,iRet);
            break;
        }
        index++;
        row_names++;
    }
    cm_system("rm -f %s",tmpfile);
    return iRet;
}

sint32 cm_cnm_exec_get_col(
    cm_cnm_exec_cbk_col_t cbk, void *info,
    const sint8 *cmd_format, ...)
{
    sint32 iRet = CM_OK;
    sint8 cmdbuff[CM_STRING_512] = {0};
    sint8 tmpbuff[CM_STRING_1K] = {0};
    sint8* cols[32] = {NULL};
    uint32 cols_num = 0;
    
    CM_ARGS_TO_BUF(cmdbuff,CM_STRING_512,cmd_format);
    iRet = cm_exec_tmout(tmpbuff,sizeof(tmpbuff),CM_CMT_REQ_TMOUT,"%s 2>/dev/null",cmdbuff);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"cmd[%s] iRet[%d]",cmdbuff,iRet);
        return iRet;
    }
    cols_num = cm_cnm_get_cols(tmpbuff,cols,sizeof(cols)/sizeof(sint8*));
    if(0 == cols_num)
    {
        CM_LOG_ERR(CM_MOD_CNM,"cmd[%s] cols_num[0]",cmdbuff);
        return CM_FAIL;
    }
    return cbk(info,cols,cols_num);
}

/*============================================================================*/
typedef struct
{
    cm_cnm_query_count_cbk_t get_count;
    cm_cnm_query_getbatch_cbk_t get_batch;
    void *param;
    uint32 eachsize; 
    uint64 offset;
    uint32 maxrow;
    void *ack_data;
    uint64 global_offset;
    uint32 global_index;    
}cm_cnm_query_inter_t;

static sint32 cm_cnm_query_each_node(cm_node_info_t *pNode, void *pArg)
{
    cm_cnm_query_inter_t *query = pArg;
    uint32 nid = pNode->id;
    uint64 cnt = 0;
    uint64 offset  = 0;
    sint32 iRet = CM_FAIL;
    void *pAck = NULL;
    uint32 AckLen = 0;
    uint32 maxrow = query->maxrow - query->global_index;
    uint8* pData = query->ack_data;

    if(CM_NODE_STATE_NORMAL != pNode->state)
    {
        CM_LOG_INFO(CM_MOD_CNM,"skip nid[%u] state[%u]",pNode->state);
        return CM_OK;
    }
    
    cnt = query->get_count(nid,query->param);
    CM_LOG_DEBUG(CM_MOD_CNM,"nid[%u] cnt[%llu] index[%u] offset[%llu]", 
        nid,cnt,query->global_index,query->global_offset);
    if(0 == cnt)
    {
        /* 这个节点没有想要的数据，跳过 */
        return CM_OK;
    }
    
    /* 跳过偏移的部分数据 */
    if((query->global_offset + cnt) <= query->offset)
    {
        /* 说明这个节点的数据不需要读取，跳过 */
        query->global_offset += cnt;
        return CM_OK;
    }
    
    /* 说明需要在这个节点取数据*/
    if(query->global_offset < query->offset)
    {
        /* 说明还需要跳过部分数据 */
        offset = query->offset - query->global_offset;
        cnt -= offset; 
    }
    else
    {
        /* 不需要跳过数据，直接读取 */
        offset = 0;
    }
    
    cnt = CM_MIN(cnt, maxrow);
    CM_LOG_DEBUG(CM_MOD_CNM,"nid[%u] offset[%llu] cnt[%u]", nid,offset,cnt);
    iRet = query->get_batch(nid,query->param, offset, cnt,&pAck,&AckLen);
    if(CM_OK != iRet)
    {
        CM_LOG_INFO(CM_MOD_CNM,"node[%u] fail[%d]", nid,iRet);
        /* 这里不返回失败，让继续查询其他节点 */
        return CM_OK;
    }
    
    if(NULL == pAck)
    {
        /* 没有获取到数据，跳过 */
        return CM_OK;
    }
    
    cnt = AckLen/query->eachsize;
    if(0 == cnt)
    {
        CM_FREE(pAck);
        CM_LOG_WARNING(CM_MOD_CNM,"AckLen[%u] eachsize[%u]", AckLen,query->eachsize);
        /* 理论上不应该出现这样的情况 */
        return CM_OK;
    }
    
    /* 计算实际需要取的条数 */
    CM_LOG_DEBUG(CM_MOD_CNM,"cnt[%llu] maxrow[%u] AckLen[%u]", cnt,maxrow,AckLen);
    cnt = CM_MIN(cnt, maxrow);    
    pData += query->global_index * query->eachsize;
    query->global_index += cnt;
    query->global_offset += offset+cnt;
    cnt *= query->eachsize;    
    CM_MEM_CPY(pData,(maxrow * query->eachsize),pAck,cnt);
    CM_FREE(pAck);
    
    if(query->global_index >= query->maxrow)
    {
        /* 这里返回失败只为让遍历函数跳出循环，不再继续查询其他节点 */
        return CM_FAIL;
    }
    return CM_OK;
}

sint32 cm_cnm_query_data(
    cm_cnm_query_count_cbk_t get_count,
    cm_cnm_query_getbatch_cbk_t get_batch,
    void *param, uint32 eachsize,
    uint64 offset, uint32 maxrow,
    void **ppAck, uint32 *pAckLen,
    bool_t sbb_only)
{
    cm_cnm_query_inter_t query = {
        get_count,get_batch,
        param,eachsize,
        offset,maxrow,
        NULL, 0,0
    };
    
    CM_LOG_DEBUG(CM_MOD_CNM,"eachsize[%u] offset[%llu] maxrow[%u]", 
        eachsize,offset,maxrow);
    query.ack_data = CM_MALLOC(maxrow * eachsize);
    if(NULL == query.ack_data)
    {
        CM_LOG_ERR(CM_MOD_CNM,"malloc fail");
        return CM_FAIL;
    }
    if(CM_FALSE == sbb_only)
    {
        (void)cm_node_traversal_all(cm_cnm_query_each_node,&query,CM_TRUE);
    }
    else
    {
        (void)cm_node_traversal_all_sbb(cm_cnm_query_each_node,&query,CM_TRUE);
    }
    
    if(0 != query.global_index)
    {
        *ppAck = query.ack_data;
        *pAckLen = eachsize * query.global_index; 
    }
    else
    {
        CM_FREE(query.ack_data);
    }
    return CM_OK;
}

static sint32 cm_cnm_query_each_count(cm_node_info_t *pNode, void *pArg)
{
    cm_cnm_query_inter_t *query = pArg;

    if(CM_NODE_STATE_NORMAL != pNode->state)
    {
        CM_LOG_INFO(CM_MOD_CNM,"skip nid[%u] state[%u]",pNode->id,pNode->state);
        return CM_OK;
    }
    
    query->global_offset += query->get_count(pNode->id,query->param);
    return CM_OK;
}

uint64 cm_cnm_query_count(
    cm_cnm_query_count_cbk_t get_count,
    void *param,bool_t sbb_only)
{
    cm_cnm_query_inter_t query = {
        get_count,NULL,
        param,0,
        0,0,
        NULL, 0,0
    };
    if(CM_FALSE == sbb_only)
    {
       (void)cm_node_traversal_all(cm_cnm_query_each_count,&query,CM_FALSE); 
    }
    else
    {
       (void)cm_node_traversal_all_sbb(cm_cnm_query_each_count,&query,CM_FALSE); 
    }
    return query.global_offset;
}

typedef struct
{
    uint32 obj;
    uint32 cmd;
    /* 批量查询: 开始偏移位置 */
    uint64 offset;
    
    /* 批量查询: 返回记录总数 */
    uint32 total;
    uint32 param_len;
    uint8 data[];
}cm_cnm_req_inter_t;

extern const cm_cnm_cfg_obj_t* g_cm_cnm_config;
static const cm_cnm_cfg_obj_t* cm_cnm_get_cfg(uint32 obj)
{
    const cm_cnm_cfg_obj_t* cfg = g_cm_cnm_config;

    while(cfg->obj != CM_OMI_OBJECT_DEMO)
    {
        if(cfg->obj == obj)
        {
            return cfg;
        }
        cfg++;
    }
    return NULL;
}
sint32 cm_cnm_cmt_cbk(void *pMsg, uint32 Len, void** ppAck, uint32 *pAckLen)
{
    cm_cnm_req_inter_t *pReq = pMsg;
    const cm_cnm_cfg_obj_t *cfg = NULL;
    const cm_cnm_cfg_cmd_t *cmd = NULL;
    uint32 index = 0;
    void *param = NULL;
    
    if(NULL == pReq)
    {
        CM_LOG_ERR(CM_MOD_CNM,"pMsg null");
        return CM_PARAM_ERR;
    }
    
    if((Len < sizeof(cm_cnm_req_inter_t)) 
        || (Len != (sizeof(cm_cnm_req_inter_t) + pReq->param_len)))
    {
        CM_LOG_ERR(CM_MOD_CNM,"Len[%u]",Len);
        return CM_PARAM_ERR;
    }

    cfg = cm_cnm_get_cfg(pReq->obj);
    if(NULL == cfg)
    {
        CM_LOG_ERR(CM_MOD_CNM,"obj[%u]",pReq->obj);
        return CM_ERR_NOT_SUPPORT;
    }
    
    if(pReq->param_len > 0)
    {
        if(pReq->param_len != cfg->param_size)
        {
            CM_LOG_ERR(CM_MOD_CNM,"obj[%u] reqsize[%u] cfgsize[%u]",
                pReq->obj,pReq->param_len,cfg->param_size);
            return CM_PARAM_ERR;
        }
        param = (void*)pReq->data;
    }
    index = cfg->cmd_num;
    cmd = cfg->cmds;
    while(index > 0)
    {
        if(cmd->cmd == pReq->cmd)
        {
            return cmd->cbk(param,pReq->param_len,pReq->offset,pReq->total,
                ppAck,pAckLen);
        }
        index--;
        cmd++;
    }
    CM_LOG_ERR(CM_MOD_CNM,"obj[%u] cmd[%u]",pReq->obj,pReq->cmd);
    return CM_ERR_NOT_SUPPORT;
}


static sint32 cm_cnm_request_each(cm_node_info_t *pNode, void *pArg)
{
    cm_cnm_req_inter_t *pReq = pArg;
    uint32 len = sizeof(cm_cnm_req_inter_t) + pReq->param_len;
    
    return cm_cmt_request(pNode->id,CM_CMT_MSG_CNM,pReq,len,NULL,NULL,CM_CMT_REQ_TMOUT);
}

static uint64 cm_cnm_remote_count_comm(uint32 nid, void *param)
{
    cm_cnm_req_inter_t *pReq = param;
    uint32 len = sizeof(cm_cnm_req_inter_t) + pReq->param_len;
    sint32 iRet = CM_OK;
    void *pCnt = NULL;
    uint64 cnt = 0;
    
    pReq->cmd = CM_OMI_CMD_COUNT;
    iRet = cm_cmt_request(nid,CM_CMT_MSG_CNM,param,len,&pCnt,&len,CM_CMT_REQ_TMOUT);
    if(CM_OK == iRet)
    {
        cnt = *((uint64*)pCnt);
        CM_FREE(pCnt);
    }
    else
    {
        CM_LOG_ERR(CM_MOD_CNM,"get node[%u] fail",nid);
    }
    return cnt;
}

static sint32 cm_cnm_remote_getbatch_comm(
    uint32 nid, void *param, uint64 offset, uint32 total,void **ppAck, uint32 *pAckLen)
{
    cm_cnm_req_inter_t *pReq = param;
    uint32 len = sizeof(cm_cnm_req_inter_t) + pReq->param_len;

    pReq->cmd = CM_OMI_CMD_GET_BATCH;
    pReq->offset = offset;
    pReq->total = total;
    return cm_cmt_request(nid,CM_CMT_MSG_CNM,pReq,len,ppAck,pAckLen,CM_CMT_REQ_TMOUT);
}

static sint32 cm_cnm_sum_count(cm_cnm_req_inter_t *pReq, void** ppAck, uint32 *pAcklen,bool_t sbb_only)
{
    uint64 cnt = cm_cnm_query_count(cm_cnm_remote_count_comm,pReq,sbb_only);
    
    return cm_cnm_ack_uint64(cnt, ppAck,pAcklen);
}

sint32 cm_cnm_request(cm_cnm_req_param_t *param)
{
    sint32 iRet = CM_OK;
    uint32 len = sizeof(cm_cnm_req_inter_t) + param->param_len;
    cm_cnm_req_inter_t *pReq = CM_MALLOC(len);
    
    if(NULL == pReq)
    {
        CM_LOG_ERR(CM_MOD_CNM,"malloc fail");
        return CM_FAIL;
    }
    pReq->cmd = param->cmd;
    pReq->obj = param->obj;
    pReq->offset = param->offset;
    pReq->total = param->total;
    pReq->param_len = param->param_len;
    if(param->param_len > 0)
    {
        CM_MEM_CPY(pReq->data,pReq->param_len,param->param,param->param_len);
    }
    
    if(CM_NODE_ID_NONE != param->nid)
    {
        iRet = cm_cmt_request(param->nid,CM_CMT_MSG_CNM,pReq,len,param->ppack,param->acklen,CM_CMT_REQ_TMOUT);
    }
    else if(CM_OMI_CMD_GET_BATCH == param->cmd)
    {
        const cm_cnm_cfg_obj_t* cfg = cm_cnm_get_cfg(param->obj);
        if(NULL != cfg)
        {
            iRet = cm_cnm_query_data(cm_cnm_remote_count_comm,cm_cnm_remote_getbatch_comm,
                pReq,cfg->each_size,param->offset,param->total,param->ppack,param->acklen,param->sbb_only);
        }
        else
        {
            CM_LOG_ERR(CM_MOD_CNM,"obj[%u] not config",param->obj);
            iRet = CM_ERR_NOT_SUPPORT;
        }        
    }
    else if(CM_OMI_CMD_COUNT == param->cmd)
    {
        iRet = cm_cnm_sum_count(pReq,param->ppack,param->acklen,param->sbb_only);
    }
    else if(CM_FALSE == param->sbb_only)
    {
        /* 此时不输出返回数据 */        
        iRet = cm_node_traversal_all(cm_cnm_request_each,pReq,param->fail_break);
    }
    else
    {
        iRet = cm_node_traversal_all_sbb(cm_cnm_request_each,pReq,param->fail_break);
    }
    CM_FREE(pReq);
    return iRet;
}

sint32 cm_cnm_ack_uint64(uint64 val, void** ppAck, uint32 *pAckLen)
{
    uint64 *pCnt = CM_MALLOC(sizeof(uint64));
    
    if(NULL == pCnt)
    {
        CM_LOG_ERR(CM_MOD_CNM,"malloc fail");
        return CM_FAIL;
    }
    *pCnt = val;
    
    *ppAck = pCnt;
    *pAckLen = sizeof(uint64);
    return CM_OK;
}

cm_omi_obj_t cm_cnm_encode_comm(void *pAckData, uint32 AckLen, uint32 eachsize,
    cm_cnm_encode_each_cbk_t cbk, void *arg)
{
    uint8 *data = pAckData;
    cm_omi_obj_t items = NULL;
    cm_omi_obj_t item = NULL;
    
    if((NULL == pAckData) 
        || (0 == AckLen)
        || (0 == eachsize)
        || (NULL == cbk))
    {
        return NULL;
    }

    items = cm_omi_obj_new_array();
    if(NULL == items)
    {
        CM_LOG_ERR(CM_MOD_CNM,"new items fail");
        return NULL;
    }
    
    AckLen = AckLen/eachsize;

    while(AckLen > 0)
    {
        AckLen--;
        item = cm_omi_obj_new();
        if(NULL == item)
        {
            CM_LOG_ERR(CM_MOD_CNM,"new item fail");
            return items;
        }
        cbk(item,(void*)data,arg);
        if(CM_OK != cm_omi_obj_array_add(items,item))
        {
            CM_LOG_ERR(CM_MOD_CNM,"add item fail");
            cm_omi_obj_delete(item);
            break;
        }
        data += eachsize;
    }
    return items;
}

sint32 cm_cnm_get_errcode(const cm_omi_map_enum_t *pEnum, const sint8* pErrorStr, sint32 default_val)
{
    uint32 iloop = pEnum->num;
    const cm_omi_map_cfg_t *pCfg = pEnum->value;
    while(iloop> 0)
    {
        if(NULL != strstr(pErrorStr, pCfg->pname))
        {
            return (sint32)pCfg->code;
        }
        iloop--;
        pCfg++;
    }
    return default_val;
}

static sint32 cm_cnm_check_exist_each(cm_node_info_t *pNode, void *pArg)
{
    cm_cnm_query_inter_t *query = pArg;
    uint64 cnt = query->get_count(pNode->id,query->param);
    return (cnt == 0)? CM_OK : CM_FAIL;
}

bool_t cm_cnm_check_exist(cm_cnm_req_param_t *param)
{
    sint32 iRet = CM_OK;
    uint32 len = sizeof(cm_cnm_req_inter_t) + param->param_len;
    cm_cnm_query_inter_t query;
    cm_cnm_req_inter_t *pReq = CM_MALLOC(len);
    
    
    if(NULL == pReq)
    {
        CM_LOG_ERR(CM_MOD_CNM,"malloc fail");
        return CM_FAIL;
    }
    pReq->cmd = CM_OMI_CMD_COUNT;
    pReq->obj = param->obj;
    pReq->offset = param->offset;
    pReq->total = param->total;
    pReq->param_len = param->param_len;
    if(param->param_len > 0)
    {
        CM_MEM_CPY(pReq->data,pReq->param_len,param->param,param->param_len);
    }
    
    CM_MEM_ZERO(&query,sizeof(query));
    query.get_count = cm_cnm_remote_count_comm;
    query.param = pReq;

    iRet = cm_node_traversal_all(cm_cnm_check_exist_each,&query,CM_TRUE);
    CM_FREE(pReq);
    
    return (iRet == CM_OK)? CM_FALSE : CM_TRUE;
}

sint32 cm_cnm_decode_comm(const cm_omi_obj_t ObjParam, uint32 eachsize,
    cm_cnm_decode_cbk_t cbk, void** ppAck)
{
    sint32 iRet = CM_OK;
    uint32 len = sizeof(cm_cnm_decode_info_t)+eachsize;
    cm_cnm_decode_info_t *decode = CM_MALLOC(len);

    if(NULL == decode)
    {
        CM_LOG_ERR(CM_MOD_CNM,"malloc fail");
        return CM_FAIL;
    }
    CM_MEM_ZERO(decode,len);
    cm_omi_decode_fields_flag(ObjParam,&decode->field);
    
    decode->nid = CM_NODE_ID_NONE;
    decode->offset = 0;
    decode->total = CM_CNM_MAX_RECORD;
    /* NID */
    (void)cm_omi_obj_key_get_u32_ex(ObjParam,CM_OMI_FIELD_POOL_NID,&decode->nid);
    (void)cm_omi_obj_key_get_u64_ex(ObjParam,CM_OMI_FIELD_FROM,&decode->offset);
    (void)cm_omi_obj_key_get_u32_ex(ObjParam,CM_OMI_FIELD_TOTAL,&decode->total);
    if(0 == decode->total)
    {
        CM_LOG_WARNING(CM_MOD_CNM,"total 0");
        CM_FREE(decode);
        return CM_PARAM_ERR;
    }
    decode->total = CM_MIN(decode->total,CM_CNM_MAX_RECORD);
    if((0 != eachsize) && (NULL != cbk))
    {
        iRet = cbk(ObjParam,(void*)decode->data,&decode->set);
        if(CM_OK != iRet)
        {
            CM_FREE(decode);
            return iRet;
        }
    }
    *ppAck = decode; 
    return CM_OK;
}

cm_omi_obj_t cm_cnm_encode_comm_ext(const void* pDecodeParam, void *pAckData, uint32 AckLen, 
    uint32 eachsize,cm_cnm_encode_each_cbk_t cbk)
{
    cm_omi_field_flag_t field;
    const cm_cnm_decode_info_t *param = pDecodeParam;
    if(pDecodeParam == NULL)
    {
        CM_OMI_FIELDS_FLAG_SET_ALL(&field);
    }
    else
    {
        CM_MEM_CPY(&field,sizeof(field),&param->field,sizeof(field));
    }
    return cm_cnm_encode_comm(pAckData,AckLen,eachsize,cbk,&field);
}

void cm_cnm_encode_str(cm_omi_obj_t item, const cm_omi_field_flag_t *field,
    const cm_cnm_map_value_str_t *map, uint32 count)
{
    while(count > 0)
    {
        count--;
        if(CM_OMI_FIELDS_FLAG_ISSET(field,map->id))
        {
            (void)cm_omi_obj_key_set_str_ex(item,map->id,map->value);
        }
        map++;
    }
    return;
}

void cm_cnm_encode_num(cm_omi_obj_t item, const cm_omi_field_flag_t *field,
    const cm_cnm_map_value_num_t *map, uint32 count)
{
    while(count > 0)
    {
        count--;
        if(CM_OMI_FIELDS_FLAG_ISSET(field,map->id))
        {
            (void)cm_omi_obj_key_set_u32_ex(item,map->id,map->value);
        }
        map++;
    }
    return;
}

void cm_cnm_encode_double(cm_omi_obj_t item, const cm_omi_field_flag_t *field,
    const cm_cnm_map_value_double_t *map, uint32 count)
{
    while(count > 0)
    {
        count--;
        if(CM_OMI_FIELDS_FLAG_ISSET(field,map->id))
        {
            (void)cm_omi_obj_key_set_double_ex(item,map->id,map->value);
        }
        map++;
    }
    return;
}


sint32 cm_cnm_request_comm(uint32 obj, uint32 cmd,
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
    return cm_cnm_request(&param);
}

void cm_cnm_comma2blank(sint8 *str)
{
    while(*str != '\0')
    {
        if(*str == ',')
        {
            *str = ' ';
        }
        str++;
    }
    return;
}

sint32 cm_cnm_exec_local(void *pMsg, uint32 Len, void** ppAck, uint32 *pAckLen)
{
    sint32 iRet = CM_OK;
    sint8* cmd = pMsg;
    sint8 *pbuff = NULL;
    if((NULL == pMsg) || (0 == Len) || (0 == strlen(cmd)))
    {
        return CM_PARAM_ERR;
    }
    pbuff = CM_MALLOC(CM_STRING_2K);
    if(NULL == pbuff)
    {
        CM_LOG_ERR(CM_MOD_CNM,"malloc fail");
        return CM_FAIL;
    }
    
    CM_MEM_ZERO(pbuff,CM_STRING_2K);
    
    iRet = cm_exec_tmout(pbuff,CM_STRING_2K,CM_CMT_REQ_TMOUT,cmd);
    if((CM_OK != iRet) || (0 == strlen(pbuff)))
    {
        CM_FREE(pbuff);
        return iRet;
    }

    *ppAck = pbuff;
    *pAckLen = strlen(pbuff)+1;
    return CM_OK;
}

typedef struct
{
    sint8 *buff;
    uint32 size;
    const sint8 *cmd;
    uint64 cnt;
}cm_cnm_exec_remote_info_t;

static sint32 cm_cnm_exec_remote_each(uint32 nid, sint8 *buff, uint32 size, const sint8* cmd)
{
    sint32 iRet  = CM_OK;
    void *pAck = NULL;
    uint32 len = 0;

    iRet = cm_cmt_request(nid,CM_CMT_MSG_EXEC,
        (const void *)cmd,(strlen(cmd)+1),&pAck,&len,CM_CMT_REQ_TMOUT);
    if(CM_OK != iRet)
    {
        return iRet;
    }
    if(NULL != pAck)
    {
        CM_SNPRINTF_ADD(buff,size,"%s",(sint8*)pAck);
        CM_FREE(pAck);
    }
    return iRet;
}

static sint32 cm_cnm_exec_remote_each_node(cm_node_info_t *pNode, void *pArg)
{
    cm_cnm_exec_remote_info_t *arg = pArg;

    return cm_cnm_exec_remote_each(pNode->id,arg->buff,arg->size,arg->cmd);
}

sint32 cm_cnm_exec_remote(uint32 nid, bool_t fail_break,sint8 *buff, uint32 size,
    const sint8* format, ...)
{
    sint8 cmdbuff[CM_STRING_1K] = {0};
    cm_cnm_exec_remote_info_t arg = {buff,size,cmdbuff,0};
    CM_ARGS_TO_BUF(cmdbuff,CM_STRING_1K,format);

    buff[0] = '\0';
    if(nid == cm_node_get_id())
    {
        return cm_exec_tmout(buff,size,CM_CMT_REQ_TMOUT,cmdbuff);
    }

    if(CM_NODE_ID_NONE != nid)
    {
        return cm_cnm_exec_remote_each(nid,buff,size,cmdbuff);
    }
    return cm_node_traversal_all(cm_cnm_exec_remote_each_node,&arg,fail_break);
}

static uint64 cm_cnm_exec_count_each(uint32 nid,const sint8* cmd)
{
    sint32 iRet  = CM_OK;
    void *pAck = NULL;
    uint32 len = 0;
    uint64 cnt = 0;

    iRet = cm_cmt_request(nid,CM_CMT_MSG_EXEC,
        (const void *)cmd,(strlen(cmd)+1),&pAck,&len,CM_CMT_REQ_TMOUT);
    if(CM_OK != iRet)
    {
        return cnt;
    }
    if(NULL != pAck)
    {
        cnt = (uint64)atoll((sint8*)pAck);
        CM_FREE(pAck);
    }
    return cnt;
}

static sint32 cm_cnm_exec_count_each_node(cm_node_info_t *pNode, void *pArg)
{
    cm_cnm_exec_remote_info_t *arg = pArg;
    arg->cnt += cm_cnm_exec_count_each(pNode->id,arg->cmd);
    return CM_OK;
}

uint64 cm_cnm_exec_count(uint32 nid,const sint8* format, ...)
{
    sint8 cmdbuff[CM_STRING_1K] = {0};
    cm_cnm_exec_remote_info_t arg = {NULL,0,cmdbuff,0};
    CM_ARGS_TO_BUF(cmdbuff,CM_STRING_1K,format);

    if(nid == cm_node_get_id())
    {
        return (uint64)cm_exec_double(cmdbuff);
    }

    if(CM_NODE_ID_NONE != nid)
    {
        return cm_cnm_exec_count_each(nid,cmdbuff);
    }
    (void)cm_node_traversal_all(cm_cnm_exec_count_each_node,&arg,CM_FALSE);
    return arg.cnt;
}

static sint32 cm_cnm_exec_isexited_each_node(cm_node_info_t *pNode, void *pArg)
{
    cm_cnm_exec_remote_info_t *arg = pArg;
    arg->cnt += cm_cnm_exec_count_each(pNode->id,arg->cmd);
    if(arg->cnt > 0)
    {
        arg->size = pNode->id; /* 复用size记录NID */
        return CM_FAIL; /* 让结束查找 */
    }
    return CM_OK;
}

uint32 cm_cnm_exec_exited_nid(const sint8* format, ...)
{
    sint8 cmdbuff[CM_STRING_1K] = {0};
    cm_cnm_exec_remote_info_t arg = {NULL,0,cmdbuff,0};
    CM_ARGS_TO_BUF(cmdbuff,CM_STRING_1K,format);
    
    (void)cm_node_traversal_all(cm_cnm_exec_isexited_each_node,&arg,CM_TRUE);
    return arg.size; /* 复用size作为NID */
}

sint32 cm_cnm_decode_str(const cm_omi_obj_t ObjParam,
    cm_cnm_decode_param_t *param, uint32 cnt,cm_omi_field_flag_t *set)
{
    sint32 iRet = CM_OK;

    for(;cnt>0; cnt--,param++)
    {
        iRet = cm_omi_obj_key_get_str_ex(ObjParam,param->id,(sint8*)param->val,param->size);
        if(CM_OK != iRet)
        {
            continue;
        }
        CM_OMI_FIELDS_FLAG_SET(set,param->id);
        if(NULL == param->check)
        {
            continue;
        }
        iRet = param->check(param->val);
        if(CM_OK != iRet)
        {
            return iRet;
        }
    }
    return CM_OK;
}

sint32 cm_cnm_decode_num(const cm_omi_obj_t ObjParam,
    cm_cnm_decode_param_t *param, uint32 cnt,cm_omi_field_flag_t *set)
{
    sint32 iRet = CM_OK;
    uint64 val = 0;

    for(;cnt>0; cnt--,param++)
    {
        val = 0;
        iRet = cm_omi_obj_key_get_u64_ex(ObjParam,param->id,&val);
        if(CM_OK != iRet)
        {
            continue;
        }
        CM_OMI_FIELDS_FLAG_SET(set,param->id);

        if(param->size == sizeof(uint16))
        {
            *((uint16*)param->val) = (uint16)val;
        }
        else if(param->size == sizeof(uint32))
        {
            *((uint32*)param->val) = (uint32)val;
        }
        else if(param->size == sizeof(uint64))
        {
            *((uint64*)param->val) = (uint64)val;
        }
        else
        {
            *((uint8*)param->val) = (uint8)val;
        }
        if(NULL == param->check)
        {
            continue;
        }
        iRet = param->check(param->val);
        if(CM_OK != iRet)
        {
            return iRet;
        }
    }
    return CM_OK;
}

sint32 cm_cnm_decode_check_bool(void *val)
{
    bool_t bs = *((bool_t*)val);
    if(bs > CM_TRUE)
    {
        CM_LOG_ERR(CM_MOD_CNM,"val[%u]",bs);
        return CM_PARAM_ERR;
    }
    return CM_OK;
}

sint32 cm_cnm_exec_ping(sint8 *ipaddr)
{
    sint32 iRet;
    sint8 buf[CM_STRING_32] = {0};
    iRet = cm_exec_tmout(buf, CM_STRING_32, CM_CNM_PING_TMOUT+1,
                         "ping %s %d 2>/dev/null | awk '{printf $3}'", ipaddr, CM_CNM_PING_TMOUT);
    if(CM_OK != iRet)
    {
        return CM_FAIL;
    }
    return strcmp("alive", buf);
}

