#include <android/log.h>
#include "jniUtils.h"
#include "methods.h"

extern "C" {

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"

} // end of extern C

#define TAG "FFMpegUtils"

struct fields_t {
	jfieldID    surface;
	jmethodID   clb_onVideoFrame;
};
static struct fields_t fields;

static const char *framesOutput = NULL;

jclass FFMpegUtils_getClass(JNIEnv *env) {
	return env->FindClass("com/media/ffmpeg/FFMpegUtils");
}

const char *FFMpegUtils_getSignature() {
	return "Lcom/media/ffmpeg/FFMpegUtils;";
}

extern "C" {

static void FFMpegUtils_saveFrame(AVFrame *pFrame, int width, int height, int iFrame) {
	FILE *pFile;
	char szFilename[256];
	int  y;

	// Open file
	if(framesOutput == NULL) {
		sprintf(szFilename, "/sdcard/frame%d.ppm", iFrame);
	} else {
		sprintf(szFilename, "%s/frame%d.ppm", framesOutput, iFrame);
	}

	pFile=fopen(szFilename, "wb");
	if(pFile==NULL)
		return;

	// Write header
	fprintf(pFile, "P6\n%d %d\n255\n", width, height);
	
	// Write pixel data
	for(y=0; y<height; y++)
		fwrite(pFrame->data[0]+y*pFrame->linesize[0], 1, width * 2, pFile);
	
	// Close file
	fclose(pFile);
}

static jobject FFMpegUtils_setInputFile(JNIEnv *env, jobject obj, jstring filePath) {
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

static void FFMpegUtils_handleOnVideoFrame(JNIEnv *env, jobject object, AVFrame *pFrame, int width, int height);
static void FFMpegUtils_print(JNIEnv *env, jobject obj, jint pAVFormatContext) {
	int i;
	AVCodecContext *pCodecCtx;
	AVFrame *pFrame;
	AVCodec *pCodec;
	AVFormatContext *pFormatCtx = (AVFormatContext *) pAVFormatContext;
	struct SwsContext *img_convert_ctx;

	__android_log_print(ANDROID_LOG_INFO, TAG, "playing");

	// Find the first video stream
	int videoStream = -1;
	for (i = 0; i < pFormatCtx->nb_streams; i++)
		if (pFormatCtx->streams[i]->codec->codec_type == CODEC_TYPE_VIDEO) {
			videoStream = i;
			break;
		}
	if (videoStream == -1) {
		jniThrowException(env,
						  "java/io/IOException",
						  "Didn't find a video stream");
		return;
	}

	// Get a pointer to the codec context for the video stream
	pCodecCtx = pFormatCtx->streams[videoStream]->codec;

	// Find the decoder for the video stream
	pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
	if (pCodec == NULL) {
		jniThrowException(env,
						  "java/io/IOException",
						  "Unsupported codec!");
		return; // Codec not found
	}
	// Open codec
	if (avcodec_open(pCodecCtx, pCodec) < 0) {
		jniThrowException(env,
						  "java/io/IOException",
						  "Could not open codec");
		return; // Could not open codec
	}

	// Allocate video frame
	pFrame = avcodec_alloc_frame();

	// Allocate an AVFrame structure
	AVFrame *pFrameRGB = avcodec_alloc_frame();
	if (pFrameRGB == NULL) {
		jniThrowException(env,
						  "java/io/IOException",
						  "Could allocate an AVFrame structure");
		return;
	}

	uint8_t *buffer;
	int numBytes;
	// Determine required buffer size and allocate buffer
	numBytes = avpicture_get_size(PIX_FMT_RGB565, pCodecCtx->width,
			pCodecCtx->height);
	buffer = (uint8_t *) av_malloc(numBytes * sizeof(uint8_t));

	// Assign appropriate parts of buffer to image planes in pFrameRGB
	// Note that pFrameRGB is an AVFrame, but AVFrame is a superset
	// of AVPicture
	avpicture_fill((AVPicture *) pFrameRGB, buffer, PIX_FMT_RGB565,
			pCodecCtx->width, pCodecCtx->height);

	int w = pCodecCtx->width;
	int h = pCodecCtx->height;
	img_convert_ctx = sws_getContext(w, h, pCodecCtx->pix_fmt, w, h,
			PIX_FMT_RGB565, SWS_BICUBIC, NULL, NULL, NULL);

	int frameFinished;
	AVPacket packet;

	i = 0;
	int result = -1;
	while ((result = av_read_frame(pFormatCtx, &packet)) >= 0) {
		// Is this a packet from the video stream?
		if (packet.stream_index == videoStream) {
			// Decode video frame
			avcodec_decode_video(pCodecCtx, pFrame, &frameFinished,
					packet.data, packet.size);

			// Did we get a video frame?
			if (frameFinished) {
				// Convert the image from its native format to RGB
				sws_scale(img_convert_ctx, pFrame->data, pFrame->linesize, 0,
						pCodecCtx->height, pFrameRGB->data, pFrameRGB->linesize);

				//FFMpegUtils_saveFrame(pFrameRGB, pCodecCtx->width,
				//		pCodecCtx->height, i);
				FFMpegUtils_handleOnVideoFrame(env, obj, pFrame, pCodecCtx->width,
						pCodecCtx->height);
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
	av_free(pFrame);

	// Close the codec
	avcodec_close(pCodecCtx);

	// Close the video file
	av_close_input_file(pFormatCtx);

	__android_log_print(ANDROID_LOG_INFO, TAG, "end of playing");
}

} // end of extern C

static void FFMpegUtils_handleOnVideoFrame(JNIEnv *env, jobject object, AVFrame *pFrame, int width, int height) {
	int size = width * 2;//) * pFrame->linesize[0];
	//__android_log_print(ANDROID_LOG_INFO, TAG, "width: %i, height: %i, linesize: %i", width, height, pFrame->linesize[0]);
	jintArray arr = env->NewIntArray(size);
	env->SetIntArrayRegion(arr, 0, size, (jint *) pFrame->data[0] /* + y *pFrame->linesize[0]*/);
    env->CallVoidMethod(object, fields.clb_onVideoFrame, arr);
}

static void FFMpegUtils_setOutput(JNIEnv *env, jobject obj, jstring path) {
	framesOutput = env->GetStringUTFChars(path, NULL);
}

static void FFMpegUtils_release(JNIEnv *env, jobject obj) {

}

/*
* JNI registration.
*/
static JNINativeMethod methods[] = {
	{ "native_av_setInputFile", "(Ljava/lang/String;)Lcom/media/ffmpeg/FFMpegAVFormatContext;", (void*) FFMpegUtils_setInputFile },
	{ "native_av_print", "(I)V", (void*) FFMpegUtils_print },
	{ "native_av_release", "()V", (void*) FFMpegUtils_release },
	{ "native_av_setOutput", "(Ljava/lang/String;)V", (void*) FFMpegUtils_setOutput}
};

int register_android_media_FFMpegUtils(JNIEnv *env) {
	jclass clazz = FFMpegUtils_getClass(env);
	fields.clb_onVideoFrame = env->GetMethodID(clazz, "onVideoFrame", "([I)V");
	if (fields.clb_onVideoFrame == NULL) {
		return JNI_ERR;
	}
	return jniRegisterNativeMethods(env, "com/media/ffmpeg/FFMpegUtils", methods, sizeof(methods) / sizeof(methods[0]));
}
