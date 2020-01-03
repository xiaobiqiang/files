/******************************************************************************
 *          Copyright (c) 2017 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_omi_server.c
 * author     : wbn
 * create date: 2017年10月23日
 * description: TODO:
 *
 *****************************************************************************/
#include "cm_omi.h"
#include "cm_log.h"
#include "cm_rpc.h"

extern uint32 g_CmOmiMaxId;
extern const cm_omi_object_cfg_t* g_CmOmiObjectCfgPtr;
static cm_omi_cbk_session_t g_omi_session_update = NULL;
static cm_omi_cbk_level_t g_omi_level_check = NULL;
extern uint32 g_CmOmiSessionObjId;
static sint32 cm_omi_rpc_callback(cm_rpc_msg_info_t *pMsg, cm_rpc_msg_info_t**ppAckMsg);

sint32 cm_omi_init_server(void)
{
    uint32 iloop = 0;
    sint32 iRet = CM_FAIL;
    const cm_omi_object_cfg_t* pCfg = g_CmOmiObjectCfgPtr;

    while(iloop < g_CmOmiMaxId)
    {
        if(NULL != pCfg->init_func)
        {
            iRet = pCfg->init_func();
            if(CM_OK != iRet)
            {
                CM_LOG_ERR(CM_MOD_OMI,"Obj[%u] Init fail[%d]!", iloop,iRet);
                return CM_FAIL;
            }
        }
        iloop++;
        pCfg++;
    }
    return cm_rpc_reg(CM_MSG_OMI, cm_omi_rpc_callback, CM_OMI_THREAD_NUM);
}

static sint32 cm_omi_get_obj_cmd(cm_omi_obj_t Obj, uint32 *pObj, uint32 *pCmd)
{
    sint32 iRet = CM_FAIL;

    iRet = cm_omi_obj_key_get_u32(Obj, CM_OMI_KEY_CMD, pCmd);
    if(CM_OK != iRet)
    {
        return CM_FAIL;
    }
    iRet = cm_omi_obj_key_get_u32(Obj, CM_OMI_KEY_OBJ, pObj);
    if(CM_OK != iRet)
    {
        return CM_FAIL;
    }
    return CM_OK;
}

const cm_omi_object_cfg_t* cm_omi_get_obj_cfg(uint32 ObjId)
{
    if(ObjId >= g_CmOmiMaxId)
    {
        return NULL;
    }
    return &g_CmOmiObjectCfgPtr[ObjId];
}

const cm_omi_object_cmd_cfg_t* cm_omi_get_obj_cmd_cfg(
    uint32 ObjId, uint32 CmdId)
{
    const cm_omi_object_cfg_t* pObj = cm_omi_get_obj_cfg(ObjId);
    uint32 iLoop = 0;
    const cm_omi_object_cmd_cfg_t* pRet = NULL;

    if(NULL == pObj)
    {
        CM_LOG_ERR(CM_MOD_OMI,"Obj[%u] not Support!", ObjId);
        return NULL;
    }

    iLoop = pObj->cmd_cnt;
    pRet = pObj->pcmd_list;
    while(iLoop > 0)
    {
        iLoop--;
        if(pRet->cmd == CmdId)
        {
            return pRet;
        }
        pRet++;
    }
    return NULL;
}

