#ifndef _JNI_UTILS_H_
#define _JNI_UTILS_H_

#include <jni.h>

int jniThrowException(JNIEnv* env, const char* className, const char* msg);

JNIEnv* getJNIEnv();

int jniRegisterNativeMethods(JNIEnv* env,
                             const char* className,
                             const JNINativeMethod* gMethods,
                             int numMethods);

#endif /* _JNI_UTILS_H_ */
