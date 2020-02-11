/******************************************************************************
 *          Copyright (c) 2018 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_jni.c
 * author     : wbn
 * create date: 2018Äê2ÔÂ6ÈÕ
 * description: TODO:
 *
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include "cm_jni.h"
#include "cm_omi.h"
#include "cm_log.h"
#include "cm_rpc.h"
#include "cm_base.h"
#include "cm_cfg_global.h"
#include "cm_cfg_cnm.h"
#include "cm_cfg_omi.h"


static bool_t g_cm_jni_init = CM_FALSE;

JNIEXPORT jint JNICALL Java_cm_jni_init(JNIEnv *Env, jobject Obj)
{
    sint32 iRet = CM_OK;
    const cm_base_config_t cfg=
        {
            CM_LOG_DIR \
            CM_LOG_FILE, CM_LOG_MODE,CM_LOG_LVL,
    
            CM_TRUE, CM_FALSE, CM_TRUE, CM_TRUE,
    
            0,NULL,
            
            CM_OMI_OBJECT_SESSION, CM_OMI_OBJECT_BUTT, 
            g_CmOmiObjCmdNoCheckPtr, CmOmiMap, NULL,
    
            NULL,NULL
        };

    if(CM_TRUE == g_cm_jni_init)
    {
        CM_LOG_ERR(CM_MOD_NONE,"reinit!");
        return CM_OK;
    }

    signal(SIGPIPE,SIG_IGN);

    iRet = cm_base_init(&cfg);

    //cm_log_all_mode_set(CM_FALSE);;
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_NONE,"omi fail[%d]",iRet);
        return iRet;
    }
    CM_LOG_WARNING(CM_MOD_NONE,"ok");
    g_cm_jni_init = CM_TRUE;
    return iRet;
}

JNIEXPORT jstring JNICALL Java_cm_jni_request(JNIEnv *Env, jobject Obj,jstring Req,jint tmout)
{
    const sint8 *pReqData = NULL;
    sint8 *pAckData = NULL;
    uint32 AckLen = 0;
    sint32 iRet = CM_OK;
    jboolean isCopy;
    jstring Ack;
    cm_omi_obj_t root = NULL;

    pReqData = (*Env)->GetStringUTFChars(Env,Req,&isCopy);
    if(NULL == pReqData)
    {
        CM_LOG_ERR(CM_MOD_NONE,"get string fail");
        return NULL;
    }

    iRet = cm_omi_request_str(pReqData,&pAckData,&AckLen,(uint32)tmout);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_NONE,"REQ:%s iRet[%u]",pReqData,iRet);
    }
	
	(*Env)->ReleaseStringUTFChars(Env,Req,pReqData);
	
    if((CM_OK == iRet) && (NULL != pAckData))
    {
        Ack = (*Env)->NewStringUTF(Env,pAckData);
        cm_omi_free(pAckData);
        return Ack;
    }

    if(CM_ERR_REDIRECT_MASTER == iRet)
    {
        CM_LOG_ERR(CM_MOD_NONE,"redirect to master");
        cm_omi_reconnect();
        iRet = CM_NEED_LOGIN;
    }
    
    root = cm_omi_obj_new();
    if(NULL == root)
    {
        CM_LOG_ERR(CM_MOD_NONE,"new obj fail");
        return NULL;
    }
    (void)cm_omi_obj_key_set_s32(root,CM_OMI_KEY_RESULT,iRet);
    Ack = (*Env)->NewStringUTF(Env,cm_omi_obj_tostr(root));
    cm_omi_obj_delete(root);
    return Ack;
}

JNIEXPORT jint JNICALL Java_cm_jni_close(JNIEnv *Evn, jobject Obj)
{
    CM_LOG_WARNING(CM_MOD_NONE,"close");
    cm_omi_close();
    return CM_OK;
}

JNIEXPORT jint JNICALL Java_cm_jni_logset
  (JNIEnv *Evn, jobject Obj, jint level)
{
    return cm_log_level_set(CM_MOD_NONE, level);
}

/*
 * Class:     cm_jni
 * Method:    remote_connect
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_cm_jni_remote_1connect
  (JNIEnv * Env, jobject Obj, jstring ipaddr)
{
    const sint8 *ip = NULL;
    jboolean isCopy;
    sint32 iret = -1;
    
    ip = (*Env)->GetStringUTFChars(Env,ipaddr,&isCopy);
    if(NULL == ip)
    {
        CM_LOG_ERR(CM_MOD_NONE,"get string fail");
        return -1;
    }
    iret = cm_omi_remote_connect(ip);
    (*Env)->ReleaseStringUTFChars(Env,ipaddr,ip);
    return iret;
}

/*
 * Class:     cm_jni
 * Method:    remote_request
 * Signature: (ILjava/lang/String;I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_cm_jni_remote_1request
  (JNIEnv * Env, jobject Obj, jint handle, jstring Req, jint tmout)
{
    const sint8 *pReqData = NULL;
    sint8 *pAckData = NULL;
    uint32 AckLen = 0;
    sint32 iRet = CM_OK;
    jboolean isCopy;
    jstring Ack;
    cm_omi_obj_t root = NULL;

    pReqData = (*Env)->GetStringUTFChars(Env,Req,&isCopy);
    if(NULL == pReqData)
    {
        CM_LOG_ERR(CM_MOD_NONE,"get string fail");
        return NULL;
    }

    iRet = cm_omi_remote_request(handle,pReqData,&pAckData,&AckLen,(uint32)tmout);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_NONE,"REQ:%s iRet[%u]",pReqData,iRet);
    }
	
	(*Env)->ReleaseStringUTFChars(Env,Req,pReqData);
	
    if((CM_OK == iRet) && (NULL != pAckData))
    {
        Ack = (*Env)->NewStringUTF(Env,pAckData);
        cm_omi_free(pAckData);
        return Ack;
    }

    root = cm_omi_obj_new();
    if(NULL == root)
    {
        CM_LOG_ERR(CM_MOD_NONE,"new obj fail");
        return NULL;
    }
    (void)cm_omi_obj_key_set_s32(root,CM_OMI_KEY_RESULT,iRet);
    Ack = (*Env)->NewStringUTF(Env,cm_omi_obj_tostr(root));
    cm_omi_obj_delete(root);
    return Ack;
}

/*
 * Class:     cm_jni
 * Method:    remote_close
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_cm_jni_remote_1close
  (JNIEnv * Env, jobject Obj, jint handle)
{
    return cm_omi_remote_close(handle);
}

 

