/*
 * mediaplayer.cpp
 */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

extern "C" {
	
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/log.h"
	
} // end of extern C

#include <android/log.h>

#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

#include "mediaplayer.h"
#include "output.h"

#define FPS_DEBUGGING false

#define LOG_TAG "FFMpegMediaPlayer"

static MediaPlayer* sPlayer;

class VideoCallback : public DecoderVideoCallback
{
public:
    virtual void onDecode(AVFrame* frame, double pts);
};

void VideoCallback::onDecode(AVFrame* frame, double pts)
{
    if(FPS_DEBUGGING)
    {
        timeval pTime;
        static int frames = 0;
        static double t1 = -1;
        static double t2 = -1;

        gettimeofday(&pTime, NULL);
        t2 = pTime.tv_sec + (pTime.tv_usec / 1000000.0);
        if (t1 == -1 || t2 > t1 + 1) {
            LOGI("Video fps:%i", frames);
	    //sPlayer->notify(MEDIA_INFO_FRAMERATE_VIDEO, frames, -1);
	    t1 = t2;
	    frames = 0;
	}
	frames++;
    }

    // Convert the image from its native format to RGB
    sws_scale(sPlayer->mConvertCtx,
	      frame->data,
	      frame->linesize,
              0,
	      sPlayer->mVideoHeight,
	      sPlayer->mFrame->data,
              sPlayer->mFrame->linesize);

    Output::VideoDriver_updateSurface();
}

class AudioCallback : public DecoderAudioCallback
{
public:
    virtual void onDecode(int16_t* buffer, int buffer_size);
};

void AudioCallback::onDecode(int16_t* buffer, int buffer_size)
{
    if(FPS_DEBUGGING)
    {
        timeval pTime;
        static int frames = 0;
        static double t1 = -1;
        static double t2 = -1;

        gettimeofday(&pTime, NULL);
        t2 = pTime.tv_sec + (pTime.tv_usec / 1000000.0);
        if (t1 == -1 || t2 > t1 + 1) {
            LOGI("Video fps:%i", frames);
	    //sPlayer->notify(MEDIA_INFO_FRAMERATE_VIDEO, frames, -1);
	    t1 = t2;
	    frames = 0;
	}
	frames++;
    }

    if(Output::AudioDriver_write(buffer, buffer_size) <= 0) {
        LOGE("Couldn't write samples to audio track");
    }
}

DecodeLoop::DecodeLoop(AVFormatContext* context, int audioStreamId, int videoStreamId, DecodeLoopCallback* callback)
{
    mCallback = callback;
    mContext = context;
    mAudioStreamId = audioStreamId;
    mVideoStreamId = videoStreamId;
    mEnding = false;
}

DecodeLoop::~DecodeLoop()
{
    LOGI("killing decode loop");
	
    mEnding = true;

    if(mDecoderAudio != NULL) {
        mDecoderAudio->stop();
    }
    if(mDecoderVideo != NULL) {
	mDecoderVideo->stop();
    }
	
    if(join() != 0) {
        LOGE("Couldn't cancel player thread");
    }
	
    // Close the codec
    delete mDecoderAudio;
    delete mDecoderVideo;
}

void DecodeLoop::run()
{
    AVPacket pPacket;
	
    AVStream* stream_audio = mContext->streams[mAudioStreamId];
    mDecoderAudio = new DecoderAudio(stream_audio, new AudioCallback());
    mDecoderAudio->start();
	
    AVStream* stream_video = mContext->streams[mVideoStreamId];
    mDecoderVideo = new DecoderVideo(stream_video, new VideoCallback());
    mDecoderVideo->start();
    
    LOGI("playing");
	
    while (!mEnding)
    {
        if (mDecoderVideo->packets() > FFMPEG_PLAYER_MAX_QUEUE_SIZE &&
                mDecoderAudio->packets() > FFMPEG_PLAYER_MAX_QUEUE_SIZE) {
            usleep(200);
            continue;
        }
		
	
        if(av_read_frame(mContext, &pPacket) < 0) {
            mEnding = true;
            continue;
        }
		
        // Is this a packet from the video stream?
        if (pPacket.stream_index == mVideoStreamId) {
            mDecoderVideo->enqueue(&pPacket);
        } 
        else if (pPacket.stream_index == mAudioStreamId) {
            mDecoderAudio->enqueue(&pPacket);
        }
        else {
            // Free the packet that was allocated by av_read_frame
            av_free_packet(&pPacket);
        }
    }

    //waits on end of video thread
    LOGI("waiting on video thread");
    int ret = -1;
    if((ret = mDecoderVideo->join()) != 0) {
        LOGE("Couldn't cancel video thread: %i", ret);
    }
	
    LOGI("waiting on audio thread");
    if((ret = mDecoderAudio->join()) != 0) {
        LOGE("Couldn't cancel audio thread: %i", ret);
    }

    mCallback->onCompleted();
}


