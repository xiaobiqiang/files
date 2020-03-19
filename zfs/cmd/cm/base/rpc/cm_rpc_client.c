/******************************************************************************
 *          Copyright (c) 2017 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_rpc_client.h
 * author     : wbn
 * create date: 2017年10月26日
 * description: TODO:
 *
 *****************************************************************************/
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <fcntl.h>

#include "cm_rpc.h"
#include "cm_log.h"
#include "cm_queue.h"

#ifdef CM_RPC_USE_NANOMSG
#include <nanomsg/nn.h>
#include <nanomsg/pair.h>
#include <nanomsg/reqrep.h>
#endif

typedef struct
{
    sint8 ip_addr[CM_IP_LEN];
    uint16 port;
    bool_t is_mutli;
    sint32 fd;
    cm_mutex_t mutex;
    cm_list_t *pwait_response;
    cm_thread_t recv_handle;

} cm_rpc_client_t;

typedef struct
{
    cm_rpc_msg_info_t *pmsg;
    cm_sem_t sem;
    cm_rpc_msg_info_t **ppack_data;
} cm_rpc_client_msg_t;

static void* cm_rpc_recv_thread(void *pArg);

static uint32 g_cm_rpc_msg_id = 0;
static cm_mutex_t g_cm_rpc_mutex;

static sint32 cm_rpc_connect_try(cm_rpc_client_t *pClient);

sint32 cm_rpc_client_init(void)
{
    CM_MUTEX_INIT(&g_cm_rpc_mutex);
    g_cm_rpc_msg_id = 0;
    return CM_OK;
}

static uint32 cm_rpc_new_msgid(void)
{
    uint32 id = 0;
    CM_MUTEX_LOCK(&g_cm_rpc_mutex);
    id = ++g_cm_rpc_msg_id;
    CM_MUTEX_UNLOCK(&g_cm_rpc_mutex);
    return id;
}


const sint8* cm_rpc_get_ipaddr(cm_rpc_handle_t Handle)
{
    cm_rpc_client_t *pClient = (cm_rpc_client_t*)Handle;

    if(NULL == pClient)
    {
        return NULL;
    }
    return pClient->ip_addr;
}

#ifndef CM_RPC_USE_NANOMSG

sint32 cm_rpc_connent(
    cm_rpc_handle_t *pHandle, const sint8 *pIpAddr, uint16 Port, bool_t isMutli)
{
    cm_rpc_client_t *pClient = NULL;
    sint32 iRet = CM_FAIL;

    pClient = CM_MALLOC(sizeof(cm_rpc_client_t));

    if(NULL == pClient)
    {
        CM_LOG_ERR(CM_MOD_RPC,"malloc fail");
        return CM_FAIL;
    }
    CM_MEM_ZERO(pClient,sizeof(cm_rpc_client_t));
    pClient->is_mutli = isMutli;
    pClient->port = Port;
    CM_MEM_CPY(pClient->ip_addr,sizeof(pClient->ip_addr)-1,pIpAddr,strlen(pIpAddr));
    pClient->fd = -1;

    if(!isMutli)
    {
        iRet = cm_rpc_connect_try(pClient);
        if(CM_OK != iRet)
        {
            CM_FREE(pClient);
            return iRet;
        }
        *pHandle = (cm_rpc_handle_t)pClient;
        return CM_OK;
    }
    
#ifdef CM_JNI_USED
    (void)cm_rpc_connect_try(pClient);
#endif

    iRet = cm_list_init(&pClient->pwait_response,CM_RPC_CLIENT_LIST_MAX);
    if(CM_OK != iRet)
    {
        CM_FREE(pClient);
        CM_LOG_ERR(CM_MOD_RPC,"Init list fail!");
        return CM_FAIL;
    }
    CM_MUTEX_INIT(&pClient->mutex);

    iRet = CM_THREAD_CREATE(&pClient->recv_handle,cm_rpc_recv_thread,pClient);
    if(CM_OK != iRet)
    {
        CM_MUTEX_DESTROY(&pClient->mutex);
        (void)cm_list_destory(pClient->pwait_response);
        CM_FREE(pClient);
        CM_LOG_ERR(CM_MOD_RPC,"create thread fail!");
        return CM_FAIL;
    }
    CM_THREAD_DETACH(pClient->recv_handle);
    *pHandle = (cm_rpc_handle_t)pClient;
    return CM_OK;
}

