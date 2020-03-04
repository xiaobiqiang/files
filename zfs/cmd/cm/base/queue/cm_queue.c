/******************************************************************************
 *          Copyright (c) 2017 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_queue.c
 * author     : wbn
 * create date: 2017年10月25日
 * description: TODO:
 *
 *****************************************************************************/

#include "cm_queue.h"
#include "cm_log.h"

/******************************************************************************
 * function     : cm_queue_init
 * description  : 队列初始化
 *
 * parameter in : uint32 len
 *
 * parameter out: cm_queue_t **ppQueue
 *
 * return type  : sint32
 *
 * author       : wbn
 * create date  : 2017.10.27
 *****************************************************************************/
sint32
cm_queue_init(cm_queue_t **ppQueue, uint32 len)
{
    cm_queue_t *pQueue = NULL;

    pQueue = (cm_queue_t*)CM_MALLOC(sizeof(cm_queue_t) + (sizeof(void*)*len));

    if(NULL == pQueue)
    {
        return CM_FAIL;
    }

    pQueue->read_index = 0;
    pQueue->write_index = 0;
    pQueue->count = 0;
    pQueue->max_count = len;

    CM_MUTEX_INIT(&pQueue->mutex);
    CM_SEM_INIT(&pQueue->sem, 0);

    *ppQueue = pQueue;
    return CM_OK;
}

/******************************************************************************
 * function     : cm_queue_destroy
 * description  : 销毁消息队列
 *
 * parameter in : cm_queue_t *pQueue
 *
 * parameter out: 无
 *
 * return type  : 无
 *
 * author       : wbn
 * create date  : 2017.10.27
 *****************************************************************************/
void
cm_queue_destroy(cm_queue_t *pQueue)
{
    CM_SEM_DESTROY(&pQueue->sem);
    CM_MUTEX_DESTROY(&pQueue->mutex);
    CM_FREE(pQueue);
    return;
}

/******************************************************************************
 * function     : cm_queue_add
 * description  : 向队列中添加消息
 *
 * parameter in : cm_queue_t *pQueue
 *                void *pMsg
 * parameter out: 无
 *
 * return type  : sint32
 *
 * author       : wbn
 * create date  : 2017.10.27
 *****************************************************************************/
sint32
cm_queue_add(cm_queue_t *pQueue, void *pMsg)
{
    CM_MUTEX_LOCK(&pQueue->mutex);

    if(pQueue->count == pQueue->max_count)
    {
        CM_MUTEX_UNLOCK(&pQueue->mutex);
        return CM_FAIL;
    }

    pQueue->pdata[pQueue->write_index] = pMsg;
    pQueue->write_index++;
    pQueue->count++;

    if(pQueue->write_index == pQueue->max_count)
    {
        pQueue->write_index = 0;
    }

    CM_MUTEX_UNLOCK(&pQueue->mutex);

    CM_SEM_POST(&pQueue->sem);
    return CM_OK;
}

/******************************************************************************
 * function     : cm_queue_get
 * description  : 从队列中取出消息
 *
 * parameter in : cm_queue_t *pQueue
 *
 * parameter out: void **ppMsg
 *
 * return type  : sint32
 *
 * author       : wbn
 * create date  : 2017.10.27
 *****************************************************************************/
sint32
cm_queue_get(cm_queue_t *pQueue, void **ppMsg)
{
    CM_SEM_WAIT(&pQueue->sem);

    CM_MUTEX_LOCK(&pQueue->mutex);

    if(0 == pQueue->count)
    {
        CM_MUTEX_UNLOCK(&pQueue->mutex);
        return CM_FAIL;
    }

    *ppMsg = pQueue->pdata[pQueue->read_index];
    pQueue->pdata[pQueue->read_index] = NULL;

    pQueue->read_index++;
    pQueue->count--;

    if(pQueue->read_index == pQueue->max_count)
    {
        pQueue->read_index = 0;
    }

    CM_MUTEX_UNLOCK(&pQueue->mutex);
    return CM_OK;
}