MediaPlayer::MediaPlayer()
{
    mDecodingLoop = NULL;
    mListener = NULL;
    mCookie = NULL;
    mDuration = -1;
    mStreamType = MUSIC;
    mCurrentPosition = -1;
    mSeekPosition = -1;
    mCurrentState = MEDIA_PLAYER_IDLE;
    mPrepareSync = false;
    mPrepareStatus = NO_ERROR;
    mLoop = false;
    mLeftVolume = mRightVolume = 1.0;
    mVideoWidth = mVideoHeight = 0;
    sPlayer = this;
}

MediaPlayer::~MediaPlayer()
{
	if(mListener != NULL) {
		free(mListener);
	}
}

status_t MediaPlayer::prepareAudio()
{
	LOGI("prepareAudio");
	mAudioStreamIndex = -1;
	for (int i = 0; i < mMovieFile->nb_streams; i++) {
		if (mMovieFile->streams[i]->codec->codec_type == CODEC_TYPE_AUDIO) {
			mAudioStreamIndex = i;
			break;
		}
	}
	
	if (mAudioStreamIndex == -1) {
		return INVALID_OPERATION;
	}

	AVStream* stream = mMovieFile->streams[mAudioStreamIndex];
	// Get a pointer to the codec context for the video stream
	AVCodecContext* codec_ctx = stream->codec;
	AVCodec* codec = avcodec_find_decoder(codec_ctx->codec_id);
	if (codec == NULL) {
		return INVALID_OPERATION;
	}
	
	// Open codec
	if (avcodec_open(codec_ctx, codec) < 0) {
		return INVALID_OPERATION;
	}

	// prepare os output
	if (Output::AudioDriver_set(MUSIC,
								stream->codec->sample_rate,
								PCM_16_BIT,
								(stream->codec->channels == 2) ? CHANNEL_OUT_STEREO
										: CHANNEL_OUT_MONO) != ANDROID_AUDIOTRACK_RESULT_SUCCESS) {
		return INVALID_OPERATION;
	}

	if (Output::AudioDriver_start() != ANDROID_AUDIOTRACK_RESULT_SUCCESS) {
		return INVALID_OPERATION;
	}

	return NO_ERROR;
}

status_t MediaPlayer::prepareVideo()
{
	LOGI("prepareVideo");
	// Find the first video stream
	mVideoStreamIndex = -1;
	for (int i = 0; i < mMovieFile->nb_streams; i++) {
		if (mMovieFile->streams[i]->codec->codec_type == CODEC_TYPE_VIDEO) {
			mVideoStreamIndex = i;
			break;
		}
	}
	
	if (mVideoStreamIndex == -1) {
		return INVALID_OPERATION;
	}
	
	AVStream* stream = mMovieFile->streams[mVideoStreamIndex];
	// Get a pointer to the codec context for the video stream
	AVCodecContext* codec_ctx = stream->codec;
	AVCodec* codec = avcodec_find_decoder(codec_ctx->codec_id);
	if (codec == NULL) {
		return INVALID_OPERATION;
	}
	
	// Open codec
	if (avcodec_open(codec_ctx, codec) < 0) {
		return INVALID_OPERATION;
	}
	
	mVideoWidth = codec_ctx->width;
	mVideoHeight = codec_ctx->height;
	mDuration =  mMovieFile->duration;
	
	mConvertCtx = sws_getContext(stream->codec->width,
								 stream->codec->height,
								 stream->codec->pix_fmt,
								 stream->codec->width,
								 stream->codec->height,
								 PIX_FMT_RGB565,
								 SWS_POINT,
								 NULL,
								 NULL,
								 NULL);

	if (mConvertCtx == NULL) {
		return INVALID_OPERATION;
	}

	void*		pixels;
	if (Output::VideoDriver_getPixels(stream->codec->width,
									  stream->codec->height,
									  &pixels) != ANDROID_SURFACE_RESULT_SUCCESS) {
		return INVALID_OPERATION;
	}

	mFrame = avcodec_alloc_frame();
	if (mFrame == NULL) {
		return INVALID_OPERATION;
	}
	// Assign appropriate parts of buffer to image planes in pFrameRGB
	// Note that pFrameRGB is an AVFrame, but AVFrame is a superset
	// of AVPicture
	avpicture_fill((AVPicture *) mFrame,
				   (uint8_t *) pixels,
				   PIX_FMT_RGB565,
				   stream->codec->width,
				   stream->codec->height);

	return NO_ERROR;
}