sint32 cm_rpc_connect_reset(cm_rpc_handle_t Handle, const sint8 *pIpAddr, uint16 Port)
{
    cm_rpc_client_t *pClient = (cm_rpc_client_t*)Handle;

    pClient->port = Port;
    CM_MEM_CPY(pClient->ip_addr,sizeof(pClient->ip_addr)-1,pIpAddr,strlen(pIpAddr));

    close(pClient->fd);
    pClient->fd = -1;
#ifdef CM_JNI_USED
    (void)cm_rpc_connect_try(pClient);
#endif    
    return CM_OK;
}

static sint32 cm_rpc_request_send(cm_rpc_client_t *pClient, cm_rpc_client_msg_t *pClientMsg)
{
    sint32 SendLen = 0;
    sint32 iRet = CM_FAIL;

    CM_MUTEX_LOCK(&pClient->mutex);
    if(pClient->fd < 0)
    {
        CM_LOG_INFO(CM_MOD_RPC,"Conn None!");
        CM_MUTEX_UNLOCK(&pClient->mutex);
        return CM_ERR_CONN_NONE;
    }

    SendLen = (sint32)(pClientMsg->pmsg->head_len + pClientMsg->pmsg->data_len);
    iRet = send(pClient->fd,(const void*)pClientMsg->pmsg, SendLen,0);
    CM_LOG_DEBUG(CM_MOD_RPC,"SendMsg[%u]:%u Len:%d",pClientMsg->pmsg->msg_type,
        pClientMsg->pmsg->msg_id, iRet);
    if(iRet < SendLen)
    {
        CM_LOG_ERR(CM_MOD_RPC,
                   "Send Msg[%u] fail",pClientMsg->pmsg->msg_id);
        CM_MUTEX_UNLOCK(&pClient->mutex);
        return CM_FAIL;
    }
    iRet = cm_list_add(pClient->pwait_response, pClientMsg);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_RPC,
                   "Add List Msg[%u] fail",pClientMsg->pmsg->msg_id);
    }
    CM_MUTEX_UNLOCK(&pClient->mutex);

    return iRet;
}

static sint32 cm_rpc_client_recv_each(sint32 fd,cm_rpc_msg_info_t **ppMsg);

sint32 cm_rpc_request_single(cm_rpc_handle_t Handle,cm_rpc_msg_info_t *pData,
                          cm_rpc_msg_info_t **ppAckData,uint32 Timeout)
{
    cm_rpc_client_t *pClient = (cm_rpc_client_t*)Handle;
    sint32 SendLen = 0;
    sint32 iRet = CM_FAIL;
    struct timeval tv = {Timeout,0};
    
    if(pClient->fd < 0)
    {
        CM_LOG_ERR(CM_MOD_RPC,"Conn None!");
        return CM_FAIL;
    }

    iRet = setsockopt(pClient->fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_RPC,"setsockopt fail (%d)",iRet);
        /* socket断开之后这里设置会失败，重新登陆 */
        return CM_ERR_CONN_FAIL;
    }
    pData->msg_id = cm_rpc_new_msgid();
    SendLen = (sint32)(pData->head_len + pData->data_len);
    iRet = send(pClient->fd,(const void*)pData, SendLen,0);
    if(iRet < SendLen)
    {
        return CM_FAIL;
    }

    CM_LOG_DEBUG(CM_MOD_RPC,"Send Msg[%u] OK Len(%d)",pData->msg_id,iRet);
    return cm_rpc_client_recv_each(pClient->fd,ppAckData);
}

