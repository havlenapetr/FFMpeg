#include <android/log.h>
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

struct screen_t {
	int width;
	int height;
};
static struct screen_t screen;

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

extern "C" {

static void FFMpegPlayerAndroid_saveFrame(AVFrame *pFrame, int width, int height, int iFrame) {
	FILE *pFile;
	char szFilename[100];
	int  y;
	
	// Open file
	sprintf(szFilename, "/sdcard/frame%d.ppm", iFrame);
	pFile=fopen(szFilename, "wb");
	if(pFile==NULL)
		return;
	
	// Write header
	fprintf(pFile, "P6\n%d %d\n255\n", width, height);
	
	// Write pixel data
	for(y=0; y<height; y++)
		fwrite(pFrame->data[0]+y*pFrame->linesize[0], 1, width*3, pFile);
	
	// Close file
	fclose(pFile);
}

static void FFMpegPlayerAndroid_init(JNIEnv *env, jobject obj, jobject pAVFormatContext) {
	ffmpeg_fields.pFormatCtx = (AVFormatContext *) env->GetIntField(pAVFormatContext, fields.avformatcontext);

	// Find the first video stream
	ffmpeg_fields.videoStream = -1;
	for (int i = 0; i < ffmpeg_fields.pFormatCtx->nb_streams; i++)
		if (ffmpeg_fields.pFormatCtx->streams[i]->codec->codec_type == CODEC_TYPE_VIDEO) {
			ffmpeg_fields.videoStream = i;
			break;
		}

	if (ffmpeg_fields.videoStream == -1) {
		jniThrowException(env,
						  "java/io/IOException",
						  "Didn't find a video stream");
		return;
	}

	// Get a pointer to the codec context for the video stream
	ffmpeg_fields.pCodecCtx = ffmpeg_fields.pFormatCtx->streams[ffmpeg_fields.videoStream]->codec;

	// Find the decoder for the video stream
	ffmpeg_fields.pCodec = avcodec_find_decoder(ffmpeg_fields.pCodecCtx->codec_id);
	if (ffmpeg_fields.pCodec == NULL) {
		jniThrowException(env,
						  "java/io/IOException",
						  "Unsupported codec!");
		return; // Codec not found
	}

	// Open codec
	if (avcodec_open(ffmpeg_fields.pCodecCtx, ffmpeg_fields.pCodec) < 0) {
		jniThrowException(env,
						  "java/io/IOException",
						  "Could not open codec");
		return; // Could not open codec
	}

	// Allocate video frame
	ffmpeg_fields.pFrame = avcodec_alloc_frame();

	int w = ffmpeg_fields.pCodecCtx->width;
	int h = ffmpeg_fields.pCodecCtx->height;
	ffmpeg_fields.img_convert_ctx = sws_getContext(w, h, ffmpeg_fields.pCodecCtx->pix_fmt, w, h,
			PIX_FMT_RGB565, SWS_POINT, NULL, NULL, NULL);
}

static void FFMpegPlayerAndroid_handleOnVideoFrame(JNIEnv *env, jobject object, AVFrame *pFrame, int width, int height);
static void FFMpegPlayerAndroid_play(JNIEnv *env, jobject obj) {
	// Determine required buffer size and allocate buffer
	int numBytes = avpicture_get_size(PIX_FMT_RGB565, ffmpeg_fields.pCodecCtx->width, ffmpeg_fields.pCodecCtx->height);
	uint8_t *buffer = (uint8_t *) av_malloc(numBytes * sizeof(uint8_t));

	// Allocate an AVFrame structure
	AVFrame *pFrameRGB = avcodec_alloc_frame();
	if (pFrameRGB == NULL) {
		jniThrowException(env,
						  "java/io/IOException",
						  "Could allocate an AVFrame structure");
		return;
	}

	// Assign appropriate parts of buffer to image planes in pFrameRGB
	// Note that pFrameRGB is an AVFrame, but AVFrame is a superset
	// of AVPicture
	avpicture_fill((AVPicture *) pFrameRGB, buffer, PIX_FMT_RGB565,
			ffmpeg_fields.pCodecCtx->width, ffmpeg_fields.pCodecCtx->height);

	int frameFinished;
	AVPacket packet;
	int i = 0;
	int result = -1;
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

				FFMpegPlayerAndroid_handleOnVideoFrame(env, obj, ffmpeg_fields.pFrame, ffmpeg_fields.pCodecCtx->width,
						ffmpeg_fields.pCodecCtx->height);
				i++;
			}
		}

		// Free the packet that was allocated by av_read_frame
		av_free_packet(&packet);
	}

	// Free the RGB image
	av_free(buffer);
	av_free(pFrameRGB);

	// Free the YUV frame
	av_free(ffmpeg_fields.pFrame);

	// Close the codec
	avcodec_close(ffmpeg_fields.pCodecCtx);

	// Close the video file
	av_close_input_file(ffmpeg_fields.pFormatCtx);

	status = STATE_STOPED;
	__android_log_print(ANDROID_LOG_INFO, TAG, "end of playing");
}

} // end of extern C

static void FFMpegPlayerAndroid_stop(JNIEnv *env, jobject object) {
	if(status != STATE_PLAYING) {
		return;
	}
	status = STATE_STOPING;
}

static void FFMpegPlayerAndroid_surfaceChanged(JNIEnv *env, jobject object, int width, int height) {
	screen.width = width;
	screen.height = height;
}

static void FFMpegPlayerAndroid_handleOnVideoFrame(JNIEnv *env, jobject object, AVFrame *pFrame, int width, int height) {
	int size = height * width * 2;//) * pFrame->linesize[0];
	//__android_log_print(ANDROID_LOG_INFO, TAG, "width: %i, height: %i, linesize: %i", width, height, pFrame->linesize[0]);
	jintArray arr = env->NewIntArray(size);
	env->SetIntArrayRegion(arr, 0, width * 15, (jint *) pFrame->data[0] /* + y *pFrame->linesize[0]*/);
    env->CallVoidMethod(object, fields.clb_onVideoFrame, arr, width, height);
    env->DeleteLocalRef(arr);
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

}

/*
* JNI registration.
*/
static JNINativeMethod methods[] = {
	{ "nativeInit", "(Lcom/media/ffmpeg/FFMpegAVFormatContext;)V", (void*) FFMpegPlayerAndroid_init},
	{ "nativeSurfaceChanged", "(II)V", (void*) FFMpegPlayerAndroid_surfaceChanged },
	{ "nativeSetInputFile", "(Ljava/lang/String;)Lcom/media/ffmpeg/FFMpegAVFormatContext;", (void*) FFMpegPlayerAndroid_setInputFile },
	{ "nativePlay", "()V", (void*) FFMpegPlayerAndroid_play },
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
	fields.clb_onVideoFrame = env->GetMethodID(FFMpegPlayerAndroid_getClass(env), "onVideoFrame", "([III)V");
	if (fields.clb_onVideoFrame == NULL) {
		return JNI_ERR;
	}
	fields.avformatcontext = env->GetFieldID(AVFormatContext_getClass(env), "pointer", "I");
	if (fields.avformatcontext == NULL) {
		return JNI_ERR;
	}
	return jniRegisterNativeMethods(env, "com/media/ffmpeg/android/FFMpegPlayerAndroid", methods, sizeof(methods) / sizeof(methods[0]));
}
