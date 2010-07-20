#ifndef _DRIVERS_MAP_H_
#define _DRIVERS_MAP_H_

#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif

// methods for access to audio on OS
typedef int (*audioDriver_register_t) (JNIEnv*, jobject);
typedef int (*audioDriver_set_t) (int,uint32_t,int,int,int);
typedef int (*audioDriver_start_t) (void);
typedef int (*audioDriver_flush_t) (void);
typedef int (*audioDriver_stop_t) (void);
typedef int (*audioDriver_reload_t) (void);
typedef int (*audioDriver_unregister_t) (void);
typedef int (*audioDriver_write_t) (void *, int);

// methods for access to surface on OS
typedef int (*videoDriver_register_t) (JNIEnv*, jobject);
typedef int (*videoDriver_getPixels_t) (int width, int height, void** pixels);
typedef int (*videoDriver_updateSurface_t) (void);
typedef int (*videoDriver_unregister_t) (void);

// create pointer on system methods for accessing audio and video system
#ifdef ANDROID

#include <android/audiotrack.h>
#include <android/surface.h>

audioDriver_register_t AudioDriver_register = AndroidAudioTrack_register;
audioDriver_set_t AudioDriver_set = AndroidAudioTrack_set;
audioDriver_start_t AudioDriver_start = AndroidAudioTrack_start;
audioDriver_flush_t AudioDriver_flush = AndroidAudioTrack_flush;
audioDriver_stop_t AudioDriver_stop = AndroidAudioTrack_stop;
audioDriver_reload_t AudioDriver_reload = AndroidAudioTrack_reload;
audioDriver_write_t AudioDriver_write = AndroidAudioTrack_write;
audioDriver_unregister_t AudioDriver_unregister = AndroidAudioTrack_unregister;

videoDriver_register_t VideoDriver_register = AndroidSurface_register;
videoDriver_getPixels_t VideoDriver_getPixels = AndroidSurface_getPixels;
videoDriver_updateSurface_t VideoDriver_updateSurface = AndroidSurface_updateSurface;
videoDriver_unregister_t VideoDriver_unregister = AndroidSurface_unregister;

#ifdef __cplusplus
} // end of cpluplus
#endif

#endif

#endif
