/******************************************************************************
 *          Copyright (c) 2017 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_htbt.c
 * author     : wbn
 * create date: 2017年10月26日
 * description: TODO:
 *
 *****************************************************************************/
#include "cm_htbt.h"
#include "cm_cmt.h"
#include "cm_queue.h"
#include "cm_log.h"
#include "cm_node.h"
#include "cm_alarm.h"

/* 运行状态定义 (子域内部消息使用)*/
#define CM_HTBT_STATE_LONELY 0
#define CM_HTBT_STATE_SELECT 1
#define CM_HTBT_STATE_STABLE 2


/* 节点状态定义 */
#define CM_HTBT_NODE_UNKNOW 0
#define CM_HTBT_NODE_ONLINE 1
#define CM_HTBT_NODE_OFFLINE 2

typedef struct
{
    /* 本节点ID */
    uint32 id;

    /* 子域ID */
    uint32 sub_domain_id;

    /* 子域主*/
    uint32 sub_master_id;

    /* 状态 */
    uint32 state;

    /* 启动时间 (进程启动时间) ，初始化时设置 */
    cm_time_t up_time;

    /* 健康指数 ，初始化时设置缺省值，后续发生改变时刷新 */
    uint32 health_index;

    /* 担任主的时常*/
    uint32 duration;

    /* 预留字段，后续扩展使用 */
    uint32 unused[5];
}cm_htbt_local_info_t;

typedef struct
{
    /* 节点ID */
    uint32 id;

    /* 在线状态 UNKNOW ONLINE OFFLINE
    *  初始值 UNKNOW, 接收到心跳后设置为ONLINE，本节点始终为ONLINE
    *  如果心跳丢失到一定次数，ONLINE -> OFFLINE, 非子域主不用修改
    */
    uint32 state;

    /* 心跳丢失次数，不管是否子域主都要记录，本节点丢失次数始终为0 */
    uint32 lost_cnt;

    /* 预留字段，后续扩展使用*/
    uint32 unused[5];
}cm_htbt_node_t;

typedef struct
{
    /* 子域 ID */
    uint32 id;

    /* 子域主*/
    uint32 sub_master_id;

    /* 子域内有效节点数 */
    uint32 node_cnt;

    /* 子域内节点状态 */
    cm_htbt_node_t node[CM_HTBT_SUBDOMAIN_NODE_MAX];
}cm_htbt_sub_domain_info_t;

#define CM_HTBT_MSG_LOCAL 0
#define CM_HTBT_MGS_SUBDOMAIN 1

typedef struct
{
    uint32 msg;
    uint32 data_len;
    uint8 data[];
}cm_htbt_msg_t;

typedef struct
{
    uint32 msg; /* CM_HTBT_MSG_LOCAL */
    uint32 data_len;
    cm_htbt_local_info_t info;
}cm_htbt_msg_local_t;

typedef struct
{
    uint32 msg; /* CM_HTBT_MGS_SUBDOMAIN */
    uint32 data_len;
    cm_htbt_sub_domain_info_t info;
}cm_htbt_msg_sub_domain_t;

struct cm_htbt_runtime_info_tt;

typedef void (*cm_htbt_local_recv_func_t)(struct cm_htbt_runtime_info_tt
*pGlobal,cm_htbt_local_info_t *pOther);

typedef void (*CM_HtbtLocalCheckFuncType)(struct cm_htbt_runtime_info_tt *pGlobal);


typedef struct cm_htbt_runtime_info_tt
{
    /* 互斥锁 */
    cm_mutex_t mutex;

    /* 记录子域内节点状态信息 */
    cm_tree_node_t *pnode_list;

    /* 记录所有子域节点信息 */
    cm_tree_node_t *psubdomain_list;

    /* 主节点信息 */
    cm_htbt_local_info_t *pmaster;

    /* 记录本节点信息 */
    cm_htbt_msg_local_t local;

    /* 子域内节点状态维护，用于子域间上报 */
    cm_htbt_msg_sub_domain_t subdomain;

    /* 心跳消息队列 */
    cm_queue_t *pmsg_queue;

    /* 当前接收处理函数 */
    cm_htbt_local_recv_func_t recv_func;

    /* 当前周期校验函数 */
    CM_HtbtLocalCheckFuncType check_func;

    /* 每收到一次清0, 每检测一次 +1 */
    uint32 lost_cnt;

    uint32 check_cnt;

}cm_htbt_runtime_info_t;

static cm_htbt_runtime_info_t g_CmHtbtGlobal;
static bool_t g_CmHtbtIsInit = CM_FALSE;

static sint32 cm_htbt_init_inter(cm_htbt_runtime_info_t *pGlobal);
static void* cm_htbt_thread_send(void *pArg);
static void* cm_htbt_thread_recv(void *pArg);

static void cm_htbt_state_recv_lonely(cm_htbt_runtime_info_t *pGlobal,cm_htbt_local_info_t
*pOther);
static void cm_htbt_state_recv_select(cm_htbt_runtime_info_t *pGlobal,cm_htbt_local_info_t
*pOther);

