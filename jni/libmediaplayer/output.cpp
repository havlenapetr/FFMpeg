#include <android/log.h>
#include "output.h"

#define TAG "FFMpegOutput"

//-------------------- Audio driver --------------------

int Output::AudioDriver_register()
{
	return AndroidAudioTrack_register();
}

int Output::AudioDriver_unregister()
{
	return AndroidAudioTrack_unregister();
}

int Output::AudioDriver_start()
{
	return AndroidAudioTrack_start();
}

int Output::AudioDriver_set(int streamType,
							uint32_t sampleRate,
							int format,
							int channels)
{
	return AndroidAudioTrack_set(streamType,
								 sampleRate,
								 format,
								 channels);
}

int Output::AudioDriver_flush()
{
	return AndroidAudioTrack_flush();
}

int Output::AudioDriver_stop()
{
	return AndroidAudioTrack_stop();
}

int Output::AudioDriver_reload()
{
	return AndroidAudioTrack_reload();
}

int Output::AudioDriver_write(void *buffer, int buffer_size)
{
	return AndroidAudioTrack_write(buffer, buffer_size);
}

//-------------------- Video driver --------------------

int Output::VideoDriver_register(JNIEnv* env, jobject jsurface)
{
	return AndroidSurface_register(env, jsurface);
}

int Output::VideoDriver_unregister()
{
	return AndroidSurface_unregister();
}

int Output::VideoDriver_getPixels(int width, int height, void** pixels)
{
	return AndroidSurface_getPixels(width, height, pixels);
}

int Output::VideoDriver_updateSurface()
{
	return AndroidSurface_updateSurface();
}