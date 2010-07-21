/* mediaplayer.cpp
**
*/

//#define LOG_NDEBUG 0
#define TAG "FFMpegMediaPlayer"

#include <sys/types.h>
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

// map system drivers methods
#include "drivers_map.h"

#include "mediaplayer.h"

MediaPlayer::MediaPlayer()
{
    //__android_log_print(ANDROID_LOG_INFO, TAG, "constructor");
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
}

MediaPlayer::~MediaPlayer()
{
    //__android_log_print(ANDROID_LOG_INFO, TAG, "destructor");
    //disconnect();
    //IPCThreadState::self()->flushCommands();
}

status_t MediaPlayer::prepareAudio()
{
	mFFmpegStorage.audio.stream = -1;
	for (int i = 0; i < mFFmpegStorage.pFormatCtx->nb_streams; i++) {
		if (mFFmpegStorage.pFormatCtx->streams[i]->codec->codec_type == CODEC_TYPE_AUDIO) {
			mFFmpegStorage.audio.stream = i;
		}
		if(mFFmpegStorage.audio.stream != -1) {
			break;
		}
	}
	
	if (mFFmpegStorage.audio.stream == -1) {
		return INVALID_OPERATION;
	}
	
	// Get a pointer to the codec context for the video stream
	mFFmpegStorage.audio.codec_ctx = mFFmpegStorage.pFormatCtx->streams[mFFmpegStorage.audio.stream]->codec;
	mFFmpegStorage.audio.codec = avcodec_find_decoder(mFFmpegStorage.audio.codec_ctx->codec_id);
	if (mFFmpegStorage.audio.codec == NULL) {
		return INVALID_OPERATION;
	}
	
	// Open codec
	if (avcodec_open(mFFmpegStorage.audio.codec_ctx, 
					 mFFmpegStorage.audio.codec) < 0) {
		return INVALID_OPERATION;	}
	return NO_ERROR;
}

status_t MediaPlayer::prepareVideo()
{
	// Find the first video stream
	mFFmpegStorage.video.stream = -1;
	for (int i = 0; i < mFFmpegStorage.pFormatCtx->nb_streams; i++) {
		if (mFFmpegStorage.pFormatCtx->streams[i]->codec->codec_type == CODEC_TYPE_VIDEO) {
			mFFmpegStorage.video.stream = i;
		}
		if(mFFmpegStorage.video.stream != -1) {
			break;
		}
	}
	
	if (mFFmpegStorage.video.stream == -1) {
		return INVALID_OPERATION;
	}
	
	// Get a pointer to the codec context for the video stream
	mFFmpegStorage.video.codec_ctx = mFFmpegStorage.pFormatCtx->streams[mFFmpegStorage.video.stream]->codec;
	mFFmpegStorage.video.codec = avcodec_find_decoder(mFFmpegStorage.video.codec_ctx->codec_id);
	if (mFFmpegStorage.video.codec == NULL) {
		return INVALID_OPERATION;
	}
	
	// Open codec
	if (avcodec_open(mFFmpegStorage.video.codec_ctx, mFFmpegStorage.video.codec) < 0) {
		return INVALID_OPERATION;
	}
	// Allocate video frame
	mFFmpegStorage.pFrame = avcodec_alloc_frame();
	
	int w = mFFmpegStorage.video.codec_ctx->width;
	int h = mFFmpegStorage.video.codec_ctx->height;
	mFFmpegStorage.img_convert_ctx = sws_getContext(w, 
												   h, 
												   mFFmpegStorage.video.codec_ctx->pix_fmt, 
												   w, 
												   h,
												   PIX_FMT_RGB565, 
												   SWS_POINT, 
												   NULL, 
												   NULL, 
												   NULL);
	
	return NO_ERROR;
}

status_t MediaPlayer::prepare()
{
	status_t ret;
	mCurrentState = MEDIA_PLAYER_PREPARING;
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
    __android_log_print(ANDROID_LOG_INFO, TAG, "setListener");
    mListener = listener;
    return NO_ERROR;
}

status_t MediaPlayer::setDataSource(const char *url)
{
    __android_log_print(ANDROID_LOG_INFO, TAG, "setDataSource(%s)", url);
    status_t err = BAD_VALUE;
	// Open video file
	if(av_open_input_file(&mFFmpegStorage.pFormatCtx, url, NULL, 0, NULL) != 0) {
		return INVALID_OPERATION;
	}
	// Retrieve stream information
	if(av_find_stream_info(mFFmpegStorage.pFormatCtx) < 0) {
		return INVALID_OPERATION;
	}
    return NO_ERROR;
}

status_t MediaPlayer::suspend() {
    return NO_ERROR;
}

status_t MediaPlayer::resume() {
    return NO_ERROR;
}

status_t MediaPlayer::setVideoSurface(JNIEnv* env, jobject jsurface)
{
	if(VideoDriver_register(env, jsurface) != ANDROID_SURFACE_RESULT_SUCCESS) {
		return INVALID_OPERATION;
	}
    return NO_ERROR;
}

status_t MediaPlayer::start()
{
	return INVALID_OPERATION;
}

