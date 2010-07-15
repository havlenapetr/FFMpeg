#include <unistd.h>
#include <android/log.h>
#include <android/bitmap.h>
#include "jniUtils.h"
#include "methods.h"

extern "C" {

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/log.h"

} // end of extern C

#define TAG "FFMpegPlayerAndroid"

struct ffmpeg_fields_t {
	int 				videoStream;
	int					audioStream;
	AVCodecContext 		*video_codec_ctx;
	AVCodecContext 		*audio_codec_ctx;
	AVFrame 			*pFrame;
	AVCodec 			*video_codec;
	AVCodec 			*audio_codec;
	AVFormatContext 	*pFormatCtx;;
	struct SwsContext 	*img_convert_ctx;
};
static struct ffmpeg_fields_t ffmpeg_fields;

struct fields_t {
	jfieldID    surface;
	jfieldID    avformatcontext;
	jmethodID   clb_onVideoFrame;
	jmethodID   clb_onAudioBuffer;
};
static struct fields_t fields;

enum State {
	STATE_STOPED,
	STATE_STOPING,
	STATE_PLAYING,
	STATE_PAUSE
};
static State status = STATE_STOPED;

jclass FFMpegPlayerAndroid_getClass(JNIEnv *env) {
	return env->FindClass("com/media/ffmpeg/android/FFMpegPlayerAndroid");
}

const char *FFMpegPlayerAndroid_getSignature() {
	return "Lcom/media/ffmpeg/android/FFMpegPlayerAndroid;";
}

static void FFMpegPlayerAndroid_handleErrors(void* ptr, int level, const char* fmt, va_list vl) {

	switch(level) {
	/**
	 * Something went really wrong and we will crash now.
	 */
	case AV_LOG_PANIC:
		__android_log_print(ANDROID_LOG_ERROR, TAG, "AV_LOG_PANIC: %s", fmt);
		break;

	/**
	* Something went wrong and recovery is not possible.
	* For example, no header was found for a format which depends
	* on headers or an illegal combination of parameters is used.
    */
	case AV_LOG_FATAL:
		__android_log_print(ANDROID_LOG_ERROR, TAG, "AV_LOG_FATAL: %s", fmt);
		break;

	/**
	 * Something went wrong and cannot losslessly be recovered.
	 * However, not all future data is affected.
	 */
	case AV_LOG_ERROR:
		__android_log_print(ANDROID_LOG_ERROR, TAG, "AV_LOG_ERROR: %s", fmt);
		break;

	/**
	 * Something somehow does not look correct. This may or may not
	 * lead to problems. An example would be the use of '-vstrict -2'.
	 */
	case AV_LOG_WARNING:
		__android_log_print(ANDROID_LOG_ERROR, TAG, "AV_LOG_WARNING: %s", fmt);
		break;

	case AV_LOG_INFO:
		__android_log_print(ANDROID_LOG_INFO, TAG, "%s", fmt);
		break;

	case AV_LOG_DEBUG:
		__android_log_print(ANDROID_LOG_DEBUG, TAG, "%s", fmt);
		break;

	}
}

static int FFMpegPlayerAndroid_initAudio(JNIEnv *env) {
	if (ffmpeg_fields.audioStream == -1) {
		jniThrowException(env,
						  "java/io/IOException",
						  "Didn't find a audio stream");
		return -1;
	}
	// Get a pointer to the codec context for the video stream
	ffmpeg_fields.audio_codec_ctx = ffmpeg_fields.pFormatCtx->streams[ffmpeg_fields.audioStream]->codec;
	ffmpeg_fields.audio_codec = avcodec_find_decoder(ffmpeg_fields.audio_codec_ctx->codec_id);
	if (ffmpeg_fields.audio_codec == NULL) {
		jniThrowException(env,
						  "java/io/IOException",
						  "Couldn't find audio codec!");
		return -1; // Codec not found
	}

	// Open codec
	if (avcodec_open(ffmpeg_fields.audio_codec_ctx, ffmpeg_fields.audio_codec) < 0) {
		jniThrowException(env,
						  "java/io/IOException",
						  "Could not open audio codec");
		return -1; // Could not open codec
	}
	return 0;
}