static void cm_htbt_state_recv_stable(cm_htbt_runtime_info_t *pGlobal,cm_htbt_local_info_t
*pOther);

static void cm_htbt_state_enter_lonely(cm_htbt_runtime_info_t *pGlobal);
static void cm_htbt_state_enter_select(cm_htbt_runtime_info_t *pGlobal);
static void cm_htbt_state_enter_stable(cm_htbt_runtime_info_t *pGlobal);
static void cm_htbt_state_check_lonely(cm_htbt_runtime_info_t *pGlobal);
static void cm_htbt_state_check_select(cm_htbt_runtime_info_t *pGlobal);
static void cm_htbt_state_check_stable(cm_htbt_runtime_info_t *pGlobal);



sint32 cm_htbt_init(void)
{
    sint32 iRet = CM_FAIL;
    cm_thread_t Thread;
    cm_htbt_runtime_info_t *pGlobal = &g_CmHtbtGlobal;

    CM_MEM_ZERO(pGlobal,sizeof(cm_htbt_runtime_info_t));

    iRet = cm_queue_init(&pGlobal->pmsg_queue, CM_HTBT_SUBDOMAIN_NODE_MAX);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_HTBT,"Init Queue Fail(%d)", iRet);
        return iRet;
    }

    iRet = cm_htbt_init_inter(pGlobal);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_HTBT,"Init Inter Fail(%d)", iRet);
        cm_queue_destroy(pGlobal->pmsg_queue);
        return iRet;
    }

    cm_htbt_state_enter_lonely(pGlobal);

    iRet = CM_MUTEX_INIT(&pGlobal->mutex);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_HTBT,"Init Mutex Fail(%d)", iRet);
        cm_queue_destroy(pGlobal->pmsg_queue);
        return iRet;
    }

    iRet = CM_THREAD_CREATE(&Thread,cm_htbt_thread_recv,pGlobal);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_HTBT,"Init Thread Recv Fail(%d)", iRet);
        cm_queue_destroy(pGlobal->pmsg_queue);
        return iRet;
    }
    CM_THREAD_DETACH(Thread);

    iRet = CM_THREAD_CREATE(&Thread,cm_htbt_thread_send,pGlobal);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_HTBT,"Init Thread Send Fail(%d)", iRet);
        cm_queue_destroy(pGlobal->pmsg_queue);
        return iRet;
    }
    CM_THREAD_DETACH(Thread);
    g_CmHtbtIsInit = CM_TRUE;
    return CM_OK;
}

static sint32 cm_htbt_init_nodes(cm_node_info_t *pNode, cm_htbt_sub_domain_info_t *pSubDomain)
{
    cm_htbt_node_t *pHtbtNode = &pSubDomain->node[pSubDomain->node_cnt];

    CM_LOG_WARNING(CM_MOD_HTBT,"%u %s %s", pNode->id,pNode->name, pNode->ip_addr);

    pHtbtNode->id = pNode->id;
    pHtbtNode->lost_cnt = 0;
    if(pHtbtNode->id == cm_node_get_id())
    {
        pHtbtNode->state = CM_HTBT_NODE_ONLINE;
        (void)cm_node_set_state(pNode->subdomain_id,pNode->id,CM_NODE_STATE_NORMAL);
    }
    else
    {
        pHtbtNode->state = CM_HTBT_NODE_UNKNOW;
        (void)cm_node_set_state(pNode->subdomain_id,pNode->id,CM_NODE_STATE_UNKNOW);
    }
    pSubDomain->node_cnt += 1;
    return CM_OK;
}

static sint32 cm_htbt_init_inter(cm_htbt_runtime_info_t *pGlobal)
{
    cm_htbt_local_info_t *pLocal = &pGlobal->local.info;
    cm_htbt_sub_domain_info_t *pSubDomain = &pGlobal->subdomain.info;
    cm_tree_node_t *pTmp = cm_tree_node_new(CM_HTBT_MSG_LOCAL);

    if(NULL == pTmp)
    {
        CM_LOG_ERR(CM_MOD_HTBT,"Init List local Fail");
        return CM_FAIL;
    }
    pGlobal->pnode_list = pTmp;

    pTmp = cm_tree_node_new(CM_HTBT_MGS_SUBDOMAIN);
    if(NULL == pTmp)
    {
        CM_LOG_ERR(CM_MOD_HTBT,"Init List SUBDOMAIN Fail");
        cm_tree_node_free(pGlobal->pnode_list);
        return CM_FAIL;
    }
    pGlobal->psubdomain_list = pTmp;

    pGlobal->local.msg = CM_HTBT_MSG_LOCAL;
    pGlobal->local.data_len = sizeof(cm_htbt_local_info_t);
    pLocal->id = cm_node_get_id();
    pLocal->sub_domain_id = cm_node_get_subdomain_id();
    pLocal->state = CM_HTBT_STATE_LONELY;
    pLocal->sub_master_id = CM_NODE_ID_NONE;
    pLocal->up_time = cm_get_time();

    pGlobal->subdomain.msg = CM_HTBT_MGS_SUBDOMAIN;
    pGlobal->subdomain.data_len = sizeof(cm_htbt_sub_domain_info_t);
    pSubDomain->id = cm_node_get_subdomain_id();
    pSubDomain->sub_master_id = CM_NODE_ID_NONE;

    CM_LOG_WARNING(CM_MOD_HTBT,"MyNid[%u], SubDomainId[%u]", pLocal->id,pSubDomain->id);
    cm_node_traversal_node(pSubDomain->id,(cm_node_trav_func_t)cm_htbt_init_nodes,pSubDomain,CM_TRUE);
    return CM_OK;
}

