// Copyright (c) 2013 The claire-common Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef _CLAIRE_COMMON_THREADING_COUNTDOWNLATCH_H_
#define _CLAIRE_COMMON_THREADING_COUNTDOWNLATCH_H_

#include <boost/noncopyable.hpp>

#include <claire/common/threading/Mutex.h>
#include <claire/common/threading/Condition.h>

namespace claire {

class CountDownLatch : boost::noncopyable
{
public:
    explicit CountDownLatch(int c)
        : mutex_(),
          condition_(mutex_),
          count_(c)
    {}

    void Wait()
    {
        MutexLock lock(mutex_);
        while (count_ > 0)
        {
            condition_.Wait();
        }
    }

    void CountDown()
    {
        MutexLock lock(mutex_);
        --count_;
        if (count_ == 0)
        {
            condition_.NotifyAll();
        }
    }

    int count() const
    {
        MutexLock lock(mutex_);
        return count_;
    }

private:
    mutable Mutex mutex_;
    Condition condition_;
    int count_;
};

} // namespace claire

#endif // _CLAIRE_COMMON_THREADING_COUNTDOWNLATCH_H_
