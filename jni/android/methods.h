#ifndef _METHODS_H_
#define _METHODS_H_

#include <jni.h>

#include "../libavformat/avformat.h"

jobject *AVFormatContext_create(JNIEnv *env, AVFormatContext *fileContext);
jobject *AVRational_create(JNIEnv *env, AVRational *rational);
jobject *AVInputFormat_create(JNIEnv *env, AVInputFormat *format);

jclass *AVFormatContext_getClass(JNIEnv *env);
const char *AVInputFormat_getClassSignature();

#endif /* _METHODS_H_ */
