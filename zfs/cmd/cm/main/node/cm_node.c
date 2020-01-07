/******************************************************************************
 *          Copyright (c) 2017 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_node.c
 * author     : wbn
 * create date: 2017年10月26日
 * description: TODO:
 *
 *****************************************************************************/
#include "cm_node.h"
#include "cm_log.h"
#include "cm_queue.h"
#include "cm_cmt.h"
#include "cm_cnm_cluster.h"
#include "cm_cnm_systime.h"

typedef struct
{
    bool_t FailBreak;
    void *pArg;
    cm_node_trav_func_t cbk;
}cm_node_traversal_t;

typedef void (*cm_node_master_change_cbk_t)(uint32 old_id,uint32 new_id);
typedef void (*cm_node_master_once_cbk_t)(void);


extern const cm_omi_map_enum_t CmOmiMapEnumDevType;
//static uint32 g_CmSubDomainId = CM_MASTER_SUBDOMAIN_ID;
//static uint32 g_CmMyNodeId = 0;
static uint32 g_CmMasterId = CM_NODE_ID_NONE;
static uint32 g_CmSubMasterId = CM_NODE_ID_NONE;
//static uint32 g_CmMyNodeId_inter = 0;
static cm_node_info_t g_CmNodeInfoLocal;

static cm_tree_node_t *g_pCmNodeList = NULL;

#define CM_NODE_DB_FILE CM_DATA_DIR"cm_node.db"
#define CM_NODE_SH_FILE CM_SCRIPT_DIR"cm_node.sh"


extern void cm_sync_master_change(uint32 old_id, uint32 new_id);
extern void cm_cluster_bind_master_ip(uint32 old_id, uint32 new_id);
static void cm_node_master_change(uint32 old_id, uint32 new_id);
static sint32 cm_node_init_from_db(cm_tree_node_t *pRoot);
extern void cm_cnm_systime_master_change(uint32 old_id,uint32 new_id);
extern void cm_cnm_ntp_master_change(uint32 old_id, uint32 new_id);


const cm_node_master_change_cbk_t g_cm_node_master_cbk[]=
{
    cm_sync_master_change,
    cm_node_master_change,
    cm_cluster_bind_master_ip,
    //cm_cnm_systime_master_change,
    cm_cnm_ntp_master_change,
    NULL,
};

static void cm_node_update_info(void);
const cm_node_master_once_cbk_t g_cm_node_master_once[]=
{
    cm_node_update_info,
    NULL,
};

static cm_subdomain_info_t* 
cm_node_get_subdomain_info(cm_tree_node_t *pRoot, uint32 id)
{
    cm_tree_node_t *pSubDomain = cm_tree_node_find(pRoot, id);

    if(NULL == pSubDomain)
    {
        return NULL;
    }
    return (cm_subdomain_info_t*)pSubDomain->pdata;
}


static sint32 cm_node_add_new(cm_tree_node_t *pRoot, cm_node_info_t *pNode)
{
    cm_node_info_t *pTmpNode = NULL;
    cm_subdomain_info_t *pSubInfo = NULL;
    cm_tree_node_t *pTreeNode = NULL;
    cm_tree_node_t *pSubDomain = cm_tree_node_find(pRoot, pNode->subdomain_id);

    CM_LOG_WARNING(CM_MOD_NODE,"Node[%s] Id[%u] SubDomain[%u] Ip[%s]",
            pNode->name,pNode->id,pNode->subdomain_id,pNode->ip_addr);

    if(NULL == pSubDomain)
    {
        pSubInfo = CM_MALLOC(sizeof(cm_subdomain_info_t));
        if(NULL == pSubInfo)
        {
            CM_LOG_ERR(CM_MOD_NODE,"Malloc Fail");
            return CM_FAIL;
        }
        pSubInfo->id = pNode->subdomain_id;
        pSubInfo->submaster_id = CM_NODE_ID_NONE;
        pSubInfo->node_cnt = 0;
        /* TODO: 添加子域信息 */
        pSubDomain = cm_tree_node_addnew(pRoot,pNode->subdomain_id);
        if(NULL == pSubDomain)
        {
            CM_LOG_ERR(CM_MOD_NODE,"Add SubDomain[%u] Fail", pNode->subdomain_id);
            CM_FREE(pSubInfo);
            return CM_FAIL;
        }
        pSubDomain->pdata = pSubInfo;
        
    }
    pTmpNode = CM_MALLOC(sizeof(cm_node_info_t));
    if(NULL == pTmpNode)
    {
        CM_LOG_ERR(CM_MOD_NODE,"Malloc Fail");
        return CM_FAIL;
    }

    CM_MEM_CPY(pTmpNode,sizeof(cm_node_info_t),pNode,sizeof(cm_node_info_t));
    pTreeNode = cm_tree_node_addnew(pSubDomain,pNode->id);
    if(NULL == pTreeNode)
    {
        CM_LOG_ERR(CM_MOD_NODE,"Add Node Fail");
        CM_FREE(pTmpNode);
        return CM_FAIL;
    }
    pTreeNode->pdata = (void*)pTmpNode;
    pSubInfo = pSubDomain->pdata;
    pSubInfo->node_cnt++;
    if((pNode->subdomain_id == cm_node_get_subdomain_id())
        && pNode->id != cm_node_get_id())
    {
        /* 和本子域节点建立链接*/
        (void)cm_cmt_node_add(pNode->id,pNode->ip_addr,CM_TRUE);
    }
    return CM_OK;
}

static bool_t g_cm_node_check = CM_TRUE;

static sint32 cm_node_state_check(uint32 nid)
{
    cm_tree_node_t *pNode = NULL;
    cm_node_info_t *info = NULL;
    cm_tree_node_t *pSubDomainNode = NULL;

    if(CM_FALSE == g_cm_node_check)
    {
        return CM_OK;
    }
    
    if(NULL == g_pCmNodeList)
    {
        CM_LOG_INFO(CM_MOD_NODE,"not init");
        return CM_FAIL;
    }

    pSubDomainNode = g_pCmNodeList->pchildren;
    
    while(NULL != pSubDomainNode)
    {
        pNode = cm_tree_node_find(pSubDomainNode,nid);
        if(NULL != pNode)
        {
            info = pNode->pdata;
            return (info->state == CM_NODE_STATE_NORMAL) ? CM_OK : CM_FAIL;
        }
        pNode = pNode->pbrother;
    }
    return CM_FAIL;
}

