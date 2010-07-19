#define TAG "android_media_FFMpegAVInputFormat"

#include <android/log.h>
#include "jniUtils.h"
#include "methods.h"

struct fields_t
{
    jmethodID constructor;
};
static struct fields_t fields;

jclass *AVInputFormat_getClass(JNIEnv *env) {
	return (*env)->FindClass(env, "com/media/ffmpeg/FFMpegAVInputFormat");
}

const char *AVInputFormat_getClassSignature() {
	return "Lcom/media/ffmpeg/FFMpegAVInputFormat;";
}

jobject *AVInputFormat_create(JNIEnv *env, AVInputFormat *format) {
	jclass *clazz = AVInputFormat_getClass(env);
	jobject result = (*env)->NewObject(env, clazz, fields.constructor);

	(*env)->SetIntField(env, result, 
						(*env)->GetFieldID(env, clazz, "mPrivDataSize", "I"), 
						format->priv_data_size);
	(*env)->SetIntField(env, result, 
						(*env)->GetFieldID(env, clazz, "mFlags", "I"), 
						format->flags);
	(*env)->SetIntField(env, result, 
						(*env)->GetFieldID(env, clazz, "mValue", "I"), 
						format->value);
	
	(*env)->SetObjectField(env, result, 
						   (*env)->GetFieldID(env, clazz, "mName", "Ljava/lang/String;"),
						   (*env)->NewStringUTF(env, format->name));
	(*env)->SetObjectField(env, result, 
						   (*env)->GetFieldID(env, clazz, "mName", "Ljava/lang/String;"),
						   (*env)->NewStringUTF(env, format->long_name));
	(*env)->SetObjectField(env, result, 
						   (*env)->GetFieldID(env, clazz, "mExtensions","Ljava/lang/String;"),
						   (*env)->NewStringUTF(env, format->extensions));

	return result;
}

static void AVInputFormat_release(JNIEnv *env, jobject thiz, jint pointer) {
	//AVFormatContext *fileContext = (AVFormatContext *) pointer;
	//__android_log_print(ANDROID_LOG_INFO, TAG,  "releasing FFMpegAVInputFormat"),
	//free(fileContext);
}

/*
 * JNI registration.
 */
static JNINativeMethod methods[] = {
	{ "nativeRelease", "(I)V", (void*) AVInputFormat_release }
};

int register_android_media_FFMpegAVInputFormat(JNIEnv *env) {
	jclass *clazz = AVInputFormat_getClass(env);
	fields.constructor = (*env)->GetMethodID(env, clazz, "<init>", "()V");
	if (fields.constructor == NULL) {
		jniThrowException(env,
	                      "java/lang/RuntimeException",
	                      "can't load constructor");
	}

	return jniRegisterNativeMethods(env, "com/media/ffmpeg/FFMpegAVInputFormat", methods, sizeof(methods) / sizeof(methods[0]));
}