/******************************************************************************
 * function     : cm_list_init
 * description  : 链表初始化
 *
 * parameter in : uint32 MaxCount
 *
 * parameter out: cm_list_t **ppList
 *
 * return type  : sint32
 *
 * author       : wbn
 * create date  : 2017.10.27
 *****************************************************************************/
sint32
cm_list_init(cm_list_t **ppList, uint32 MaxCount)
{
    uint32 Len = sizeof(cm_list_t) + sizeof(void*)*MaxCount;
    cm_list_t *pList = NULL;

    pList = (cm_list_t*)CM_MALLOC(Len);

    if(NULL == pList)
    {
        CM_LOG_ERR(CM_MOD_QUEUE, "malloc fail!");
        return CM_FAIL;
    }

    CM_MEM_ZERO(pList, Len);
    CM_MUTEX_INIT(&pList->mutex);
    pList->max_count = MaxCount;
    *ppList = pList;
    return CM_OK;
}

/******************************************************************************
 * function     : cm_list_add
 * description  : 在链表中添加数据
 *
 * parameter in : cm_list_t *pList
 *                void *pData
 * parameter out: 无
 *
 * return type  : sint32
 *
 * author       : wbn
 * create date  : 2017.10.27
 *****************************************************************************/
sint32
cm_list_add(cm_list_t *pList, void *pData)
{
    uint32 iLoop = pList->max_count;
    void **ppTemp = pList->pdata;

    CM_MUTEX_LOCK(&pList->mutex);

    while(iLoop > 0)
    {
        if(NULL == *ppTemp)
        {
            *ppTemp = pData;
            CM_MUTEX_UNLOCK(&pList->mutex);
            return CM_OK;
        }

        iLoop--;
        ppTemp++;
    }

    CM_MUTEX_UNLOCK(&pList->mutex);
    CM_LOG_ERR(CM_MOD_QUEUE, "List Full! Count=%u", pList->max_count);
    return CM_FAIL;
}

/******************************************************************************
 * function     : cm_list_find
 * description  : 查找链表
 *
 * parameter in : cm_list_t *pList
 *                cm_list_cbk_func_t FindFunc
 *                *pFindArg
 * parameter out: void **ppData
 *
 * return type  : sint32
 *
 * author       : wbn
 * create date  : 2017.10.27
 *****************************************************************************/
sint32 
cm_list_find(cm_list_t *pList, cm_list_cbk_func_t FindFunc, void
                    *pFindArg, void **ppData)
{
    uint32 iLoop = pList->max_count;
    void **ppTemp = pList->pdata;

    CM_MUTEX_LOCK(&pList->mutex);

    while(iLoop > 0)
    {
        if((NULL != *ppTemp) && (CM_OK == FindFunc(*ppTemp, pFindArg)))
        {
            *ppData = *ppTemp;
            CM_MUTEX_UNLOCK(&pList->mutex);
            return CM_OK;
        }

        iLoop--;
        ppTemp++;
    }

    CM_MUTEX_UNLOCK(&pList->mutex);

    CM_LOG_INFO(CM_MOD_QUEUE, "Not Found!");
    return CM_FAIL;
}

/******************************************************************************
 * function     : cm_list_find_delete
 * description  : 查找数据，找到后删除
 *
 * parameter in : cm_list_t *pList
 *                cm_list_cbk_func_t FindFunc
 *                *pFindArg
 * parameter out: void **ppData
 *
 * return type  : sint32
 *
 * author       : wbn
 * create date  : 2017.10.27
 *****************************************************************************/
sint32
cm_list_find_delete(cm_list_t *pList, cm_list_cbk_func_t FindFunc,
                    void *pFindArg, void **ppData)
{
    uint32 iLoop = pList->max_count;
    void **ppTemp = pList->pdata;

    CM_MUTEX_LOCK(&pList->mutex);

    while(iLoop > 0)
    {
        if((NULL != *ppTemp) && (CM_OK == FindFunc(*ppTemp, pFindArg)))
        {
            *ppData = *ppTemp;
            *ppTemp = NULL;
            CM_MUTEX_UNLOCK(&pList->mutex);
            return CM_OK;
        }

        iLoop--;
        ppTemp++;
    }

    CM_MUTEX_UNLOCK(&pList->mutex);

    CM_LOG_INFO(CM_MOD_QUEUE, "Not Found!");
    return CM_FAIL;
}

