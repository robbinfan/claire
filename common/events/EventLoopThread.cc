// Copyright (c) 2013 The claire-common Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include <claire/common/events/EventLoopThread.h>

#include <boost/bind.hpp>

#include <claire/common/events/EventLoop.h>
#include <claire/common/logging/Logging.h>

namespace claire {

EventLoopThread::EventLoopThread(const ThreadInitCallback& callback)
    : loop_(NULL),
      thread_(boost::bind(&EventLoopThread::ThreadMain, this), std::string()),
      mutex_(),
      condition_(mutex_),
      callback_(callback)
{}

EventLoopThread::~EventLoopThread()
{
    loop_->quit();
    thread_.Join();
}

EventLoop* EventLoopThread::StartLoop()
{
    DCHECK(!thread_.started());
    thread_.Start();

    {
        MutexLock lock(mutex_);
        while(!loop_)
        {
            condition_.Wait();
        }
    }

    return loop_;
}

void EventLoopThread::ThreadMain()
{
    EventLoop loop;
    if (callback_)
    {
        callback_(&loop);
    }

    {
        MutexLock lock(mutex_);
        loop_ = &loop;
        condition_.Notify();
    }

    loop.loop();
}

} // namespace claire
