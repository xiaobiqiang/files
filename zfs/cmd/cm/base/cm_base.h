/******************************************************************************
 *          Copyright (c) 2019 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_base.h
 * author     : wbn
 * create date: 2019Äê9ÔÂ29ÈÕ
 * description: TODO:
 *
 *****************************************************************************/
#ifndef BASE_CM_BASE_H_
#define BASE_CM_BASE_H_
#include "cm_common.h"
#include "cm_log.h"
#include "cm_omi.h"
#include "cm_cmt.h"
#include "cm_rpc.h"
#include "cm_db.h"
#include "cm_xml.h"
#include "cm_queue.h"

typedef struct
{
    const sint8* logfile;
    const uint8 logmode;
    const uint8 loglvldef;
    
    const bool_t isclient;
    const bool_t usecmt;
    const bool_t useomi;
    const bool_t omiclimulti;
    
    const uint32 cmtmsgmax;
    const cm_cmt_config_t* cmtcfg;

    const uint32 omisessionid;
    const uint32 omiobjmax;    
    const uint32* ominocheck;
    const cm_omi_map_object_t** omiobj;
    const cm_omi_object_cfg_t* omiobjcbk;

    const cm_get_node_func_t getmyid;
    const cm_get_node_func_t getmasterid;
    
}cm_base_config_t;


extern sint32 cm_base_init(const cm_base_config_t* cfg);


#endif /* BASE_CM_BASE_H_ */

