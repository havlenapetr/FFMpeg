#include <android/log.h>

extern "C" {

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"

}

#include "output.h"
#include "renderer.h"

#define TAG "FFMpegRenderer"

Renderer::Renderer()
{
	mQueue = new PacketQueue();
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

PacketQueue* Renderer::queue()
{
	return mQueue;
}

bool Renderer::startAsync(const char* err)
{
	pthread_create(&mThread, NULL, startRendering, this);
    return true;
}

void* Renderer::startRendering(void* ptr)
{
	__android_log_print(ANDROID_LOG_INFO, TAG, "starting renderer thread");
	Renderer* renderer = (Renderer *) ptr;
	renderer->mRendering = true;
    renderer->render(ptr);
	renderer->mRendering = false;
	__android_log_print(ANDROID_LOG_INFO, TAG, "decoder renderer ended");
}

bool Renderer::render(void* ptr)
{
	AVPacket        pPacket;
	
	while(mRendering)
    {
        if(mQueue->get(&pPacket, true) < 0)
        {
            mRendering = false;
            return false;
        }

        // Free the packet that was allocated by av_read_frame
        av_free_packet(&pPacket);
    }
	
    return true;
}

int	Renderer::wait()
{
	return pthread_join(mThread, NULL);
}

void Renderer::stop()
{
	mQueue->abort();
    __android_log_print(ANDROID_LOG_INFO, TAG, "waiting on end of renderer thread");
    int ret = -1;
    if((ret = wait()) != 0) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "Couldn't cancel renderer: %i", ret);
        return;
    }
}