static cm_htbt_node_t* cm_htbt_node_getinfo(cm_htbt_sub_domain_info_t *pSubDomain,uint32 Id)
{
    uint32 Cnt = CM_HTBT_SUBDOMAIN_NODE_MAX;
    cm_htbt_node_t *pTem = pSubDomain->node;

    for(;Cnt>0;Cnt--,pTem++)
    {
        if(pTem->id == Id)
        {
            return pTem;
        }
    }
    return NULL;
}

static void cm_htbt_add_newnode_to_list(cm_tree_node_t *pNodeList,cm_htbt_local_info_t *pData)
{
    cm_htbt_local_info_t *pNewNode = NULL;
    cm_tree_node_t *pListNode = NULL;

    pNewNode = CM_MALLOC(sizeof(cm_htbt_local_info_t));
    if(NULL == pNewNode)
    {
        CM_LOG_ERR(CM_MOD_HTBT,"Node[%u] SubDomainId[%u] Malloc",
            pData->id,pData->sub_domain_id);
        return;
    }

    pListNode = cm_tree_node_addnew(pNodeList, pData->id);
    if(NULL == pListNode)
    {
        CM_LOG_ERR(CM_MOD_HTBT,"Node[%u] SubDomainId[%u] Add List Fail",
            pData->id,pData->sub_domain_id);
        CM_FREE(pNewNode);
        return;
    }
    CM_LOG_WARNING(CM_MOD_HTBT,"Node[%u] SubDomainId[%u] Add New Ok",
        pData->id,pData->sub_domain_id);
    pListNode->pdata = (void*)pNewNode;
    return;
}


static void cm_htbt_add_newnode(cm_htbt_runtime_info_t *pGlobal,cm_htbt_local_info_t *pData)
{
    cm_htbt_node_t *pTem = cm_htbt_node_getinfo(&pGlobal->subdomain.info,CM_NODE_ID_NONE);

    if(NULL == pTem)
    {
        CM_LOG_ERR(CM_MOD_HTBT,"Node[%u] SubDomainId[%u] Full",
            pData->id,pData->sub_domain_id);
        return;
    }
    /* 找到空位，加入 */
    pTem->id = pData->id;
    pTem->lost_cnt = 0;
    pTem->state = CM_HTBT_NODE_ONLINE;
    
    pGlobal->subdomain.info.node_cnt++;

    /* 添加到子域内节点链表中 */
    cm_htbt_add_newnode_to_list(pGlobal->pnode_list,pData);
    return;
}

static void cm_htbt_recv_local(cm_htbt_runtime_info_t *pGlobal,cm_htbt_local_info_t *pData)
{
    sint32 iRet = CM_FAIL;
    cm_node_info_t NodeInfo;
    cm_htbt_local_info_t *pLocal = &pGlobal->local.info;
    cm_htbt_node_t *pFromNode = NULL;
    cm_tree_node_t *pListNode = NULL;

    CM_LOG_INFO(CM_MOD_HTBT,"Node[%u]:Duration[%u],Health[%u],up_time[%u]",
        pData->id,pData->duration,pData->health_index,pData->up_time);

    if(pData->sub_domain_id != pLocal->sub_domain_id)
    {
        /* 子域内心跳，只处理相同子域的 */
        CM_LOG_ERR(CM_MOD_HTBT,"Node[%u] SubDomainId[%u] local[%u]",
            pData->id,pData->sub_domain_id, pLocal->sub_domain_id);
        return;
    }

    pFromNode = cm_htbt_node_getinfo(&pGlobal->subdomain.info,pData->id);
    if(NULL == pFromNode)
    {
        /* 原来的节点列表中没有, 有新节点加入 */
        CM_LOG_WARNING(CM_MOD_HTBT,"NewNode[%u] SubDomainId[%u]",
                pData->id,pData->sub_domain_id);

        iRet = cm_node_get_info(pData->sub_domain_id,pData->id, &NodeInfo);
        if(CM_OK != iRet)
        {
            CM_LOG_ERR(CM_MOD_HTBT,"Node[%u] SubDomainId[%u] Not Found",
                pData->id,pData->sub_domain_id);
            return;
        }
        cm_htbt_add_newnode(pGlobal, pData);
        (void)cm_node_set_state(pGlobal->subdomain.info.id,pData->id,CM_NODE_STATE_NORMAL);
        return;
    }

    /* 接收到心跳，丢失次数清空 */
    pFromNode->lost_cnt = 0;
    if(CM_HTBT_NODE_ONLINE != pFromNode->state)
    {
        CM_LOG_WARNING(CM_MOD_HTBT,"node[%u] state From[%u] To[%u]",
                pData->id,pFromNode->state, CM_HTBT_NODE_ONLINE);
        pFromNode->state = CM_HTBT_NODE_ONLINE;

        /* TODO: 0号子域上报节点加入之类的 */
        (void)cm_node_set_state(pGlobal->subdomain.info.id,pData->id,CM_NODE_STATE_NORMAL);
        if(pLocal->id == pLocal->sub_master_id)
        {
            CM_ALARM_RECOVERY(CM_ALARM_FAULT_NODE_OFFLINE,cm_node_get_name(pData->id));
        }
    }


    pListNode = cm_tree_node_find(pGlobal->pnode_list, pData->id);
    if(NULL == pListNode)
    {
        /* 说明之前没有接收到过 */
        cm_htbt_add_newnode_to_list(pGlobal->pnode_list, pData);
        return;
    }

    /* 刷新信息 */
    CM_MEM_CPY(pListNode->pdata,sizeof(cm_htbt_local_info_t),pData,sizeof(cm_htbt_local_info_t));

    pGlobal->recv_func(pGlobal,pData);

    return;
}

