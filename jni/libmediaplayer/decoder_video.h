#ifndef FFMPEG_DECODER_VIDEO_H
#define FFMPEG_DECODER_VIDEO_H

#include "decoder.h"

struct DecoderVideoConfig
{
	int					width;
	int					height;
	struct SwsContext*	img_convert_ctx;
	AVFrame*			frame;
};

class DecoderVideo : public IDecoder
{
public:
    DecoderVideo(AVCodecContext*			codec_ctx,
				 struct DecoderVideoConfig*	config);

    ~DecoderVideo();

private:
	struct DecoderVideoConfig*	mConfig;
	AVFrame*					mFrame;
    bool                        prepare(const char *err);
    bool                        decode(void* ptr);
    bool                        process(AVPacket *packet);
};

#endif //FFMPEG_DECODER_AUDIO_H
