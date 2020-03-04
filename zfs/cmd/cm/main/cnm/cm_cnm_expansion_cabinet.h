/* cm_cnm_expansion_cabinet.h */
#ifndef _CM_CNM_EXPANSION_CABINET_H
#define _CM_CNM_EXPANSION_CABINET_H
#include "cm_cnm_common.h"

typedef struct
{
    uint32 enid;
    uint32 type;
    uint32 U_num;
    uint32 slot_num;
}cm_cnm_cabinet_info_t;

extern void cm_cnm_cabinet_thread(void);

extern sint32 cm_cnm_cabinet_local_getbatch(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);
	
extern sint32 cm_cnm_cabinet_local_count(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

#endif