static int FFMpegPlayerAndroid_initVideo(JNIEnv *env) {
	if (ffmpeg_fields.videoStream == -1) {
		jniThrowException(env,
						  "java/io/IOException",
						  "Didn't find a video stream");
		return -1;
	}
	// Get a pointer to the codec context for the video stream
	ffmpeg_fields.video_codec_ctx = ffmpeg_fields.pFormatCtx->streams[ffmpeg_fields.videoStream]->codec;
	ffmpeg_fields.video_codec = avcodec_find_decoder(ffmpeg_fields.video_codec_ctx->codec_id);
	if (ffmpeg_fields.video_codec == NULL) {
		jniThrowException(env,
						  "java/io/IOException",
						  "Couldn't find video codec!");
		return -1; // Codec not found
	}

	// Open codec
	if (avcodec_open(ffmpeg_fields.video_codec_ctx, ffmpeg_fields.video_codec) < 0) {
		jniThrowException(env,
						  "java/io/IOException",
						  "Could not open video codec");
		return -1; // Could not open codec
	}
	return 0;
}

static jobject FFMpegPlayerAndroid_init(JNIEnv *env, jobject obj, jobject pAVFormatContext) {
	av_log_set_callback(FFMpegPlayerAndroid_handleErrors);

	ffmpeg_fields.pFormatCtx = (AVFormatContext *) env->GetIntField(pAVFormatContext, fields.avformatcontext);

	// Find the first video stream
	ffmpeg_fields.videoStream = -1;
	ffmpeg_fields.audioStream = -1;
	for (int i = 0; i < ffmpeg_fields.pFormatCtx->nb_streams; i++) {
		if (ffmpeg_fields.pFormatCtx->streams[i]->codec->codec_type == CODEC_TYPE_VIDEO) {
			ffmpeg_fields.videoStream = i;
		}
		if (ffmpeg_fields.pFormatCtx->streams[i]->codec->codec_type == CODEC_TYPE_AUDIO) {
			ffmpeg_fields.audioStream = i;
		}
		if(ffmpeg_fields.audioStream != -1 && ffmpeg_fields.videoStream != -1) {
			break;
		}
	}

	//__android_log_print(ANDROID_LOG_INFO, TAG, "audio: %i, video: %i", ffmpeg_fields.videoStream, ffmpeg_fields.audioStream);

	if(FFMpegPlayerAndroid_initAudio(env) != 0) {
		return NULL;
	}

	if(FFMpegPlayerAndroid_initVideo(env) != 0) {
		return NULL;
	}

	// Allocate video frame
	ffmpeg_fields.pFrame = avcodec_alloc_frame();

	int w = ffmpeg_fields.video_codec_ctx->width;
	int h = ffmpeg_fields.video_codec_ctx->height;
	ffmpeg_fields.img_convert_ctx = sws_getContext(w, h, ffmpeg_fields.video_codec_ctx->pix_fmt, w, h,
			PIX_FMT_RGB565, SWS_POINT, NULL, NULL, NULL);
	
	return AVCodecContext_create(env, ffmpeg_fields.video_codec_ctx);
}
	
static AVFrame *FFMpegPlayerAndroid_createFrame(JNIEnv *env, jobject bitmap) {
	void*					pixels;
	AVFrame*				pFrame;
	AndroidBitmapInfo		info;
	int						result = -1;
	
	if ((result = AndroidBitmap_getInfo(env, bitmap, &info)) < 0) {
        return NULL;
    }
	
	if ((result = AndroidBitmap_lockPixels(env, bitmap, &pixels)) < 0) {
		return NULL;
    }
	
	AndroidBitmap_unlockPixels(env, bitmap);
	
	// Allocate an AVFrame structure
	pFrame = avcodec_alloc_frame();
	if (pFrame == NULL) {
		return NULL;
	}
	
	// Assign appropriate parts of buffer to image planes in pFrameRGB
	// Note that pFrameRGB is an AVFrame, but AVFrame is a superset
	// of AVPicture
	avpicture_fill((AVPicture *) pFrame, (uint8_t *)pixels, PIX_FMT_RGB565,
				   info.width, info.height);
	return pFrame;
}