static void cm_htbt_recv_subdomain(cm_htbt_sub_domain_info_t *pData)
{
    return;

}


static void* cm_htbt_thread_recv(void *pArg)
{
    cm_htbt_runtime_info_t *pGlobal = (cm_htbt_runtime_info_t*)pArg;
    sint32 iRet = CM_FAIL;
    cm_htbt_msg_t *pMsg = NULL;

    while(1)
    {
        iRet = cm_queue_get(pGlobal->pmsg_queue, (void**)&pMsg);
        if((CM_OK != iRet)
            ||(NULL == pMsg))
        {
            continue;
        }
        CM_MUTEX_LOCK(&pGlobal->mutex);
        CM_LOG_INFO(CM_MOD_HTBT,"msg[%u]",pMsg->msg);
        switch(pMsg->msg)
        {
            case CM_HTBT_MSG_LOCAL:
                if(pMsg->data_len != sizeof(cm_htbt_local_info_t))
                {
                    CM_LOG_WARNING(CM_MOD_HTBT,"data_len[%d] Err",pMsg->data_len);
                    break;
                }
                cm_htbt_recv_local(pGlobal, (cm_htbt_local_info_t*)pMsg->data);
                break;
            case CM_HTBT_MGS_SUBDOMAIN:
                if(pMsg->data_len != sizeof(cm_htbt_sub_domain_info_t))
                {
                    CM_LOG_WARNING(CM_MOD_HTBT,"data_len[%d] Err",pMsg->data_len);
                    break;
                }
                cm_htbt_recv_subdomain((cm_htbt_sub_domain_info_t*)pMsg->data);
                break;
            default:
                break;
        }

        CM_MUTEX_UNLOCK(&pGlobal->mutex);
        CM_FREE(pMsg);
        pMsg = NULL;
    }
    return NULL;
}

static void cm_htbt_local_check(cm_htbt_runtime_info_t *pGlobal, cm_htbt_node_t *pNode)
{
    cm_htbt_local_info_t *pLocal = &pGlobal->local.info;

    pNode->lost_cnt++;
    if(pNode->lost_cnt < CM_HTBT_STABLE_NUM)
    {
        return;
    }

    if(pNode->id == pLocal->sub_master_id)
    {
        cm_htbt_state_enter_select(pGlobal);
        return;
    }

    if(pNode->lost_cnt < CM_HTBT_OFFLINE_NUM)
    {
        CM_LOG_WARNING(CM_MOD_HTBT,"node[%u] lost_cnt[%u]",pNode->id,pNode->lost_cnt);
        return;
    }
    
    if(CM_HTBT_NODE_OFFLINE!= pNode->state)
    {
        CM_LOG_WARNING(CM_MOD_HTBT,"node[%u] state From[%u] To[%u]",
                pNode->id,pNode->state, CM_HTBT_NODE_OFFLINE);
        pNode->state = CM_HTBT_NODE_OFFLINE;

        (void)cm_node_set_state(pGlobal->subdomain.info.id,pNode->id,CM_NODE_STATE_OFFLINE);
        if(pLocal->id == pLocal->sub_master_id)
        {
            /* TODO: 0号子域上报节点退出之类的 */
            CM_ALARM_REPORT(CM_ALARM_EVENT_NODE_OUT,cm_node_get_name(pNode->id));
            return;
        }        
    }

    if ((pNode->lost_cnt == CM_HTBT_OFFLINE_LONG_NUM)
        && (pLocal->id == pLocal->sub_master_id))
    {
        CM_ALARM_REPORT(CM_ALARM_FAULT_NODE_OFFLINE,cm_node_get_name(pNode->id));
    }
    return;
}

