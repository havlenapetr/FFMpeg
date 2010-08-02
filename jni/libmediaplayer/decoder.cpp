#include <android/log.h>
#include "decoder.h"

#define TAG "FFMpegIDecoder"

IDecoder::IDecoder(AVStream* stream)
{
	mQueue = new PacketQueue();
	mStream = stream;
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

int IDecoder::packets()
{
	return mQueue->size();
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

    pthread_create(&mThread, NULL, startDecoding, this);
    return true;
}

int IDecoder::wait()
{
	if (!mDecoding) {
		return 0;
	}
    return pthread_join(mThread, NULL);
}

void IDecoder::stop()
{
    mQueue->abort();
    __android_log_print(ANDROID_LOG_INFO, TAG, "waiting on end of decoder thread");
    int ret = -1;
    if((ret = wait()) != 0) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "Couldn't cancel audio IDecoder: %i", ret);
        return;
    }
}

void* IDecoder::startDecoding(void* ptr)
{
    __android_log_print(ANDROID_LOG_INFO, TAG, "starting decoder thread");
	IDecoder* decoder = (IDecoder *) ptr;
	decoder->mDecoding = true;
    decoder->decode(ptr);
	decoder->mDecoding = false;
	__android_log_print(ANDROID_LOG_INFO, TAG, "decoder thread ended");
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