sint32 cm_node_init(void)
{
    sint32 iRet = CM_FAIL;
    cm_node_info_t *info = &g_CmNodeInfoLocal;
    sint8 buff[CM_STRING_256] = {0};
    g_pCmNodeList = cm_tree_node_new(0);
    if(NULL == g_pCmNodeList)
    {
        CM_LOG_ERR(CM_MOD_NODE,"New Node Fail");
        return CM_FAIL;
    }

    /* 初始化本节点信息 */
    CM_MEM_ZERO(info,sizeof(cm_node_info_t));
    info->state = CM_NODE_STATE_NORMAL;

    iRet = cm_exec(buff,sizeof(buff),  "/var/cm/script/cm_shell_exec.sh cm_get_nodeinfo");
    if(iRet != CM_OK)
    {
        CM_LOG_ERR(CM_MOD_NODE,"get info fail");
        return iRet;
    }
    info->id = cm_get_local_nid_x();
    iRet = sscanf(buff,"%u %s %u %u %s %u %s", &info->inter_id, info->name, &info->dev_type, 
        &info->slot_num,info->version,&info->ram_size,info->ip_addr);
    if(iRet != 7)
    {
        CM_LOG_ERR(CM_MOD_NODE,"sscanf %d",iRet);
        return CM_FAIL;
    }    

    iRet = cm_node_init_from_db(g_pCmNodeList);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_NODE,"Get Nodes fail[%u]",iRet);
        return CM_FAIL;
    }
    info->sbbid = (info->inter_id + 1)>>1;
    info->ismaster = (bool_t)(info->inter_id & 1);
    cm_cmt_node_state_cbk_reg(cm_node_state_check);
    return CM_OK;
}

uint32 cm_node_get_subdomain_id(void)
{
    return g_CmNodeInfoLocal.subdomain_id;
}

uint32 cm_node_get_id(void)
{
    return g_CmNodeInfoLocal.id;
}

uint32 cm_node_get_sbbid(void)
{
    return g_CmNodeInfoLocal.sbbid;
}


uint32 cm_node_get_master(void)
{
    return g_CmMasterId;
}

uint32 cm_node_get_submaster(uint32 SubDomainId)
{
    cm_subdomain_info_t* pinfo = cm_node_get_subdomain_info(g_pCmNodeList,SubDomainId);

    if(NULL == pinfo)
    {
       CM_LOG_INFO(CM_MOD_NODE,"SubDomain[%u] not found", SubDomainId); 
       return CM_NODE_ID_NONE;
    }
    return pinfo->submaster_id;
}

sint32 cm_node_set_master(uint32 id)
{
    uint32 old_id = g_CmMasterId;
    const cm_node_master_change_cbk_t *cbk = g_cm_node_master_cbk;
    g_CmMasterId = id;
    CM_LOG_WARNING(CM_MOD_NODE,"Set Master[%u]", id);

    while(*cbk != NULL)
    {
        (*cbk)(old_id,id);
        cbk++;
    }
    return CM_OK;
}

sint32 cm_node_set_submaster(uint32 SubDomainId,uint32 masterid)
{
    cm_subdomain_info_t* pinfo = cm_node_get_subdomain_info(g_pCmNodeList,SubDomainId);

    if(NULL == pinfo)
    {
        CM_LOG_ERR(CM_MOD_NODE,"SubDomain[%u] not found", SubDomainId); 
        return CM_FAIL;
    }
    CM_LOG_WARNING(CM_MOD_NODE,"Set SubMaster[%u]=[%u]", SubDomainId,masterid);
    pinfo->submaster_id = masterid;
    if(SubDomainId == g_CmNodeInfoLocal.subdomain_id)
    {
        g_CmSubMasterId = masterid;
    }
    return CM_OK;
}

uint32 cm_node_get_submaster_current(void)
{
    return g_CmSubMasterId;
}

sint32 cm_node_set_submaster_current(uint32 masterid)
{
    return cm_node_set_submaster(g_CmNodeInfoLocal.subdomain_id,masterid);
}

static cm_node_info_t* cm_node_get_info_inter(uint32 SubDomainId, uint32 NodeId)
{
    cm_tree_node_t *pTmp = cm_tree_node_find(g_pCmNodeList,SubDomainId);

    if(NULL == pTmp)
    {
        CM_LOG_ERR(CM_MOD_NODE,"Get SubDomain[%u] fail", SubDomainId);
        return NULL;
    }

    pTmp = cm_tree_node_find(pTmp, NodeId);
    if(NULL == pTmp)
    {
        CM_LOG_ERR(CM_MOD_NODE,"Get SubDomain[%u] Nid[%u] fail", SubDomainId,NodeId);
        return NULL;
    }

    if(NULL == pTmp->pdata)
    {
        CM_LOG_ERR(CM_MOD_NODE,"Get SubDomain[%u] Nid[%u] Data NULL", SubDomainId,NodeId);
        return NULL;
    }
    return (cm_node_info_t*)pTmp->pdata;
}

sint32 cm_node_get_info(uint32 SubDomainId, uint32 NodeId,cm_node_info_t *pInfo)
{
    cm_node_info_t *info = cm_node_get_info_inter(SubDomainId,NodeId);

    if(NULL == info)
    {
        return CM_FAIL;
    }
    CM_MEM_CPY(pInfo,sizeof(cm_node_info_t),info,sizeof(cm_node_info_t));
    return CM_OK;
}

const sint8* cm_node_get_name(uint32 nid)
{
    cm_tree_node_t *pSubDomainNode = g_pCmNodeList->pchildren;
    cm_tree_node_t *pNode = NULL;
    cm_node_info_t *info = NULL;
    
    if((CM_NODE_ID_NONE == nid)
        || (nid == cm_node_get_id()))
    {
        return g_CmNodeInfoLocal.name;
    }
    
    while(NULL != pSubDomainNode)
    {
        pNode = cm_tree_node_find(pSubDomainNode,nid);
        if(NULL != pNode)
        {
            info = pNode->pdata;
            return info->name;
        }
        pNode = pNode->pbrother;
    }
    return (const sint8*)"-";
}

sint32 cm_node_addto_subdomain(uint32 NodeId, uint32 SubDomainId)
{
    return CM_OK;
}

sint32 cm_node_deletefrom_subdomain(uint32 NodeId, uint32 SubDomainId)
{
    return CM_OK;
}


sint32 cm_node_traversal_node(uint32 SubDomainId, cm_node_trav_func_t Cbk,
void *pArg, bool_t FailBreak)
{
    cm_tree_node_t *pSubDomain = NULL;
    if(NULL == g_pCmNodeList)
    {
        CM_LOG_ERR(CM_MOD_NODE,"not init");
        return CM_FAIL;
    }
    pSubDomain = cm_tree_node_find(g_pCmNodeList,SubDomainId);

    if(NULL == pSubDomain)
    {
        CM_LOG_ERR(CM_MOD_NODE,"Not Found SubDomainId[%u]", SubDomainId);
        return CM_FAIL;
    }

    return cm_tree_node_traversal(pSubDomain,(cm_list_cbk_func_t)Cbk,pArg,FailBreak);
}


