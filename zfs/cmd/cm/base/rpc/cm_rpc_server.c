/******************************************************************************
 *          Copyright (c) 2017 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_rpc_server.c
 * author     : wbn
 * create date: 2017Äê10ÔÂ26ÈÕ
 * description: TODO:
 *
 *****************************************************************************/
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "cm_rpc.h"

#include "cm_log.h"
#include "cm_queue.h"

typedef struct
{
    cm_rpc_msg_e msg_type;
    cm_queue_t *pqueue;
    cm_rpc_server_cbk_func_t cbk;
}cm_rpc_server_info_t;

typedef struct
{
    sint32 conn_fd;
    cm_rpc_msg_info_t msg;
}cm_rpc_server_msg_t;

static cm_rpc_server_info_t g_cm_rpc_servers[CM_MSG_BUTT];

static cm_mutex_t g_cm_rpc_server_mutex;
static void* cm_rpc_server_thread(void *pArg);

static cm_rpc_server_info_t* cm_rpc_server_get(cm_rpc_msg_e msg_type)
{
    cm_rpc_server_info_t *pInfo = g_cm_rpc_servers;
    uint32 iloop = (uint32)CM_MSG_BUTT;
    CM_MUTEX_LOCK(&g_cm_rpc_server_mutex);
    while(iloop > 0)
    {
        iloop--;
        if((pInfo->msg_type == msg_type) && (NULL != pInfo->cbk))
        {
            CM_MUTEX_UNLOCK(&g_cm_rpc_server_mutex);
            return pInfo;
        }
        pInfo++;
    }
    CM_MUTEX_UNLOCK(&g_cm_rpc_server_mutex);
    return NULL;
}

sint32 cm_rpc_reg(cm_rpc_msg_e msg_type, cm_rpc_server_cbk_func_t cbk, uint32 ThreadNum)
{
#ifndef CM_RPC_USE_NANOMSG
    sint32 iRet = CM_FAIL;
    cm_queue_t *pQueue = NULL;
    cm_thread_t Handle;
#endif
    if((msg_type >= CM_MSG_BUTT) || (NULL == cbk))
    {
        CM_LOG_ERR(CM_MOD_RPC,"msg_type:%u, Max:%u", (uint32)msg_type, (uint32)CM_MSG_BUTT);
        return CM_PARAM_ERR;
    }

    CM_MUTEX_LOCK(&g_cm_rpc_server_mutex);
    if(NULL != g_cm_rpc_servers[msg_type].cbk)
    {
        CM_LOG_ERR(CM_MOD_RPC,"msg_type:%u, Already Reg", (uint32)msg_type);
        CM_MUTEX_UNLOCK(&g_cm_rpc_server_mutex);
        return CM_FAIL;
    }

#ifndef CM_RPC_USE_NANOMSG
    iRet = cm_queue_init(&pQueue,CM_RPC_SERVER_QUEUE_LEN);
    if(CM_OK!=iRet)
    {
        CM_LOG_ERR(CM_MOD_RPC,"msg_type:%u, Init Queue Fail", (uint32)msg_type);
        CM_MUTEX_UNLOCK(&g_cm_rpc_server_mutex);
        return CM_FAIL;
    }

    g_cm_rpc_servers[msg_type].pqueue = pQueue;
#endif

    g_cm_rpc_servers[msg_type].cbk = cbk;
    g_cm_rpc_servers[msg_type].msg_type = msg_type;
    CM_MUTEX_UNLOCK(&g_cm_rpc_server_mutex);

#ifndef CM_RPC_USE_NANOMSG
    while(ThreadNum > 0)
    {
        ThreadNum--;
        iRet = CM_THREAD_CREATE(&Handle,cm_rpc_server_thread,(void*)&g_cm_rpc_servers[msg_type]);
        if(CM_OK != iRet)
        {

            CM_LOG_ERR(CM_MOD_RPC,"create thread fail");
            return CM_FAIL;
        }
        CM_THREAD_DETACH(Handle);
    }
#endif

    return CM_OK;
}

#ifndef CM_RPC_USE_NANOMSG
static void* cm_rpc_server_accpet(void *pArg);

