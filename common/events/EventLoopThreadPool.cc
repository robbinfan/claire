// Copyright (c) 2013 The claire-common Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include <claire/common/events/EventLoopThreadPool.h>

#include <claire/common/events/EventLoop.h>
#include <claire/common/events/EventLoopThread.h>
#include <claire/common/logging/Logging.h>

namespace claire {

EventLoopThreadPool::EventLoopThreadPool(EventLoop* loop)
    : base_loop_(loop),
      started_(false),
      num_threads_(0),
      next_(0)
{}

void EventLoopThreadPool::Start()
{
    ThreadInitCallback callback;
    Start(callback);
}

void EventLoopThreadPool::Start(const ThreadInitCallback& callback)
{
    DCHECK(!started_);
    base_loop_->AssertInLoopThread();

    started_ = true;
    for (int i = 0;i < num_threads_; ++i)
    {
        auto thread = new EventLoopThread(callback);
        threads_.push_back(thread);
        loops_.push_back(thread->StartLoop());
    }

    if (num_threads_ == 0 && callback)
    {
        callback(base_loop_);
    }
}

EventLoop* EventLoopThreadPool::NextLoop()
{
    base_loop_->AssertInLoopThread();

    auto loop = base_loop_;
    if (!loops_.empty())
    {
        // round-robin
        loop = loops_[next_++];
        if (next_ >= static_cast<int>(loops_.size()))
        {
            next_ = 0;
        }
    }

    return loop;
}

} // namespace claire