sint32 cm_node_traversal_subdomain(cm_node_trav_func_t Cbk,
void *pArg, bool_t FailBreak)
{
    if(NULL == g_pCmNodeList)
    {
        CM_LOG_ERR(CM_MOD_NODE,"not init");
        return CM_FAIL;
    }
    return cm_tree_node_traversal(g_pCmNodeList,(cm_list_cbk_func_t)Cbk,pArg,FailBreak);
}

static sint32 cm_node_for_subdomain(cm_node_info_t *pInfo, cm_node_traversal_t *pArg)
{
    return pArg->cbk(pInfo,pArg->pArg);
}

static sint32 cm_node_for_cluster(cm_subdomain_info_t *pSubDomain, cm_node_traversal_t *pArg)
{
    return cm_node_traversal_node(pSubDomain->id,
        (cm_node_trav_func_t)cm_node_for_subdomain,pArg,pArg->FailBreak);
}

sint32 cm_node_traversal_all(cm_node_trav_func_t Cbk,
void *pArg, bool_t FailBreak)
{
    cm_node_traversal_t traversal= {FailBreak,pArg,Cbk};
    if(NULL == g_pCmNodeList)
    {
        CM_LOG_ERR(CM_MOD_NODE,"not init");
        return CM_FAIL;
    }
    return cm_node_traversal_subdomain(
        (cm_node_trav_func_t)cm_node_for_cluster,&traversal,FailBreak);
}

sint32 cm_node_traversal_all_sbb(cm_node_trav_func_t Cbk,
void *pArg, bool_t FailBreak)
{
    uint32 sbbid = 0;
    uint32 nid = 0;
    uint32 subdomain = 0;
    sint32 iRet = CM_OK;
    sint8 buff[CM_STRING_64] = {0};
    cm_node_info_t* pInfo = NULL;
    uint32 iloop = 0;
    
    do
    {
        buff[0] = '\0';
        iRet = cm_exec_tmout(buff,sizeof(buff),2,"sqlite3 "CM_NODE_DB_FILE
            " 'SELECT id,subdomain,sbbid FROM record_t "
            "WHERE sbbid>%u AND id<>%u ORDER BY sbbid,idx LIMIT 1'",sbbid,nid);
        if((CM_OK != iRet) || ('\0' == buff[0]))
        {
            break;
        }
        iRet = sscanf(buff,"%u|%u|%u",&nid,&subdomain,&sbbid);
        if(3 != iRet)
        {
            break;
        }
        CM_LOG_DEBUG(CM_MOD_NODE,"sbbid[%u] nid[%u]", sbbid,nid);
        for(iloop=2;iloop>0;iloop--)
        {
            pInfo = cm_node_get_info_inter(subdomain,nid);
            if(NULL == pInfo)
            {
                CM_LOG_INFO(CM_MOD_NODE,"SubDomainId[%u] nid[%u] null", subdomain,nid);
                break;
            }
            CM_LOG_DEBUG(CM_MOD_NODE,"SubDomainId[%u] nid[%u]", subdomain,nid);
            iRet = Cbk(pInfo,pArg);
            if(CM_OK == iRet)
            {
                break;
            }
            /* 取兄弟节点 */
            buff[0] = '\0';
            iRet = cm_exec_tmout(buff,sizeof(buff),2,"sqlite3 "CM_NODE_DB_FILE
                " 'SELECT id,subdomain FROM record_t "
                "WHERE sbbid=%u AND id<>%u LIMIT 1'",sbbid,nid);
            if((CM_OK != iRet) || ('\0' == buff[0]))
            {
                break;
            }
            iRet = sscanf(buff,"%u|%u",&nid,&subdomain);
            if(2 != iRet)
            {
                break;
            }                
        }
        if((CM_FALSE != FailBreak) && (CM_OK != iRet))
        {
            break;
        }
    }while(1);
    
    return iRet;
}

static sint32 cm_node_for_submaster(cm_subdomain_info_t *pSubDomain, cm_node_traversal_t *pArg)
{
    cm_node_info_t *info = NULL;
    if(CM_MASTER_SUBDOMAIN_ID == pSubDomain->id)
    {
        /* 前面已经执行过了，这里不再执行 */
        return CM_OK;
    }
    if(0 == pSubDomain->node_cnt)
    {
        return CM_OK;
    }
    
    if(CM_NODE_ID_NONE == pSubDomain->submaster_id)
    {
        CM_LOG_ERR(CM_MOD_NODE,"SubDomainId[%u] nodecnt[%u]", 
            pSubDomain->submaster_id,pSubDomain->node_cnt);
        return CM_OK;
    }

    info = cm_node_get_info_inter(pSubDomain->id,pSubDomain->submaster_id);
    if(NULL == info)
    {
        CM_LOG_ERR(CM_MOD_NODE,"SubDomainId[%u] get[%u] fail", 
            pSubDomain->id,pSubDomain->submaster_id);
        return CM_FAIL;
    }
    return pArg->cbk(info,pArg->pArg);
}


sint32 cm_node_traversal_masters(cm_node_trav_func_t Cbk,
void *pArg, bool_t FailBreak)
{
    cm_node_traversal_t traversal= {FailBreak,pArg,Cbk};
    /* 先遍历0号子域 */
    sint32 iRet = cm_node_traversal_node(CM_MASTER_SUBDOMAIN_ID,Cbk,pArg,FailBreak);
    
    if((CM_OK != iRet) && (CM_FALSE != FailBreak))
    {
        return iRet;
    }
    return cm_tree_node_traversal(g_pCmNodeList,
        (cm_list_cbk_func_t)cm_node_for_submaster,&traversal,FailBreak);
}


sint32 cm_node_set_state(uint32 SubDomainId, uint32 NodeId,uint32 state)
{
    cm_node_info_t *info = cm_node_get_info_inter(SubDomainId,NodeId);

    if(NULL == info)
    {
        return CM_FAIL;
    }
    CM_LOG_WARNING(CM_MOD_NODE,"sub[%u] node[%u] state[%u] to [%u]",
        SubDomainId,NodeId,info->state,state);
    info->state = state;
    return CM_OK;
}

static void* cm_node_master_enter(void* arg)
{
    cm_node_master_once_cbk_t* cbk = arg;
    
    if(NULL == cbk)
    {
        return NULL;
    }
    CM_LOG_WARNING(CM_MOD_NODE,"start");
    while(NULL != *cbk)
    {
        (*cbk)();
        cbk++;
    }
    return NULL;
}

static void cm_node_master_change(uint32 old_id, uint32 new_id)
{
    cm_thread_t handle;
    
    if((CM_NODE_ID_NONE == new_id)
        || (new_id != cm_node_get_id()))
    {
        return;
    }
    /* 启动一次行同步任务 */
    if(CM_OK != CM_THREAD_CREATE(&handle,cm_node_master_enter,(void*)g_cm_node_master_once))
    {
        CM_LOG_ERR(CM_MOD_NODE,"create thread fail");
        return;
    }
    CM_THREAD_DETACH(handle);
    return;
}

