#include <android/log.h>
#include "decoder.h"

#define TAG "FFMpegIDecoder"

static IDecoder* sInstance;

IDecoder::IDecoder()
{
	mQueue = new PacketQueue();
    sInstance = this;
}

IDecoder::~IDecoder()
{
	if(mDecoding)
    {
        stop();
    }
	free(mQueue);
}

void IDecoder::enqueue(AVPacket* packet)
{
	mQueue->put(packet);
}

bool IDecoder::start(const char* err)
{
    if(!prepare(err))
    {
        return false;
    }
    return decode(NULL);
}

bool IDecoder::startAsync(const char* err)
{
    if(!prepare(err))
    {
        return false;
    }

    pthread_create(&mThread, NULL, startDecoding, NULL);
    return true;
}

int IDecoder::wait()
{
    return pthread_join(mThread, NULL);
}

void IDecoder::stop()
{
    mQueue->abort();
    __android_log_print(ANDROID_LOG_INFO, TAG, "waiting on end of audio IDecoder");
    int ret = -1;
    if((ret = wait()) != 0) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "Couldn't cancel audio IDecoder: %i", ret);
        return;
    }
    __android_log_print(ANDROID_LOG_INFO, TAG, "audio IDecoder stopped");
}

void* IDecoder::startDecoding(void* ptr)
{
    __android_log_print(ANDROID_LOG_INFO, TAG, "starting audio thread");
    sInstance->decode(ptr);
}

bool IDecoder::prepare(const char *err)
{
	err = "Not implemented";
    return false;
}

bool IDecoder::process(AVPacket *packet)
{
	return false;
}

bool IDecoder::decode(void* ptr)
{
    return false;
}