static sint32 cm_rpc_close_wait(void *pNodeData, void *pArg)
{
    uint32 *cnt=(uint32*)pArg;
    (*cnt)++;
    return CM_OK;    
}
static void* cm_rpc_close_thread(void* arg)
{
    uint32 cnt=0;
    void* pdata=NULL;
    cm_rpc_client_t *pClient = (cm_rpc_client_t*)arg;
    CM_MUTEX_LOCK(&pClient->mutex);
    close(pClient->fd);
    pClient->fd = -1;
    CM_MUTEX_UNLOCK(&pClient->mutex);

    /* wait for delete msg */
    do
    {
        cnt=0;
        (void)cm_list_find(pClient->pwait_response, 
            cm_rpc_close_wait,&cnt,&pdata);
        CM_SLEEP_S(1);
    }while(cnt > 0);
    
    CM_THREAD_CANCEL(pClient->recv_handle);
    CM_MUTEX_DESTROY(&pClient->mutex);
    cm_list_destory(pClient->pwait_response);
    CM_FREE(pClient);
    return NULL;
}

sint32 cm_rpc_close(cm_rpc_handle_t Handle)
{
    cm_rpc_client_t *pClient = (cm_rpc_client_t*)Handle;

    if(pClient->is_mutli)
    {
        return cm_thread_start(cm_rpc_close_thread,Handle);
    }
    close(pClient->fd);
    pClient->fd = -1;
    CM_FREE(pClient);
    return CM_OK;
}


sint32 cm_rpc_request(
    cm_rpc_handle_t Handle,cm_rpc_msg_info_t *pData,
    cm_rpc_msg_info_t **ppAckData,uint32 Timeout)
{
    cm_rpc_client_msg_t *pClientMsg = NULL;
    cm_rpc_client_t *pClient = (cm_rpc_client_t*)Handle;
    sint32 iRet = CM_FAIL;

    if(NULL == pClient)
    {
        CM_LOG_ERR(CM_MOD_RPC,"Handle NULL!");
        return CM_PARAM_ERR;
    }

    if(!pClient->is_mutli)
    {
        return cm_rpc_request_single(Handle,pData,ppAckData,Timeout);
    }

    pClientMsg = CM_MALLOC(sizeof(cm_rpc_client_msg_t));
    if(NULL == pClientMsg)
    {
        CM_LOG_ERR(CM_MOD_RPC,"malloc fail!");
        return CM_FAIL;
    }

    CM_MEM_ZERO(pClientMsg,sizeof(cm_rpc_client_msg_t));
    CM_SEM_INIT(&pClientMsg->sem,0);           
    pClientMsg->ppack_data = ppAckData;
    pClientMsg->pmsg = pData;
    pClientMsg->pmsg->msg_id = cm_rpc_new_msgid();
    CM_LOG_DEBUG(CM_MOD_RPC,"Send Msg[%u]",pClientMsg->pmsg->msg_id);
    iRet = cm_rpc_request_send(pClient,pClientMsg);
    if(CM_OK != iRet)
    {
        CM_LOG_INFO(CM_MOD_RPC,"Send[%u] iRet[%u]",pClientMsg->pmsg->msg_id,iRet);
        CM_SEM_DESTROY(&pClientMsg->sem);
        CM_FREE(pClientMsg);
        return iRet;
    }
    CM_SEM_WAIT_TIMEOUT(&pClientMsg->sem,Timeout,iRet);
    CM_LOG_DEBUG(CM_MOD_RPC,"Send iRet[%u]",iRet);
    if(CM_OK != iRet)
    {
        (void)cm_list_delete(pClient->pwait_response, pClientMsg);
        CM_LOG_ERR(CM_MOD_RPC,"To[%s] Timeout[%u] iRet[%d]!",pClient->ip_addr,Timeout,iRet);
        iRet = CM_ERR_TIMEOUT;
    }
    else
    {
        iRet = (*ppAckData)->result;
    }
    CM_SEM_DESTROY(&pClientMsg->sem);
    CM_FREE(pClientMsg);
    return iRet;
}

static sint32 cm_rpc_find_msg(void *pNodeData, void *pArg)
{
    cm_rpc_msg_info_t *pMsg = pArg;
    cm_rpc_client_msg_t *pSendMgs = pNodeData;

    CM_LOG_DEBUG(CM_MOD_RPC,"SRC:[%u,%u], DEST:[%u, %u]",
        pSendMgs->pmsg->msg_id, pSendMgs->pmsg->msg_type,
        pMsg->msg_id, pMsg->msg_type);
    if((pSendMgs->pmsg->msg_id != pMsg->msg_id)
       || (pSendMgs->pmsg->msg_type != pMsg->msg_type))
    {
        return CM_FAIL;
    }
    *(pSendMgs->ppack_data) = pMsg;
    CM_SEM_POST(&pSendMgs->sem);
    return CM_OK;
}