sint32 cm_node_update_info_each(cm_node_info_t *pNode, void *pArg)
{
    void *pAck = NULL;
    uint32 AckLen = 0;
    sint32 iRet = CM_OK;
    
    if(pNode->id == cm_node_get_id())
    {
        /* 跳过自己 */
        CM_MEM_CPY(pNode,sizeof(cm_node_info_t),&g_CmNodeInfoLocal,sizeof(cm_node_info_t));
        return CM_OK;
    }
    iRet = cm_cmt_request(pNode->id,CM_CMT_MSG_NODE,NULL,0,&pAck,&AckLen,10);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_NODE,"%s[%u] fail[%d]",pNode->name,pNode->id,iRet);
        return iRet;
    }
    if(NULL == pAck)
    {
        CM_LOG_ERR(CM_MOD_NODE,"%s[%u] ack null",pNode->name,pNode->id);
        return CM_FAIL;
    }
    if(AckLen != sizeof(cm_node_info_t))
    {
        CM_LOG_ERR(CM_MOD_NODE,"%s[%u] AckLen[%u]",pNode->name,pNode->id,AckLen);
        CM_FREE(pAck);
        return CM_FAIL;
    }
    
    CM_MEM_CPY(pNode,sizeof(cm_node_info_t),pAck,AckLen);
    CM_LOG_INFO(CM_MOD_NODE,"%s[%u] iid[%u]",pNode->name,pNode->id,pNode->inter_id);
    CM_FREE(pAck);
    return CM_OK;
    
}

static void cm_node_update_info(void)
{
    cm_node_traversal_all(cm_node_update_info_each,NULL,CM_FALSE);
    return;
}

sint32 cm_node_cmt_cbk(void *pMsg, uint32 Len, void** ppAck, uint32 *pAckLen)
{
    void *pAck = CM_MALLOC(sizeof(cm_node_info_t));
    if(NULL == pAck)
    {
        CM_LOG_ERR(CM_MOD_NODE,"malloc fail");
        return CM_FAIL;
    }

    CM_MEM_CPY(pAck,sizeof(cm_node_info_t),&g_CmNodeInfoLocal,sizeof(cm_node_info_t));

    *ppAck = pAck;
    *pAckLen = sizeof(cm_node_info_t);
    return CM_OK;
}

typedef struct
{
    uint32 nid;
    uint32 nid_inter;
}cm_node_id_map_t;

static sint32 cm_node_get_info_each(cm_node_info_t *pNode, void *pArg)
{
    cm_node_id_map_t *pMap = pArg;

    if(pMap->nid_inter == pNode->inter_id)
    {
        pMap->nid = pNode->id;
        return CM_FAIL; /* 跳出循环 */
    }
    return CM_OK;
}

uint32 cm_node_get_id_by_inter(uint32 inter_id)
{
    cm_node_id_map_t mapx = {CM_NODE_ID_NONE,inter_id};

    if(g_CmNodeInfoLocal.inter_id == inter_id)
    {
        return g_CmNodeInfoLocal.id;
    }
    cm_node_traversal_all(cm_node_get_info_each,&mapx,CM_TRUE);
    if(CM_NODE_ID_NONE == mapx.nid)
    {
        return g_CmNodeInfoLocal.id;
    }
    return mapx.nid;
}

/* ========================================================================== */
/* 改用从数据库读取节点信息 */
#include "cm_db.h"

static sint32 cm_node_add_to_eachnode(cm_node_info_t *pNode,void *arg);
static sint32 cm_node_delete_from_eachnode(cm_node_info_t *pNode,void *arg);

extern sint32 cm_htbt_add(uint32 subdomain,uint32 nid);

extern sint32 cm_htbt_delete(uint32 subdomain,uint32 nid);

static sint32 cm_node_init_data_from_db(cm_node_info_t *info, sint32 col_cnt, sint8 **col_vals)
{
    if(col_cnt != 7)
    {
        CM_LOG_ERR(CM_MOD_NODE,"col_cnt[%u]",col_cnt);
        return CM_FAIL;
    }
    CM_MEM_ZERO(info,sizeof(cm_node_info_t));

    info->id = (uint32)atoi(col_vals[0]);
    CM_VSPRINTF(info->ip_addr,sizeof(info->ip_addr),"%s",col_vals[1]);
    CM_VSPRINTF(info->name,sizeof(info->name),"%s",col_vals[2]);
    info->subdomain_id = (uint32)atoi(col_vals[3]);
    info->sbbid = (uint32)atoi(col_vals[4]);
    info->ismaster = (uint32)atoi(col_vals[5]);
    info->inter_id= (uint32)atoi(col_vals[6]);
    return CM_OK;
}

static sint32 cm_node_init_each(
    void *arg, sint32 col_cnt, sint8 **col_vals, sint8 **col_names)
{
    cm_node_info_t NodeInfo;
    cm_node_info_t *info = &g_CmNodeInfoLocal;
    sint32 iRet = cm_node_init_data_from_db(&NodeInfo,col_cnt,col_vals);

    if(CM_OK != iRet)
    {
        return CM_OK;
    }
    if(NodeInfo.id == info->id)
    {
        info->sbbid = NodeInfo.sbbid;
    }
    return cm_node_add_new((cm_tree_node_t*)arg,&NodeInfo);
}

static sint32 cm_node_init_from_db(cm_tree_node_t *pRoot)
{
    sint32 iRet = CM_OK;
    cm_db_handle_t handle = NULL;
    uint64 subid = 0;
    const sint8* table = "CREATE TABLE IF NOT EXISTS record_t ("
            "id INT,"            
            "ipaddr VARCHAR(64),"
            "name VARCHAR(64),"
            "subdomain INT,"
            "sbbid INT,"
            "ismaster TINYINT,"
            "idx INT"
            ")";
    cm_node_info_t *info = &g_CmNodeInfoLocal;
    
    iRet = cm_db_open_ext(CM_NODE_DB_FILE,&handle);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_NODE,"init db fail");
        return iRet;
    }
    iRet = cm_db_exec_ext(handle,table);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_SCHE,"create table fail[%d]",iRet);
        cm_db_close(handle);
        return iRet;
    }
    /* 如果没有加入集群, 该表中记录为空 */
    (void)cm_db_exec_get_count(handle,&subid,"SELECT subdomain FROM record_t"
        " WHERE id=%u",cm_node_get_id());
    info->subdomain_id = (uint32)subid;
    
    iRet = cm_db_exec(handle,cm_node_init_each,pRoot,"SELECT * FROM record_t");
    cm_db_close(handle);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_NODE,"init nodes fail[%d]",iRet);
        return iRet;
    }
        
    cm_system("%s init",CM_NODE_SH_FILE);
    return CM_OK;    
}

