// Copyright (c) 2013 The claire-common Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef _CLAIRE_COMMON_THREADING_CONDITION_H_
#define _CLAIRE_COMMON_THREADING_CONDITION_H_

#include <pthread.h>

#include <boost/noncopyable.hpp>

#include <claire/common/threading/Mutex.h>
#include <claire/common/logging/Logging.h>

namespace claire {

class Condition : boost::noncopyable
{
public:
    explicit Condition(Mutex& mutex)
        : mutex_(mutex)
    {
        DCHECK_ERR(pthread_cond_init(&pcond_, NULL));
    }

    ~Condition()
    {
        DCHECK_ERR(pthread_cond_destroy(&pcond_));
    }

    void Wait()
    {
        Mutex::UnAssignGuard ug(mutex_);
        DCHECK_ERR(pthread_cond_wait(&pcond_, &(mutex_.mutex_)));
    }

    // return true if timeout, otherwise return false
    bool WaitForSeconds(int seconds)
    {
        struct timespec abstime;
        ::clock_gettime(CLOCK_REALTIME, &abstime);
        abstime.tv_sec += seconds;
        Mutex::UnAssignGuard ug(mutex_);
        return ETIMEDOUT == pthread_cond_timedwait(&pcond_, &(mutex_.mutex_), &abstime);
    }

    void Notify()
    {
        DCHECK_ERR(pthread_cond_signal(&pcond_));
    }

    void NotifyAll()
    {
        DCHECK_ERR(pthread_cond_broadcast(&pcond_));
    }

private:
    Mutex& mutex_;
    pthread_cond_t pcond_;
};

} // namespace claire

#endif // _CLAIRE_COMMON_THREADING_CONDITION_H_