static void cm_htbt_local_send(cm_htbt_runtime_info_t *pGlobal)
{
    sint32 iRet = CM_FAIL;
    uint32 iLoop = CM_HTBT_SUBDOMAIN_NODE_MAX;
    cm_htbt_node_t *pTmpNode = pGlobal->subdomain.info.node;
    cm_htbt_local_info_t *pLocal = &pGlobal->local.info;

    CM_LOG_INFO(CM_MOD_HTBT,"Node[%u]:duration[%u],Health[%u],up_time[%u]",
        pLocal->id,pLocal->duration,pLocal->health_index,pLocal->up_time);

    for(;iLoop > 0; pTmpNode++,iLoop--)
    {
        if(CM_NODE_ID_NONE == pTmpNode->id)
        {
            continue;
        }

        if(pLocal->id == pTmpNode->id)
        {
            continue;
        }

        iRet = cm_cmt_request(pTmpNode->id,
            CM_CMT_MSG_HTBT,(void*)&pGlobal->local,sizeof(pGlobal->local),
            NULL,NULL,1);

        CM_LOG_INFO(CM_MOD_HTBT,"SendTo[%u] iRet[%u]",pTmpNode->id,iRet);

        cm_htbt_local_check(pGlobal, pTmpNode);
    }
}

static void* cm_htbt_thread_send(void *pArg)
{
    cm_htbt_runtime_info_t *pGlobal = (cm_htbt_runtime_info_t*)pArg;

    while(1)
    {
        CM_SLEEP_S(CM_HTBT_MSG_PERIOD_LOCAL);
        CM_MUTEX_LOCK(&pGlobal->mutex);
        pGlobal->check_func(pGlobal);
        cm_htbt_local_send(pGlobal);
        CM_MUTEX_UNLOCK(&pGlobal->mutex);
    }
    return NULL;
}

sint32 cm_htbt_cbk_recv(void *pMsg, uint32 Len, void**ppAck, uint32 *pAckLen)
{
    sint32 iRet = CM_FAIL;
    void *pMsgRecv = NULL;

    if((NULL == pMsg) || (0 == Len) || (CM_FALSE == g_CmHtbtIsInit))
    {
        return CM_OK;
    }
    
    pMsgRecv = CM_MALLOC(Len);
    if(NULL == pMsgRecv)
    {
        CM_LOG_ERR(CM_MOD_HTBT,"Malloc Fail");
        return CM_FAIL;
    }
    CM_MEM_CPY(pMsgRecv,Len,pMsg,Len);
    iRet = cm_queue_add(g_CmHtbtGlobal.pmsg_queue,pMsgRecv);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_HTBT,"Add Queue Fail[%u]",iRet);
        CM_FREE(pMsgRecv);
        return CM_FAIL;
    }
    return CM_OK;
}

static cm_htbt_local_info_t* cm_htbt_get_local_info(cm_tree_node_t *pNodeList, uint32 Nid)
{
    cm_tree_node_t *pNode = cm_tree_node_find(pNodeList,Nid);

    if(NULL != pNode)
    {
        return (cm_htbt_local_info_t*)pNode->pdata;
    }
    return NULL;
}

static void cm_htbt_master_update(cm_htbt_runtime_info_t *pGlobal,uint32 MasterNid)
{
    cm_htbt_local_info_t *pLocal = &pGlobal->local.info;

    CM_LOG_WARNING(CM_MOD_HTBT,"Master[%u] Change New[%u]",
        pLocal->sub_master_id, MasterNid);
    pGlobal->check_cnt = 0;
    pLocal->sub_master_id = MasterNid;
    pGlobal->subdomain.info.sub_master_id = MasterNid;
    if(MasterNid == pLocal->id)
    {
        pGlobal->pmaster = pLocal;
    }
    else
    {
        pGlobal->pmaster = cm_htbt_get_local_info(pGlobal->pnode_list,MasterNid);
        if(NULL == pGlobal->pmaster)
        {
            CM_LOG_WARNING(CM_MOD_HTBT,"pmaster[%u] NULL",MasterNid);
            pGlobal->pmaster = pLocal;
        }
    }
}

static bool_t cm_htbt_master_pk(cm_htbt_local_info_t *pOld, cm_htbt_local_info_t *pNew)
{
    CM_LOG_WARNING(CM_MOD_HTBT,"Old[%u]:duration[%u],Health[%u],up_time[%u]",
        pOld->id,pOld->duration,pOld->health_index,pOld->up_time);

    CM_LOG_WARNING(CM_MOD_HTBT,"New[%u]:duration[%u],Health[%u],up_time[%u]",
        pNew->id,pNew->duration,pNew->health_index,pNew->up_time);

    /* 哪个稳定选哪个 */
    if(pOld->duration > pNew->duration)
    {
        return CM_FALSE;
    }

    if(pOld->duration < pNew->duration)
    {
        return CM_TRUE;
    }

    /* 选更健康的一个 */
    if(pOld->health_index > pNew->health_index)
    {
        return CM_FALSE;
    }
    if(pOld->health_index < pNew->health_index)
    {
        return CM_TRUE;
    }

    /* 选启动早的 */
    if(pOld->up_time < pNew->up_time)
    {
        return CM_FALSE;
    }
    if(pOld->up_time > pNew->up_time)
    {
        return CM_TRUE;
    }

    /* 以上都分不出胜负，就只有比较ID */
    if(pOld->id < pNew->id)
    {
        return CM_FALSE;
    }
    return CM_TRUE;
}

