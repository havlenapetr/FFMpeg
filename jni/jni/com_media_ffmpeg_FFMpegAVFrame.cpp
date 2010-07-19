#define TAG "android_media_FFMpegAVFrame"

#include <android/log.h>
#include "jniUtils.h"
#include "methods.h"

struct fields_t
{
    jmethodID constructor;
};
static struct fields_t fields;

jclass AVFrame_getClass(JNIEnv *env) {
	return env->FindClass("com/media/ffmpeg/FFMpegAVFrame");
}

const char *AVFrame_getClassSignature() {
	return "Lcom/media/ffmpeg/FFMpegAVFrame;";
}

jobject AVFrame_create(JNIEnv *env, AVFrame *frame) {
	jclass clazz = AVFrame_getClass(env);
	jobject result = env->NewObject(clazz, fields.constructor);

	env->SetIntField(result,
					 env->GetFieldID(clazz, "mPointer", "I"),
					 (jint)frame);
	env->SetIntField(result, 
					 env->GetFieldID(clazz, "mKeyFrame", "I"),
					 frame->key_frame);
	env->SetIntField(result, 
					 env->GetFieldID(clazz, "mPictType", "I"),
					 frame->pict_type);
	env->SetIntField(result, 
					 env->GetFieldID(clazz, "mCodedPictureNumber", "I"),
					 frame->coded_picture_number);
	env->SetIntField(result, 
					 env->GetFieldID(clazz, "mDisplayPictureNumber", "I"),
					 frame->display_picture_number);
	env->SetIntField(result, 
					 env->GetFieldID(clazz, "mQuality", "I"),
					 frame->quality);
	env->SetIntField(result, 
					 env->GetFieldID(clazz, "mAge", "I"),
					 frame->age);
	env->SetIntField(result, 
					 env->GetFieldID(clazz, "mReference", "I"),
					 frame->reference);
	env->SetIntField(result, 
					 env->GetFieldID(clazz, "mQstride", "I"),
					 frame->qstride);
	env->SetIntField(result, 
					 env->GetFieldID(clazz, "mType", "I"),
					 frame->type);
	env->SetIntField(result, 
					 env->GetFieldID(clazz, "mRepeatPict", "I"),
					 frame->repeat_pict);
	env->SetIntField(result, 
					 env->GetFieldID(clazz, "mQscaleType", "I"),
					 frame->qscale_type);
	env->SetIntField(result, 
					 env->GetFieldID(clazz, "mInterlacedFrame", "I"),
					 frame->interlaced_frame);
	env->SetIntField(result, 
					 env->GetFieldID(clazz, "mTopFieldFirst", "I"),
					 frame->top_field_first);
	env->SetIntField(result, 
					 env->GetFieldID(clazz, "mPaletteHasChanged", "I"),
					 frame->palette_has_changed);
	env->SetIntField(result, 
					 env->GetFieldID(clazz, "mBufferHints", "I"),
					 frame->buffer_hints);

	return result;
}

static void AVFrame_release(JNIEnv *env, jobject thiz, jint pointer) {
	//AVFormatContext *fileContext = (AVFormatContext *) pointer;
	//__android_log_print(ANDROID_LOG_INFO, TAG,  "releasing FFMpegAVFrame"),
	//free(fileContext);
}

/*
 * JNI registration.
 */
static JNINativeMethod methods[] = {
	{ "nativeRelease", "(I)V", (void*) AVFrame_release }
};

int register_android_media_FFMpegAVFrame(JNIEnv *env) {
	jclass clazz = AVFrame_getClass(env);
	fields.constructor = env->GetMethodID(clazz, "<init>", "()V");
	if (fields.constructor == NULL) {
		jniThrowException(env,
	                      "java/lang/RuntimeException",
	                      "can't load constructor");
	}

	return jniRegisterNativeMethods(env, "com/media/ffmpeg/FFMpegAVFrame", methods, sizeof(methods) / sizeof(methods[0]));
}