status_t MediaPlayer::stop()
{
    return INVALID_OPERATION;
}

status_t MediaPlayer::pause()
{
	return INVALID_OPERATION;
}

bool MediaPlayer::isPlaying()
{
    return false;
}

status_t MediaPlayer::getVideoWidth(int *w)
{
    return NO_ERROR;
}

status_t MediaPlayer::getVideoHeight(int *h)
{
    return NO_ERROR;
}

status_t MediaPlayer::getCurrentPosition(int *msec)
{
	return INVALID_OPERATION;
}

status_t MediaPlayer::getDuration(int *msec)
{
    return INVALID_OPERATION;
}

status_t MediaPlayer::seekTo(int msec)
{
    return INVALID_OPERATION;
}

status_t MediaPlayer::reset()
{
    return INVALID_OPERATION;
}

status_t MediaPlayer::setAudioStreamType(int type)
{
	return NO_ERROR;
}

void MediaPlayer::notify(int msg, int ext1, int ext2)
{
    __android_log_print(ANDROID_LOG_INFO, TAG, "message received msg=%d, ext1=%d, ext2=%d", msg, ext1, ext2);
	/*
    bool send = true;
    bool locked = false;

    // TODO: In the future, we might be on the same thread if the app is
    // running in the same process as the media server. In that case,
    // this will deadlock.
    //
    // The threadId hack below works around this for the care of prepare
    // and seekTo within the same process.
    // FIXME: Remember, this is a hack, it's not even a hack that is applied
    // consistently for all use-cases, this needs to be revisited.
     if (mLockThreadId != getThreadId()) {
        mLock.lock();
        locked = true;
    }

    if (mPlayer == 0) {
        __android_log_print(ANDROID_LOG_INFO, TAG, "notify(%d, %d, %d) callback on disconnected mediaplayer", msg, ext1, ext2);
        if (locked) mLock.unlock();   // release the lock when done.
        return;
    }

    switch (msg) {
    case MEDIA_NOP: // interface test message
        break;
    case MEDIA_PREPARED:
        __android_log_print(ANDROID_LOG_INFO, TAG, "prepared");
        mCurrentState = MEDIA_PLAYER_PREPARED;
        if (mPrepareSync) {
            __android_log_print(ANDROID_LOG_INFO, TAG, "signal application thread");
            mPrepareSync = false;
            mPrepareStatus = NO_ERROR;
            mSignal.signal();
        }
        break;
    case MEDIA_PLAYBACK_COMPLETE:
        __android_log_print(ANDROID_LOG_INFO, TAG, "playback complete");
        if (!mLoop) {
            mCurrentState = MEDIA_PLAYER_PLAYBACK_COMPLETE;
        }
        break;
    case MEDIA_ERROR:
        // Always log errors.
        // ext1: Media framework error code.
        // ext2: Implementation dependant error code.
        LOGE("error (%d, %d)", ext1, ext2);
        mCurrentState = MEDIA_PLAYER_STATE_ERROR;
        if (mPrepareSync)
        {
            __android_log_print(ANDROID_LOG_INFO, TAG, "signal application thread");
            mPrepareSync = false;
            mPrepareStatus = ext1;
            mSignal.signal();
            send = false;
        }
        break;
    case MEDIA_INFO:
        // ext1: Media framework error code.
        // ext2: Implementation dependant error code.
        LOGW("info/warning (%d, %d)", ext1, ext2);
        break;
    case MEDIA_SEEK_COMPLETE:
        __android_log_print(ANDROID_LOG_INFO, TAG, "Received seek complete");
        if (mSeekPosition != mCurrentPosition) {
            __android_log_print(ANDROID_LOG_INFO, TAG, "Executing queued seekTo(%d)", mSeekPosition);
            mSeekPosition = -1;
            seekTo_l(mCurrentPosition);
        }
        else {
            __android_log_print(ANDROID_LOG_INFO, TAG, "All seeks complete - return to regularly scheduled program");
            mCurrentPosition = mSeekPosition = -1;
        }
        break;
    case MEDIA_BUFFERING_UPDATE:
        __android_log_print(ANDROID_LOG_INFO, TAG, "buffering %d", ext1);
        break;
    case MEDIA_SET_VIDEO_SIZE:
        __android_log_print(ANDROID_LOG_INFO, TAG, "New video size %d x %d", ext1, ext2);
        mVideoWidth = ext1;
        mVideoHeight = ext2;
        break;
    default:
        __android_log_print(ANDROID_LOG_INFO, TAG, "unrecognized message: (%d, %d, %d)", msg, ext1, ext2);
        break;
    }

    sp<MediaPlayerListener> listener = mListener;
    if (locked) mLock.unlock();

    // this prevents re-entrant calls into client code
    if ((listener != 0) && send) {
        Mutex::Autolock _l(mNotifyLock);
        __android_log_print(ANDROID_LOG_INFO, TAG, "callback application");
        listener->notify(msg, ext1, ext2);
        __android_log_print(ANDROID_LOG_INFO, TAG, "back from callback");
    }
	*/
}