sint32 cm_rpc_server_init(void)
{
    sint32 ServFd = -1;
    sint32 iRet = CM_FAIL;
    struct sockaddr_in servaddr;
	uint32 option = 1;
    cm_thread_t Handle;

    CM_MEM_ZERO(g_cm_rpc_servers,sizeof(g_cm_rpc_servers));

    ServFd = socket(AF_INET, SOCK_STREAM,0);
    if(ServFd < 0)
    {
        CM_LOG_ERR(CM_MOD_RPC,"socket fail");
        return CM_FAIL;
    }

    CM_MEM_ZERO(&servaddr,sizeof(servaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(CM_RPC_SERVER_PORT);
	
	if(setsockopt(ServFd,SOL_SOCKET,SO_REUSEADDR,&option,sizeof(option)) < 0)
	{
		CM_LOG_ERR(CM_MOD_RPC,"reuseaddr fail");
	}
    while(1)
    {   
        iRet = bind(ServFd, (struct sockaddr*)&servaddr, sizeof(servaddr));
        if(iRet < 0)
        {
            //close(ServFd);
            CM_LOG_ERR(CM_MOD_RPC,"Bind port(%u) fail", CM_RPC_SERVER_PORT);
            CM_SLEEP_S(3);
            continue;
        }
        break;
    }

    iRet = listen(ServFd,CM_RPC_SERVER_MAX_CONN);
    if(iRet < 0)
    {
        close(ServFd);
        CM_LOG_ERR(CM_MOD_RPC,"listen port(%u) fail", CM_RPC_SERVER_PORT);
        return CM_FAIL;
    }

    CM_MUTEX_INIT(&g_cm_rpc_server_mutex);
    iRet = CM_THREAD_CREATE(&Handle,cm_rpc_server_accpet,(void*)ServFd);
    if(CM_OK != iRet)
    {

        close(ServFd);
        CM_MUTEX_DESTROY(&g_cm_rpc_server_mutex);
        CM_LOG_ERR(CM_MOD_RPC,"create thread fail");
        return CM_FAIL;
    }
    CM_THREAD_DETACH(Handle);
    return CM_OK;
}
static void* cm_rpc_server_thread(void *pArg)
{
    cm_rpc_server_info_t *pServer = (cm_rpc_server_info_t*)pArg;
    sint32 iRet = CM_FAIL;
    cm_rpc_server_msg_t *pServerMsg = NULL;
    cm_rpc_msg_info_t *pAckMsg = NULL;
    cm_rpc_msg_info_t *pData = NULL;
    sint32 SendLen = 0;
    sint32 AckLen = 0;

    while(1)
    {
        iRet = cm_queue_get(pServer->pqueue,(void**)&pServerMsg);
        if(CM_OK != iRet)
        {
            continue;
        }
        pData = &pServerMsg->msg;
        iRet = pServer->cbk(pData, &pAckMsg);
        if(NULL == pAckMsg)
        {
            pData->head_len = sizeof(cm_rpc_msg_info_t);
            pData->data_len = 0;
            pData->result = iRet;
            SendLen = sizeof(cm_rpc_msg_info_t);
            AckLen = send(pServerMsg->conn_fd,(void*)pData, SendLen,0);
        }
        else
        {
            pAckMsg->msg_type = pData->msg_type;
            pAckMsg->head_len = sizeof(cm_rpc_msg_info_t);
            pAckMsg->result = iRet;
            pAckMsg->msg_id = pData->msg_id;
            SendLen = (sint32)(pAckMsg->head_len+pAckMsg->data_len);
            AckLen = send(pServerMsg->conn_fd,(void*)pAckMsg, SendLen,0);
            CM_FREE(pAckMsg);
            pAckMsg = NULL;
        }

        CM_LOG_DEBUG(CM_MOD_RPC,"Ack: Msg[%u] result[%d] Send[%d] SendOk[%d]", \
            pData->msg_id, iRet, SendLen,AckLen);

        if(AckLen < SendLen)
        {
            CM_LOG_WARNING(CM_MOD_RPC,"Ack: Msg[%u] result[%d] Send[%d] SendOk[%d]", \
                pData->msg_id, iRet, SendLen,AckLen);
        }
        CM_FREE(pServerMsg);
        pServerMsg = NULL;
    }
    return NULL;
}


static sint32 cm_rpc_server_recv_each(sint32 fd,cm_rpc_server_msg_t **ppMsg)
{
    cm_rpc_msg_info_t msghead;
    sint32 datalen = sizeof(msghead);
    sint32 recvlen = 0;
    sint32 eachlen = 0;
    uint8 *pbuff = (uint8*)&msghead;
    cm_rpc_server_msg_t *pMsg = NULL;

    for(;datalen>0;datalen -= eachlen,pbuff += recvlen)
    {
        eachlen = recv(fd,pbuff,datalen,0);
        if(eachlen <= 0)
        {            
            return CM_FAIL;
        }
    }
    
    if(msghead.magic_number != CM_RPC_MAGIC_NUM)
    {
        CM_LOG_ERR(CM_MOD_RPC,"msg_type:%d magic_number(0x%08X)not support",
            msghead.msg_type,msghead.magic_number);
        return CM_FAIL;
    }
    datalen = sizeof(cm_rpc_server_msg_t) + msghead.data_len;
    if(msghead.data_len > 16384)
    {
        CM_LOG_WARNING(CM_MOD_RPC,"msg_type:%d msgid:%u, datalen:%u",
            msghead.msg_type,msghead.msg_id,msghead.data_len);
    }
    pMsg = CM_MALLOC(datalen);
    if(NULL == pMsg)
    {
        CM_LOG_ERR(CM_MOD_RPC,"msg_type:%d msgid:%u, datalen:%u malloc fail",
            msghead.msg_type,msghead.msg_id,msghead.data_len);
        return CM_FAIL;
    }
    pMsg->conn_fd = fd;
    CM_MEM_CPY(&pMsg->msg,sizeof(msghead),&msghead,sizeof(msghead));
    for(pbuff = pMsg->msg.data,datalen = msghead.data_len,eachlen = 0;
        datalen > 0; 
        datalen -= eachlen, pbuff += eachlen)
    {
        eachlen = recv(fd,pbuff,datalen,0);
        if(eachlen <= 0)
        {        
            CM_FREE(pMsg);
            return CM_FAIL;
        }
    }
    *ppMsg = pMsg;
    return CM_OK;
}

static void* cm_rpc_server_recv(void *pArg)
{
    sint32 ConnFd = (sint32)pArg;
    sint32 iRet = CM_OK;
    cm_rpc_server_msg_t *pMsg = NULL;
    cm_rpc_msg_info_t *pRpcMsg = NULL;
    cm_rpc_server_info_t *pServer = NULL;
    while(1)
    {
        iRet = cm_rpc_server_recv_each(ConnFd,&pMsg);
        if(iRet != CM_OK)
        {
            break;
        }
        pRpcMsg = &pMsg->msg;
        pServer = cm_rpc_server_get(pRpcMsg->msg_type);
        if(NULL == pServer)
        {
            CM_LOG_ERR(CM_MOD_RPC,"msg_type:%d msgid:%u, datalen:%u not support",
                pRpcMsg->msg_type, pRpcMsg->msg_id, pRpcMsg->data_len);
            CM_FREE(pMsg);
            pMsg = NULL;
            continue;
        }
        if(CM_OK != cm_queue_add(pServer->pqueue,pMsg))
        {
            CM_LOG_ERR(CM_MOD_RPC,"msg_type:%d msgid:%u, datalen:%u add queue fail",
                pRpcMsg->msg_type, pRpcMsg->msg_id, pRpcMsg->data_len);
            pRpcMsg->head_len = sizeof(cm_rpc_msg_info_t);
            pRpcMsg->data_len = 0;
            pRpcMsg->result = CM_ERR_BUSY;
            iRet = send(ConnFd,(void*)pRpcMsg, sizeof(cm_rpc_msg_info_t),0);
            if(iRet < 0)
            {
                CM_LOG_ERR(CM_MOD_RPC,"msg_type:%d msgid:%u, datalen:%u send ack fail",
                    pRpcMsg->msg_type, pRpcMsg->msg_id, pRpcMsg->data_len);
            }
            CM_FREE(pMsg);
            pMsg = NULL;
            continue;
        } 
    }
    close(ConnFd);
    CM_LOG_INFO(CM_MOD_RPC,"ConnFd:%d close", ConnFd);
    return NULL;
}

static void* cm_rpc_server_accpet(void *pArg)
{
    sint32 ServFd = (sint32)pArg;
    sint32 ConnFd = -1;
    struct sockaddr_in clientaddr;
    cm_thread_t Handle;
    sint32 iRet = CM_FAIL;
    sint32 len = 0;

    while(1)
    {
        len = sizeof(clientaddr);
        ConnFd = accept(ServFd, (struct sockaddr*)&clientaddr, &len);
        if(ConnFd < 0)
        {
            continue;
        }
        CM_LOG_DEBUG(CM_MOD_RPC,"IP:%u, PORT:%u, ConnFd:%d", \
            clientaddr.sin_addr.s_addr, clientaddr.sin_port, ConnFd);
        #if 0
        iRet = fork();
        CM_LOG_WARNING(CM_MOD_RPC, "fork:%d", iRet);
        if(0 == iRet)
        {
            (void)cm_rpc_server_recv((void*)conn_fd);
            exit(0);
        }
        //close(ConnFd);
        #else
        iRet = CM_THREAD_CREATE(&Handle,cm_rpc_server_recv,(void*)ConnFd);
        if(CM_OK != iRet)
        {

            close(ConnFd);
            CM_LOG_ERR(CM_MOD_RPC,"create thread fail");
            continue;
        }
        CM_THREAD_DETACH(Handle);
        #endif
    }
    return NULL;
}

#else
#include <nanomsg/nn.h>
#include <nanomsg/pair.h>
#include <nanomsg/reqrep.h>

static void* cm_rpc_server_thread(void* ptr);

sint32 cm_rpc_server_init(void)
{
    uint32 iloop = CM_RPC_SERVER_MAX_CONN;
    sint32 iRet = CM_OK;
    sint8 url[CM_STRING_64] = {0};
    sint32 ServFd = nn_socket(AF_SP, NN_REP);

    if(ServFd < 0)
    {
        CM_LOG_ERR(CM_MOD_RPC,"ServFd:%d",ServFd);
        return CM_FAIL;
    }
    CM_MEM_ZERO(g_cm_rpc_servers,sizeof(g_cm_rpc_servers));
    CM_MUTEX_INIT(&g_cm_rpc_server_mutex);
    
    CM_VSPRINTF(url,sizeof(url),"tcp://*:%u",CM_RPC_SERVER_PORT);

    iRet = nn_bind(ServFd, url);
    if(iRet < 0)
    {
        CM_LOG_ERR(CM_MOD_RPC,"band:%u fail",CM_RPC_SERVER_PORT);
        return CM_FAIL;
    }
    while(iloop > 0)
    {
        iloop--;
        iRet = cm_thread_start(cm_rpc_server_thread,(void*)ServFd);
        if(CM_OK != iRet)
        {
            CM_LOG_ERR(CM_MOD_RPC,"%u fail",iloop);
            return CM_FAIL;
        }
    }
    return CM_OK;
}

static void* cm_rpc_server_thread(void* ptr)
{
    sint32 ServFd = (sint32)ptr;
    sint32 datalen = 0;
    void *pReq = NULL;
    cm_rpc_msg_info_t *pData = NULL;
    cm_rpc_server_info_t* pServer = NULL;
    sint32 iRet = CM_OK;
    cm_rpc_msg_info_t *pAckMsg = NULL;
    while(1)
    {
        if(NULL != pReq)
        {
            nn_freemsg(pReq);
            pReq = NULL;
        }
        datalen = nn_recv(ServFd,&pReq,NN_MSG,0);
        if(datalen < 0)
        {
            CM_LOG_WARNING(CM_MOD_RPC,"datalen:%d",datalen);
            break;
        }
        CM_LOG_WARNING(CM_MOD_RPC,"fd:%d,datalen:%d",ServFd,datalen);
        if(0 == datalen)
        {
            continue;
        }
        if(NULL == pReq)
        {
            continue;
        }
        pData = (cm_rpc_msg_info_t*)pReq;
        pServer = cm_rpc_server_get(pData->msg_type);
        if((NULL == pServer)
            ||(CM_RPC_MAGIC_NUM != pData->magic_number))
        {
            /* not support */
            CM_LOG_ERR(CM_MOD_RPC,"msg_type:%d magic_number(0x%08X)not support",
                pData->msg_type,pData->magic_number);
            pData->head_len = sizeof(cm_rpc_msg_info_t);
            pData->data_len = 0;
            pData->result = CM_ERR_NOT_SUPPORT;
            (void)nn_send(ServFd,(void*)pData, sizeof(cm_rpc_msg_info_t),0);
            continue;
        }

        iRet = pServer->cbk(pData, &pAckMsg);
        if((CM_OK != iRet) || (NULL == pAckMsg))
        {
            pData->head_len = sizeof(cm_rpc_msg_info_t);
            pData->data_len = 0;
            pData->result = iRet;
            datalen = sizeof(cm_rpc_msg_info_t);
            datalen = nn_send(ServFd,(void*)pData, datalen,0);            
        }
        else
        {
            pAckMsg->msg_type = pData->msg_type;
            pAckMsg->head_len = sizeof(cm_rpc_msg_info_t);
            pAckMsg->result = iRet;
            pAckMsg->msg_id = pData->msg_id;
            datalen = (sint32)(pAckMsg->head_len+pAckMsg->data_len);
            datalen = nn_send(ServFd,(void*)pAckMsg, datalen,0);
        }
        if(NULL != pAckMsg)
        {
            nn_freemsg(pAckMsg);
            pAckMsg = NULL;
        }

        CM_LOG_DEBUG(CM_MOD_RPC,"Ack: Msg[%u] result[%d]", \
            pData->msg_id, iRet);
    }
    
    CM_LOG_WARNING(CM_MOD_RPC,"exit:%d",ServFd);
    nn_close(ServFd);
    return NULL;
}

#endif


