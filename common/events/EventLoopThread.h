// Copyright (c) 2013 The claire-common Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef _CLAIRE_COMMON_EVENTS_EVENTLOOPTHREAD_H_
#define _CLAIRE_COMMON_EVENTS_EVENTLOOPTHREAD_H_

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>

#include <claire/common/threading/Mutex.h>
#include <claire/common/threading/Thread.h>
#include <claire/common/threading/Condition.h>

namespace claire {

class EventLoop;

class EventLoopThread : boost::noncopyable
{
public:
    typedef boost::function<void(EventLoop*)> ThreadInitCallback;

    EventLoopThread(const ThreadInitCallback& callback);
    ~EventLoopThread();

    EventLoop* StartLoop();

private:
    void ThreadMain();

    EventLoop* loop_;
    Thread thread_;
    Mutex mutex_;
    Condition condition_;
    ThreadInitCallback callback_;
};

} // namespace claire

#endif // _CLAIRE_COMMON_EVENTS_EVENTLOOPTHREAD_H_
