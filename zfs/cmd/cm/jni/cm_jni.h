/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class cm_jni */

#ifndef _Included_cm_jni
#define _Included_cm_jni
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     cm_jni
 * Method:    init
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_cm_jni_init
  (JNIEnv *, jobject);

/*
 * Class:     cm_jni
 * Method:    request
 * Signature: (Ljava/lang/String;I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_cm_jni_request
  (JNIEnv *, jobject, jstring, jint);

/*
 * Class:     cm_jni
 * Method:    close
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_cm_jni_close
  (JNIEnv *, jobject);

/*
 * Class:     cm_jni
 * Method:    logset
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_cm_jni_logset
  (JNIEnv *, jobject, jint);

/*
 * Class:     cm_jni
 * Method:    remote_connect
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_cm_jni_remote_1connect
  (JNIEnv *, jobject, jstring);

/*
 * Class:     cm_jni
 * Method:    remote_request
 * Signature: (ILjava/lang/String;I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_cm_jni_remote_1request
  (JNIEnv *, jobject, jint, jstring, jint);

/*
 * Class:     cm_jni
 * Method:    remote_close
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_cm_jni_remote_1close
  (JNIEnv *, jobject, jint);

#ifdef __cplusplus
}
#endif
#endif