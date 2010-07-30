#ifndef FFMPEG_DECODER_H
#define FFMPEG_DECODER_H

#include <pthread.h>

// map system drivers methods
#include <drivers_map.h>

#include "packetqueue.h"

class IDecoder
{
public:
    virtual bool start(const char* err);
    virtual bool startAsync(const char* err);
    virtual int wait();
    virtual void stop();

protected:
    PacketQueue*                mQueue;
    AVCodecContext*             mCodecCtx;
    bool                        mDecoding;
    pthread_t                   mThread;

    virtual bool                prepare(const char *err);
    virtual bool                decode(void* ptr);
    virtual bool                process(AVPacket *packet);
};

#endif //FFMPEG_DECODER_H
