#ifndef FFMPEG_PACKETQUEUE_H
#define FFMPEG_PACKETQUEUE_H

#include <pthread.h>

extern "C" {
	
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
	
} // end of extern C

class PacketQueue
{
public:
	PacketQueue();
	~PacketQueue();
	
    void flush();
	int put(AVPacket* pkt);
	
	/* return < 0 if aborted, 0 if no packet and > 0 if packet.  */
	int get(AVPacket *pkt, bool block);
	
	int size();
	
	void abort();
	
private:
	AVPacketList*		mFirst;
	AVPacketList*		mLast;
    int					mNbPackets;
    int					mSize;
    bool				mAbortRequest;
	pthread_mutex_t     mLock;
	pthread_cond_t		mCondition;
};

#endif // FFMPEG_MEDIAPLAYER_H