static sint32 cm_node_add_to_db_only(cm_node_info_t *info)
{
    const sint8* sql= "sqlite3 "CM_NODE_DB_FILE;
    return cm_system("%s \"INSERT INTO record_t VALUES(%u,'%s','%s',%u,%u,%u,%u)\"",
        sql, info->id,info->ip_addr,info->name,info->subdomain_id,info->sbbid,
        info->ismaster,info->inter_id);
}
static sint32 cm_node_add_to_db(cm_node_info_t *info)
{
    const sint8* sql= "sqlite3 "CM_NODE_DB_FILE;
    uint32 subdomainid = CM_MASTER_SUBDOMAIN_ID;
    /* 检查ID 是否存在 */
    uint32 cnt = cm_exec_int("%s \"SELECT COUNT(id) FROM record_t WHERE id=%u\"",
        sql,info->id);

    if(0 != cnt)
    {
        CM_LOG_ERR(CM_MOD_NODE,"nid[%u] existed",info->id);
        return CM_ERR_ALREADY_EXISTS;
    }
    
    /* inter_id 添加时已经获取, sbbid通过 inter_id 计算
    底层id从1-16, 强制要求配置时 2n-1 和 2n是同一对双控
    默认2n-1 是主， 2n是备
    sbbid 1-8
    */
    info->sbbid = (info->inter_id + 1)>>1;
    info->ismaster = (bool_t)(info->inter_id & 1);

    /* 计算subdomainid */
    while(1)
    {
        cnt = cm_exec_int("%s \"SELECT COUNT(id) FROM record_t WHERE subdomain=%u\"",
            sql,subdomainid);
        if(((CM_MASTER_SUBDOMAIN_ID==subdomainid) && (CM_MAX_MASTER_DOMAIN_NODE_NUM > cnt))
            ||((CM_MASTER_SUBDOMAIN_ID!=subdomainid) && (CM_MAX_SUB_DOMAIN_NODE_NUM > cnt)))
        {
            break;
        }
        subdomainid++;
    }
    info->subdomain_id = subdomainid;

    CM_LOG_WARNING(CM_MOD_NODE,"nid[%u] ip[%s] host[%s] [%u,%u,%u,%u]",
        info->id,info->ip_addr,info->name,info->subdomain_id,info->sbbid,
        info->ismaster,info->inter_id);
    
    return cm_node_add_to_db_only(info);
}

static sint32 cm_node_delete_from_db(uint32 nid)
{
    const sint8* sql= "sqlite3 "CM_NODE_DB_FILE;
    if(nid == cm_node_get_id())
    {
        CM_LOG_WARNING(CM_MOD_NODE,"delete all");
        return cm_system("%s \"DELETE FROM record_t\"",sql);
    }
    return cm_system("%s \"DELETE FROM record_t WHERE id=%u\"",sql,nid);
}
static sint32 cm_node_delete_from_list(cm_tree_node_t *pRoot, cm_node_info_t *pNode)
{
    cm_node_info_t *pTmpNode = NULL;
    cm_subdomain_info_t *pSubInfo = NULL;
    cm_tree_node_t *pTreeNode = NULL;
    cm_tree_node_t *pSubDomain = cm_tree_node_find(pRoot, pNode->subdomain_id);

    CM_LOG_WARNING(CM_MOD_NODE,"Node[%s] Id[%u] SubDomain[%u] Ip[%s]",
            pNode->name,pNode->id,pNode->subdomain_id,pNode->ip_addr);

    if(NULL == pSubDomain)
    {
        CM_LOG_ERR(CM_MOD_NODE,"get subdomain[%u] fail",pNode->subdomain_id);
        return CM_FAIL;
    }

    pTreeNode = cm_tree_node_find(pSubDomain,pNode->id);
    if(NULL == pTreeNode)
    {
        CM_LOG_ERR(CM_MOD_NODE,"get[nid] subdomain[%u] fail",pNode->id,pNode->subdomain_id);
        return CM_FAIL;
    }
    pTmpNode = pTreeNode->pdata;
    if(NULL == pTmpNode)
    {
        CM_LOG_ERR(CM_MOD_NODE,"get nid[%u] fail",pNode->id);
        return CM_FAIL;
    }
    
    if((pNode->subdomain_id == cm_node_get_subdomain_id())
        && pNode->id != cm_node_get_id())
    {
        /* 断开连接*/
        CM_LOG_WARNING(CM_MOD_NODE,"disconn nid[%u]",pNode->id);
        (void)cm_cmt_node_delete(pNode->id);
    }
    /*删除节点内存*/    
    pTreeNode->pdata = NULL;
    CM_LOG_WARNING(CM_MOD_NODE,"delete nid[%u]",pNode->id);
    /* 从链表中删除 */
    cm_tree_node_delete(pSubDomain,pTreeNode);
    CM_FREE(pTmpNode);
    pSubInfo = pSubDomain->pdata;
    pSubInfo->node_cnt--;
    if((pSubInfo->submaster_id == cm_node_get_id())
        && (CM_MASTER_SUBDOMAIN_ID != cm_node_get_subdomain_id()))
    {
        CM_LOG_WARNING(CM_MOD_NODE,"disconn master nid[%u]",cm_node_get_master());
        (void)cm_cmt_node_delete(cm_node_get_master());
    }
    if(0 == pSubInfo->node_cnt)
    {
        /* 删除子域信息 */
        CM_LOG_WARNING(CM_MOD_NODE,"delete sub[%u]",pNode->subdomain_id);
        pSubDomain->pdata = NULL;
        cm_tree_node_delete(pRoot,pSubDomain);
        CM_FREE(pSubInfo);
    }
    
    return CM_OK;
}

