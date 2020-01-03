/******************************************************************************
 *          Copyright (c) 2017 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_node.h
 * author     : wbn
 * create date: 2017Äê10ÔÂ26ÈÕ
 * description: TODO:
 *
 *****************************************************************************/
#ifndef MAIN_NODE_CM_NODE_H_
#define MAIN_NODE_CM_NODE_H_
#include "cm_common.h"


typedef sint32 (*cm_node_trav_func_t)(cm_node_info_t *pNode, void *pArg);

extern sint32 cm_node_init(void);

extern uint32 cm_node_get_subdomain_id(void);

extern uint32 cm_node_get_id(void);

extern uint32 cm_node_get_sbbid(void);

extern uint32 cm_node_get_master(void);

extern const sint8* cm_node_get_name(uint32 nid);

extern uint32 cm_node_get_submaster(uint32 SubDomainId);

extern uint32 cm_node_get_submaster_current(void);

extern sint32 cm_node_set_submaster_current(uint32 masterid);

extern sint32 cm_node_set_master(uint32 id);

extern sint32 cm_node_set_submaster(uint32 SubDomainId,uint32 masterid);

extern sint32 cm_node_set_state(uint32 SubDomainId, uint32 NodeId,uint32 state);

extern sint32 cm_node_get_info(uint32 SubDomainId, uint32 NodeId,cm_node_info_t *pInfo);

extern sint32 cm_node_addto_subdomain(uint32 NodeId, uint32 SubDomainId);

extern sint32 cm_node_deletefrom_subdomain(uint32 NodeId, uint32 SubDomainId);

extern sint32 cm_node_traversal_node(uint32 SubDomainId, cm_node_trav_func_t Cbk,
void *pArg, bool_t FailBreak);

extern sint32 cm_node_traversal_subdomain(cm_node_trav_func_t Cbk,
void *pArg, bool_t FailBreak);

extern sint32 cm_node_traversal_all(cm_node_trav_func_t Cbk,
void *pArg, bool_t FailBreak);

extern sint32 cm_node_traversal_all_sbb(cm_node_trav_func_t Cbk,
void *pArg, bool_t FailBreak);

extern sint32 cm_node_cmt_cbk(void *pMsg, uint32 Len, void** ppAck, uint32 
*pAckLen);

extern uint32 cm_node_get_id_by_inter(uint32 inter_id);

extern sint32 cm_node_update_info_each(cm_node_info_t *pNode, void *pArg);

extern sint32 cm_node_add(const sint8* ipaddr, uint32 sbbid);
extern sint32 cm_node_delete(uint32 nid);

extern uint32 cm_node_get_brother_nid(uint32 nid);

extern sint32 cm_node_cmt_cbk_add(void *pMsg, uint32 Len, void** ppAck, uint32 
*pAckLen);

extern sint32 cm_node_cmt_cbk_delete(void *pMsg, uint32 Len, void** ppAck, uint32 
*pAckLen);

extern sint32 cm_node_power_on(uint32 nid, const sint8 *user, const sint8 *passwd);

extern sint32 cm_node_power_off(uint32 nid);

extern sint32 cm_node_reboot(uint32 nid);

extern sint32 cm_node_check_all_online(void);

#endif /* MAIN_NODE_CM_NODE_H_ */
