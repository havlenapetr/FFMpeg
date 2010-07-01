#ifndef _METHODS_H_
#define _METHODS_H_

#include <jni.h>

#ifdef __cplusplus
extern "C"
{
#endif

#include "../libavformat/avformat.h"

jobject AVFormatContext_create(JNIEnv *env, AVFormatContext *fileContext);
jobject *AVRational_create(JNIEnv *env, AVRational *rational);
jobject *AVInputFormat_create(JNIEnv *env, AVInputFormat *format);

jclass *AVFormatContext_getClass(JNIEnv *env);
const char *AVInputFormat_getClassSignature();

#ifdef __cplusplus
}
#endif

#endif /* _METHODS_H_ */
