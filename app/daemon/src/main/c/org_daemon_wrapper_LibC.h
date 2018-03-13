/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class org_daemon_wrapper_LibC */

#ifndef _Included_org_daemon_wrapper_LibC
#define _Included_org_daemon_wrapper_LibC
#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT jint JNICALL Java_org_daemon_wrapper_LibC_init
  (JNIEnv *, jclass, jint);
JNIEXPORT jint JNICALL Java_org_daemon_wrapper_LibC_notifydp
  (JNIEnv *, jclass);
/*
 * Class:     org_daemon_wrapper_LibC
 * Method:    getpid
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_org_daemon_wrapper_LibC_getpid
  (JNIEnv *, jclass);

/*
 * Class:     org_daemon_wrapper_LibC
 * Method:    getcpid
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_org_daemon_wrapper_LibC_getwpid
  (JNIEnv *, jclass);

/*
 * Class:     org_daemon_wrapper_LibC
 * Method:    daemon
 * Signature: (II)I
 */
JNIEXPORT jint JNICALL Java_org_daemon_wrapper_LibC_daemon
  (JNIEnv *, jclass, jint, jint);

/*
 * Class:     org_daemon_wrapper_LibC
 * Method:    fork
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_org_daemon_wrapper_LibC_fork
  (JNIEnv *, jclass);

/*
 * Class:     org_daemon_wrapper_LibC
 * Method:    exit
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_org_daemon_wrapper_LibC_exit
  (JNIEnv *, jclass, jint);

/*
 * Class:     org_daemon_wrapper_LibC
 * Method:    kill
 * Signature: (II)I
 */
JNIEXPORT jint JNICALL Java_org_daemon_wrapper_LibC_kill
  (JNIEnv *, jclass, jint, jint);

/*
 * Class:     org_daemon_wrapper_LibC
 * Method:    pipe
 * Signature: ()[I
 */
JNIEXPORT jintArray JNICALL Java_org_daemon_wrapper_LibC_pipe
  (JNIEnv *, jclass);

/*
 * Class:     org_daemon_wrapper_LibC
 * Method:    read
 * Signature: (II)[B
 */
JNIEXPORT jbyteArray JNICALL Java_org_daemon_wrapper_LibC_read
  (JNIEnv *, jclass, jint, jint);

/*
 * Class:     org_daemon_wrapper_LibC
 * Method:    write
 * Signature: (I[B)I
 */
JNIEXPORT jint JNICALL Java_org_daemon_wrapper_LibC_write
  (JNIEnv *, jclass, jint, jbyteArray);

/*
 * Class:     org_daemon_wrapper_LibC
 * Method:    waitpid
 * Signature: (I[II)I
 */
JNIEXPORT jint JNICALL Java_org_daemon_wrapper_LibC_waitpid
  (JNIEnv *, jclass, jint, jintArray, jint);

#ifdef __cplusplus
}
#endif
#endif
