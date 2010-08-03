#ifndef FFMPEG_RENDERER_H
#define FFMPEG_RENDERER_H

#include <jni.h>
#include <pthread.h>

#include "packetqueue.h"

class Renderer
{
public:
	Renderer();
	~Renderer();
	
	bool						init(JNIEnv *env, jobject jsurface);
	PacketQueue*				queue();
	bool						startAsync(const char* err);
	int							wait();
    void						stop();
	
private:
	PacketQueue*                mQueue;
	pthread_t                   mThread;
	bool						mRendering;
	
	bool						render(void* ptr);
	static void*				startRendering(void* ptr);
};

#endif //FFMPEG_DECODER_H