static sint32 cm_omi_process(const cm_omi_object_cmd_cfg_t* pCmdCfg,
    const cm_omi_obj_t ObjParam, const cm_omi_obj_t ObjSession,cm_omi_obj_t *pObjAck)
{
    sint32 iRet = CM_OK;
    void *pDecodeData = NULL;
    void *pAckData = NULL;
    uint32 AckLen = 0;

    if(NULL == pCmdCfg->process_func)
    {
        CM_LOG_INFO(CM_MOD_OMI,"cmd[%u] process_func NULL!", pCmdCfg->cmd);
        return CM_ERR_NOT_SUPPORT;
    }

    if((NULL != ObjParam) && (NULL != pCmdCfg->decode_func))
    {
        iRet = pCmdCfg->decode_func(ObjParam,&pDecodeData);
        if(CM_OK != iRet)
        {
            CM_LOG_INFO(CM_MOD_OMI,"cmd[%u] Decode Fail[%d]!", pCmdCfg->cmd,iRet);
            return iRet;
        }
    }

    do
    {
        iRet = pCmdCfg->process_func(pDecodeData,&pAckData,&AckLen);

        if(NULL != pCmdCfg->oplog_func)
        {
            const sint8* sessionid = (NULL != ObjSession)? cm_omi_obj_value(ObjSession) : NULL; 
            (void)pCmdCfg->oplog_func(sessionid,pDecodeData, iRet);
        }

        if(CM_OK != iRet)
        {
            CM_LOG_INFO(CM_MOD_OMI,"cmd[%u] Process Fail[%d]!", pCmdCfg->cmd,iRet);
            break;
        }

        if(NULL != pCmdCfg->encode_func)
        {
            *pObjAck = pCmdCfg->encode_func(pDecodeData,pAckData,AckLen);;
        }
    }while(0);

    if(NULL != pDecodeData)
    {
        CM_FREE(pDecodeData);
    }

    if(NULL != pAckData)
    {
        CM_FREE(pAckData);
    }

    return iRet;
}

void cm_omi_session_cbk_reg(cm_omi_cbk_session_t cbk)
{
    g_omi_session_update = cbk;
}

void cm_omi_level_cbk_reg(cm_omi_cbk_level_t cbk)
{
    g_omi_level_check = cbk;
}

static sint32 cm_omi_check_login(cm_omi_obj_t pObjSession, uint32 objid,uint32 *level)
{
    const sint8* pSession = NULL;
    if(g_CmOmiSessionObjId == objid)
    {
        *level = 0;
        return CM_OK;
    }
    
    if(NULL == pObjSession)
    {
        return CM_NEED_LOGIN;
    }
    
    pSession = cm_omi_obj_value(pObjSession);
    if(NULL == pSession)
    {
        return CM_NEED_LOGIN;
    }
    if(NULL == g_omi_session_update)
    {
        CM_LOG_ERR(CM_MOD_OMI,"cbk null!");
        return CM_FAIL;
    }
    return g_omi_session_update(pSession,level);
    
}

static sint32 cm_omi_check_level(uint32 obj,uint32 cmd,uint32 level)
{
    if(0 == level)
    {   
        return CM_OK;
    }
    
    if(level > 32)
    {
        return CM_ERR_NO_PERMISSION;
    }
    
    if(CM_OK == cm_omi_check_permission(obj,cmd))
    {
        return CM_OK;
    }

    if(NULL == g_omi_level_check)
    {
        CM_LOG_ERR(CM_MOD_OMI,"cbk null!");
        return CM_FAIL;
    }

    return g_omi_level_check(obj,cmd,level);
}

