/******************************************************************************
 *          Copyright (c) 2018 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_xml.h
 * author     : wbn
 * create date: 2018.04.25
 * description: TODO:
 *
 *****************************************************************************/
#ifndef BASE_XML_CM_XML_H_
#define BASE_XML_CM_XML_H_


typedef void* cm_xml_node_ptr;
typedef sint32 (*cm_xml_cbk_t)(cm_xml_node_ptr curnode,void *arg);

typedef struct
{
    cm_xml_node_ptr root;
    const sint8* nodename;
    const sint8* attr;
    const sint8* value;
    void *param;
    uint32 offset;
    uint32 total;
    uint32 eachsize;
    cm_xml_cbk_t check;
    cm_xml_cbk_t geteach;
    void **ppAck;
    uint32 *pAckLen;
}cm_xml_request_param_t;

extern sint32 cm_xml_request(cm_xml_request_param_t *req);
extern sint32 cm_xml_request_count(cm_xml_request_param_t *req);

extern cm_xml_node_ptr cm_xml_parse_file(const sint8* filename);
extern cm_xml_node_ptr cm_xml_parse_str(const sint8* str);
extern void cm_xml_free(cm_xml_node_ptr root);

extern sint32 cm_xml_find_node(cm_xml_node_ptr root,
    const sint8* nodename, const sint8* attr, const sint8* value,
    cm_xml_cbk_t cbk,void *arg);
    
extern cm_xml_node_ptr cm_xml_get_node(cm_xml_node_ptr root,
    const sint8* nodename, const sint8* attr, const sint8* value);

extern const sint8* cm_xml_get_name(cm_xml_node_ptr node);
/* 这里需要注意的是，如果含有空格，换行之类的 需要通过 
cm_xml_get_next_sibling遍历取, 解析过程中会把空格这些去掉
*/
extern const sint8* cm_xml_get_text(cm_xml_node_ptr node);
extern const sint8* cm_xml_get_child_text(cm_xml_node_ptr node,const sint8* child);

extern cm_xml_node_ptr cm_xml_get_child(cm_xml_node_ptr node);
extern cm_xml_node_ptr cm_xml_get_next_sibling(cm_xml_node_ptr node);
extern cm_xml_node_ptr cm_xml_get_parent(cm_xml_node_ptr node);

/* 返回相应路径的第一个子节点 */
extern cm_xml_node_ptr cm_xml_find_path(cm_xml_node_ptr node, const sint8* path);

extern const sint8* cm_xml_get_attr(cm_xml_node_ptr node, const sint8* name);

extern sint32 cm_xml_get_file(const sint8* filename,const sint8* nodename,cm_xml_cbk_t cbk,void 
*arg);

#endif /* BASE_XML_CM_XML_H_ */