static void cm_htbt_state_recv_lonely(cm_htbt_runtime_info_t *pGlobal,cm_htbt_local_info_t
*pOther)
{
    pGlobal->lost_cnt = 0;

    /* 只要接收到一个其他节点的消息就进入选主状态 */
    CM_LOG_WARNING(CM_MOD_HTBT,"OtherNode[%u] state[%u]", pOther->id,pOther->state);
    cm_htbt_state_enter_select(pGlobal);
    return;
}

static void cm_htbt_state_recv_select(cm_htbt_runtime_info_t *pGlobal,cm_htbt_local_info_t
*pOther)
{
    pGlobal->lost_cnt = 0;

    /* 只要对方不认为自己是主就不管了 */
    if(pOther->id != pOther->sub_master_id)
    {
        return;
    }

    if(CM_FALSE == cm_htbt_master_pk(pGlobal->pmaster,pOther))
    {
        /* 地位稳固 */
        return;
    }

    /* 换新主 */
    cm_htbt_master_update(pGlobal,pOther->id);
    cm_htbt_state_enter_stable(pGlobal);
    return;
}

static void cm_htbt_state_recv_stable(cm_htbt_runtime_info_t *pGlobal,cm_htbt_local_info_t
*pOther)
{
    cm_htbt_local_info_t *pLocal = &pGlobal->local.info;

    pGlobal->lost_cnt = 0;

    /* 只关心主的消息，或者非主想升主 */
    if(pLocal->sub_master_id == pOther->id)
    {
        if(pOther->sub_master_id != pOther->id)
        {
            CM_LOG_INFO(CM_MOD_HTBT,"sub_master_id[%u] nid[%u] state[%u]", 
                pOther->sub_master_id,pOther->id,pOther->state);
            cm_htbt_state_enter_select(pGlobal);
            return;
            
        }
        if(CM_HTBT_STATE_STABLE == pOther->state)
        {
            /* 主稳定，不管了 */
            return;
        }
        /* 否则，重新选择主 */
        cm_htbt_state_enter_select(pGlobal);
        return;
    }

    if((CM_HTBT_STATE_STABLE != pOther->state)
        ||(pOther->id != pOther->sub_master_id))
    {
        /* 对方不稳定，不管 */
        return;
    }

    if(CM_FALSE == cm_htbt_master_pk(pGlobal->pmaster,pOther))
    {
        /* 地位稳固 */
        return;
    }

    /* 换新主 */
    cm_htbt_master_update(pGlobal,pOther->id);
    (void)cm_node_set_submaster(cm_node_get_subdomain_id(),pLocal->sub_master_id);
    if(pLocal->sub_domain_id == CM_MASTER_SUBDOMAIN_ID)
    {
        (void)cm_node_set_master(pLocal->sub_master_id);
    }
    return;
}

static void cm_htbt_state_enter_lonely(cm_htbt_runtime_info_t *pGlobal)
{
    cm_htbt_local_info_t *pLocal = &pGlobal->local.info;

    CM_LOG_WARNING(CM_MOD_HTBT,"Enter Lonely");

    pGlobal->recv_func = cm_htbt_state_recv_lonely;
    pGlobal->check_func = cm_htbt_state_check_lonely;
    pGlobal->lost_cnt = 0;
    pGlobal->check_cnt = 0;
    pLocal->state = CM_HTBT_STATE_LONELY;
    pLocal->sub_master_id = CM_NODE_ID_NONE;
    pLocal->duration = 0;
    pGlobal->pmaster = NULL;
    (void)cm_node_set_submaster(cm_node_get_subdomain_id(),CM_NODE_ID_NONE);
    (void)cm_node_set_master(CM_NODE_ID_NONE);
    return;
}

static void cm_htbt_state_enter_select(cm_htbt_runtime_info_t *pGlobal)
{
    cm_htbt_local_info_t *pLocal = &pGlobal->local.info;

    CM_LOG_WARNING(CM_MOD_HTBT,"Enter Select");

    pGlobal->recv_func = cm_htbt_state_recv_select;
    pGlobal->check_func = cm_htbt_state_check_select;
    pGlobal->lost_cnt = 0;
    pGlobal->check_cnt = 0;
    pLocal->state = CM_HTBT_STATE_SELECT;
    cm_htbt_master_update(pGlobal,pLocal->id);
    (void)cm_node_set_state(pLocal->sub_domain_id,pLocal->id,CM_NODE_STATE_NORMAL);
    return;
}
static void cm_htbt_state_enter_stable(cm_htbt_runtime_info_t *pGlobal)
{
    cm_htbt_local_info_t *pLocal = &pGlobal->local.info;

    CM_LOG_WARNING(CM_MOD_HTBT,"Enter Stable Master[%u]",pLocal->sub_master_id);

    pGlobal->recv_func = cm_htbt_state_recv_stable;
    pGlobal->check_func = cm_htbt_state_check_stable;
    pGlobal->lost_cnt = 0;
    pGlobal->check_cnt = 0;
    pLocal->state = CM_HTBT_STATE_STABLE;

    (void)cm_node_set_submaster(cm_node_get_subdomain_id(),pLocal->sub_master_id);
    if(pLocal->sub_domain_id == CM_MASTER_SUBDOMAIN_ID)
    {
        (void)cm_node_set_master(pLocal->sub_master_id);
    }
    return;
}

