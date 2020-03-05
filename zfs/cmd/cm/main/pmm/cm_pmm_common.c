
/******************************************************************************
 *          Copyright (c) 2018 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_pmm_common.c
 * author     : xar
 * create date: 2018.10.24
 * description: TODO:
 *
 *****************************************************************************/
#include "cm_pmm_common.h"
#include "cm_log.h"
#include "cm_omi.h"
#include "cm_cfg_cnm.h"
#include "cm_pmm_pool.h"
#include "cm_cnm_pmm_lun.h"

#define CM_PMM_NODE_TIMEOUT 60


const cm_omi_map_cfg_t CmPmmNodeMap[] = 
{
    {"pool",CM_PMM_TYPE_POOL},
    {"nas",CM_PMM_TYPE_NAS},
    {"sas",CM_PMM_TYPE_SAS},
    {"protocol",CM_PMM_TYPE_PROTO},
    {"cache",CM_PMM_TYPE_CACHE},
	{"lun",CM_PMM_TYPE_LUN},  
    {"nic",CM_PMM_TYPE_NIC},
    {"disk",CM_PMM_TYPE_DISK}, 
    {"cifs",CM_PMM_TYPE_CIFS},
};

const cm_omi_map_enum_t CmOmiMapEnumNode =
{
    sizeof(CmPmmNodeMap)/sizeof(cm_omi_map_cfg_t),
    CmPmmNodeMap
};

const cm_pmm_config_t  g_cm_pmm_config[CM_PMM_TYPE_BUTT] =
{
    {
        CM_PMM_TYPE_POOL,
        sizeof(cm_pmm_pool_info_t),
        cm_pmm_pool_get_data,
        cm_pmm_pool_cal_data,
        cm_pmm_pool_check_data
    },
	{
        CM_PMM_TYPE_NAS,
        sizeof(cm_pmm_nas_data_t),
        cm_pmm_nas_get_data,
        cm_pmm_nas_cal_data,
        NULL
    },
    {
        CM_PMM_TYPE_SAS,
        sizeof(cm_pmm_sas_data_t),
        cm_pmm_sas_get_data,
        cm_pmm_sas_cal_data,
        cm_pmm_sas_check_data,
    },
    {
        CM_PMM_TYPE_PROTO,
        sizeof(cm_pmm_protocol_data_t),
        cm_pmm_protocol_get_data,
        cm_pmm_protocol_cal_data,
        cm_pmm_protocol_check_data
    },
    {
        CM_PMM_TYPE_CACHE,
        sizeof(cm_pmm_cache_data_t),
        cm_pmm_cache_get_data,
        cm_pmm_cache_cal_data,
        NULL
    },
	{
        CM_PMM_TYPE_LUN,
        sizeof(cm_cnm_pmm_lun_info_t),
        cm_pmm_lun_get_data,
        cm_pmm_lun_cal_data,
        NULL
    },
    {
        CM_PMM_TYPE_NIC,
        sizeof(cm_cnm_pmm_nic_info_t),
        cm_pmm_nic_get_data,
        cm_pmm_nic_cal_data,
        cm_pmm_nic_check_data
    },
    {
        CM_PMM_TYPE_DISK,
        sizeof(cm_cnm_pmm_disk_info_t),
        cm_pmm_disk_get_data,
        cm_pmm_disk_cal_data,
        NULL
    },
    {
        CM_PMM_TYPE_CIFS,
        sizeof(cm_pmm_protocol_data_t),
        cm_pmm_protocol_get_data,
        cm_pmm_protocol_cifs_cal_data,
        cm_pmm_protocol_check_data
    },
};

cm_pmm_tree_node_t* cm_pmm_node_root = NULL;
cm_mutex_t g_cm_pmm_common_mutex;

cm_pmm_config_t* cm_pmm_get_obj_cfg(uint32 ObjId)
{
    if(ObjId >= CM_PMM_TYPE_BUTT)
    {
        return NULL;
    }
    return &g_cm_pmm_config[ObjId];
}

cm_pmm_tree_node_t* cm_pmm_tree_node_new(const sint8 * name)
{
    cm_pmm_tree_node_t* pNode = (cm_pmm_tree_node_t*)CM_MALLOC(sizeof(cm_pmm_tree_node_t));
    
    if(NULL == pNode)
    {
        CM_LOG_ERR(CM_MOD_QUEUE, "Malloc fail!");
        return NULL;
    }

    CM_MEM_CPY(pNode->name,sizeof(pNode->name),name,sizeof(pNode->name));
    pNode->pbrother = NULL;
    pNode->pchildren = NULL;
    pNode->pdata = NULL;
    pNode->child_num = 0;
    return pNode;
}

