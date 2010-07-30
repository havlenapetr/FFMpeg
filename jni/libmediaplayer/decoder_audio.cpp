#include <android/log.h>
#include "decoder_audio.h"

extern "C" {

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"

} // end of extern C

#define TAG "FFMpegAudioDecoder"

static DecoderAudio* sInstance;

DecoderAudio::DecoderAudio(PacketQueue*               queue,
                           AVCodecContext*            codec_ctx,
                           struct DecoderAudioConfig* config)
{
    mQueue = queue;
    mCodecCtx = codec_ctx;
    mConfig = config;
    sInstance = this;
}

DecoderAudio::~DecoderAudio()
{
    if(mDecoding)
    {
        stop();
    }
}

bool DecoderAudio::start(const char* err)
{
    if(!prepare(err))
    {
        return false;
    }
    return decode(NULL);
}

bool DecoderAudio::startAsync(const char* err)
{
    if(!prepare(err))
    {
        return false;
    }

    pthread_create(&mThread, NULL, startDecoding, NULL);
    return true;
}

int DecoderAudio::wait()
{
    return pthread_join(mThread, NULL);
}

void DecoderAudio::stop()
{
    mQueue->abort();
    __android_log_print(ANDROID_LOG_INFO, TAG, "waiting on end of audio decoder");
    int ret = -1;
    if((ret = wait()) != 0) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "Couldn't cancel audio decoder: %i", ret);
        return;
    }
    __android_log_print(ANDROID_LOG_INFO, TAG, "audio decoder stopped");
}

void* DecoderAudio::startDecoding(void* ptr)
{
    __android_log_print(ANDROID_LOG_INFO, TAG, "starting audio thread");
    sInstance->decode(ptr);
}

bool DecoderAudio::prepare(const char *err)
{
    mSamplesSize = AVCODEC_MAX_AUDIO_FRAME_SIZE;
    mSamples = (int16_t *) av_malloc(mSamplesSize);

    if(AudioDriver_set(mConfig->streamType,
                       mConfig->sampleRate,
                       mConfig->format,
                       mConfig->channels) != ANDROID_AUDIOTRACK_RESULT_SUCCESS) {
       err = "Couldnt' set audio track";
       return false;
    }
    if(AudioDriver_start() != ANDROID_AUDIOTRACK_RESULT_SUCCESS) {
       err = "Couldnt' start audio track";
       return false;
    }
    return true;
}

bool DecoderAudio::process(AVPacket *packet)
{
    int size = mSamplesSize;
    int len = avcodec_decode_audio3(mCodecCtx, mSamples, &size, packet);
    if(AudioDriver_write(mSamples, size) <= 0) {
        return false;
    }
    return true;
}

bool DecoderAudio::decode(void* ptr)
{
    AVPacket        pPacket;

    __android_log_print(ANDROID_LOG_INFO, TAG, "decoding audio");

    mDecoding = true;
    while(mDecoding)
    {
        if(mQueue->get(&pPacket, true) < 0)
        {
            mDecoding = false;
            return false;
        }
        if(!process(&pPacket))
        {
            mDecoding = false;
            return false;
        }
        // Free the packet that was allocated by av_read_frame
        av_free_packet(&pPacket);
    }

    __android_log_print(ANDROID_LOG_INFO, TAG, "decoding audio ended");

    AudioDriver_unregister();

    // Free audio samples buffer
    av_free(mSamples);
    return true;
}
