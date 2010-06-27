#define TAG "android_media_FFMpegAVRational"

#include <android/log.h>
#include "jniUtils.h"
#include "methods.h"

struct fields_t
{
    jmethodID constructor;
};
static struct fields_t fields;

jclass *AVRational_getClass(JNIEnv *env) {
	return (*env)->FindClass(env, "com/media/ffmpeg/FFMpegAVRational");
}

const char *AVRational_getClassSignature() {
	return "Lcom/media/ffmpeg/FFMpegAVRational;";
}

jobject *AVRational_create(JNIEnv *env, AVRational *rational) {
	jclass *clazz = AVRational_getClass(env);
	jobject result = (*env)->NewObject(env, clazz, fields.constructor);

	// set native pointer to java class for later use
	(*env)->SetIntField(env, result, (*env)->GetFieldID(env, clazz, "mNum", "I"), (jint)rational->num);
	(*env)->SetIntField(env, result, (*env)->GetFieldID(env, clazz, "mDen", "I"), (jint)rational->den);
	return result;
}

static void AVRational_release(JNIEnv *env, jobject thiz, jint pointer) {
	/*AVFormatContext *fileContext = (AVFormatContext *) pointer;
	__android_log_print(ANDROID_LOG_INFO, TAG,  "releasing FFMpegAVFormatContext"),
	free(fileContext);*/
}
/*
 * JNI registration.
 */
static JNINativeMethod methods[] = {
	{ "nativeRelease", "(I)V", (void*) AVRational_release }
};

int register_android_media_FFMpegAVRational(JNIEnv *env) {
	jclass *clazz = AVRational_getClass(env);
	fields.constructor = (*env)->GetMethodID(env, clazz, "<init>", "()V");
	if (fields.constructor == NULL) {
		jniThrowException(env,
	                      "java/lang/RuntimeException",
	                      "can't load clb_onReport callback");
	}

	return jniRegisterNativeMethods(env, "com/media/ffmpeg/FFMpegAVRational", methods, sizeof(methods) / sizeof(methods[0]));
}