sint32
cm_list_check_delete(cm_list_t *pList, cm_list_cbk_func_t CheckFunc, void *pArg)
{
	uint32 iLoop = pList->max_count;
    void **ppTemp = pList->pdata;

    CM_MUTEX_LOCK(&pList->mutex);

    while(iLoop > 0)
    {
        if((NULL != *ppTemp) && (CM_OK == CheckFunc(*ppTemp, pArg)))
        {
            *ppTemp = NULL;
        }

        iLoop--;
        ppTemp++;
    }

    CM_MUTEX_UNLOCK(&pList->mutex);

    return CM_OK;
}

/******************************************************************************
 * function     : cm_list_delete
 * description  : 从链表中删除指定数据
 *
 * parameter in : cm_list_t *pList
 *                void *pData
 * parameter out: 无
 *
 * return type  : sint32
 *
 * author       : wbn
 * create date  : 2017.10.27
 *****************************************************************************/
sint32
cm_list_delete(cm_list_t *pList, void *pData)
{
    uint32 iLoop = pList->max_count;
    void **ppTemp = pList->pdata;

    CM_MUTEX_LOCK(&pList->mutex);

    while(iLoop > 0)
    {
        if(pData == *ppTemp)
        {
            *ppTemp = NULL;
            CM_MUTEX_UNLOCK(&pList->mutex);
            return CM_OK;
        }

        iLoop--;
        ppTemp++;
    }

    CM_MUTEX_UNLOCK(&pList->mutex);

    CM_LOG_INFO(CM_MOD_QUEUE, "Not Find!");
    return CM_FAIL;
}

/******************************************************************************
 * function     : cm_list_destory
 * description  : 销毁链表
 *
 * parameter in : cm_list_t *pList
 *
 * parameter out: 无
 *
 * return type  : 无
 *
 * author       : wbn
 * create date  : 2017.10.27
 *****************************************************************************/
void 
cm_list_destory(cm_list_t *pList)
{
    CM_MUTEX_DESTROY(&pList->mutex);
    CM_FREE(pList);
    return;
}

/******************************************************************************
 * function     : cm_tree_node_new
 * description  : 分配节点
 *
 * parameter in : uint32 Id
 *
 * parameter out: 无
 *
 * return type  : cm_tree_node_t*
 *
 * author       : wbn
 * create date  : 2017.10.27
 *****************************************************************************/
cm_tree_node_t*
cm_tree_node_new(uint32 Id)
{
    cm_tree_node_t *pNode = (cm_tree_node_t*)CM_MALLOC(sizeof(cm_tree_node_t));

    if(NULL == pNode)
    {
        CM_LOG_ERR(CM_MOD_QUEUE, "Malloc fail!");
        return NULL;
    }

    pNode->id = Id;
    pNode->pbrother = NULL;
    pNode->pchildren = NULL;
    pNode->pdata = NULL;
    pNode->child_num = 0;
    return pNode;
}

/******************************************************************************
 * function     : cm_tree_node_find
 * description  : 通过ID查找节点
 *
 * parameter in : cm_tree_node_t *pParent
 *                uint32 Id
 * parameter out: 无
 *
 * return type  : cm_tree_node_t*
 *
 * author       : wbn
 * create date  : 2017.10.27
 *****************************************************************************/
cm_tree_node_t* 
cm_tree_node_find(cm_tree_node_t *pParent, uint32 Id)
{
    cm_tree_node_t *pNode = NULL;

    if(NULL == pParent)
    {
        return NULL;
    }
    pNode = pParent->pchildren;

    while(NULL != pNode)
    {
        if(Id == pNode->id)
        {
            return pNode;
        }

        pNode = pNode->pbrother;
    }

    return NULL;
}

/******************************************************************************
 * function     : cm_tree_node_add
 * description  : 添加子节点
 *
 * parameter in : cm_tree_node_t *pParent
 *                cm_tree_node_t *pChild
 * parameter out: 无
 *
 * return type  : sint32
 *
 * author       : wbn
 * create date  : 2017.10.27
 *****************************************************************************/