static sint32 cm_node_add_to_local(cm_node_info_t *newinfo)
{
    sint32 iRet = CM_OK;
    cm_tree_node_t *pSubDomain = NULL;
    cm_node_info_t *info = &g_CmNodeInfoLocal;
    cm_subdomain_info_t *pSubInfo = NULL;
    
    pSubDomain = cm_tree_node_find(g_pCmNodeList,CM_MASTER_SUBDOMAIN_ID);
    if(NULL == pSubDomain)
    {
        /* 当前没有配置集群，把本节点细信息和新节点均加入集群 */
        iRet = cm_node_add_to_db(info);

        if(CM_OK != iRet)
        {
            CM_LOG_ERR(CM_MOD_NODE,"add [%u] fail[%d]",info->id,iRet);
            return iRet;
        }
        iRet = cm_node_add_new(g_pCmNodeList,info);
        if(CM_OK != iRet)
        {
            CM_LOG_ERR(CM_MOD_NODE,"add localnode fail[%d]",iRet);
            (void)cm_node_delete_from_db(info->id);
            return iRet;
        }
    }
    info = newinfo;
    iRet = cm_node_add_to_db(info);

    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_NODE,"add [%u] fail[%d]",info->id,iRet);
        return iRet;
    }
    
    iRet = cm_node_add_new(g_pCmNodeList,info);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_SCHE,"add localnode fail[%d]",iRet);
        (void)cm_node_delete_from_db(info->id);
        return iRet;
    }
    iRet = cm_system("%s add %u",CM_NODE_SH_FILE,info->id);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_NODE,"add[%u] sh fail[%d]",info->id,iRet);
    }
    (void)cm_htbt_add(info->subdomain_id,info->id);
    
    if(CM_MASTER_SUBDOMAIN_ID == info->subdomain_id)
    {
        return CM_OK;
    }
    
    /* 判断是否是该子域的第一个节点 如果是第一个，直接设置为主节点，并建立连接*/
    pSubDomain = cm_tree_node_find(g_pCmNodeList,info->subdomain_id);
     /* 刚添加了，这里不可能为空 */
    pSubInfo = (cm_subdomain_info_t*)pSubDomain->pdata;
    if(CM_NODE_ID_NONE != pSubInfo->submaster_id)
    {
        /* 主存在就直接退出 */
        return CM_OK;
    }
    
    /* 直接设置为子域主 */
    pSubInfo->submaster_id = info->id;   

    /* 建立连接 */
    (void)cm_cmt_node_add(info->id,info->ip_addr,CM_TRUE);
    return CM_OK;
}

static sint32 cm_node_send_to_db_each(
    void *arg, sint32 col_cnt, sint8 **col_vals, sint8 **col_names)
{
    cm_node_info_t NodeInfo;
    cm_node_info_t *info = &NodeInfo;
    sint8 cmd[CM_STRING_256] = {0};
    const sint8* sql= "sqlite3 "CM_NODE_DB_FILE;
    sint32 iRet = cm_node_init_data_from_db(&NodeInfo,col_cnt,col_vals);
    
    if(CM_OK != iRet)
    {
        return CM_OK;
    }

    CM_VSPRINTF(cmd,sizeof(cmd),"%s \"INSERT INTO record_t VALUES(%u,'%s','%s',%u,%u,%u,%u)\"",
        sql,info->id,info->ip_addr,info->name,info->subdomain_id,info->sbbid,
        info->ismaster,info->inter_id);
    info = arg;  
    iRet = cm_cmt_request(info->id,CM_CMT_MSG_EXEC,cmd,strlen(cmd)+1,NULL,NULL,CM_CMT_REQ_TMOUT);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_NODE,"add [%u]to nid[%u] fail[%d]",NodeInfo.id,info->id,iRet);
        return iRet;
    }
    return CM_OK;
}   

static void* cm_node_add_thread(void* pArg)
{
    cm_node_info_t *info = pArg;
    cm_node_info_t *pLocalInfo = &g_CmNodeInfoLocal;
    sint32 iRet = cm_node_add_to_db_only(info);

    CM_LOG_WARNING(CM_MOD_NODE,"nid[%u] ip[%s] host[%s] [%u,%u,%u,%u]",
        info->id,info->ip_addr,info->name,info->subdomain_id,info->sbbid,
        info->ismaster,info->inter_id);
        
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_NODE,"add[%u] todb fail ",info->id);
        CM_FREE(info);
        return NULL;
    }
    
    /* 判断自己是否新添加的节点 */
    if(info->id == cm_node_get_id())
    {
        /* 更新本节点信息 */
        pLocalInfo->inter_id = info->inter_id;
        pLocalInfo->sbbid = info->sbbid;
        pLocalInfo->ismaster = info->ismaster;
        pLocalInfo->subdomain_id = info->subdomain_id;
        CM_LOG_WARNING(CM_MOD_NODE,"add[%u] self ",info->id);
        cm_node_init_from_db(g_pCmNodeList);
        (void)cm_htbt_add(info->subdomain_id,info->id);
        CM_FREE(info);        
        return NULL;
    }

    /* 如果自己不是新节点 */
    iRet = cm_node_add_new(g_pCmNodeList,info);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_NODE,"add[%u] tolist fail ",info->id);
        CM_FREE(info);
        return NULL;
    }

    iRet = cm_system("%s add %u",CM_NODE_SH_FILE,info->id);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_NODE,"add[%u] sh fail[%d]",info->id,iRet);
    }

    (void)cm_htbt_add(info->subdomain_id,info->id);
    
    if(CM_MASTER_SUBDOMAIN_ID == cm_node_get_subdomain_id())
    {
        /* 不是0号子域就不用执行后面的了 */
        CM_FREE(info);
        return NULL;
    }

    if(cm_node_get_id() != cm_node_get_submaster_current())
    {
        /* 不是主就不用执行后面的了 */
        CM_FREE(info);
        return NULL;
    }

    iRet = cm_node_traversal_node(cm_node_get_subdomain_id(),
            cm_node_add_to_eachnode,info,CM_TRUE);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_NODE,"add[%u] to other fail ",info->id);
    }
    CM_FREE(info);
    return NULL;    
}


static sint32 cm_node_cmt_cbk_req(void *pMsg, uint32 Len, cm_thread_func_t func)
{
    cm_node_info_t *pNewNode = pMsg;
    cm_thread_t handle;
    cm_node_info_t *pNode = NULL;
    sint32 iRet = CM_OK;
    if((NULL == pNewNode) || Len != sizeof(cm_node_info_t))
    {
        CM_LOG_ERR(CM_MOD_NODE,"Len [%u] ",Len);
        return CM_PARAM_ERR;
    }
    pNode = CM_MALLOC(sizeof(cm_node_info_t));
    if(NULL == pNode)
    {
        CM_LOG_ERR(CM_MOD_NODE,"malloc fail");
        return CM_FAIL;
    }
    CM_MEM_CPY(pNode,sizeof(cm_node_info_t),pNewNode,sizeof(cm_node_info_t));
    
    /* 启动一个后台线程来完成操作，这里必须保证操作成功 
    ，失败就只有定位原因了*/
    iRet = CM_THREAD_CREATE(&handle,func,pNode);
    if(CM_OK != iRet)
    {
        CM_FREE(pNode);
        CM_LOG_ERR(CM_MOD_NODE,"create thread fail");
        return CM_FAIL;
    }
    CM_THREAD_DETACH(handle);

    /* pNode 在 func 中释放 */
    return CM_OK;
}

sint32 cm_node_cmt_cbk_add(void *pMsg, uint32 Len, void** ppAck, uint32 *pAckLen)
{
    return cm_node_cmt_cbk_req(pMsg,Len,cm_node_add_thread);
}

