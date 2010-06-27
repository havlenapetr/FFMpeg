#ifndef _JNI_UTILS_H_
#define _JNI_UTILS_H_

#include <jni.h>

int jniThrowException(JNIEnv* env, const char* className, const char* msg);

JNIEnv* getJNIEnv();

int jniRegisterNativeMethods(JNIEnv* env,
                             const char* className,
                             const JNINativeMethod* gMethods,
                             int numMethods);

int getArgs(JNIEnv* env, jobjectArray *args, int *argc, char **argv) {
	int i = 0;
	int _argc = 0;
	char **_argv = NULL;
	if (args == NULL) {
		return 1;
	}
	
	_argc = (*env)->GetArrayLength(env, args);
	_argv = (char **) malloc(sizeof(char *) * _argc);
	for(i=0;i<_argc;i++) {
		jstring str = (jstring)(*env)->GetObjectArrayElement(env, args, i);
		_argv[i] = (char *)(*env)->GetStringUTFChars(env, str, NULL);   
	}
	
	argc = _argc;
	argv = _argv;
	
	return 0;
};

#endif /* _JNI_UTILS_H_ */
