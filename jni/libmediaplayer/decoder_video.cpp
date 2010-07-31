#include <android/log.h>
#include "decoder_video.h"

#define TAG "FFMpegVideoDecoder"

DecoderVideo::DecoderVideo(AVCodecContext* codec_ctx)
{
    mQueue = new PacketQueue();
    mCodecCtx = codec_ctx; 
}

DecoderVideo::~DecoderVideo()
{
    if(mDecoding)
    {
        stop();
    }
}

bool DecoderVideo::prepare(const char *err)
{
	return false;
}

bool DecoderVideo::process(AVPacket *packet)
{
    return false;
}

bool DecoderVideo::decode(void* ptr)
{
	return false;
}