static sint32 cm_node_add_to_eachnode(cm_node_info_t *pNode,void *arg)
{
    sint32 iRet = CM_OK;
    cm_node_info_t *pNewNode = arg;
    cm_db_handle_t handle = NULL;
    
    if(pNode->id == cm_node_get_id())
    {
        /* 本节点就不用再添加了 */
        return CM_OK;
    }

    if(pNode->id == pNewNode->id)
    {
        /* 如果要推送的节点正是要添加的新节点，先把其他节点信息推送过去 */
        iRet = cm_db_open_ext(CM_NODE_DB_FILE,&handle);
        if(CM_OK != iRet)
        {
            CM_LOG_ERR(CM_MOD_NODE,"init db fail");
            return iRet;
        }
        iRet = cm_db_exec(handle,cm_node_send_to_db_each,pNewNode,
            "SELECT * FROM record_t WHERE id<>%u", pNewNode->id);
        cm_db_close(handle);
        if(CM_OK != iRet)
        {
            CM_LOG_ERR(CM_MOD_NODE,"send nodes fail[%d]",iRet);
            return iRet;
        }
        
    }
    /* 再通知添加节点*/
    iRet = cm_cmt_request(pNode->id,CM_CMT_MSG_NODE_ADD,pNewNode,
            sizeof(cm_node_info_t),NULL,NULL,CM_CMT_REQ_TMOUT);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_NODE,"addto nid[%u] fail[%d]",pNode->id,iRet);
        return iRet;
    }
    
    return CM_OK;
}

static sint32 cm_node_delete_from_conn(
    void *arg, sint32 col_cnt, sint8 **col_vals, sint8 **col_names)
{
    cm_node_info_t NodeInfo;
    sint32 iRet = cm_node_init_data_from_db(&NodeInfo,col_cnt,col_vals);
    
    if(CM_OK != iRet)
    {
        return CM_OK;
    }
    return cm_node_delete_from_list(g_pCmNodeList,&NodeInfo);
}

static void cm_node_delete_all_conn(void)
{
    sint32 iRet = CM_OK;
    cm_db_handle_t handle = NULL;
    iRet = cm_db_open_ext(CM_NODE_DB_FILE,&handle);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_NODE,"init db fail");
        return;
    }
    iRet = cm_db_exec(handle,cm_node_delete_from_conn,NULL,
        "SELECT * FROM record_t");
    cm_db_close(handle);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_NODE,"delete nodes fail[%d]",iRet);
    }
    return;
}

static sint32 cm_node_delete_from_local(cm_node_info_t *info)
{
    sint32 iRet = CM_OK;
    
    (void)cm_htbt_delete(info->subdomain_id,info->id);
    if(info->id == cm_node_get_id())
    {
        cm_node_delete_all_conn();
    }
    else
    {    
        iRet = cm_node_delete_from_list(g_pCmNodeList,info);
        if(CM_OK != iRet)
        {
            CM_LOG_ERR(CM_MOD_NODE,"delete list fail[%d]",iRet);
            return iRet;
        }
    }
    iRet = cm_system("%s delete %u",CM_NODE_SH_FILE,info->id);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_NODE,"delete[%u] sh fail[%d]",info->id,iRet);
    }
    return cm_node_delete_from_db(info->id);
}

static void* cm_node_delete_thread(void* pArg)
{
    cm_node_info_t *info = pArg;
    cm_node_info_t *pLocalInfo = &g_CmNodeInfoLocal;
    sint32 iRet = CM_OK;    
    
    CM_LOG_WARNING(CM_MOD_NODE,"nid[%u] ip[%s] host[%s] [%u,%u,%u,%u]",
        info->id,info->ip_addr,info->name,info->subdomain_id,info->sbbid,
        info->ismaster,info->inter_id);

    if(info->id == pLocalInfo->id)
    {
        (void)cm_htbt_delete(info->subdomain_id,info->id);
        
        /* 自己是要删除的节点 ，删除所有链接,以及节点信息*/
        
        cm_node_delete_all_conn();
        CM_FREE(info);
        iRet = cm_system("%s delete %u",CM_NODE_SH_FILE,pLocalInfo->id);
        if(CM_OK != iRet)
        {
            CM_LOG_ERR(CM_MOD_NODE,"delete[%u] sh fail[%d]",pLocalInfo->id,iRet);
        }
        cm_node_delete_from_db(pLocalInfo->id);
        return NULL;
    }

    /* 不是自己的场景 */
    if((CM_MASTER_SUBDOMAIN_ID != cm_node_get_subdomain_id())
        && (cm_node_get_id() == cm_node_get_submaster_current()))
    {
        /* 通知其他节点删除 */
        iRet = cm_node_traversal_node(cm_node_get_subdomain_id(),
            cm_node_delete_from_eachnode,info,CM_TRUE);
        if(CM_OK != iRet)
        {
            CM_LOG_ERR(CM_MOD_NODE,"delete[%u] to other fail ",info->id);
        }
    }

    iRet = cm_node_delete_from_local(info);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_NODE,"delete[%u] local fail[%s]",info->id,iRet);
    }
    CM_FREE(info);
    return NULL;
}

sint32 cm_node_cmt_cbk_delete(void *pMsg, uint32 Len, void** ppAck, uint32 *pAckLen)
{
    return cm_node_cmt_cbk_req(pMsg,Len,cm_node_delete_thread);
}

static sint32 cm_node_delete_from_eachnode(cm_node_info_t *pNode,void *arg)
{
    sint32 iRet = CM_OK;
    
    if(pNode->id == cm_node_get_id())
    {
        return CM_OK;
    }
    iRet = cm_cmt_request(pNode->id,CM_CMT_MSG_NODE_DELETE,arg,
            sizeof(cm_node_info_t),NULL,NULL,CM_CMT_REQ_TMOUT);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_NODE,"deletefrom nid[%u] fail[%d]",pNode->id,iRet);
        return iRet;
    }
    
    return CM_OK;
}