static sint32 cm_rpc_connect_try(cm_rpc_client_t *pClient)
{
    sint32 Fd = -1;
    struct sockaddr_in servaddr;
    struct timeval tm;
    fd_set set;
    sint32 flags;

    if(pClient->fd >= 0)
    {
        return CM_OK;
    }
#ifndef CM_OMI_LOCAL    
#ifdef CM_JNI_USED
    {
        sint32 iRet = cm_exec_tmout(pClient->ip_addr,CM_IP_LEN,2,"ceres_cmd master |grep node_ip "
            "|awk -F':' '{print $2}' |awk -F' ' '{printf $1}'");
        if((CM_OK != iRet) || (0 == strlen(pClient->ip_addr)))
        {
            CM_LOG_ERR(CM_MOD_OMI,"get master fail");
            strcpy(pClient->ip_addr,"127.0.0.1");
        }
        CM_LOG_ERR(CM_MOD_RPC,"conn:%s",pClient->ip_addr);
    }
#endif
#endif

    Fd = socket(AF_INET,SOCK_STREAM,0);
    if(Fd < 0)
    {
        CM_LOG_ERR(CM_MOD_RPC,"socket fail!");
        return CM_FAIL;
    }
    CM_MEM_ZERO(&servaddr,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(pClient->port);
    //servaddr.sin_addr = htonl(inet_addr(pClient->IpAddr))
    inet_pton(AF_INET, pClient->ip_addr,&servaddr.sin_addr);
    
    flags = fcntl(Fd,F_GETFL);
    flags |= O_NONBLOCK;
    fcntl(Fd,F_SETFL,flags);
    if(-1 == connect(Fd, (struct sockaddr*)&servaddr, sizeof(servaddr)))
    {
        tm.tv_sec = CM_RPC_TIMEOUT;
        tm.tv_usec = 0;
        FD_ZERO(&set);
        FD_SET(Fd,&set);
    }
    if(0 == select(Fd+1,NULL,&set,NULL,&tm))
    {
        CM_LOG_ERR(CM_MOD_RPC,"connect %s:%u fail",pClient->ip_addr,pClient->port);
        close(Fd);
        return CM_ERR_CONN_FAIL;
    }
    
    pClient->fd = Fd;
    flags &= ~O_NONBLOCK;
    fcntl(Fd,F_SETFL,flags);
    /* wait server create recv thread */
    CM_SLEEP_US(1000);
    return CM_OK;
}

static sint32 cm_rpc_client_recv_each(sint32 fd,cm_rpc_msg_info_t **ppMsg)
{
    cm_rpc_msg_info_t msghead;
    sint32 datalen = sizeof(msghead);
    sint32 recvlen = 0;
    sint32 eachlen = 0;
    uint8 *pbuff = (uint8*)&msghead;
    cm_rpc_msg_info_t *pMsg = NULL;

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
    datalen = sizeof(cm_rpc_msg_info_t) + msghead.data_len;
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
    CM_MEM_CPY(pMsg,sizeof(msghead),&msghead,sizeof(msghead));
    for(pbuff = pMsg->data,datalen = msghead.data_len,eachlen = 0;
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

static void*  cm_rpc_recv_thread(void *pArg)
{
    cm_rpc_client_t *pClient = pArg;
    cm_rpc_msg_info_t *pMsg = NULL;
    cm_rpc_client_msg_t *pSendMgs = NULL;
    sint32 iRet = CM_OK;

    while(1)
    {
        if(CM_OK != cm_rpc_connect_try(pClient))
        {
            CM_SLEEP_S(5);
            continue;
        }
        iRet = cm_rpc_client_recv_each(pClient->fd, &pMsg);
        if(CM_OK != iRet)
        {
            CM_LOG_INFO(CM_MOD_RPC,"recv fail");
            close(pClient->fd);
            pClient->fd = -1;
            CM_SLEEP_S(1);
            continue;
        }
        
        CM_MUTEX_LOCK(&pClient->mutex);
        iRet = cm_list_find_delete(pClient->pwait_response,cm_rpc_find_msg,
                                        pMsg,(void**)&pSendMgs);
        CM_MUTEX_UNLOCK(&pClient->mutex);
        if(CM_OK != iRet)
        {
            CM_LOG_WARNING(CM_MOD_RPC,"Recv:%u Not Found",pMsg->msg_id);
            CM_FREE(pMsg);
        }
        pMsg = NULL;
    }
    return NULL;
}

#else

sint32 cm_rpc_connent(
    cm_rpc_handle_t *pHandle, const sint8 *pIpAddr, uint16 Port, bool_t isMutli)
{
    cm_rpc_client_t *pClient = NULL;

    pClient = CM_MALLOC(sizeof(cm_rpc_client_t));

    if(NULL == pClient)
    {
        CM_LOG_ERR(CM_MOD_RPC,"malloc fail");
        return CM_FAIL;
    }
    CM_MEM_ZERO(pClient,sizeof(cm_rpc_client_t));
    pClient->is_mutli = isMutli;
    pClient->port = Port;
    CM_VSPRINTF(pClient->ip_addr,sizeof(pClient->ip_addr),"tcp://%s:%u",pIpAddr,CM_RPC_SERVER_PORT);
    pClient->fd = -1;
    *pHandle = (cm_rpc_handle_t)pClient;
    return CM_OK;
    
}    

sint32 cm_rpc_connect_reset(cm_rpc_handle_t Handle, const sint8 *pIpAddr, uint16 Port)
{
    cm_rpc_client_t *pClient = (cm_rpc_client_t*)Handle;

    pClient->port = Port;
    CM_VSPRINTF(pClient->ip_addr,sizeof(pClient->ip_addr),"tcp://%s:%u",pIpAddr,CM_RPC_SERVER_PORT);
    return CM_OK;
}

sint32 cm_rpc_request(
    cm_rpc_handle_t Handle, cm_rpc_msg_info_t *pData,
    cm_rpc_msg_info_t **ppAckData,uint32 Timeout)
{
    sint32 fd = nn_socket(AF_SP,NN_REQ);
    sint32 iRet = CM_OK;
    sint32 datalen = 0;
    cm_rpc_client_t *pClient = (cm_rpc_client_t*)Handle;

    if(fd < 0)
    {
        CM_LOG_ERR(CM_MOD_RPC,"fd:%d",fd);
        return CM_FAIL;
    }

    iRet = nn_connect(fd,pClient->ip_addr);
    if(iRet < 0)
    {
        nn_close(fd);
        CM_LOG_ERR(CM_MOD_RPC,"conn: %s fail",pClient->ip_addr);
        return CM_FAIL;
    }
    
    Timeout *= 1000;
    iRet = nn_setsockopt(fd,NN_SOL_SOCKET, NN_RCVTIMEO,&Timeout, sizeof(Timeout));
    if(iRet < 0)
    {
        nn_close(fd);
        CM_LOG_ERR(CM_MOD_RPC,"setopt: %u fail",Timeout);
        return CM_FAIL;
    }
    pData->msg_id = cm_rpc_new_msgid();
    datalen = (sint32)(pData->head_len + pData->data_len);
    iRet = nn_send(fd,(void*)pData,datalen,0);
    if(iRet < datalen)
    {
        nn_close(fd);
        CM_LOG_ERR(CM_MOD_RPC,"Send Msg[%u] fail (%d)",pData->msg_id,iRet);        
        return CM_FAIL;
    }
    
    iRet = nn_recv(fd,(void*)ppAckData,NN_MSG,0);
    if(iRet <= 0)
    {
        CM_LOG_ERR(CM_MOD_RPC,"Recv Msg[%u] fail (%d)",pData->msg_id,iRet);        
    }
    else
    {
        iRet = (*ppAckData)->result;
    }
    nn_close(fd);
    return iRet;
}

sint32 cm_rpc_close(cm_rpc_handle_t Handle)
{
    CM_FREE(Handle);
    return CM_OK;
}


#endif


