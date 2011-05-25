/*
 * Copyright (c) 2011 Petr Havlena  havlenapetr@gmail.com
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <stdio.h>

#include "thread.h"

#define THREAD_NAME_MAX 50

#define LOG_TAG "Thread"

static int sThreadCounter = 0;

Thread::Thread() :
    mName(NULL)
{
    init();
}

Thread::Thread(const char* name) :
    mName(name)
{
    init();
}

Thread::~Thread()
{
}

void Thread::init()
{
    pthread_mutex_init(&mLock, NULL);
    pthread_cond_init(&mCondition, NULL);
    sThreadCounter++;
}

void Thread::start()
{
    pthread_create(&mThread, NULL, handleThreadStart, this);
}

int Thread::join()
{
    if(!mRunning)
    {
        return 0;
    }
    return pthread_join(mThread, NULL);
}

void* Thread::handleThreadStart(void* ptr)
{
    Thread* thread = (Thread *) ptr;

    thread->mRunning = true;

    thread->run();

    thread->mRunning = false;

    return 0;
}

void Thread::run()
{
}
