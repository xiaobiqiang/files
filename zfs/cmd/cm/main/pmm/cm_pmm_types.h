/******************************************************************************
 *          Copyright (c) 2017 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_pmm_types.h
 * author     : wbn
 * create date: 2017年10月27日
 * description: TODO:
 *
 *****************************************************************************/
#ifndef MAIN_PMM_CM_PMM_TYPES_H_
#define MAIN_PMM_CM_PMM_TYPES_H_
#include "cm_types.h"
#include "cm_cfg_omi.h"
typedef enum
{
    CM_PMM_MSG_GET_CURRENT = 0,
    CM_PMM_MSG_GET_HISTORY,
    CM_PMM_MSG_GET_CURRENT_MEM,
    CM_PMM_MSG_BUTT
}cm_pmm_msg_e;

typedef struct
{
    uint32 obj;
    uint32 id;
    uint32 parent_id;
    sint8 fields[CM_OMI_FIELDS_LEN];
    uint32 max_cnt;
    uint64 start;
    uint64 end;
    sint8 param[CM_STRING_256];
    uint32 grain_size;
}cm_pmm_decode_t;

typedef struct
{
    uint32 id;
    cm_time_t tmstamp;    
    double val[];
}cm_pmm_data_t;

typedef struct
{
    uint32 obj_id;
    sint32 (*init)(void);
    void (*task)(struct tm *tin);
    sint32 (*get_current)(const void *pDecode,void **ppAckData, uint32 *pAckLen);
}cm_pmm_obj_cfg_t;

typedef struct
{
    uint32 save_hours;
    /* TODO: 添加其他配置 */
}cm_pmm_cfg_t;

typedef struct
{
    uint32 id;
    cm_time_t tmstamp;
    double cpu_max;
    double cpu_avg;
    double mem_max;
    double mem_avg;
    double bw_total;
    double bw_read;
    double bw_write;
    double iops_total;
    double iops_read;
    double iops_write;
}cm_pmm_cluster_data_t;

typedef struct
{
    uint32 id;
    cm_time_t tmstamp;
    double cpu;
    double mem;
    double bw_total;
    double bw_read;
    double bw_write;
    double iops_total;
    double iops_read;
    double iops_write;
}cm_pmm_node_data_t;


#define CM_PMM_NODE_DATA_CAL(val, old_val,new_val,diff) \
    do{ \
        if(((old_val) >= (new_val)) || ((old_val) == 0.0)) \
        { \
            (val) = 0.0; \
        } \
        else \
        { \
            (val) = (double)((new_val) - (old_val))/(diff); \
        } \
    }while(0)



#endif /* MAIN_PMM_CM_PMM_TYPES_H_ */
