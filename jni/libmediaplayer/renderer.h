#ifndef FFMPEG_RENDERER_H
#define FFMPEG_RENDERER_H

#include <jni.h>

#include <utils/Vector.h>

#include "thread.h"
#include "packetqueue.h"

using namespace android;

#define	EVENT_TYPE_VIDEO 1;
#define	EVENT_TYPE_AUDIO 2;

class Renderer : public Thread
{
public:

	class Event
	{
	public:
		int 	type;
	};

	class VideoEvent : public Event
	{
		VideoEvent()
		{
			type = EVENT_TYPE_VIDEO;
		}
	};

	class AudioEvent : public Event
	{
		AudioEvent()
		{
			type = EVENT_TYPE_AUDIO;
		}
	};

	Renderer(AVStream* video_stream, AVStream* audio_stream);
	~Renderer();
	
	bool						init(JNIEnv *env, jobject jsurface);
	bool 						prepare(const char* err);

	void 						enqueue(Event* event);
    void						stop();
	
private:
	AVStream*					mAudioStream;
	AVStream*					mVideoStream;
	Vector<Event*>				mEventBuffer;
	
	void						handleRun(void* ptr);
};

#endif //FFMPEG_DECODER_H
