#include <android/log.h>

extern "C" {

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"

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
	void*		pixels;

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

	mConvertCtx = sws_getContext(mVideoStream->codec->width,
								 mVideoStream->codec->height,
								 mVideoStream->codec->pix_fmt,
								 mVideoStream->codec->width,
								 mVideoStream->codec->height,
								 PIX_FMT_RGB565,
								 SWS_POINT,
								 NULL,
								 NULL,
								 NULL);

	if (mConvertCtx == NULL) {
		//err = "Couldn't allocate mConvertCtx";
		return false;
	}

	if (Output::VideoDriver_getPixels(mVideoStream->codec->width,
									  mVideoStream->codec->height,
								      &pixels) != ANDROID_SURFACE_RESULT_SUCCESS) {
		//err = "Couldn't get pixels from android surface wrapper";
		return false;
	}

	// Assign appropriate parts of buffer to image planes in pFrameRGB
	// Note that pFrameRGB is an AVFrame, but AVFrame is a superset
	// of AVPicture
	avpicture_fill((AVPicture *) mFrame, (uint8_t *) pixels, PIX_FMT_RGB565,
			mVideoStream->codec->width, mVideoStream->codec->height);

	return true;
}

void Renderer::enqueue(Event* event)
{
	mEventBuffer.add(event);
	notify();
}

bool Renderer::processAudio(Event* event)
{
	AudioEvent* e = (AudioEvent *) event;
	if (Output::AudioDriver_write(e->samples, e->samples_size) <= 0) {
		return false;
	}
	return true;
}

bool Renderer::processVideo(Event* event)
{
	VideoEvent* e = (VideoEvent *) event;
	sws_scale(mConvertCtx,
			  e->frame->data,
			  e->frame->linesize,
			  0,
			  mVideoStream->codec->height,
			  mFrame->data,
		      mFrame->linesize);

	Output::VideoDriver_updateSurface();
	return true;
}

void Renderer::handleRun(void* ptr)
{
	AVPacket        pPacket;
	
	while(true)
    {
		waitOnNotify();

		int length = mEventBuffer.size();
		Event** events = mEventBuffer.editArray();
		mEventBuffer.clear();

		// execute this events
		for (int i = 0; i < length; i++) {
			Event *e = events[i];
			if(e->type == EVENT_TYPE_VIDEO) {
				processVideo(e);
			}
			else if(e->type == EVENT_TYPE_AUDIO) {
				processAudio(e);
			}
			else {
				__android_log_print(ANDROID_LOG_ERROR, TAG,
						"Failed to encode event type: %i", e->type);
			}
			free(e);
		}
    }
}

void Renderer::stop()
{
	notify();
    __android_log_print(ANDROID_LOG_INFO, TAG, "waiting on end of renderer thread");
    int ret = -1;
    if((ret = wait()) != 0) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "Couldn't cancel renderer: %i", ret);
        return;
    }
}
