#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <linux/ioctl.h>

#include <android/log.h>
#include <android/audiotrack.h>
#include <android/surface.h>
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
	AVFrame 			*pFrame;
	AVFormatContext 	*pFormatCtx;
	struct SwsContext 	*img_convert_ctx;
} ffmpeg_fields;

struct ffmpeg_video_t {
	bool				initzialized;
	int 				stream;
	AVCodecContext 		*codec_ctx;
	AVCodec 			*codec;
} ffmpeg_video;

struct ffmpeg_audio_t {
	bool				initzialized;
	bool				decode;
	int 				stream;
	AVCodecContext 		*codec_ctx;
	AVCodec 			*codec;
} ffmpeg_audio;

enum State {
	STATE_STOPED,
	STATE_STOPING,
	STATE_PLAYING,
	STATE_PAUSE
};
static State status = STATE_STOPED;

// create pointer on system methods for accessing audio and video system
#ifdef ANDROID

int (*AudioDriver_register) (void) = AndroidAudioTrack_register;
int (*AudioDriver_set) (int,uint32_t,int,int,int) = AndroidAudioTrack_set;
int (*AudioDriver_start) (void) = AndroidAudioTrack_start;
int (*AudioDriver_flush) (void) = AndroidAudioTrack_flush;
int (*AudioDriver_stop) (void) = AndroidAudioTrack_stop;
int (*AudioDriver_reload) (void) = AndroidAudioTrack_reload;
int (*AudioDriver_unregister) (void) = AndroidAudioTrack_unregister;
int (*AudioDriver_write) (void *, int) = AndroidAudioTrack_write;

int (*VideoDriver_register) (JNIEnv*, jobject) = AndroidSurface_register;
int (*VideoDriver_getPixels) (int width, int height, void** pixels) = AndroidSurface_getPixels;
int (*VideoDriver_updateSurface) (void) = AndroidSurface_updateSurface;
int (*VideoDriver_unregister) (void) = AndroidSurface_unregister;

#endif

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

static void FFMpegPlayerAndroid_enableErrorCallback(JNIEnv *env, jobject obj) {
	av_log_set_callback(FFMpegPlayerAndroid_handleErrors);
}

static jobject FFMpegPlayerAndroid_initAudio(JNIEnv *env, jobject obj, jobject pAVFormatContext) {
	ffmpeg_audio.stream = -1;
	for (int i = 0; i < ffmpeg_fields.pFormatCtx->nb_streams; i++) {
		if (ffmpeg_fields.pFormatCtx->streams[i]->codec->codec_type == CODEC_TYPE_AUDIO) {
			ffmpeg_audio.stream = i;
		}
		if(ffmpeg_audio.stream != -1) {
			break;
		}
	}

	if (ffmpeg_audio.stream == -1) {
		jniThrowException(env,
						  "java/io/IOException",
						  "Didn't find a audio stream");
		return NULL;
	}
	// Get a pointer to the codec context for the video stream
	ffmpeg_audio.codec_ctx = ffmpeg_fields.pFormatCtx->streams[ffmpeg_audio.stream]->codec;
	ffmpeg_audio.codec = avcodec_find_decoder(ffmpeg_audio.codec_ctx->codec_id);
	if (ffmpeg_audio.codec == NULL) {
		jniThrowException(env,
						  "java/io/IOException",
						  "Couldn't find audio codec!");
		return NULL; // Codec not found
	}

	// Open codec
	if (avcodec_open(ffmpeg_audio.codec_ctx, ffmpeg_audio.codec) < 0) {
		jniThrowException(env,
						  "java/io/IOException",
						  "Could not open audio codec");
		return NULL; // Could not open codec
	}
	ffmpeg_audio.initzialized = ffmpeg_audio.decode = true;
	return AVCodecContext_create(env, ffmpeg_audio.codec_ctx);
}

