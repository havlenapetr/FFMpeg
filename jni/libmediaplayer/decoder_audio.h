#ifndef FFMPEG_DECODER_AUDIO_H
#define FFMPEG_DECODER_AUDIO_H

#include <pthread.h>

// map system drivers methods
#include <drivers_map.h>

#include "packetqueue.h"

struct DecoderAudioConfig
{
     int                streamType;
     int                sampleRate;
     int                format;
     int                channels;
};

class DecoderAudio
{
public:
    DecoderAudio(PacketQueue*               queue,
                 AVCodecContext*            codec_ctx,
                 struct DecoderAudioConfig* config);

    ~DecoderAudio();

    bool start(const char* err);
    bool startAsync(const char* err);
    int wait();
    void stop();

private:
    PacketQueue*                mQueue;
    AVCodecContext*             mCodecCtx;
    struct DecoderAudioConfig*  mConfig;
    bool                        mDecoding;
    pthread_t                   mThread;
    int16_t*                    mSamples;
    int                         mSamplesSize;

    bool                        prepare(const char *err);
    bool                        decode(void* ptr);
    bool                        process(AVPacket *packet);
    static void*                startDecoding(void* ptr);
};

#endif //FFMPEG_DECODER_AUDIO_H
