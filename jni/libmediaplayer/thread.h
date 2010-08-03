#ifndef FFMPEG_THREAD_H
#define FFMPEG_THREAD_H

#include <pthread.h>

class Thread
{
public:
	Thread();
	~Thread();
	
	void						start();
    void						startAsync();
    int							wait();
    virtual void				stop();

protected:
    pthread_t                   mThread;
	bool						mRunning;

    virtual void                handleRun(void* ptr);
	
private:
	static void*				startThread(void* ptr);
};

#endif //FFMPEG_DECODER_H
