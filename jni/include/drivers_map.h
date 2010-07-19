#ifndef _DRIVERS_MAP_H_
#define _DRIVERS_MAP_H_

// create pointer on system methods for accessing audio and video system
#ifdef ANDROID

#include <jni.h>
#include <android/audiotrack.h>
#include <android/surface.h>

int (*AudioDriver_register) (JNIEnv*, jobject) = AndroidAudioTrack_register;
int (*AudioDriver_set) (int,uint32_t,int,int,int) = AndroidAudioTrack_set;
int (*AudioDriver_start) (void) = AndroidAudioTrack_start;
int (*AudioDriver_flush) (void) = AndroidAudioTrack_flush;
int (*AudioDriver_stop) (void) = AndroidAudioTrack_stop;
int (*AudioDriver_reload) (void) = AndroidAudioTrack_reload;
int (*AudioDriver_unregister) (void) = AndroidAudioTrack_unregister;
int (*AudioDriver_write) (void *, int) = AndroidAudioTrack_write;

int (*VideoDriver_register) (JNIEnv*, jobject) = AndroidSurface_register;
int (*VideoDriver_getPixels) (int width, int height, void** pixels) = AndroidSurface_getPixels;
int (*VideoDriver_updateSurface) (void) = AndroidSurface_updateSurface;
int (*VideoDriver_unregister) (void) = AndroidSurface_unregister;

#endif

#endif