sint32 cm_node_add(const sint8* ipaddr, uint32 sbbid)
{
    sint32 iRet = CM_OK;
    cm_node_info_t NodeInfo;
    sint8 buff[CM_STRING_256] = {0};
    
    /* 不能加自己 */
    if(0 == strcmp(ipaddr,g_CmNodeInfoLocal.ip_addr))
    {
        CM_LOG_ERR(CM_MOD_NODE,"ip %s self",ipaddr);
        return CM_PARAM_ERR;
    }
    
    /* 检查节点连通性 */
    if(CM_OK != cm_cnm_exec_ping(ipaddr))
    {
        CM_LOG_ERR(CM_MOD_NODE,"ping %s fail",ipaddr);
        return CM_ERR_CONN_FAIL;
    }
    CM_MEM_ZERO(&NodeInfo,sizeof(NodeInfo));
    NodeInfo.id = cm_ipaddr_to_nid(ipaddr);
    CM_VSPRINTF(NodeInfo.ip_addr,sizeof(NodeInfo.ip_addr),"%s",ipaddr);
    
    /* 获取主机名 和 ID */
    iRet = cm_exec_tmout(buff,sizeof(buff),5,
        "ceres_exec %s 'hostname;hostid'",ipaddr);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_NODE,"get hostname [%s] fail[%d]",ipaddr,iRet);
        return iRet;
    }
    
    iRet = sscanf(buff,"%s\n%x",NodeInfo.name,&NodeInfo.inter_id);
    if(2 != iRet)
    {
        CM_LOG_ERR(CM_MOD_NODE,"get sscanf [%s] fail[%d]",buff,iRet);
        return iRet;
    }
    
    CM_LOG_WARNING(CM_MOD_NODE,"name[%s] idx[%u]",NodeInfo.name,NodeInfo.inter_id);
    
    //NodeInfo.name[strlen(NodeInfo.name)-1] = '\0';
    /* 判断该节点是否添加到集群中 */
    iRet = cm_exec_int("ceres_exec %s \"sqlite3 "CM_NODE_DB_FILE
        " \'SELECT COUNT(id) FROM record_t\'\"", ipaddr);
    if(0 != iRet)
    {
        CM_LOG_ERR(CM_MOD_NODE,"ipaddr[%s] already in cluster[%d]",ipaddr,iRet);
        return CM_FAIL;
    }

    iRet = cm_system(CM_SCRIPT_DIR"cm_cnm.sh node_rdma_checkadd %s %s %d",
        ipaddr,NodeInfo.name,NodeInfo.inter_id);
    if(0 != iRet)
    {
        CM_LOG_ERR(CM_MOD_NODE,"node_rdma_checkadd %s %d ",ipaddr,iRet);
        return iRet;
    }
    /*添加到本地数据库和链表*/
    iRet = cm_node_add_to_local(&NodeInfo);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_NODE,"addto local[%s] fail[%d]",ipaddr,iRet);
        (void)cm_system(CM_SCRIPT_DIR"cm_cnm.sh node_rdma_checkdel %s",
            NodeInfo.name);
        return iRet;
    }

    /* 同步添加到集群中其他节点 */
    g_cm_node_check = CM_FALSE;
    iRet = cm_node_traversal_masters(cm_node_add_to_eachnode,&NodeInfo,CM_TRUE);
    g_cm_node_check = CM_TRUE;
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_NODE,"addto local[%s] fail[%d]",ipaddr,iRet);
        cm_node_traversal_masters(cm_node_delete_from_eachnode,&NodeInfo,CM_FALSE);
        (void)cm_system(CM_SCRIPT_DIR"cm_cnm.sh node_rdma_checkdel %s",
            NodeInfo.name);
        return iRet;
    }
    
    CM_LOG_WARNING(CM_MOD_NODE,"add hostname [%s] ok",ipaddr);
    
    return CM_OK;
}

static sint32 cm_node_get_info_from_db(
    void *arg, sint32 col_cnt, sint8 **col_vals, sint8 **col_names)
{
    return cm_node_init_data_from_db(arg,col_cnt,col_vals);
}

sint32 cm_node_delete(uint32 nid)
{
    cm_node_info_t NodeInfo;
    cm_node_info_t *info = &NodeInfo;
    sint32 iRet = CM_OK;
    uint32 cnt = cm_db_exec_file_get(CM_NODE_DB_FILE,cm_node_get_info_from_db,
        &NodeInfo,1,sizeof(NodeInfo),"SELECT * FROM record_t WHERE id=%u",nid);

    if(0 == cnt)
    {
        CM_LOG_ERR(CM_MOD_NODE,"get [%u] fail",nid);
        return CM_ERR_NOT_EXISTS;
    }
    
    CM_LOG_WARNING(CM_MOD_NODE,"nid[%u] ip[%s] host[%s] [%u,%u,%u,%u]",
        info->id,info->ip_addr,info->name,info->subdomain_id,info->sbbid,
        info->ismaster,info->inter_id);
        
    if(CM_MASTER_SUBDOMAIN_ID == info->subdomain_id)
    {
        /* 检查节点数 */
        cm_tree_node_t *pSubDomain = cm_tree_node_find(g_pCmNodeList, CM_MASTER_SUBDOMAIN_ID);
        if(2 >= pSubDomain->child_num)
        {
            CM_LOG_ERR(CM_MOD_NODE,"nodenum[%u]",pSubDomain->child_num);
            return CM_ERR_NOT_SUPPORT;
        }        
    }
    
    /* 从其他节点中删除 */
    iRet = cm_node_traversal_masters(cm_node_delete_from_eachnode,info,CM_TRUE);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_NODE,"delete masters fail[%d]",iRet);
        return iRet;
    }

    iRet= cm_node_delete_from_local(info);
    
    CM_LOG_WARNING(CM_MOD_NODE,"nid[%u] iRet[%u]",info->id,iRet);
    return iRet;
}

uint32 cm_node_get_brother_nid(uint32 nid)
{
    return (uint32)cm_exec_int("sqlite3 /var/cm/data/cm_node.db "
        "\"select id from record_t WHERE id<>%u "
        "AND sbbid IN (SELECT sbbid FROM record_t WHERE id=%u)\"",nid,nid);
}

sint32 cm_node_power_on(uint32 nid, const sint8 *user, const sint8 *passwd)
{
    if( strlen(passwd) == 0 )
    {
        passwd="admin";
    }
    CM_LOG_WARNING(CM_MOD_NODE, "nid:%u user:%s passwd:%s", nid, user, passwd);
    return cm_system("%s on %u %s %s", CM_NODE_SH_FILE, nid, user, passwd);
}

sint32 cm_node_power_off(uint32 nid)
{
    CM_LOG_WARNING(CM_MOD_NODE, "nid:%u", nid);
    return cm_system("%s off %u", CM_NODE_SH_FILE, nid);
}

sint32 cm_node_reboot(uint32 nid)
{
    CM_LOG_WARNING(CM_MOD_NODE, "nid:%u", nid);
    return cm_system("%s reboot %u", CM_NODE_SH_FILE, nid);
}

static sint32 cm_node_check_online_each(cm_node_info_t *pNode, void *pArg)
{
    return (CM_NODE_STATE_NORMAL == pNode->state)? CM_OK : CM_FAIL;
}
sint32 cm_node_check_all_online(void)
{
    return cm_node_traversal_all(cm_node_check_online_each,NULL,CM_TRUE);
}

const sint8* cm_node_getversion(void)
{
    return g_CmNodeInfoLocal.version;
}

