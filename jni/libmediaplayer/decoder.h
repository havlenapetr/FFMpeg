#ifndef FFMPEG_DECODER_H
#define FFMPEG_DECODER_H

#include <pthread.h>

extern "C" {
	
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"

}

#include "packetqueue.h"

class IDecoder
{
public:
	IDecoder();
	~IDecoder();
	
	bool						start(const char* err);
    bool						startAsync(const char* err);
    int							wait();
    void						stop();
	void						enqueue(AVPacket* packet);
	int							packets();

protected:
    PacketQueue*                mQueue;
    AVCodecContext*             mCodecCtx;
    bool                        mDecoding;
    pthread_t                   mThread;

	static void*				startDecoding(void* ptr);
    virtual bool                prepare(const char *err);
    virtual bool                decode(void* ptr);
    virtual bool                process(AVPacket *packet);
};

#endif //FFMPEG_DECODER_H