static void cm_htbt_state_check_lonely(cm_htbt_runtime_info_t *pGlobal)
{
    pGlobal->check_cnt++;
    pGlobal->lost_cnt++;

    if(0 == pGlobal->lost_cnt%10)
    {
        CM_LOG_WARNING(CM_MOD_HTBT,"lost_cnt[%u]",pGlobal->lost_cnt);
    }
    return;
}

static void cm_htbt_state_check_select(cm_htbt_runtime_info_t *pGlobal)
{
    pGlobal->check_cnt++;
    pGlobal->lost_cnt++;

    if(CM_HTBT_LONELY_NUM <= pGlobal->lost_cnt)
    {
        cm_htbt_state_enter_lonely(pGlobal);
        return;
    }

    if(CM_HTBT_STABLE_NUM <= pGlobal->check_cnt)
    {
        cm_htbt_state_enter_stable(pGlobal);
        return;
    }
}

static void cm_htbt_state_check_stable(cm_htbt_runtime_info_t *pGlobal)
{
    cm_htbt_local_info_t *pLocal = &pGlobal->local.info;

    pGlobal->check_cnt++;
    pGlobal->lost_cnt++;

    if(CM_HTBT_LONELY_NUM <= pGlobal->lost_cnt)
    {
        cm_htbt_state_enter_lonely(pGlobal);
        return;
    }

    if(pLocal->id == pLocal->sub_master_id)
    {
        pLocal->duration++;
    }
    else
    {
        pLocal->duration = 0;
    }

}

void cm_htbt_get_local_node(uint32 *ptotal, uint32 *ponline)
{
    cm_htbt_runtime_info_t *pGlobal = &g_CmHtbtGlobal;
    cm_htbt_sub_domain_info_t *pSub = &pGlobal->subdomain.info;
    cm_htbt_node_t *pnode = pSub->node;
    uint32 iloop = CM_HTBT_SUBDOMAIN_NODE_MAX;
    
    CM_MUTEX_LOCK(&pGlobal->mutex);
    *ptotal = pSub->node_cnt;
    *ponline = 0;
    while(iloop > 0)
    {
        if(CM_NODE_ID_NONE != pnode->id)
        {
            if(CM_HTBT_NODE_ONLINE == pnode->state)
            {
                (*ponline)++;
            }
        }
        iloop--;
        pnode++;
    }
    CM_MUTEX_UNLOCK(&pGlobal->mutex);
    return;
}

static sint32 cm_htbt_add_to_subdomain(cm_node_info_t *pNode, cm_htbt_sub_domain_info_t *pSubDomain)
{
    cm_htbt_node_t *pHtbtNode = cm_htbt_node_getinfo(pSubDomain,CM_NODE_ID_NONE);
    CM_LOG_INFO(CM_MOD_HTBT,"%u %s %s", pNode->id,pNode->name, pNode->ip_addr);

    if(NULL == pHtbtNode)
    {
        CM_LOG_ERR(CM_MOD_HTBT,"full");
        return CM_FAIL;
    }
    pHtbtNode->id = pNode->id;
    pHtbtNode->lost_cnt = 0;
    pHtbtNode->state = CM_HTBT_NODE_UNKNOW;
    (void)cm_node_set_state(pNode->subdomain_id,pNode->id,CM_NODE_STATE_UNKNOW);
    pSubDomain->node_cnt += 1;
    return CM_OK;
}

static sint32 cm_htbt_delete_from_subdomain(cm_node_info_t *pNode, cm_htbt_sub_domain_info_t *pSubDomain)
{
    cm_htbt_node_t *pHtbtNode = cm_htbt_node_getinfo(pSubDomain,pNode->id);
    CM_LOG_INFO(CM_MOD_HTBT,"%u %s %s", pNode->id,pNode->name, pNode->ip_addr);

    if(NULL == pHtbtNode)
    {
        CM_LOG_ERR(CM_MOD_HTBT,"not found");
        return CM_FAIL;
    }
    (void)cm_node_set_state(pNode->subdomain_id,pNode->id,CM_NODE_STATE_UNKNOW);
    pHtbtNode->id = CM_NODE_ID_NONE;
    pHtbtNode->lost_cnt = 0;
    pHtbtNode->state = CM_HTBT_NODE_UNKNOW;            
    pSubDomain->node_cnt -= 1;
    CM_LOG_INFO(CM_MOD_HTBT,"submaster[%u]",pSubDomain->sub_master_id);
    if(pNode->id == pSubDomain->sub_master_id)
    {
        cm_htbt_state_enter_select(&g_CmHtbtGlobal);
    }
    return CM_OK;
}

