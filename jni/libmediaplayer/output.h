#ifndef FFMPEG_OUTPUT_H
#define FFMPEG_OUTPUT_H

#include <jni.h>

#include <android/audiotrack.h>
#include <android/surface.h>

class Output
{
public:	
	static int					AudioDriver_register();
    static int					AudioDriver_set(int streamType,
												uint32_t sampleRate,
												int format,
												int channels);
    static int					AudioDriver_start();
    static int					AudioDriver_flush();
	static int					AudioDriver_stop();
    static int					AudioDriver_reload();
	static int					AudioDriver_write(void *buffer, int buffer_size);
	static int					AudioDriver_unregister();
	
	static int					VideoDriver_register(JNIEnv* env, jobject jsurface);
    static int					VideoDriver_getPixels(int width, int height, void** pixels);
    static int					VideoDriver_updateSurface();
    static int					VideoDriver_unregister();
};

#endif //FFMPEG_DECODER_H