status_t MediaPlayer::prepare()
{
	status_t ret;
	mCurrentState = MEDIA_PLAYER_PREPARING;
	av_log_set_callback(ffmpegNotify);
	if ((ret = prepareVideo()) != NO_ERROR) {
		mCurrentState = MEDIA_PLAYER_STATE_ERROR;
		return ret;
	}
	if ((ret = prepareAudio()) != NO_ERROR) {
		mCurrentState = MEDIA_PLAYER_STATE_ERROR;
		return ret;
	}
	mCurrentState = MEDIA_PLAYER_PREPARED;
	return NO_ERROR;
}

status_t MediaPlayer::setListener(MediaPlayerListener* listener)
{
    LOGI("setListener");
    mListener = listener;
    return NO_ERROR;
}

status_t MediaPlayer::setDataSource(const char *url)
{
    LOGI("setDataSource(%s)", url);
    status_t err = BAD_VALUE;
	// Open video file
	if(av_open_input_file(&mMovieFile, url, NULL, 0, NULL) != 0) {
		return INVALID_OPERATION;
	}
	// Retrieve stream information
	if(av_find_stream_info(mMovieFile) < 0) {
		return INVALID_OPERATION;
	}
	mCurrentState = MEDIA_PLAYER_INITIALIZED;
    return NO_ERROR;
}

status_t MediaPlayer::suspend() {
    LOGI("suspend");

    mCurrentState = MEDIA_PLAYER_STOPPED;
	
    delete mDecodingLoop;
    mDecodingLoop = NULL;
	
    // Close the video file
    av_close_input_file(mMovieFile);

    //close OS drivers
    Output::AudioDriver_unregister();
    Output::VideoDriver_unregister();

    LOGE("suspended");
    return NO_ERROR;
}

void MediaPlayer::onCompleted()
{
    if(mCurrentState == MEDIA_PLAYER_STATE_ERROR) {
        LOGI("playing err");
    }

    mCurrentState = MEDIA_PLAYER_PLAYBACK_COMPLETE;
    LOGI("end of playing");
}

status_t MediaPlayer::resume() {
    Mutex::AutoLock _l(&mLock);

    mCurrentState = MEDIA_PLAYER_STARTED;
    return NO_ERROR;
}

status_t MediaPlayer::setVideoSurface(JNIEnv* env, jobject jsurface)
{ 
	if(Output::VideoDriver_register(env, jsurface) != ANDROID_SURFACE_RESULT_SUCCESS) {
		return INVALID_OPERATION;
	}
	if(Output::AudioDriver_register() != ANDROID_AUDIOTRACK_RESULT_SUCCESS) {
		return INVALID_OPERATION;
	}	
    return NO_ERROR;
}

bool MediaPlayer::shouldCancel(PacketQueue* queue)
{
	return (mCurrentState == MEDIA_PLAYER_STATE_ERROR || mCurrentState == MEDIA_PLAYER_STOPPED ||
			 ((mCurrentState == MEDIA_PLAYER_DECODED || mCurrentState == MEDIA_PLAYER_STARTED) 
			  && queue->size() == 0));
}

status_t MediaPlayer::start()
{
    Mutex::AutoLock _l(&mLock);

    if (mDecodingLoop != NULL || mCurrentState != MEDIA_PLAYER_PREPARED) {
        return INVALID_OPERATION;
    }

    mDecodingLoop = new DecodeLoop(mMovieFile, mAudioStreamIndex, mVideoStreamIndex, this);
    mDecodingLoop->start();

    return NO_ERROR;
}