sint32 
cm_tree_node_add(cm_tree_node_t *pParent,
                        cm_tree_node_t *pChild)
{
    cm_tree_node_t *pNode = NULL;

    if(NULL == pParent)
    {
        return CM_FAIL;
    }
    pNode = pParent->pchildren;

    CM_LOG_DEBUG(CM_MOD_QUEUE, "Add Parent[%u] Child[%d]", pParent->id, pChild->id);

    if(NULL == pNode)
    {
        pParent->pchildren = pChild;
        pParent->child_num++;
        return CM_OK;
    }

    if(pChild->id < pNode->id)
    {
        pChild->pbrother = pNode;
        pParent->pchildren = pChild;
        pParent->child_num++;
        return CM_OK;
    }

    if(pChild->id == pNode->id)
    {
        CM_LOG_ERR(CM_MOD_QUEUE, "NodeId [%u] exitted!", pNode->id);
        return CM_FAIL;
    }

    while(NULL != pNode->pbrother)
    {
        if(pChild->id < pNode->pbrother->id)
        {
            break;
        }

        if(pChild->id == pNode->pbrother->id)
        {
            CM_LOG_ERR(CM_MOD_QUEUE, "NodeId [%u] exitted!", pChild->id);
            return CM_FAIL;
        }

        pNode = pNode->pbrother;
    }

    pChild->pbrother = pNode->pbrother;
    pNode->pbrother = pChild;
    pParent->child_num++;
    return CM_OK;
}

/******************************************************************************
 * function     : cm_tree_node_addnew
 * description  : 分配并添加一个子节点
 *
 * parameter in : cm_tree_node_t *pParent
 *                uint32 Id
 * parameter out: 无
 *
 * return type  : cm_tree_node_t*
 *
 * author       : wbn
 * create date  : 2017.10.27
 *****************************************************************************/
cm_tree_node_t*
cm_tree_node_addnew(cm_tree_node_t *pParent, uint32 Id)
{
    sint32 iRet = CM_FAIL;
    cm_tree_node_t *pNode = cm_tree_node_find(pParent, Id);

    if(NULL != pNode)
    {
        CM_LOG_INFO(CM_MOD_QUEUE, "NodeId [%u] existed!", Id);
        return pNode;
    }

    pNode = cm_tree_node_new(Id);

    if(NULL == pNode)
    {
        CM_LOG_ERR(CM_MOD_QUEUE, "NodeId [%u] New Fail!", Id);
        return NULL;
    }

    iRet = cm_tree_node_add(pParent, pNode);

    if(iRet != CM_OK)
    {
        CM_LOG_ERR(CM_MOD_QUEUE, "NodeId [%u] Add Fail[%d]!", Id, iRet);
        cm_tree_node_free(pNode);
        return NULL;
    }

    return pNode;
}

/******************************************************************************
 * function     : cm_tree_node_delete
 * description  : 节点删除
 *
 * parameter in : cm_tree_node_t *pParent
 *                cm_tree_node_t *pChild
 * parameter out: 无
 *
 * return type  : sint32
 *
 * author       : wbn
 * create date  : 2017.10.27
 *****************************************************************************/
sint32
cm_tree_node_delete(cm_tree_node_t *pParent,
                    cm_tree_node_t *pChild)
{
    cm_tree_node_t *pNode = NULL;

    if(NULL == pParent)
    {
        return CM_FAIL;
    }
    pNode = pParent->pchildren;

    CM_LOG_DEBUG(CM_MOD_QUEUE, "Delete Parent[%u] Child[%d]", pParent->id, pChild->id);

    if(NULL != pChild->pdata)
    {
        CM_LOG_ERR(CM_MOD_QUEUE, "NodeId [%u] pData != NULL!", pChild->id);
        return CM_FAIL;
    }

    if(NULL != pChild->pchildren)
    {
        CM_LOG_ERR(CM_MOD_QUEUE, "NodeId [%u] pChildren != NULL!", pChild->id);
        return CM_FAIL;
    }

    if(pNode == pChild)
    {
        pParent->pchildren = pChild->pbrother;
        CM_FREE(pChild);
        pParent->child_num--;
        return CM_OK;
    }

    while(NULL != pNode->pbrother)
    {
        if(pNode->pbrother == pChild)
        {
            pNode->pbrother = pChild->pbrother;
            CM_FREE(pChild);
            pParent->child_num--;
            return CM_OK;
        }

        pNode = pNode->pbrother;
    }

    CM_LOG_ERR(CM_MOD_QUEUE, "ParentId [%u] NodeId [%u] Not Found!",
               pParent->id, pChild->id);
    return CM_FAIL;
}

