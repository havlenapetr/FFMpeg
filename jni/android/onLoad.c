#define TAG "ffmpeg_onLoad"

#include <stdlib.h>
#include <android/log.h>
#include "jniUtils.h"

extern int register_android_media_FFMpeg(JNIEnv *env);
extern int register_android_media_FFMpegAVFormatContext(JNIEnv *env);

static JavaVM *sVm;

/*
 * Throw an exception with the specified class and an optional message.
 */
int jniThrowException(JNIEnv* env, const char* className, const char* msg) {
    jclass exceptionClass = (*env)->FindClass(env, className);
    if (exceptionClass == NULL) {
        __android_log_print(ANDROID_LOG_ERROR,
			    TAG,
			    "Unable to find exception class %s",
	                    className);
        return -1;
    }

    if ((*env)->ThrowNew(env, exceptionClass, msg) != JNI_OK) {
        __android_log_print(ANDROID_LOG_ERROR,
			    TAG,
			    "Failed throwing '%s' '%s'",
			    className, msg);
    }
    return 0;
}

JNIEnv* getJNIEnv() {
    JNIEnv* env = NULL;
    if ((*sVm)->GetEnv(sVm, (void**) &env, JNI_VERSION_1_4) != JNI_OK) {
	__android_log_print(ANDROID_LOG_ERROR,
			    TAG,
			    "Failed to obtain JNIEnv");
        return NULL;
    }
    return env;
}

/*
 * Register native JNI-callable methods.
 *
 * "className" looks like "java/lang/String".
 */
int jniRegisterNativeMethods(JNIEnv* env,
                             const char* className,
                             const JNINativeMethod* gMethods,
                             int numMethods)
{
    jclass clazz;

    __android_log_print(ANDROID_LOG_INFO, TAG, "Registering %s natives\n", className);
    clazz = (*env)->FindClass(env, className);
    if (clazz == NULL) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "Native registration unable to find class '%s'\n", className);
        return -1;
    }
    if ((*env)->RegisterNatives(env, clazz, gMethods, numMethods) < 0) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "RegisterNatives failed for '%s'\n", className);
        return -1;
    }
    return 0;
}

jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    JNIEnv* env = NULL;
    jint result = JNI_ERR;
	sVm = vm;

    if ((*vm)->GetEnv(vm, (void**) &env, JNI_VERSION_1_4) != JNI_OK) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "GetEnv failed!");
        return result;
    }

    __android_log_print(ANDROID_LOG_INFO, TAG, "loading . . .");

    if(register_android_media_FFMpeg(env) != JNI_OK) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "can't load android_media_FFMpeg");
        goto end;
    }

    if(register_android_media_FFMpegAVFormatContext(env) != JNI_OK) {
    	__android_log_print(ANDROID_LOG_ERROR, TAG, "can't load android_media_FFMpegAVFormatContext");
        goto end;
	}

    __android_log_print(ANDROID_LOG_INFO, TAG, "loaded");

    result = JNI_VERSION_1_4;

end:
    return result;
}
