/******************************************************************************
 *          Copyright (c) 2018 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_xml.c
 * author     : wbn
 * create date: 2018.04.25
 * description: TODO:
 *
 *****************************************************************************/
#include "mxml.h"
#include "cm_common.h"
#include "cm_xml.h"
#include "cm_log.h"

sint32 cm_xml_request(cm_xml_request_param_t *req)
{
    sint32 iRet = CM_OK;
    uint32 offset = 0;
    uint32 cnt = 0;
    mxml_node_t* node = NULL;
    mxml_node_t* rootx = req->root;
    uint8 *pEach = NULL;
    uint8 *pData = CM_MALLOC(req->total * req->eachsize);

    if(NULL == pData)
    {
        CM_LOG_ERR(CM_MOD_NONE,"malloc fail");
        return CM_FAIL;
    }
    
    pEach = pData;
    for(node = mxmlFindElement(rootx,rootx,req->nodename,req->attr,req->value,MXML_DESCEND);
        (NULL != node) && (cnt < req->total); 
        node = mxmlFindElement(node,rootx,req->nodename,req->attr,req->value,MXML_DESCEND))
    {
        if(NULL != req->check)
        {
            iRet = req->check(node,req->param);
            if(CM_OK != iRet)
            {
                continue;
            }
        }
        if(offset < req->offset)
        {
            offset++;
            continue;
        }

        iRet = req->geteach(node,pEach);
        if(iRet != CM_OK)
        {
            continue;
        }
        cnt++;
        pEach += req->eachsize;
    }

    if(cnt > 0)
    {
        *(req->ppAck) = pData;
        *(req->pAckLen) = cnt * req->eachsize;
    }
    else
    {
        CM_FREE(pData);
        *(req->ppAck) = NULL;
        *(req->pAckLen) = 0;
    }
    return CM_OK;
}

sint32 cm_xml_request_count(cm_xml_request_param_t *req)
{
    sint32 iRet = CM_OK;
    mxml_node_t* node = NULL;
    mxml_node_t* rootx = req->root;
    uint64 *pData = CM_MALLOC(sizeof(uint64));

    if(NULL == pData)
    {
        CM_LOG_ERR(CM_MOD_NONE,"malloc fail");
        return CM_FAIL;
    }
    
    *pData = 0;
    for(node = mxmlFindElement(rootx,rootx,req->nodename,req->attr,req->value,MXML_DESCEND);
        NULL != node; 
        node = mxmlFindElement(node,rootx,req->nodename,req->attr,req->value,MXML_DESCEND))
    {
        if(NULL != req->check)
        {
            iRet = req->check(node,req->param);
            if(CM_OK != iRet)
            {
                continue;
            }
        }
        (*pData)++;
    }
    
    *(req->ppAck) = pData;
    *(req->pAckLen) = sizeof(uint64);
    return CM_OK;
}


sint32 cm_xml_find_node(cm_xml_node_ptr root,
    const sint8* nodename, const sint8* attr, const sint8* value,
    cm_xml_cbk_t cbk,void *arg)
{
    sint32 iRet = CM_OK;
    mxml_node_t* node=NULL;
    mxml_node_t* rootx = root;

    for(node = mxmlFindElement(rootx,rootx,nodename,attr,value,MXML_DESCEND);
        NULL != node; 
        node = mxmlFindElement(node,rootx,nodename,attr,value,MXML_DESCEND))
    {
        iRet = cbk(node,arg);
        if(CM_OK != iRet)
        {
            break;
        }
    }
    return CM_OK;
}

cm_xml_node_ptr cm_xml_get_node(cm_xml_node_ptr root,
    const sint8* nodename, const sint8* attr, const sint8* value)
{
    return mxmlFindElement(root,root,nodename,attr,value,MXML_DESCEND);
}

const sint8* cm_xml_get_text(cm_xml_node_ptr node)
{
    int tmp = 0;
    return mxmlGetText((mxml_node_t*)node,&tmp);
}

const sint8* cm_xml_get_child_text(cm_xml_node_ptr node,const sint8* child)
{
    node = cm_xml_find_path(node,child);
    if(NULL != node)
    {
        return cm_xml_get_text(node);
    }
    return NULL;
}

const sint8* cm_xml_get_name(cm_xml_node_ptr node)
{
    return mxmlGetElement((mxml_node_t*)node);
}

const sint8* cm_xml_get_attr(cm_xml_node_ptr node, const sint8* name)
{
    return mxmlElementGetAttr((mxml_node_t*)node,name);
}

cm_xml_node_ptr cm_xml_get_child(cm_xml_node_ptr node)
{
    return mxmlGetFirstChild((mxml_node_t*)node);
}

cm_xml_node_ptr cm_xml_get_next_sibling(cm_xml_node_ptr node)
{
    return mxmlGetNextSibling((mxml_node_t*)node);
}

cm_xml_node_ptr cm_xml_get_parent(cm_xml_node_ptr node)
{
    return mxmlGetParent((mxml_node_t*)node);
}

cm_xml_node_ptr cm_xml_find_path(cm_xml_node_ptr node, const sint8* path)
{
    return mxmlFindPath((mxml_node_t*)node,path);
}

sint32 cm_xml_get_file(const sint8* filename,const sint8* nodename,cm_xml_cbk_t cbk,void *arg)
{
    sint32 iRet = CM_FAIL;
    mxml_node_t* root = NULL;
    FILE *fp = fopen(filename,"r");
    if(NULL == fp)
    {
        return CM_FAIL;
    }
    root = mxmlLoadFile(NULL,fp,NULL);
    fclose(fp);
    if(NULL == root)
    {
        return CM_FAIL;
    }
    iRet = cm_xml_find_node((cm_xml_node_ptr)root,nodename,NULL,NULL,cbk,arg);
    mxmlDelete(root);
    return iRet;
}   

cm_xml_node_ptr cm_xml_parse_file(const sint8* filename)
{
    mxml_node_t* root = NULL;
    FILE *fp = fopen(filename,"r");
    if(NULL == fp)
    {
        return NULL;
    }
    root = mxmlLoadFile(NULL,fp,NULL);
    fclose(fp);
    return root;
}
cm_xml_node_ptr cm_xml_parse_str(const sint8* str)
{
    return mxmlLoadString(NULL,str,NULL);
}
void cm_xml_free(cm_xml_node_ptr root)
{
    mxmlDelete((mxml_node_t*)root);
}


