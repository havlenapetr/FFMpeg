#ifndef FFMPEG_DECODER_AUDIO_H
#define FFMPEG_DECODER_AUDIO_H

#include "decoder.h"

class DecoderAudioCallback
{
public:
    virtual void onDecode(int16_t* buffer, int buffer_size);
};

class DecoderAudio : public IDecoder
{
public:
    DecoderAudio(AVStream* stream, DecoderAudioCallback* callback);
    ~DecoderAudio();


private:
    int16_t*                                            mSamples;
    int                                                 mSamplesSize;
    DecoderAudioCallback*                               mCallback;

    bool                                                prepare();
    bool                                                decode();
    bool                                                process(AVPacket *packet);
};

#endif //FFMPEG_DECODER_AUDIO_H
