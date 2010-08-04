#include <android/log.h>

extern "C" {

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"

}

#include "output.h"
#include "renderer.h"

#define TAG "FFMpegRenderer"

Renderer::Renderer(AVStream* video_stream, AVStream* audio_stream)
{
	mVideoStream = video_stream;
	mAudioStream = audio_stream;
}

Renderer::~Renderer()
{
}

bool Renderer::init(JNIEnv *env, jobject jsurface)
{
	if(Output::VideoDriver_register(env, jsurface) != ANDROID_SURFACE_RESULT_SUCCESS) {
		return false;
	}
	if(Output::AudioDriver_register() != ANDROID_AUDIOTRACK_RESULT_SUCCESS) {
		return false;
	}
	return true;
}

bool Renderer::prepare(const char* err)
{
	if (Output::AudioDriver_set(MUSIC, mAudioStream->codec->sample_rate, PCM_16_BIT,
			(mAudioStream->codec->channels == 2) ? CHANNEL_OUT_STEREO
					: CHANNEL_OUT_MONO) != ANDROID_AUDIOTRACK_RESULT_SUCCESS) {
		err = "Couldnt' set audio track";
		return false;
	}
	if (Output::AudioDriver_start() != ANDROID_AUDIOTRACK_RESULT_SUCCESS) {
		err = "Couldnt' start audio track";
		return false;
	}
	return true;
}

void Renderer::handleRun(void* ptr)
{
	AVPacket        pPacket;
	
	while(true)
    {
    }
}

void Renderer::stop()
{
    __android_log_print(ANDROID_LOG_INFO, TAG, "waiting on end of renderer thread");
    int ret = -1;
    if((ret = wait()) != 0) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "Couldn't cancel renderer: %i", ret);
        return;
    }
}
