// Copyright (c) 2013 The claire-common Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include <assert.h>

#include <claire/common/threading/Mutex.h>
#include <claire/common/threading/ThisThread.h>
#include <claire/common/logging/Logging.h>

namespace claire {

Mutex::Mutex()
    : holder_(0)
{
#ifndef NDEBUG
    DCHECK_ERR(pthread_mutexattr_init(&attr_));
    DCHECK_ERR(pthread_mutexattr_settype(&attr_, PTHREAD_MUTEX_ERRORCHECK));
    DCHECK_ERR(pthread_mutex_init(&mutex_, &attr_));
#else
    DCHECK_ERR(pthread_mutex_init(&mutex_, NULL));
#endif
}

Mutex::~Mutex()
{
    DCHECK_ERR(pthread_mutex_destroy(&mutex_));
#ifndef NDEBUG
    DCHECK_ERR(pthread_mutexattr_destroy(&attr_));
#endif
}

void Mutex::Lock()
{
    DCHECK_ERR(pthread_mutex_lock(&mutex_));
    AssignHolder();
}

void Mutex::Unlock()
{
    UnAssignHolder();
    DCHECK_ERR(pthread_mutex_unlock(&mutex_));
}

bool Mutex::IsLockedByThisThread() const
{
    return holder_ == ThisThread::tid();
}

void Mutex::AssertLocked() const
{
    assert(IsLockedByThisThread());
}

void Mutex::AssignHolder()
{
    holder_ = ThisThread::tid();
}

void Mutex::UnAssignHolder()
{
    holder_ = 0;
}

} // namespace claire
