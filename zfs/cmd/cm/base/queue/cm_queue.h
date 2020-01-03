/******************************************************************************
 *          Copyright (c) 2017 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_queue.h
 * author     : wbn
 * create date: 2017年10月25日
 * description: TODO:
 *
 *****************************************************************************/
#ifndef BASE_QUEUE_CM_QUEUE_H_
#define BASE_QUEUE_CM_QUEUE_H_

#include "cm_common.h"

typedef struct
{
    cm_mutex_t mutex;
    cm_sem_t sem;
    uint32 read_index;
    uint32 write_index;
    uint32 max_count;
    uint32 count;
    void *pdata[];
} cm_queue_t;

typedef struct
{
    cm_mutex_t mutex;
    uint32 max_count;
    void *pdata[];
} cm_list_t;

typedef struct cm_tree_node_tx
{
    struct cm_tree_node_tx *pbrother;
    struct cm_tree_node_tx *pchildren;
    uint32 id;
    uint32 child_num;
    void *pdata;
} cm_tree_node_t;

typedef sint32 (*cm_list_cbk_func_t)(void *pNodeData, void *pArg);

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
extern sint32 
cm_queue_init(cm_queue_t **ppQueue, uint32 len);

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
extern sint32 
cm_queue_add(cm_queue_t *pQueue, void *pMsg);

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
extern sint32 
cm_queue_get(cm_queue_t *pQueue, void **ppMsg);

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
extern void 
cm_queue_destroy(cm_queue_t *pQueue);

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
extern sint32 
cm_list_init(cm_list_t **ppList, uint32 MaxCount);

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
extern sint32 
cm_list_add(cm_list_t *pList, void *pData);

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
extern sint32 
cm_list_find(cm_list_t *pList, cm_list_cbk_func_t FindFunc, void
                        *pFindArg, void **ppData);

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
extern sint32 
cm_list_find_delete(cm_list_t *pList,
    cm_list_cbk_func_t FindFunc, void *pFindArg, void **ppData);

extern sint32
cm_list_check_delete(cm_list_t *pList, cm_list_cbk_func_t CheckFunc,void *pArg);
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
extern sint32 
cm_list_delete(cm_list_t *pList, void *pData);

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
extern void 
cm_list_destory(cm_list_t *pList);

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
extern cm_tree_node_t* 
cm_tree_node_new(uint32 Id);

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
extern cm_tree_node_t* 
cm_tree_node_addnew(cm_tree_node_t *pParent, uint32 Id);

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
extern cm_tree_node_t* 
cm_tree_node_find(cm_tree_node_t *pParent, uint32 Id);

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
extern sint32 
cm_tree_node_add(cm_tree_node_t *pParent,
                            cm_tree_node_t *pChild);

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
extern sint32 
cm_tree_node_delete(cm_tree_node_t *pParent,
                               cm_tree_node_t *pChild);

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
extern sint32 
cm_tree_node_free(cm_tree_node_t *pNode);

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
extern sint32 
cm_tree_destory(cm_tree_node_t *pRoot);

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
extern sint32 
cm_tree_node_traversal(cm_tree_node_t *pParent, cm_list_cbk_func_t
ExecFunc, void *pArg, bool_t FailBreak);



#endif /* BASE_QUEUE_CM_QUEUE_H_ */