// Allocate a structure for storing decoded samples
static void FFMpegPlayerAndroid_processAudio(JNIEnv *env, jobject obj, AVPacket *packet, int16_t *samples, int samples_length) {
	// Try to decode the audio from the packet into the frame
	int out_size = samples_length;
	int len = avcodec_decode_audio3(ffmpeg_fields.audio_codec_ctx, samples,
								&out_size, packet);

	//__android_log_print(ANDROID_LOG_INFO, TAG, "size: %i, len: %i, out_size: %i", packet->size, len, out_size);

	/*
	 * call java callback for writing audio buffer to android audio track
	 */
	if(len > 0) {
		jbyteArray arr = env->NewByteArray(out_size);
		env->SetByteArrayRegion(arr, 0, out_size, (jbyte *) samples);
		env->CallVoidMethod(obj, fields.clb_onAudioBuffer, arr);
		env->DeleteLocalRef(arr);
	}
}

static void FFMpegPlayerAndroid_play(JNIEnv *env, jobject obj, jobject bitmap) {
	AVPacket				packet;
	int						result = -1;
	int						frameFinished;
	int 					audio_sample_size = AVCODEC_MAX_AUDIO_FRAME_SIZE;

	// Allocate an AVFrame structure
	AVFrame *pFrameRGB = FFMpegPlayerAndroid_createFrame(env, bitmap);
	if (pFrameRGB == NULL) {
		jniThrowException(env,
						  "java/io/IOException",
						  "Couldn't allocate an AVFrame structure");
		return;
	}
	
	int16_t *samples = (int16_t *) av_malloc(audio_sample_size);
	
	status = STATE_PLAYING;
	while (status != STATE_STOPING) {

		if(status == STATE_PAUSE) {
			usleep(50);
			continue;
		}

		if((result = av_read_frame(ffmpeg_fields.pFormatCtx, &packet)) < 0) {
			status = STATE_STOPING;
			continue;
		}

		// Is this a packet from the video stream?
		if (packet.stream_index == ffmpeg_fields.videoStream) {
			// Decode video frame
			avcodec_decode_video(ffmpeg_fields.video_codec_ctx, ffmpeg_fields.pFrame, &frameFinished,
					packet.data, packet.size);
			
			// Did we get a video frame?
			if (frameFinished) {
				// Convert the image from its native format to RGB
				sws_scale(ffmpeg_fields.img_convert_ctx, ffmpeg_fields.pFrame->data, ffmpeg_fields.pFrame->linesize, 0,
						ffmpeg_fields.video_codec_ctx->height, pFrameRGB->data, pFrameRGB->linesize);
				env->CallVoidMethod(obj, fields.clb_onVideoFrame);
			}
		} else if (packet.stream_index == ffmpeg_fields.audioStream) {
			int sample_size = FFMAX(packet.size * sizeof(*samples), audio_sample_size);
			//__android_log_print(ANDROID_LOG_INFO, TAG, "orig. %i should be %i", audio_sample_size, sample_size);
			if(audio_sample_size < sample_size) {
				__android_log_print(ANDROID_LOG_INFO, TAG, "resizing audio buffer from %i to %i", audio_sample_size, sample_size);
				av_free(samples);
				audio_sample_size = sample_size;
				samples = (int16_t *) av_malloc(sample_size);
			}
			FFMpegPlayerAndroid_processAudio(env, obj, &packet, samples, audio_sample_size);
		}

		// Free the packet that was allocated by av_read_frame
		av_free_packet(&packet);
	}

	av_free( samples );
	
	// Free the RGB image
	av_free(pFrameRGB);

	status = STATE_STOPED;
	__android_log_print(ANDROID_LOG_INFO, TAG, "end of playing");
}

static jboolean FFMpegPlayerAndroid_pause(JNIEnv *env, jobject object, jboolean pause) {
	switch(pause) {
	case JNI_TRUE:
		if(status != STATE_PLAYING) {
			return JNI_FALSE;
		}
		status = STATE_PAUSE;
		break;

	case JNI_FALSE:
		if(status != STATE_PAUSE) {
			return JNI_FALSE;
		}
		status = STATE_PLAYING;
		break;
	}
	return JNI_TRUE;
}

