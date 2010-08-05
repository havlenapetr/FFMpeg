#include <android/log.h>
#include "decoder_video.h"

#include "output.h"

extern "C" {
#include "libswscale/swscale.h"
} // end of extern C

#define TAG "FFMpegVideoDecoder"

static uint64_t global_video_pkt_pts = AV_NOPTS_VALUE;

DecoderVideo::DecoderVideo(AVStream* stream) : IDecoder(stream)
{
	 mStream->codec->get_buffer = getBuffer;
	 mStream->codec->release_buffer = releaseBuffer;
}

DecoderVideo::~DecoderVideo()
{
}

bool DecoderVideo::prepare()
{
	void*		pixels;
	
	mFrame = avcodec_alloc_frame();
	if (mFrame == NULL) {
		//err = "Couldn't allocate mFrame";
		return false;
	}
	
	mTempFrame = avcodec_alloc_frame();
	if (mTempFrame == NULL) {
		//err = "Couldn't allocate mTempFrame";
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
		//err = "Couldn't allocate mConvertCtx";
		return false;
	}

	if(Output::VideoDriver_getPixels(mStream->codec->width,
									 mStream->codec->height,
									 &pixels) != ANDROID_SURFACE_RESULT_SUCCESS) {
		//err = "Couldn't get pixels from android surface wrapper";
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

double DecoderVideo::synchronize(AVFrame *src_frame, double pts) {

	double frame_delay;

	if (pts != 0) {
		/* if we have pts, set video clock to it */
		mVideoClock = pts;
	} else {
		/* if we aren't given a pts, set it to the clock */
		pts = mVideoClock;
	}
	/* update the video clock */
	frame_delay = av_q2d(mStream->codec->time_base);
	/* if we are repeating a frame, adjust clock accordingly */
	frame_delay += src_frame->repeat_pict * (frame_delay * 0.5);
	mVideoClock += frame_delay;
	return pts;
}

bool DecoderVideo::process(AVPacket *packet)
{
    int	completed;
    int pts = 0;

	// Decode video frame
	avcodec_decode_video(mStream->codec,
						 mTempFrame,
						 &completed,
						 packet->data, 
						 packet->size);
	
	if (packet->dts == AV_NOPTS_VALUE && mTempFrame->opaque
			&& *(uint64_t*) mTempFrame->opaque != AV_NOPTS_VALUE) {
		pts = *(uint64_t *) mTempFrame->opaque;
	} else if (packet->dts != AV_NOPTS_VALUE) {
		pts = packet->dts;
	} else {
		pts = 0;
	}
	pts *= av_q2d(mStream->time_base);

	if (completed) {
		pts = synchronize(mTempFrame, pts);

		//onDecode(frame, pts);

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
	
    while(mRunning)
    {
        if(mQueue->get(&pPacket, true) < 0)
        {
            mRunning = false;
            return false;
        }
        if(!process(&pPacket))
        {
            mRunning = false;
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

/* These are called whenever we allocate a frame
 * buffer. We use this to store the global_pts in
 * a frame at the time it is allocated.
 */
int DecoderVideo::getBuffer(struct AVCodecContext *c, AVFrame *pic) {
	int ret = avcodec_default_get_buffer(c, pic);
	uint64_t *pts = (uint64_t *)av_malloc(sizeof(uint64_t));
	*pts = global_video_pkt_pts;
	pic->opaque = pts;
	return ret;
}
void DecoderVideo::releaseBuffer(struct AVCodecContext *c, AVFrame *pic) {
	if (pic)
		av_freep(&pic->opaque);
	avcodec_default_release_buffer(c, pic);
}
