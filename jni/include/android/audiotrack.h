/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ANDROID_AUDIOTRACK_WRAPPER_H
#define ANDROID_AUDIOTRACK_WRAPPER_H

#include <stdint.h>
#include <jni.h>

#define ANDROID_AUDIOTRACK_RESULT_SUCCESS				 0
#define ANDROID_AUDIOTRACK_RESULT_BAD_PARAMETER			-1
#define ANDROID_AUDIOTRACK_RESULT_JNI_EXCEPTION			-2
#define ANDROID_AUDIOTRACK_RESULT_ALLOCATION_FAILED		-3
#define ANDROID_AUDIOTRACK_RESULT_ERRNO					-4

#ifdef __cplusplus
extern "C" {
#endif

int AndroidAudioTrack_register(JNIEnv* env, jobject jaudioTrack);
	
int AndroidAudioTrack_start();

int AndroidAudioTrack_flush();
	
int AndroidAudioTrack_stop();
	
int AndroidAudioTrack_reload();

int AndroidAudioTrack_unregister();

int AndroidAudioTrack_write(void *buffer, int buffer_size);
	
#ifdef __cplusplus
}
#endif

#endif
