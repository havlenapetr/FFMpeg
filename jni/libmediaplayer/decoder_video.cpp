#include <android/log.h>
#include "decoder_video.h"

#include "output.h"

extern "C" {
#include "libswscale/swscale.h"
} // end of extern C

#define TAG "FFMpegVideoDecoder"

DecoderVideo::DecoderVideo(AVStream* stream) : IDecoder(stream)
{
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
		err = "Couldn't allocate mFrame";
		return false;
	}
	
	mTempFrame = avcodec_alloc_frame();
	if (mTempFrame == NULL) {
		err = "Couldn't allocate mTempFrame";
		return false;
	}

	mConvertCtx = sws_getContext(mStream->codec->width,
								 mStream->codec->height,
								 mStream->codec->pix_fmt,
								 mStream->codec->width,
								 mStream->codec->height,
								 PIX_FMT_RGB565,
								 SWS_POINT,
								 NULL,
								 NULL,
								 NULL);
	if (mConvertCtx == NULL) {
		err = "Couldn't allocate mConvertCtx";
		return false;
	}

	if(Output::VideoDriver_getPixels(mStream->codec->width,
									 mStream->codec->height,
									 &pixels) != ANDROID_SURFACE_RESULT_SUCCESS) {
		err = "Couldn't get pixels from android surface wrapper";
		return false;
	}
	
	// Assign appropriate parts of buffer to image planes in pFrameRGB
	// Note that pFrameRGB is an AVFrame, but AVFrame is a superset
	// of AVPicture
	avpicture_fill((AVPicture *) mFrame, 
				   (uint8_t *)pixels, 
				   PIX_FMT_RGB565, 
				   mStream->codec->width,
				   mStream->codec->height);
	
	return true;
}

bool DecoderVideo::process(AVPacket *packet)
{
    int	completed;
	
	// Decode video frame
	avcodec_decode_video(mStream->codec,
						 mTempFrame,
						 &completed,
						 packet->data, 
						 packet->size);
	
	if (completed) {
		// Convert the image from its native format to RGB
		sws_scale(mConvertCtx,
			      mTempFrame->data,
			      mTempFrame->linesize,
				  0,
				  mStream->codec->height,
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
    // Free the RGB image
    av_free(mTempFrame);

    return true;
}
