// Copyright (c) 2013 The claire-common Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef _CLAIRE_COMMON_BASE_BLOCKINGQUEUE_H_
#define _CLAIRE_COMMON_BASE_BLOCKINGQUEUE_H_

#include <deque>

#include <boost/noncopyable.hpp>

#include <claire/common/threading/Mutex.h>
#include <claire/common/threading/Condition.h>

namespace claire {

template<typename T>
class BlockingQueue : boost::noncopyable
{
public:
    BlockingQueue()
      : mutex_(),
        not_empty_(mutex_),
        queue_()
    {}

    void Put(const T& x)
    {
        MutexLock lock(mutex_);
        queue_.push_back(x);
        not_empty_.Notify(); // wait morphing saves us
        // http://www.domaigne.com/blog/computing/condvars-signal-with-mutex-locked-or-not/
    }

    T Take()
    {
        MutexLock lock(mutex_);
        // always use a while-loop, due to spurious wakeup
        while (queue_.empty())
        {
            not_empty_.Wait();
        }
        T front(queue_.front());
        queue_.pop_front();
        return front;
    }

    size_t size() const
    {
        MutexLock lock(mutex_);
        return queue_.size();
    }

private:
    mutable Mutex mutex_;
    Condition not_empty_;
    std::deque<T> queue_;
};

} // namespace claire

#endif // _CLAIRE_COMMON_BASE_BLOCKINGQUEUE_H_
