#ifndef FFMPEG_DECODER_VIDEO_H
#define FFMPEG_DECODER_VIDEO_H

#include "decoder.h"

class DecoderVideo : public IDecoder
{
public:
    DecoderVideo(AVCodecContext* codec_ctx);

    ~DecoderVideo();

private:
	
    bool                        prepare(const char *err);
    bool                        decode(void* ptr);
    bool                        process(AVPacket *packet);
};

#endif //FFMPEG_DECODER_AUDIO_H