sint32 cm_pmm_tree_node_add(cm_pmm_tree_node_t * pParent,cm_pmm_tree_node_t * pChild)
{
    cm_pmm_tree_node_t *pNode = pParent->pchildren;

    if(NULL == pNode)
    {
        pParent->pchildren = pChild;
        pParent->child_num++;
        return CM_OK;
    }

    while(NULL != pNode->pbrother)
    {
        pNode = pNode->pbrother;
    }

    pChild->pbrother = pNode->pbrother;
    pNode->pbrother = pChild;
    pParent->child_num++;
    return CM_OK;
}

cm_pmm_tree_node_t* cm_pmm_tree_node_addnew(cm_pmm_tree_node_t * pParent,const sint8 * name)
{
    cm_pmm_tree_node_t* pNode = cm_pmm_tree_node_new(name);
    if(NULL == pNode)
    {
        CM_LOG_ERR(CM_MOD_PMM, "Malloc fail!");
        return NULL;
    }

    (void)cm_pmm_tree_node_add(pParent,pNode);

    return pNode;
}

sint32 cm_pmm_tree_node_delete(cm_pmm_tree_node_t *pParent,cm_pmm_tree_node_t *pChild)
{
    cm_pmm_tree_node_t *pNode = pParent->pchildren;


    if(NULL != pChild->pdata)
    {
        CM_LOG_ERR(CM_MOD_PMM, "Node pData != NULL!");
        return CM_FAIL;
    }

    if(NULL != pChild->pchildren)
    {
        CM_LOG_ERR(CM_MOD_PMM, "Node pChildren != NULL!");
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

    return CM_FAIL;
}

sint32 cm_pmm_tree_node_free(cm_pmm_tree_node_t * pNode)
{
    CM_FREE(pNode);
    return CM_OK;
}

sint32 cm_pmm_tree_destory(cm_pmm_tree_node_t * pRoot)
{
    sint32 iRet = CM_FAIL;

    if(NULL != pRoot->pdata)
    {
        CM_LOG_ERR(CM_MOD_QUEUE, "pData != NULL!");
        return CM_FAIL;
    }

    if(NULL != pRoot->pbrother)
    {
        iRet = cm_pmm_tree_destory(pRoot->pbrother);

        if(CM_OK != iRet)
        {
            CM_LOG_ERR(CM_MOD_QUEUE, "[%s] destory fail!", pRoot->name);
            return iRet;
        }

        pRoot->pbrother = NULL;
    }

    if(NULL != pRoot->pchildren)
    {
        iRet = cm_pmm_tree_destory(pRoot->pchildren);

        if(CM_OK != iRet)
        {
            CM_LOG_ERR(CM_MOD_QUEUE, "[%s] destory fail!", pRoot->name);
            return iRet;
        }

        pRoot->pchildren = NULL;
    }

    CM_FREE(pRoot);
    return CM_OK;
}

cm_pmm_tree_node_t* cm_pmm_tree_node_find(cm_pmm_tree_node_t * pParent,const sint8 * name)
{
     cm_pmm_tree_node_t *pNode = pParent->pchildren;

    while(NULL != pNode)
    {
        if(0 == strcmp(name,pNode->name))
        {
            return pNode;
        }

        pNode = pNode->pbrother;
    }

    return NULL;
}

void cm_pmm_tree_node_update(const cm_pmm_tree_node_t* root)
{
    cm_pmm_tree_node_t* pNode = root->pchildren;

    if(NULL == pNode)
    {
        return;
    }

    
    /*遍历一级节点*/
    while(NULL != pNode)
    {
        cm_pmm_tree_node_t *ppNode = pNode->pchildren;
        cm_pmm_config_t* pobj = cm_pmm_get_obj_cfg(pNode->type);
        cm_pmm_data_t *pData = NULL;
        /*遍历二级节点*/
        while(NULL != ppNode)
        {
            cm_pmm_common_cincfog_t* cincfog_t = (cm_pmm_common_cincfog_t*)ppNode->pdata;
            cincfog_t->timeout += 5;
          
            /*一分钟没有get操作删除节点*/
            if(cincfog_t->timeout >= CM_PMM_NODE_TIMEOUT)
            {
                cm_pmm_tree_node_t *pNext = ppNode->pbrother;
                
				CM_FREE(cincfog_t);
                ppNode->pdata = NULL;
                cm_pmm_tree_node_delete(pNode,ppNode);

                ppNode = pNext;
                continue;
            }
           
            /*update数据*/
            CM_MEM_CPY(cincfog_t->olddata,pobj->size,cincfog_t->newdata,pobj->size);
            pobj->get_data(ppNode->name,cincfog_t->newdata);

            /*update statdata数据*/
            CM_MUTEX_LOCK(&g_cm_pmm_common_mutex);
            pobj->cal_data(cincfog_t->olddata,cincfog_t->newdata,cincfog_t->statdata);
            pData = (cm_pmm_data_t*)cincfog_t->statdata;
            pData->tmstamp = cm_get_time();
            CM_MUTEX_UNLOCK(&g_cm_pmm_common_mutex);
            
            ppNode = ppNode->pbrother;
        }

        pNode = pNode->pbrother;
    }

    return;
}

static sint32 cm_pmm_common_add(uint32 type,const sint8* name)
{
    cm_pmm_tree_node_t* pNode = NULL;
    cm_pmm_tree_node_t* ppNode = NULL;
    cm_pmm_common_cincfog_t *cincfog = NULL;
    sint32 iRet = CM_OK;
    const sint8* type_name = cm_cnm_get_enum_str(&CmOmiMapEnumNode,type);
    cm_pmm_config_t* pobj = cm_pmm_get_obj_cfg(type);
    
    if(NULL != pobj->check_data)
    {
        iRet =  pobj->check_data(name);
        if(CM_OK != iRet)
        {
            CM_LOG_ERR(CM_MOD_PMM,"%s:%s no exsit",type_name,name);
            return CM_FAIL;
        }
    }
    
    pNode = cm_pmm_tree_node_find(cm_pmm_node_root,type_name);
    if(NULL == pNode)
    {
        /*增加新的一级节点*/
        CM_MUTEX_LOCK(&g_cm_pmm_common_mutex);
        pNode = cm_pmm_tree_node_addnew(cm_pmm_node_root,type_name);
        CM_MUTEX_UNLOCK(&g_cm_pmm_common_mutex);
        if(NULL == pNode)
        {
            CM_LOG_ERR(CM_MOD_PMM,"create node fail");
            return CM_FAIL;
        }

        pNode->type = type;
    }

    /*增加新的二级节点*/
    CM_MUTEX_LOCK(&g_cm_pmm_common_mutex);
    ppNode = cm_pmm_tree_node_addnew(pNode,name);
    if(NULL == ppNode)
    {
        CM_LOG_ERR(CM_MOD_PMM,"create node fail");
        CM_MUTEX_UNLOCK(&g_cm_pmm_common_mutex);
        return CM_FAIL;
    }
   
    cincfog = CM_MALLOC(sizeof(cm_pmm_common_cincfog_t) + (3 * pobj->size));
    if(NULL == cincfog)
    {
        cm_pmm_tree_node_delete(pNode,ppNode);
        CM_LOG_ERR(CM_MOD_PMM,"mollac fail");
        CM_MUTEX_UNLOCK(&g_cm_pmm_common_mutex);
        return CM_FAIL;
    }
    
    cincfog->olddata = (uint8*)cincfog + sizeof(cm_pmm_common_cincfog_t);
    cincfog->newdata = (uint8 *)cincfog->olddata + pobj->size;
    cincfog->statdata= (uint8 *)cincfog->newdata + pobj->size;
    CM_MEM_ZERO(cincfog->olddata,(3 * pobj->size));
    cincfog->timeout = 0;
        
    pobj->get_data(name,cincfog->newdata);
    ppNode->pdata = (void *)cincfog;
    CM_MUTEX_UNLOCK(&g_cm_pmm_common_mutex);
    return CM_OK;
}

static sint32 cm_pmm_conmmon_find(uint32 type,const sint8* name,void* data)
{
    cm_pmm_tree_node_t* pNode = NULL;
    cm_pmm_common_cincfog_t *cincfog_t = NULL;
    const sint8* type_name = cm_cnm_get_enum_str(&CmOmiMapEnumNode,type);
    cm_pmm_config_t* pobj = cm_pmm_get_obj_cfg(type);

    pNode = cm_pmm_tree_node_find(cm_pmm_node_root,type_name);
    if(NULL == pNode)
    {
        CM_LOG_ERR(CM_MOD_PMM,"find type[%s] fail",type_name);
        return CM_FAIL;
    }

    pNode = cm_pmm_tree_node_find(pNode,name);
    if(NULL == pNode)
    {
        CM_LOG_ERR(CM_MOD_PMM,"find node[%s] fail",name);
        return CM_FAIL;
    }

    /*获取节点保存的数据*/
    cincfog_t = (cm_pmm_common_cincfog_t *)pNode->pdata;

    /*重置超时时间*/
    cincfog_t->timeout = 0;
    CM_MUTEX_LOCK(&g_cm_pmm_common_mutex);
    CM_MEM_CPY(data,pobj->size,cincfog_t->statdata,pobj->size);
    CM_MUTEX_UNLOCK(&g_cm_pmm_common_mutex);

    return CM_OK;
}

sint32 cm_pmm_common_get_data(uint32 type,const sint8* name,void *data)
{
    sint32 iRet = CM_OK;

    if(NULL == cm_pmm_node_root)
    {
        CM_LOG_ERR(CM_MOD_CNM,"root fail");
        return CM_FAIL;
    }

    iRet =  cm_pmm_conmmon_find(type,name,data);
    if(CM_OK != iRet)
    {
        iRet = cm_pmm_common_add(type,name);
        if(CM_OK != iRet)
        {
            CM_LOG_ERR(CM_MOD_PMM,"add new node fail");
            return CM_FAIL;
        }
        return CM_OK;
    }
    
    return CM_OK;
}

