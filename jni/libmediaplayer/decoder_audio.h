#ifndef FFMPEG_DECODER_AUDIO_H
#define FFMPEG_DECODER_AUDIO_H

#include "decoder.h"
#include "renderer.h"

class DecoderAudio : public IDecoder
{
public:
    DecoderAudio(Renderer* renderer);

    ~DecoderAudio();

private:
    int16_t*                    mSamples;
    int                         mSamplesSize;

    bool                        prepare();
    bool                        decode(void* ptr);
    bool                        process(AVPacket *packet);
};

#endif //FFMPEG_DECODER_AUDIO_H
