#include <android/log.h>
#include <android/bitmap.h>
#include "jniUtils.h"
#include "methods.h"

extern "C" {

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"

} // end of extern C

#define TAG "FFMpegPlayerAndroid"

struct ffmpeg_fields_t {
	int 				videoStream;
	int					audioStream;
	AVCodecContext 		*pCodecCtx;
	AVFrame 			*pFrame;
	AVCodec 			*pCodec;
	AVFormatContext 	*pFormatCtx;;
	struct SwsContext 	*img_convert_ctx;
};
static struct ffmpeg_fields_t ffmpeg_fields;

struct fields_t {
	jfieldID    surface;
	jfieldID    avformatcontext;
	jmethodID   clb_onVideoFrame;
};
static struct fields_t fields;

enum State {
	STATE_STOPED,
	STATE_STOPING,
	STATE_PLAYING,
};
static State status = STATE_STOPED;

jclass FFMpegPlayerAndroid_getClass(JNIEnv *env) {
	return env->FindClass("com/media/ffmpeg/android/FFMpegPlayerAndroid");
}

const char *FFMpegPlayerAndroid_getSignature() {
	return "Lcom/media/ffmpeg/android/FFMpegPlayerAndroid;";
}

static jintArray FFMpegPlayerAndroid_init(JNIEnv *env, jobject obj, jobject pAVFormatContext) {
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

	if (ffmpeg_fields.videoStream == -1) {
		jniThrowException(env,
						  "java/io/IOException",
						  "Didn't find a video stream");
		return NULL;
	}

	// Get a pointer to the codec context for the video stream
	ffmpeg_fields.pCodecCtx = ffmpeg_fields.pFormatCtx->streams[ffmpeg_fields.videoStream]->codec;

	// Find the decoder for the video stream
	ffmpeg_fields.pCodec = avcodec_find_decoder(ffmpeg_fields.pCodecCtx->codec_id);
	if (ffmpeg_fields.pCodec == NULL) {
		jniThrowException(env,
						  "java/io/IOException",
						  "Unsupported codec!");
		return NULL; // Codec not found
	}

	// Open codec
	if (avcodec_open(ffmpeg_fields.pCodecCtx, ffmpeg_fields.pCodec) < 0) {
		jniThrowException(env,
						  "java/io/IOException",
						  "Could not open codec");
		return NULL; // Could not open codec
	}

	// Allocate video frame
	ffmpeg_fields.pFrame = avcodec_alloc_frame();

	int size[2];
	size[0] = ffmpeg_fields.pCodecCtx->width;
	size[1] = ffmpeg_fields.pCodecCtx->height;
	ffmpeg_fields.img_convert_ctx = sws_getContext(size[0], size[1], ffmpeg_fields.pCodecCtx->pix_fmt, size[0], size[1],
			PIX_FMT_RGB565, SWS_POINT, NULL, NULL, NULL);
	
	jintArray arr = env->NewIntArray(2);
	env->SetIntArrayRegion(arr, 0, 2, (jint *) size);
	return arr;
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
static void FFMpegPlayerAndroid_processAudio(AVPacket *packet, int16_t *samples) {
	// Try to decode the audio from the packet into the frame
	int out_size, len;
	len = avcodec_decode_audio3(ffmpeg_fields.pCodecCtx, samples, 
								&out_size, packet);
}

static void FFMpegPlayerAndroid_play(JNIEnv *env, jobject obj, jobject bitmap) {
	AVPacket				packet;
	int						result = -1;
	int						frameFinished;

	// Allocate an AVFrame structure
	AVFrame *pFrameRGB = FFMpegPlayerAndroid_createFrame(env, bitmap);
	if (pFrameRGB == NULL) {
		jniThrowException(env,
						  "java/io/IOException",
						  "Couldn't allocate an AVFrame structure");
		return;
	}
	
	int16_t *samples = (int16_t *) av_malloc(AVCODEC_MAX_AUDIO_FRAME_SIZE);
	memset(samples, 0, AVCODEC_MAX_AUDIO_FRAME_SIZE);
	
	status = STATE_PLAYING;
	while ((result = av_read_frame(ffmpeg_fields.pFormatCtx, &packet)) >= 0 &&
			status == STATE_PLAYING) {
		// Is this a packet from the video stream?
		if (packet.stream_index == ffmpeg_fields.videoStream) {
			// Decode video frame
			avcodec_decode_video(ffmpeg_fields.pCodecCtx, ffmpeg_fields.pFrame, &frameFinished,
					packet.data, packet.size);
			
			// Did we get a video frame?
			if (frameFinished) {
				// Convert the image from its native format to RGB
				sws_scale(ffmpeg_fields.img_convert_ctx, ffmpeg_fields.pFrame->data, ffmpeg_fields.pFrame->linesize, 0,
						ffmpeg_fields.pCodecCtx->height, pFrameRGB->data, pFrameRGB->linesize);
				env->CallVoidMethod(obj, fields.clb_onVideoFrame);
			}
		} else if (packet.stream_index == ffmpeg_fields.audioStream) {
			FFMpegPlayerAndroid_processAudio(&packet, samples);
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

static void FFMpegPlayerAndroid_stop(JNIEnv *env, jobject object) {
	if(status != STATE_PLAYING) {
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
	avcodec_close(ffmpeg_fields.pCodecCtx);
	
	// Close the video file
	av_close_input_file(ffmpeg_fields.pFormatCtx);
}

/*
* JNI registration.
*/
static JNINativeMethod methods[] = {
	{ "nativeInit", "(Lcom/media/ffmpeg/FFMpegAVFormatContext;)[I", (void*) FFMpegPlayerAndroid_init},
	{ "nativeSetInputFile", "(Ljava/lang/String;)Lcom/media/ffmpeg/FFMpegAVFormatContext;", (void*) FFMpegPlayerAndroid_setInputFile },
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
	fields.avformatcontext = env->GetFieldID(AVFormatContext_getClass(env), "pointer", "I");
	if (fields.avformatcontext == NULL) {
		return JNI_ERR;
	}
	return jniRegisterNativeMethods(env, "com/media/ffmpeg/android/FFMpegPlayerAndroid", methods, sizeof(methods) / sizeof(methods[0]));
}
