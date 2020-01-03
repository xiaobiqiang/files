/******************************************************************************
 *          Copyright (c) 2018 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_main_cmd.c
 * author     : wbn
 * create date: 2018Äê1ÔÂ8ÈÕ
 * description: TODO:
 *
 *****************************************************************************/
#include "cm_cfg_global.h"
#include "cm_base.h"
#include "cm_cmd_client.h"     
/******************************************************************************
 * function     :
 * description  :
 *
 * parameter in :
 *
 * parameter out:
 *
 * return type  :
 *
 * author       : ${user}
 * create date  : ${date}
 *****************************************************************************/
sint32 cm_main_init(void)
{
    const cm_base_config_t cfg=
    {
        CM_LOG_DIR \
        CM_LOG_FILE, CM_LOG_MODE,CM_LOG_LVL,

        CM_TRUE, CM_TRUE,CM_FALSE, CM_FALSE,

        0,NULL,
        
        0, 0, 
        NULL, NULL, NULL,

        NULL,NULL
    };
    if(CM_OK != cm_base_init(&cfg))
    {
        return CM_FAIL;
    }
    
    return cm_cmd_client_init();
}

int main(int argc, const char *argv[])
{
    sint32 iRet = CM_OK;
    if((argc < 2) 
        || ((CM_CMD_PARAM_MAX_NUM+1) < argc))
    {
        printf("param error!\n");
        return CM_FAIL;
    }
#ifndef CM_CMD_TEST_MOD    
    if(CM_OK != cm_main_init())
    {
        return CM_FAIL;
    }
#endif
    iRet = cm_cmd_client_main(argc-1,argv+1);
    cm_cmt_node_delete(CM_NODE_ID_LOCAL); 
    if(CM_OK != iRet)
    {
        sint8 buff[CM_STRING_512] = {0};
        (void)cm_exec(buff,sizeof(buff)-1,"grep 'id=\"%d\"' "
            CM_CONFIG_DIR"res_en/error_code.xml"
            " |sed 's/.*desc=\"\\(.*\\)/\\1/g' |awk -F'\"' '{printf $1}'",iRet);
        printf("error[%d]:%s\n",iRet,buff);
    }
    return iRet;
}