status_t MediaPlayer::stop()
{
    Mutex::AutoLock _l(&mLock);

    mCurrentState = MEDIA_PLAYER_STOPPED;
    return NO_ERROR;
}

status_t MediaPlayer::pause()
{
    Mutex::AutoLock _l(&mLock);

    mCurrentState = MEDIA_PLAYER_PAUSED;
    return NO_ERROR;
}

bool MediaPlayer::isPlaying()
{
    Mutex::AutoLock _l(&mLock);
    return mCurrentState == MEDIA_PLAYER_STARTED || 
		mCurrentState == MEDIA_PLAYER_DECODED;
}

status_t MediaPlayer::getVideoWidth(int *w)
{
    Mutex::AutoLock _l(&mLock);
	
    if (mCurrentState < MEDIA_PLAYER_PREPARED) {
        return INVALID_OPERATION;
    }
    *w = mVideoWidth;
    return NO_ERROR;
}

status_t MediaPlayer::getVideoHeight(int *h)
{
	if (mCurrentState < MEDIA_PLAYER_PREPARED) {
		return INVALID_OPERATION;
	}
	*h = mVideoHeight;
    return NO_ERROR;
}

status_t MediaPlayer::getCurrentPosition(int *msec)
{
    Mutex::AutoLock _l(&mLock);

    if (mCurrentState < MEDIA_PLAYER_PREPARED) {
        return INVALID_OPERATION;
    }
    *msec = 0/*av_gettime()*/;
    //LOGI("position %i", *msec);
    return NO_ERROR;
}

status_t MediaPlayer::getDuration(int *msec)
{
    Mutex::AutoLock _l(&mLock);

    if (mCurrentState < MEDIA_PLAYER_PREPARED) {
        return INVALID_OPERATION;
    }
    *msec = mDuration / 1000;
    return NO_ERROR;
}

status_t MediaPlayer::seekTo(int msec)
{
    Mutex::AutoLock _l(&mLock);
    return INVALID_OPERATION;
}

status_t MediaPlayer::reset()
{
    Mutex::AutoLock _l(&mLock);
    return INVALID_OPERATION;
}

status_t MediaPlayer::setAudioStreamType(int type)
{
    return NO_ERROR;
}

void MediaPlayer::ffmpegNotify(void* ptr, int level, const char* fmt, va_list vl) {
	
	switch(level) {
			/**
			 * Something went really wrong and we will crash now.
			 */
		case AV_LOG_PANIC:
			LOGE("AV_LOG_PANIC: %s", fmt);
			//sPlayer->mCurrentState = MEDIA_PLAYER_STATE_ERROR;
			break;
			
			/**
			 * Something went wrong and recovery is not possible.
			 * For example, no header was found for a format which depends
			 * on headers or an illegal combination of parameters is used.
			 */
		case AV_LOG_FATAL:
			LOGE("AV_LOG_FATAL: %s", fmt);
			//sPlayer->mCurrentState = MEDIA_PLAYER_STATE_ERROR;
			break;
			
			/**
			 * Something went wrong and cannot losslessly be recovered.
			 * However, not all future data is affected.
			 */
		case AV_LOG_ERROR:
			LOGE("AV_LOG_ERROR: %s", fmt);
			//sPlayer->mCurrentState = MEDIA_PLAYER_STATE_ERROR;
			break;
			
			/**
			 * Something somehow does not look correct. This may or may not
			 * lead to problems. An example would be the use of '-vstrict -2'.
			 */
		case AV_LOG_WARNING:
			LOGE("AV_LOG_WARNING: %s", fmt);
			break;
			
		case AV_LOG_INFO:
			LOGI("%s", fmt);
			break;
			
		case AV_LOG_DEBUG:
			LOGI("%s", fmt);
			break;
			
	}
}

void MediaPlayer::notify(int msg, int ext1, int ext2)
{
    //LOGI("message received msg=%d, ext1=%d, ext2=%d", msg, ext1, ext2);
    bool send = true;
    bool locked = false;

    if ((mListener != 0) && send) {
       //LOGI("callback application");
       mListener->notify(msg, ext1, ext2);
       //LOGI("back from callback");
	}
}
