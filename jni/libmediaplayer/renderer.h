#ifndef FFMPEG_RENDERER_H
#define FFMPEG_RENDERER_H

#include <jni.h>

#include <utils/Vector.h>

#include "thread.h"
#include "packetqueue.h"

using namespace android;

#define	EVENT_TYPE_VIDEO 1
#define	EVENT_TYPE_AUDIO 2

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
	public:
		AVFrame* 				frame;
		double					pts;

		VideoEvent(AVFrame* f, double p)
		{
			type = EVENT_TYPE_VIDEO;
			frame = f;
			pts = p;
		}
	};

	class AudioEvent : public Event
	{
	public:
		int16_t*                    samples;
		int                         samples_size;

		AudioEvent(int16_t* data, int data_size)
		{
			type = EVENT_TYPE_AUDIO;
			samples = data;
			samples_size = data_size;
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
	
	AVFrame*					mFrame;
	struct SwsContext*			mConvertCtx;

	bool						processAudio(Event* event);
	bool 						processVideo(Event* event);
	void						handleRun(void* ptr);
};

#endif //FFMPEG_RENDERER_H