sint32 cm_htbt_add(uint32 subdomain,uint32 nid)
{    
    cm_htbt_runtime_info_t *pGlobal = &g_CmHtbtGlobal;
    cm_htbt_local_info_t *pLocal = &pGlobal->local.info;
    cm_htbt_sub_domain_info_t *pSubDomain = &pGlobal->subdomain.info;
    cm_node_info_t NodeInfo;
    sint32 iRet = CM_OK;
    cm_htbt_node_t *pTem;
    
    CM_LOG_WARNING(CM_MOD_HTBT,"subdomain[%u] nid[%u]",subdomain,nid);
    CM_MUTEX_LOCK(&pGlobal->mutex);
    if(nid == cm_node_get_id())
    {
        /* 添加自己的场景 */
        pLocal->sub_domain_id = subdomain;
        pLocal->state = CM_HTBT_STATE_LONELY;
        pLocal->sub_master_id = CM_NODE_ID_NONE;
        pLocal->up_time = cm_get_time();
        CM_MEM_ZERO(pSubDomain,sizeof(cm_htbt_sub_domain_info_t));
        pSubDomain->id = subdomain;
        pSubDomain->sub_master_id = CM_NODE_ID_NONE;
        iRet = cm_node_traversal_node(pSubDomain->id,(cm_node_trav_func_t)cm_htbt_init_nodes,pSubDomain,CM_TRUE);
        CM_MUTEX_UNLOCK(&pGlobal->mutex);
        cm_htbt_state_enter_lonely(pGlobal);
        return iRet;
    }

    if(subdomain == pLocal->sub_domain_id)
    {        
        iRet = cm_node_get_info(subdomain,nid,&NodeInfo);
        if(CM_OK != iRet)
        {
            CM_LOG_ERR(CM_MOD_HTBT,"get info fail subdomain[%u] nid[%u]",subdomain,nid);
            CM_MUTEX_UNLOCK(&pGlobal->mutex);
            return iRet;
        }
        iRet = cm_htbt_add_to_subdomain(&NodeInfo,pSubDomain);

        pTem = cm_htbt_node_getinfo(pSubDomain,cm_node_get_id());
        if(pTem == NULL)
        {
            pTem = cm_htbt_node_getinfo(pSubDomain,CM_NODE_ID_NONE);
            if(pTem != NULL)
            {
                pTem->id = cm_node_get_id();
                pTem->state = CM_HTBT_NODE_ONLINE;
                pTem->lost_cnt = 0;
                pSubDomain->node_cnt++;
                (void)cm_node_set_state(subdomain,cm_node_get_id(),CM_NODE_STATE_NORMAL);
            }
        }
        CM_MUTEX_UNLOCK(&pGlobal->mutex);
        return iRet;
    }
    CM_MUTEX_UNLOCK(&pGlobal->mutex);
    return CM_OK;
}

sint32 cm_htbt_delete(uint32 subdomain,uint32 nid)
{    
    cm_htbt_runtime_info_t *pGlobal = &g_CmHtbtGlobal;
    cm_htbt_local_info_t *pLocal = &pGlobal->local.info;
    cm_htbt_sub_domain_info_t *pSubDomain = &pGlobal->subdomain.info;
    cm_node_info_t NodeInfo;
    sint32 iRet = CM_OK;
    
    CM_LOG_WARNING(CM_MOD_HTBT,"subdomain[%u] nid[%u]",subdomain,nid);
    CM_MUTEX_LOCK(&pGlobal->mutex);
    if(nid == cm_node_get_id())
    {
        pLocal->sub_domain_id = subdomain;
        pLocal->state = CM_HTBT_STATE_LONELY;
        pLocal->sub_master_id = CM_NODE_ID_NONE;
        CM_MEM_ZERO(pSubDomain,sizeof(cm_htbt_sub_domain_info_t));
        cm_htbt_state_enter_lonely(pGlobal);
        CM_MUTEX_UNLOCK(&pGlobal->mutex);        
        return iRet;
    }

    if(subdomain == pLocal->sub_domain_id)
    {        
        iRet = cm_node_get_info(subdomain,nid,&NodeInfo);
        if(CM_OK != iRet)
        {
            CM_LOG_ERR(CM_MOD_HTBT,"get info fail subdomain[%u] nid[%u]",subdomain,nid);
            CM_MUTEX_UNLOCK(&pGlobal->mutex);
            return iRet;
        }
        iRet = cm_htbt_delete_from_subdomain(&NodeInfo,pSubDomain);
        CM_MUTEX_UNLOCK(&pGlobal->mutex);
        return iRet;
    }
    CM_MUTEX_UNLOCK(&pGlobal->mutex);
    return CM_OK;
}


