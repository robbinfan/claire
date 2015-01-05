// Copyright (c) 2013 The claire-common Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef _CLAIRE_COMMON_EVENTS_EVENTLOOPTHREADPOOL_H_
#define _CLAIRE_COMMON_EVENTS_EVENTLOOPTHREADPOOL_H_

#include <vector>

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

namespace claire {

class EventLoop;
class EventLoopThread;

class EventLoopThreadPool : boost::noncopyable
{
public:
    typedef boost::function<void(EventLoop*)> ThreadInitCallback;

    EventLoopThreadPool(EventLoop* base_loop);

    void set_num_threads(int num_threads)
    {
        num_threads_ = num_threads;
    }

    void Start();
    void Start(const ThreadInitCallback& callback);
    EventLoop* NextLoop();

private:
    EventLoop* base_loop_;
    bool started_;
    int num_threads_;
    int next_;
    boost::ptr_vector<EventLoopThread> threads_;
    std::vector<EventLoop*> loops_;
};

} // namespace claire

#endif // _CLAIRE_COMMON_EVENTS_EVENTLOOPTHREADPOOL_H_