/******************************************************************************
 * function     : cm_tree_node_free
 * description  : 释放节点
 *
 * parameter in : cm_tree_node_t *pNode
 *
 * parameter out: 无
 *
 * return type  : sint32
 *
 * author       : wbn
 * create date  : 2017.10.27
 *****************************************************************************/
sint32 
cm_tree_node_free(cm_tree_node_t *pNode)
{
    CM_LOG_DEBUG(CM_MOD_QUEUE, "Delete pNode[%d]", pNode->id);

    if(NULL != pNode->pdata)
    {
        CM_LOG_ERR(CM_MOD_QUEUE, "NodeId [%u] pData != NULL!", pNode->id);
        return CM_FAIL;
    }

    if(NULL != pNode->pchildren)
    {
        CM_LOG_ERR(CM_MOD_QUEUE, "NodeId [%u] pChildren != NULL!", pNode->id);
        return CM_FAIL;
    }

    CM_FREE(pNode);
    return CM_OK;
}

/******************************************************************************
 * function     : cm_tree_destory
 * description  : 销毁节点树
 *
 * parameter in : cm_tree_node_t *pRoot
 *
 * parameter out: 无
 *
 * return type  : sint32
 *
 * author       : wbn
 * create date  : 2017.10.27
 *****************************************************************************/
sint32
cm_tree_destory(cm_tree_node_t *pRoot)
{
    sint32 iRet = CM_FAIL;

    if(NULL != pRoot->pdata)
    {
        CM_LOG_ERR(CM_MOD_QUEUE, "NodeId [%u] pData != NULL!", pRoot->id);
        return CM_FAIL;
    }

    if(NULL != pRoot->pbrother)
    {
        iRet = cm_tree_destory(pRoot->pbrother);

        if(CM_OK != iRet)
        {
            CM_LOG_ERR(CM_MOD_QUEUE, "NodeId [%u] destory fail!", pRoot->id);
            return iRet;
        }

        pRoot->pbrother = NULL;
    }

    if(NULL != pRoot->pchildren)
    {
        iRet = cm_tree_destory(pRoot->pchildren);

        if(CM_OK != iRet)
        {
            CM_LOG_ERR(CM_MOD_QUEUE, "NodeId [%u] destory fail!", pRoot->id);
            return iRet;
        }

        pRoot->pchildren = NULL;
    }

    CM_FREE(pRoot);
    CM_LOG_DEBUG(CM_MOD_QUEUE, "NodeId [%u] delete OK!", pRoot->id);
    return CM_OK;
}

/******************************************************************************
 * function     : cm_tree_node_traversal
 * description  : 遍历子节点
 *
 * parameter in : cm_tree_node_t *pParent
 *                cm_list_cbk_func_t ExecFunc
 *                void *pArg
 *                bool_t FailBreak
 * parameter out: 无
 *
 * return type  : sint32
 *
 * author       : wbn
 * create date  : 2017.10.27
 *****************************************************************************/
sint32
cm_tree_node_traversal(cm_tree_node_t *pParent,
                       cm_list_cbk_func_t ExecFunc, void *pArg, bool_t FailBreak)
{
    sint32 iRet = CM_OK;
    cm_tree_node_t *pNode = NULL;

    if(NULL == pParent)
    {
        return CM_FAIL;
    }
    pNode = pParent->pchildren;

    while(NULL != pNode)
    {
        iRet = ExecFunc(pNode->pdata, pArg);

        if((CM_OK != iRet) && (CM_TRUE == FailBreak))
        {
            break;
        }

        pNode = pNode->pbrother;
    }
    if(CM_FALSE == FailBreak)
    {
        return CM_OK;
    }
    return iRet;
}

