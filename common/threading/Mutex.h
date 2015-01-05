// Copyright (c) 2013 The claire-common Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef _CLAIRE_COMMON_THREADING_MUTEX_H_
#define _CLAIRE_COMMON_THREADING_MUTEX_H_

#include <pthread.h>

#include <boost/noncopyable.hpp>

namespace claire {

class Mutex : boost::noncopyable
{
public:
    Mutex();
    ~Mutex();

    void Lock();
    void Unlock();

    bool IsLockedByThisThread() const;
    void AssertLocked() const;

private:
    friend class Condition;

    class UnAssignGuard
    {
    public:
        UnAssignGuard(Mutex& owner)
            : owner_(owner)
        {
            owner_.UnAssignHolder();
        }

        ~UnAssignGuard()
        {
            owner_.AssignHolder();
        }

    private:
        Mutex& owner_;
    };

    void AssignHolder();
    void UnAssignHolder();

    pthread_mutex_t mutex_;
#ifndef NDEBUG
    pthread_mutexattr_t attr_;
#endif
    pid_t holder_;
};

class MutexLock : boost::noncopyable
{
public:
    explicit MutexLock(Mutex& mutex)
        : mutex_(mutex)
    {
        mutex_.Lock();
    }

    ~MutexLock()
    {
        mutex_.Unlock();
    }

private:
    Mutex& mutex_;
};

} // namespace claire

#endif // _CLAIRE_COMMON_THREADING_MUTEX_H_
