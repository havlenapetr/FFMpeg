#include <android/log.h>
#include "decoder_video.h"

#include "output.h"

extern "C" {
#include "libswscale/swscale.h"
} // end of extern C

#define TAG "FFMpegVideoDecoder"

DecoderVideo::DecoderVideo(AVCodecContext*				codec_ctx,
						   struct DecoderVideoConfig*	config)
{
    mCodecCtx = codec_ctx; 
	mConfig = config;
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
	void*		pixels;
	
	mFrame = avcodec_alloc_frame();
	if (mFrame == NULL) {
		return false;
	}
	
	if(Output::VideoDriver_getPixels(mConfig->width, 
									 mConfig->height, 
									 &pixels) != ANDROID_SURFACE_RESULT_SUCCESS) {
		return false;
	}
	
	// Assign appropriate parts of buffer to image planes in pFrameRGB
	// Note that pFrameRGB is an AVFrame, but AVFrame is a superset
	// of AVPicture
	avpicture_fill((AVPicture *) mFrame, 
				   (uint8_t *)pixels, 
				   PIX_FMT_RGB565, 
				   mConfig->width, 
				   mConfig->height);
	
	return true;
}

bool DecoderVideo::process(AVPacket *packet)
{
    int	completed;
	
	// Decode video frame
	avcodec_decode_video(mCodecCtx, 
						 mConfig->frame, 
						 &completed,
						 packet->data, 
						 packet->size);
	
	if (completed) {
		// Convert the image from its native format to RGB
		sws_scale(mConfig->img_convert_ctx, 
				  mConfig->frame->data, 
				  mConfig->frame->linesize, 
				  0,
				  mConfig->height, 
				  mFrame->data,
				  mFrame->linesize);
		
		Output::VideoDriver_updateSurface();
		return true;
	}
	return false;
}

bool DecoderVideo::decode(void* ptr)
{
	AVPacket        pPacket;
	
	__android_log_print(ANDROID_LOG_INFO, TAG, "decoding video");
	
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
	
    __android_log_print(ANDROID_LOG_INFO, TAG, "decoding video ended");
	
    Output::VideoDriver_unregister();
	
    // Free the RGB image
    av_free(mFrame);
    return true;
}