static void FFMpegPlayerAndroid_stop(JNIEnv *env, jobject object) {
	if(status == STATE_STOPING || status == STATE_STOPED) {
		return;
	}
	status = STATE_STOPING;
}

static jobject FFMpegPlayerAndroid_setInputFile(JNIEnv *env, jobject obj, jstring filePath) {
	AVFormatContext *pFormatCtx;
	const char *_filePath = env->GetStringUTFChars(filePath, NULL);
	// Open video file
	if(av_open_input_file(&pFormatCtx, _filePath, NULL, 0, NULL) != 0) {
		jniThrowException(env,
						  "java/io/IOException",
					      "Can't create input file");
	}
	// Retrieve stream information
	if(av_find_stream_info(pFormatCtx)<0) {
		jniThrowException(env,
						  "java/io/IOException",
						  "Couldn't find stream information");
	}
	return AVFormatContext_create(env, pFormatCtx);
}

static void FFMpegPlayerAndroid_setSurface(JNIEnv *env, jobject obj, jobject surface) {
	__android_log_print(ANDROID_LOG_INFO, TAG, "setting surface");

	int surface_ptr = env->GetIntField(surface, fields.surface);
	/*if(sSurface == NULL) {
		__android_log_print(ANDROID_LOG_ERROR, TAG, "Native surface is NULL");
	    return;
	}*/
}

static void FFMpegPlayerAndroid_release(JNIEnv *env, jobject obj) {
	// Free the YUV frame
	av_free(ffmpeg_fields.pFrame);
	
	// Close the codec
	avcodec_close(ffmpeg_fields.video_codec_ctx);
	avcodec_close(ffmpeg_fields.audio_codec_ctx);
	
	// Close the video file
	av_close_input_file(ffmpeg_fields.pFormatCtx);
}

/*
* JNI registration.
*/
static JNINativeMethod methods[] = {
	{ "nativeInit", "(Lcom/media/ffmpeg/FFMpegAVFormatContext;)Lcom/media/ffmpeg/FFMpegAVCodecContext;", (void*) FFMpegPlayerAndroid_init},
	{ "nativeSetInputFile", "(Ljava/lang/String;)Lcom/media/ffmpeg/FFMpegAVFormatContext;", (void*) FFMpegPlayerAndroid_setInputFile },
	{ "nativePause", "(Z)Z", (void*) FFMpegPlayerAndroid_pause},
	{ "nativePlay", "(Landroid/graphics/Bitmap;)V", (void*) FFMpegPlayerAndroid_play },
	{ "nativeStop", "()V", (void*) FFMpegPlayerAndroid_stop },
	{ "nativeSetSurface", "(Landroid/view/Surface;)V", (void*) FFMpegPlayerAndroid_setSurface },
	{ "nativeRelease", "()V", (void*) FFMpegPlayerAndroid_release },
};
	
int register_android_media_FFMpegPlayerAndroid(JNIEnv *env) {
	jclass clazz = env->FindClass("android/view/Surface");
	if(clazz == NULL) {
		__android_log_print(ANDROID_LOG_ERROR, TAG, "can't load native surface");
	    return JNI_ERR;
	}
	fields.surface = env->GetFieldID(clazz, "mSurface", "I");
	if(fields.surface == NULL) {
		__android_log_print(ANDROID_LOG_ERROR, TAG, "can't load native mSurface");
	    return JNI_ERR;
	}
	fields.clb_onVideoFrame = env->GetMethodID(FFMpegPlayerAndroid_getClass(env), "onVideoFrame", "()V");
	if (fields.clb_onVideoFrame == NULL) {
		return JNI_ERR;
	}
	fields.clb_onAudioBuffer = env->GetMethodID(FFMpegPlayerAndroid_getClass(env), "onAudioBuffer", "([B)V");
	if (fields.clb_onAudioBuffer == NULL) {
		return JNI_ERR;
	}
	fields.avformatcontext = env->GetFieldID(AVFormatContext_getClass(env), "pointer", "I");
	if (fields.avformatcontext == NULL) {
		return JNI_ERR;
	}
	return jniRegisterNativeMethods(env, "com/media/ffmpeg/android/FFMpegPlayerAndroid", methods, sizeof(methods) / sizeof(methods[0]));
}
