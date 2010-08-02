#ifndef FFMPEG_DECODER_AUDIO_H
#define FFMPEG_DECODER_AUDIO_H

#include "decoder.h"

struct DecoderAudioConfig
{
     int                streamType;
     int                sampleRate;
     int                format;
     int                channels;
};

class DecoderAudio : public IDecoder
{
public:
    DecoderAudio(AVStream*            		stream,
                 struct DecoderAudioConfig* config);

    ~DecoderAudio();

private:
    struct DecoderAudioConfig*  mConfig;
    int16_t*                    mSamples;
    int                         mSamplesSize;

    bool                        prepare(const char *err);
    bool                        decode(void* ptr);
    bool                        process(AVPacket *packet);
};

#endif //FFMPEG_DECODER_AUDIO_H
