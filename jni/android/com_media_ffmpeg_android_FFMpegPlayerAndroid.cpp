#include <android/log.h>
#include "jniUtils.h"
#include "methods.h"

extern "C" {

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"

} // end of extern C

#define TAG "FFMpegPlayerAndroid"

struct fields_t {
	jfieldID    surface;
};
static struct fields_t fields;

extern "C" {

void FFMpegPlayerAndroid_saveFrame(AVFrame *pFrame, int width, int height, int iFrame) {
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

void FFMpegPlayerAndroid_play(int argc, char **argv) {
	int i;
		AVCodecContext *pCodecCtx;
		AVFrame *pFrame;
		AVCodec *pCodec;
		AVFormatContext *pFormatCtx;
		struct SwsContext *img_convert_ctx;

		__android_log_print(ANDROID_LOG_INFO, TAG, "playing");

		// Open video file
		if(av_open_input_file(&pFormatCtx, argv[1], NULL, 0, NULL)!=0)
			return; // Couldn't open file
		// Retrieve stream information
		if(av_find_stream_info(pFormatCtx)<0)
			return; // Couldn't find stream information

		// Find the first video stream
		int videoStream=-1;
		for(i=0; i<pFormatCtx->nb_streams; i++)
			if(pFormatCtx->streams[i]->codec->codec_type==CODEC_TYPE_VIDEO) {
				videoStream=i;
				break;
			}
		if(videoStream==-1)
			return; // Didn't find a video stream

		// Get a pointer to the codec context for the video stream
		pCodecCtx=pFormatCtx->streams[videoStream]->codec;

		// Find the decoder for the video stream
		pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
		if(pCodec==NULL) {
			fprintf(stderr, "Unsupported codec!\n");
			return; // Codec not found
		}
		// Open codec
		if(avcodec_open(pCodecCtx, pCodec)<0)
			return; // Could not open codec

		// Allocate video frame
		pFrame=avcodec_alloc_frame();

		// Allocate an AVFrame structure
		AVFrame *pFrameRGB=avcodec_alloc_frame();
		if(pFrameRGB==NULL)
			return;

		uint8_t *buffer;
		int numBytes;
		// Determine required buffer size and allocate buffer
		numBytes=avpicture_get_size(PIX_FMT_RGB24, pCodecCtx->width,
									pCodecCtx->height);
		buffer=(uint8_t *)av_malloc(numBytes*sizeof(uint8_t));

		// Assign appropriate parts of buffer to image planes in pFrameRGB
		// Note that pFrameRGB is an AVFrame, but AVFrame is a superset
		// of AVPicture
		avpicture_fill((AVPicture *)pFrameRGB, buffer, PIX_FMT_RGB24,
					   pCodecCtx->width, pCodecCtx->height);

		int w = pCodecCtx->width;
		int h = pCodecCtx->height;
		img_convert_ctx = sws_getContext(w, h,
								pCodecCtx->pix_fmt,
		                        w, h, PIX_FMT_RGB24, SWS_BICUBIC,
		                        NULL, NULL, NULL);

		int frameFinished;
		AVPacket packet;

		i=0;
		while(av_read_frame(pFormatCtx, &packet)>=0) {
			// Is this a packet from the video stream?
			if(packet.stream_index==videoStream) {
				// Decode video frame
				avcodec_decode_video(pCodecCtx, pFrame, &frameFinished,
									 packet.data, packet.size);

				// Did we get a video frame?
				if(frameFinished) {
					// Convert the image from its native format to RGB
					sws_scale(img_convert_ctx, pFrame->data,
					              pFrame->linesize, 0,
					              pCodecCtx->height,
					              pFrameRGB->data, pFrameRGB->linesize);

					// Save the frame to disk
					if(++i<=5)
						FFMpegPlayerAndroid_saveFrame(pFrameRGB,
													  pCodecCtx->width,
													  pCodecCtx->height,
													  i);
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

static int FFMpegPlayerAndroid_drawFrame(AVFrame *pFrame, int width, int height) {

}

static void FFMpegPlayerAndroid_runPlayer(JNIEnv *env, jobject obj, jobjectArray args) {
	int argc = 0;
	char **argv = NULL;
	
	__android_log_print(ANDROID_LOG_INFO, TAG, "parsing args");

	if (args != NULL) {
		argc = env->GetArrayLength(args);
		argv = (char **) malloc(sizeof(char *) * argc);
		int i=0;
		for(i=0;i<argc;i++) {
			jstring str = (jstring)env->GetObjectArrayElement(args, i);
			argv[i] = (char *)env->GetStringUTFChars(str, NULL);
		}
	}
	
	FFMpegPlayerAndroid_play(argc, argv);
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
	{ "native_av_runPlayer", "([Ljava/lang/String;)V", (void*) FFMpegPlayerAndroid_runPlayer },
	{ "native_av_setSurface", "(Landroid/view/Surface;)V", (void*) FFMpegPlayerAndroid_setSurface },
	{ "native_av_release", "()V", (void*) FFMpegPlayerAndroid_release },
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

	return jniRegisterNativeMethods(env, "com/media/ffmpeg/android/FFMpegPlayerAndroid", methods, sizeof(methods) / sizeof(methods[0]));
}