static sint32 cm_omi_rpc_callback(cm_rpc_msg_info_t *pMsg, cm_rpc_msg_info_t
**ppAckMsg)
{
    uint32 ObjId = 0;
    uint32 CmdId = 0;
    uint32 AckLen = 0;
    cm_time_t CostTime = cm_get_time();
    const sint8* pAckData = NULL;
    sint32 iRet = CM_FAIL;
    sint8 *pReqData = (sint8*)(((sint8*)pMsg) + pMsg->head_len);
    const cm_omi_object_cmd_cfg_t* pCmdCfg = NULL;
    cm_omi_obj_t pObjRoot= cm_omi_obj_parse(pReqData);
    cm_omi_obj_t pObjParam = NULL;
    cm_omi_obj_t pObjAck = NULL;
    cm_rpc_msg_info_t *pRpcAck = NULL;
    cm_omi_obj_t pObjSession = NULL;
    uint32 level = 0xFF;
    
    if(NULL == pObjRoot)
    {
        CM_LOG_ERR(CM_MOD_OMI,"Format Err!");
        return CM_FAIL;
    }
    CM_LOG_DEBUG(CM_MOD_OMI,"%s",pReqData);
    iRet = cm_omi_get_obj_cmd(pObjRoot,&ObjId,&CmdId);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_OMI,"Get obj cmd fail!");
        cm_omi_obj_delete(pObjRoot);
        return CM_FAIL;
    }

    //CM_LOG_INFO(CM_MOD_OMI,"Obj[%u] cmd[%u]",ObjId,CmdId);

    pCmdCfg = cm_omi_get_obj_cmd_cfg(ObjId,CmdId);
    if(NULL == pCmdCfg)
    {
        CM_LOG_ERR(CM_MOD_OMI,"Obj[%u] cmd[%u] not Support!", ObjId,CmdId);
        cm_omi_obj_delete(pObjRoot);
        return CM_FAIL;
    }

    /* TODO: 登录校验和权限校验 */
    pObjSession = cm_omi_obj_key_find(pObjRoot,CM_OMI_KEY_SESSION);
    iRet = cm_omi_check_login(pObjSession, ObjId,&level);
    if((CM_OK != iRet)
#ifdef CM_SESSION_TEST_FILE    
        && (CM_FALSE == cm_file_exist(CM_SESSION_TEST_FILE))
#endif
        )
    {
        CM_LOG_INFO(CM_MOD_OMI,"Obj[%u] cmd[%u] login[%d]!", ObjId,CmdId,iRet);
        cm_omi_obj_delete(pObjRoot);
        return iRet;
    }
    iRet = cm_omi_check_level(ObjId,CmdId,level);
    if((CM_OK != iRet)
#ifdef CM_SESSION_TEST_FILE    
        && (CM_FALSE == cm_file_exist(CM_SESSION_TEST_FILE))
#endif        
        )
    {
        CM_LOG_INFO(CM_MOD_OMI,"Obj[%u] cmd[%u] level[%u]!", ObjId,CmdId,level);
        cm_omi_obj_delete(pObjRoot);
        return iRet;
    }
    pObjParam = cm_omi_obj_key_find(pObjRoot,CM_OMI_KEY_PARAM);

    iRet = cm_omi_process(pCmdCfg,pObjParam,pObjSession, &pObjAck);

    if(NULL != pObjParam)
    {
        cm_omi_obj_key_delete(pObjRoot, CM_OMI_KEY_PARAM);
    }
    cm_omi_obj_key_delete(pObjRoot, CM_OMI_KEY_SESSION);
    (void)cm_omi_obj_key_set_s32(pObjRoot,CM_OMI_KEY_RESULT,iRet);

    if(NULL != pObjAck)
    {
        iRet = cm_omi_obj_key_add(pObjRoot,CM_OMI_KEY_ITEMS,pObjAck);
        if(CM_OK != iRet)
        {
            CM_LOG_ERR(CM_MOD_OMI,"Add items Fail!");
            cm_omi_obj_delete(pObjAck);
        }
    }

    pAckData = cm_omi_obj_tostr(pObjRoot);
    if(NULL == pAckData)
    {
        CM_LOG_ERR(CM_MOD_OMI,"Obj[%u] cmd[%u] Get String Fail!",ObjId,CmdId);
        cm_omi_obj_delete(pObjRoot);
        return CM_FAIL;
    }

    AckLen = strlen(pAckData) + 1;
    pRpcAck = cm_rpc_msg_new(CM_MSG_OMI, AckLen);
    if(NULL == pRpcAck)
    {
        CM_LOG_ERR(CM_MOD_OMI,"Obj[%u] cmd[%u] AckLen[%u] malloc fail!",ObjId,CmdId,AckLen);
        cm_omi_obj_delete(pObjRoot);
        return CM_ERR_MSG_TOO_LONG;
    }

    CM_MEM_CPY((sint8*)pRpcAck->data,AckLen,pAckData,AckLen);
    cm_omi_obj_delete(pObjRoot);
    *ppAckMsg = pRpcAck;

    CostTime = cm_get_time() - CostTime;
    if(CostTime >= 5)
    {
        CM_LOG_WARNING(CM_MOD_OMI,"Obj[%u] cmd[%u] cost %u seconds!", ObjId,CmdId,CostTime);
    }

    return CM_OK;
}


