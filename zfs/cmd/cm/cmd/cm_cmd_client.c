/******************************************************************************
 *          Copyright (c) 2018 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cmd_client.c
 * author     : wbn
 * create date: 2018年1月8日
 * description: TODO:
 *
 *****************************************************************************/
#include "cm_cmt.h"
#include "cm_cmd_client.h"

static sint8 g_cm_cmd_buff[CM_RPC_MAX_MSG_LEN] = {0};

sint32 cm_cmd_client_init(void)
{
    return cm_cmt_node_add(CM_NODE_ID_LOCAL,"127.0.0.1",CM_FALSE);
}

static sint32 cm_cmd_client_argv_check(const sint8* argv)
{
    const sint8 *pch = argv;
    for(;*pch != '\0'; pch++)
    {
        if((!(*pch >='!' && *pch <='~')) && (*pch != ' '))
        {
            /* 处理无效字符的场景 */
            return CM_PARAM_ERR;
        }
    }
    return CM_OK;
}

static uint32 cm_cmd_client_msg_encode(sint32 argc, sint8 **argv, sint8 *buff, 
uint32 len)
{
    uint32 offset = 0;

    offset = CM_VSPRINTF(buff,len,"%d",argc);
    buff[offset++] = '\0';

    while(argc > 0)
    {
        argc--;
        len -= offset;
        if(len <= strlen(*argv))
        {
            return 0;
        }
        if(CM_OK != cm_cmd_client_argv_check(*argv))
        {
            printf("arg:%s checkfail!\n",*argv);
            return 0;
        }
        offset += CM_VSPRINTF(buff+offset,len,"%s",*argv);
        buff[offset++] = '\0';
        argv++;
    }
    return offset;  
}

#ifdef CM_CMD_TEST_MOD

static sint32 cm_cmd_client_msg_decode(sint8 *Msg, uint32 len, sint8 
**ppParam, uint32 *pCnt)
{
    uint32 cnt = (uint32)atoi(Msg);
    uint32 index = 0;
    uint32 tmp_len = strlen(Msg)+1;
    /* 3 aa bb cc  */
    while((len > tmp_len) && (index < cnt))
    {
        Msg += tmp_len;
        len -= tmp_len;
        *ppParam = Msg;
        index++;
        ppParam++;
        tmp_len = strlen(Msg)+1;
    }
    *pCnt = index;
    return CM_OK;  
}
#endif
sint32 cm_cmd_client_main(sint32 argc, sint8 **argv)
{
    uint32 len = cm_cmd_client_msg_encode(argc,argv,g_cm_cmd_buff,sizeof(g_cm_cmd_buff));
    sint32 iRet = CM_FAIL;
#ifdef CM_CMD_TEST_MOD
    sint8 *pParam[CM_CMD_PARAM_MAX_NUM] = {NULL};
    uint32 ParamNum = CM_CMD_PARAM_MAX_NUM;
#else
    sint8 *pAckData = NULL;
    uint32 AckLen = 0;
#endif

    if(0 == len)
    {
        return CM_PARAM_ERR;
    }

#ifndef CM_CMD_TEST_MOD
    iRet = cm_cmt_request(CM_NODE_ID_LOCAL,CM_CMT_MSG_CMD,
        g_cm_cmd_buff, len, (void**)&pAckData, &AckLen,2);
    if(NULL != pAckData)
    {
        printf("%s",pAckData);
        CM_FREE(pAckData);
    }
#else
    iRet = cm_cmd_client_msg_decode(g_cm_cmd_buff,len,pParam,&ParamNum);
    if(CM_OK != iRet)
    {
        printf("cm_cmd_msg_decode fail\n");
        return iRet;
    }
    for(len=0;len<ParamNum;len++)
    {
        printf("Param[%u]:val[%s]\n",len,pParam[len]);
    }
#endif    
    return iRet;
}


