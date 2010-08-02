#ifndef FFMPEG_DECODER_VIDEO_H
#define FFMPEG_DECODER_VIDEO_H

#include "decoder.h"

struct DecoderVideoConfig
{
	int					width;
	int					height;
	struct SwsContext*	img_convert_ctx;
};

class DecoderVideo : public IDecoder
{
public:
    DecoderVideo(AVStream*					stream,
				 struct DecoderVideoConfig*	config);

    ~DecoderVideo();

private:
	struct DecoderVideoConfig*	mConfig;
	AVFrame*					mFrame;
	AVFrame*					mTempFrame;
    bool                        prepare(const char *err);
    bool                        decode(void* ptr);
    bool                        process(AVPacket *packet);
};

#endif //FFMPEG_DECODER_AUDIO_H
