/******************************************************************************
 *          Copyright (c) 2018 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_main_exec.c
 * author     : wbn
 * create date: 2018Äê1ÔÂ8ÈÕ
 * description: TODO:
 *
 *****************************************************************************/
#include "cm_cfg_global.h"
#include "cm_base.h"


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

    return cm_base_init(&cfg);
}

int main(int argc, const char *argv[])
{
    const uint32 nid=CM_NODE_ID_LOCAL;
    sint32 iRet = CM_OK;
    const sint8* serverip = NULL;
    const sint8* cmd = NULL;
    void* pAck = NULL;
    uint32 len = 0;
    
    if((argc != 3) 
        || (0 == strlen(argv[1]))
        || (0 == strlen(argv[2])))
    {
        printf("format: %s <ipaddr> <cmd>\n",argv[0]);
        return CM_PARAM_ERR;
    }

    serverip = argv[1];
    cmd = argv[2];
    if(CM_OK != cm_main_init())
    {
        return CM_FAIL;
    }

    iRet = cm_cmt_node_add(nid,serverip,CM_FALSE);
    if(CM_OK != iRet)
    {
        fprintf(stderr,"connect[%s] fail[%d]\n",serverip,iRet);
        return iRet;
    }
    iRet = cm_cmt_request(nid,CM_CMT_MSG_EXEC,
        (const void *)cmd,(strlen(cmd)+1),&pAck,&len,CM_CMT_REQ_TMOUT);
    cm_cmt_node_delete(nid);    
    if((CM_OK != iRet) || (NULL == pAck))
    {
        return iRet;
    }

    printf("%s",(sint8*)pAck);
    CM_FREE(pAck);
    return iRet;
}


