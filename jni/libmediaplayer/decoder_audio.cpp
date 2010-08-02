#include <android/log.h>
#include "decoder_audio.h"

#include "output.h"

#define TAG "FFMpegAudioDecoder"

DecoderAudio::DecoderAudio(AVStream*            		stream,
						   struct DecoderAudioConfig* 	config)  : IDecoder(stream)
{
    mConfig = config;
}

DecoderAudio::~DecoderAudio()
{
    if(mDecoding)
    {
        stop();
    }
}

bool DecoderAudio::prepare(const char *err)
{
    mSamplesSize = AVCODEC_MAX_AUDIO_FRAME_SIZE;
    mSamples = (int16_t *) av_malloc(mSamplesSize);

    if(Output::AudioDriver_set(mConfig->streamType,
							   mConfig->sampleRate,
							   mConfig->format,
							   mConfig->channels) != ANDROID_AUDIOTRACK_RESULT_SUCCESS) {
       err = "Couldnt' set audio track";
       return false;
    }
    if(Output::AudioDriver_start() != ANDROID_AUDIOTRACK_RESULT_SUCCESS) {
       err = "Couldnt' start audio track";
       return false;
    }
    return true;
}

bool DecoderAudio::process(AVPacket *packet)
{
    int size = mSamplesSize;
    int len = avcodec_decode_audio3(mStream->codec, mSamples, &size, packet);
    if(Output::AudioDriver_write(mSamples, size) <= 0) {
        return false;
    }
    return true;
}

bool DecoderAudio::decode(void* ptr)
{
    AVPacket        pPacket;

    __android_log_print(ANDROID_LOG_INFO, TAG, "decoding audio");

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

    Output::AudioDriver_unregister();

    // Free audio samples buffer
    av_free(mSamples);
    return true;
}
