#define TAG "android_media_FFMpegAVCodecContext"

#include <android/log.h>
#include "jniUtils.h"
#include "methods.h"

struct fields_t
{
    jmethodID constructor;
};
static struct fields_t fields;

jclass AVCodecContext_getClass(JNIEnv *env) {
	return env->FindClass("com/media/ffmpeg/FFMpegAVCodecContext");
}

const char *AVCodecContext_getClassSignature() {
	return "Lcom/media/ffmpeg/FFMpegAVCodecContext;";
}

jobject AVCodecContext_create(JNIEnv *env, AVCodecContext *codecContext) {
	jclass clazz = AVCodecContext_getClass(env);
	jobject result = env->NewObject(clazz, fields.constructor);

	//env->SetIntField(result,
	//				 env->GetFieldID(clazz, "mPointer", "I"),
	//				 (jint)frame);
	env->SetIntField(result, 
					 env->GetFieldID(clazz, "mBitRate", "I"),
					 codecContext->bit_rate);
	env->SetIntField(result, 
					 env->GetFieldID(clazz, "mBitRateTolerance", "I"),
					 codecContext->bit_rate_tolerance);
	env->SetIntField(result, 
					 env->GetFieldID(clazz, "mFlags", "I"),
					 codecContext->flags);
	env->SetIntField(result, 
					 env->GetFieldID(clazz, "mSubId", "I"),
					 codecContext->sub_id);
	env->SetIntField(result, 
					 env->GetFieldID(clazz, "mMeMethod", "I"),
					 codecContext->me_method);
	env->SetIntField(result, 
					 env->GetFieldID(clazz, "mExtradataSize", "I"),
					 codecContext->extradata_size);
	env->SetIntField(result, 
					 env->GetFieldID(clazz, "mWidth", "I"),
					 codecContext->width);
	env->SetIntField(result, 
					 env->GetFieldID(clazz, "mHeight", "I"),
					 codecContext->height);
	env->SetIntField(result, 
					 env->GetFieldID(clazz, "mGopSize", "I"),
					 codecContext->gop_size);
	env->SetIntField(result, 
					 env->GetFieldID(clazz, "mRateEmu", "I"),
					 codecContext->rate_emu);
	env->SetIntField(result, 
					 env->GetFieldID(clazz, "mSampleRate", "I"),
					 codecContext->sample_rate);
	env->SetIntField(result, 
					 env->GetFieldID(clazz, "mChannels", "I"),
					 codecContext->channels);
	env->SetIntField(result, 
					 env->GetFieldID(clazz, "mFrameSize", "I"),
					 codecContext->frame_size);
	env->SetIntField(result, 
					 env->GetFieldID(clazz, "mFrameNumber", "I"),
					 codecContext->frame_number);

	return result;
}

static void AVCodecContext_release(JNIEnv *env, jobject thiz) {
	//AVFormatContext *fileContext = (AVFormatContext *) pointer;
	//__android_log_print(ANDROID_LOG_INFO, TAG,  "releasing FFMpegAVCodecContext"),
	//free(fileContext);
}

/*
 * JNI registration.
 */
static JNINativeMethod methods[] = {
	{ "release", "()V", (void*) AVCodecContext_release }
};

int register_android_media_FFMpegAVCodecContext(JNIEnv *env) {
	jclass clazz = AVCodecContext_getClass(env);
	fields.constructor = env->GetMethodID(clazz, "<init>", "()V");
	if (fields.constructor == NULL) {
		jniThrowException(env,
	                      "java/lang/RuntimeException",
	                      "can't load constructor");
	}

	return jniRegisterNativeMethods(env, "com/media/ffmpeg/FFMpegAVCodecContext", methods, sizeof(methods) / sizeof(methods[0]));
}