static jobject FFMpegPlayerAndroid_initVideo(JNIEnv *env, jobject obj, jobject pAVFormatContext) {
	// Find the first video stream
	ffmpeg_video.stream = -1;
	for (int i = 0; i < ffmpeg_fields.pFormatCtx->nb_streams; i++) {
		if (ffmpeg_fields.pFormatCtx->streams[i]->codec->codec_type == CODEC_TYPE_VIDEO) {
			ffmpeg_video.stream = i;
		}
		if(ffmpeg_video.stream != -1) {
			break;
		}
	}

	if (ffmpeg_video.stream == -1) {
		jniThrowException(env,
						  "java/io/IOException",
						  "Didn't find a video stream");
		return NULL;
	}

	// Get a pointer to the codec context for the video stream
	ffmpeg_video.codec_ctx = ffmpeg_fields.pFormatCtx->streams[ffmpeg_video.stream]->codec;
	ffmpeg_video.codec = avcodec_find_decoder(ffmpeg_video.codec_ctx->codec_id);
	if (ffmpeg_video.codec == NULL) {
		jniThrowException(env,
						  "java/io/IOException",
						  "Couldn't find video codec!");
		return NULL; // Codec not found
	}

	// Open codec
	if (avcodec_open(ffmpeg_video.codec_ctx, ffmpeg_video.codec) < 0) {
		jniThrowException(env,
						  "java/io/IOException",
						  "Could not open video codec");
		return NULL; // Could not open codec
	}
	// Allocate video frame
	ffmpeg_fields.pFrame = avcodec_alloc_frame();

	int w = ffmpeg_video.codec_ctx->width;
	int h = ffmpeg_video.codec_ctx->height;
	ffmpeg_fields.img_convert_ctx = sws_getContext(w, h, ffmpeg_video.codec_ctx->pix_fmt, w, h,
				PIX_FMT_RGB565, SWS_POINT, NULL, NULL, NULL);

	ffmpeg_video.initzialized = true;
	return AVCodecContext_create(env, ffmpeg_video.codec_ctx);
}

static AVFrame *FFMpegPlayerAndroid_createFrame(JNIEnv* env) {
	void*					pixels;
	AVFrame*				pFrame;
	
	pFrame = avcodec_alloc_frame();
	if (pFrame == NULL) {
		jniThrowException(env,
						  "java/io/IOException",
						  "Couldn't allocate an AVFrame structure");
		return NULL;
	}
	
	if(VideoDriver_getPixels(ffmpeg_video.codec_ctx->width, 
								ffmpeg_video.codec_ctx->height, 
								&pixels) != ANDROID_SURFACE_RESULT_SUCCESS) {
		__android_log_print(ANDROID_LOG_ERROR, TAG, "couldn't get surface's pixels");
		return NULL;
	}
	
	// Assign appropriate parts of buffer to image planes in pFrameRGB
	// Note that pFrameRGB is an AVFrame, but AVFrame is a superset
	// of AVPicture
	avpicture_fill((AVPicture *) pFrame, (uint8_t *)pixels, PIX_FMT_RGB565,
				   ffmpeg_video.codec_ctx->width, ffmpeg_video.codec_ctx->height);
	
	return pFrame;
}

static bool called = false;
static int FFMpegPlayerAndroid_processAudio(JNIEnv *env, AVPacket *packet, int16_t *samples, int samples_size) {
	/*
	int size = FFMAX(packet->size * sizeof(*samples), samples_size);
	if(samples_size < size) {
		__android_log_print(ANDROID_LOG_INFO, TAG, "resizing audio buffer from %i to %i", samples_size, size);
		av_free(samples);
		samples_size = size;
		samples = (int16_t *) av_malloc(samples_size);
	}
	*/
	int len = avcodec_decode_audio3(ffmpeg_audio.codec_ctx, samples, &samples_size, packet);
	
	if(!called) {
		__android_log_print(ANDROID_LOG_INFO, TAG, "starting android audio track");
		if(AudioDriver_set(MUSIC, 
								 ffmpeg_audio.codec_ctx->sample_rate,
								 PCM_16_BIT,
								 (ffmpeg_audio.codec_ctx->channels == 2) ? CHANNEL_OUT_STEREO : CHANNEL_OUT_MONO,
								 samples_size) != ANDROID_AUDIOTRACK_RESULT_SUCCESS) {
			jniThrowException(env,
							  "java/io/IOException",
							  "Couldn't set audio track parametres");
			return -1;
		}
		if(AudioDriver_start() != ANDROID_AUDIOTRACK_RESULT_SUCCESS) {
			jniThrowException(env,
							  "java/io/IOException",
							  "Couldn't start audio track");
			return -1;
		}
		called = true;
	}
	
	if(AudioDriver_write(samples, samples_size) <= 0) {
		jniThrowException(env,
						  "java/io/IOException",
						  "Couldn't write bytes to audio track");
		return -1;
	}
	AudioDriver_flush();
	return 0;
}

static int FFMpegPlayerAndroid_processVideo(JNIEnv *env, jobject obj, AVPacket *packet, AVFrame *pFrameRGB) {
	int						frameFinished;

	// Decode video frame
	avcodec_decode_video(ffmpeg_video.codec_ctx, ffmpeg_fields.pFrame, &frameFinished,
					packet->data, packet->size);

	// Did we get a video frame?
	if (frameFinished) {
		// Convert the image from its native format to RGB
		sws_scale(ffmpeg_fields.img_convert_ctx, ffmpeg_fields.pFrame->data, ffmpeg_fields.pFrame->linesize, 0,
							ffmpeg_video.codec_ctx->height, pFrameRGB->data, pFrameRGB->linesize);
		VideoDriver_updateSurface();
		return 0;
	}
	return -1;
}

static void FFMpegPlayerAndroid_play(JNIEnv *env, jobject obj) {
	AVPacket				packet;
	int						result = -1;
	int 					samples_size = AVCODEC_MAX_AUDIO_FRAME_SIZE;
	int16_t*				samples;
	AVFrame*				pFrameRGB;
	
	// Allocate an AVFrame structure
	if(ffmpeg_video.initzialized) {
		pFrameRGB = FFMpegPlayerAndroid_createFrame(env);
		if (pFrameRGB == NULL) {
			jniThrowException(env,
							  "java/io/IOException",
							  "Couldn't crate frame buffer");
			return;
		}
	}
	
	if(ffmpeg_audio.initzialized) {
		samples = (int16_t *) av_malloc(samples_size);
		if(AudioDriver_register() != ANDROID_AUDIOTRACK_RESULT_SUCCESS) {
			jniThrowException(env,
							  "java/io/IOException",
							  "Couldn't register audio track");
			return;
		}
	}
	
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
		if (packet.stream_index == ffmpeg_video.stream &&
				ffmpeg_video.initzialized) {
			if(FFMpegPlayerAndroid_processVideo(env, obj, &packet, pFrameRGB) < 0) {
				__android_log_print(ANDROID_LOG_ERROR, TAG, "Frame wasn't finished by video decoder");
			}
		} else if (packet.stream_index == ffmpeg_audio.stream &&
				ffmpeg_audio.initzialized && ffmpeg_audio.decode) {
			if(FFMpegPlayerAndroid_processAudio(env, &packet, samples, samples_size) < 0 ) {
				return; // exception occured so return to java
			}
		}

		// Free the packet that was allocated by av_read_frame
		av_free_packet(&packet);
	}
	
	if(ffmpeg_video.initzialized) {
		if(VideoDriver_unregister() != ANDROID_SURFACE_RESULT_SUCCESS) {
			jniThrowException(env,
							  "java/io/IOException",
							  "Couldn't unregister vide surface");
		}
	}
	if(ffmpeg_audio.initzialized) {
		if(AudioDriver_unregister() != ANDROID_AUDIOTRACK_RESULT_SUCCESS) {
			jniThrowException(env,
							  "java/io/IOException",
							  "Couldn't unregister audio track");
		}
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

static void FFMpegPlayerAndroid_decodeAudio(JNIEnv *env, jobject object, jboolean decode) {
	ffmpeg_audio.decode = decode;
}

static jboolean FFMpegPlayerAndroid_isDecodingAudio(JNIEnv *env, jobject object) {
	return ffmpeg_audio.decode;
}

static void FFMpegPlayerAndroid_stop(JNIEnv *env, jobject object) {
	if(status == STATE_STOPING || status == STATE_STOPED) {
		return;
	}
	status = STATE_STOPING;
}

static jobject FFMpegPlayerAndroid_setInputFile(JNIEnv *env, jobject obj, jstring filePath) {
	const char *_filePath = env->GetStringUTFChars(filePath, NULL);
	// Open video file
	if(av_open_input_file(&ffmpeg_fields.pFormatCtx, _filePath, NULL, 0, NULL) != 0) {
		jniThrowException(env,
						  "java/io/IOException",
					      "Can't create input file");
	}
	// Retrieve stream information
	if(av_find_stream_info(ffmpeg_fields.pFormatCtx) < 0) {
		jniThrowException(env,
						  "java/io/IOException",
						  "Couldn't find stream information");
	}
	return AVFormatContext_create(env, ffmpeg_fields.pFormatCtx);
}

static void FFMpegPlayerAndroid_setSurface(JNIEnv *env, jobject obj, jobject jsurface) {
	__android_log_print(ANDROID_LOG_INFO, TAG, "setting surface");

	if(VideoDriver_register(env, jsurface) != ANDROID_SURFACE_RESULT_SUCCESS) {
		__android_log_print(ANDROID_LOG_ERROR, TAG, "couldn't set surface");
	}
}

static void FFMpegPlayerAndroid_release(JNIEnv *env, jobject obj) {
	// Free the YUV frame
	av_free(ffmpeg_fields.pFrame);
	
	// Close the codec
	avcodec_close(ffmpeg_video.codec_ctx);
	avcodec_close(ffmpeg_audio.codec_ctx);
	
	// Close the video file
	av_close_input_file(ffmpeg_fields.pFormatCtx);
}

/*
* JNI registration.
*/
static JNINativeMethod methods[] = {
	{ "nativeInitAudio", "(Lcom/media/ffmpeg/FFMpegAVFormatContext;)Lcom/media/ffmpeg/FFMpegAVCodecContext;", (void*) FFMpegPlayerAndroid_initAudio},
	{ "nativeInitVideo", "(Lcom/media/ffmpeg/FFMpegAVFormatContext;)Lcom/media/ffmpeg/FFMpegAVCodecContext;", (void*) FFMpegPlayerAndroid_initVideo},
	{ "nativeEnableErrorCallback", "()V", (void*) FFMpegPlayerAndroid_enableErrorCallback},
	{ "nativeSetInputFile", "(Ljava/lang/String;)Lcom/media/ffmpeg/FFMpegAVFormatContext;", (void*) FFMpegPlayerAndroid_setInputFile },
	{ "nativePause", "(Z)Z", (void*) FFMpegPlayerAndroid_pause},
	{ "nativeDecodeAudio", "(Z)V", (void*) FFMpegPlayerAndroid_decodeAudio},
	{ "isDecodingAudio", "()Z", (void*) FFMpegPlayerAndroid_isDecodingAudio},
	{ "nativePlay", "()V", (void*) FFMpegPlayerAndroid_play },
	{ "nativeStop", "()V", (void*) FFMpegPlayerAndroid_stop },
	{ "nativeSetSurface", "(Landroid/view/Surface;)V", (void*) FFMpegPlayerAndroid_setSurface },
	{ "nativeRelease", "()V", (void*) FFMpegPlayerAndroid_release },
};
	
int register_android_media_FFMpegPlayerAndroid(JNIEnv *env) {
	ffmpeg_audio.initzialized = false;
	ffmpeg_video.initzialized = false;

	return jniRegisterNativeMethods(env, "com/media/ffmpeg/android/FFMpegPlayerAndroid", methods, sizeof(methods) / sizeof(methods[0]));
}
