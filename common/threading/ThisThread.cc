// Copyright (c) 2013 The claire-common Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include <claire/common/threading/ThisThread.h>

#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/syscall.h>

#include <boost/type_traits/is_same.hpp>

namespace claire {
namespace ThisThread {

namespace {

pid_t gettid()
{
    return static_cast<pid_t>(::syscall(SYS_gettid));
}

void after_fork()
{
    claire::ThisThread::t_cached_tid = 0;
    claire::ThisThread::t_thread_name = "main";
    claire::ThisThread::tid();
}

class ThreadNameInitializer
{
public:
    ThreadNameInitializer()
    {
        claire::ThisThread::t_thread_name = "main";
        claire::ThisThread::tid();
        pthread_atfork(NULL, NULL, &after_fork);
    }
};

ThreadNameInitializer initializer;

} // namespace


__thread int t_cached_tid = 0;
__thread char t_tid_string[32];
__thread const char* t_thread_name = "unknown";

static_assert(boost::is_same<int, pid_t>::value, "pid_t should equal int");

void CacheTid()
{
    if (t_cached_tid == 0)
    {
        t_cached_tid = gettid();
        int n = snprintf(t_tid_string, sizeof t_tid_string, "%5d ", t_cached_tid);
        assert(n == 6);
        (void) n;
    }
}

bool IsMainThread()
{
    return tid() == ::getpid();
}

void SleepForMicroSeconds(int64_t microseconds)
{
    struct timespec ts = {0, 0};
    ts.tv_sec = static_cast<time_t>(microseconds/10000000);
    ts.tv_nsec = static_cast<long>(microseconds%1000000 * 1000);
    ::nanosleep(&ts, NULL);
}

} // namespace ThisThread
} // namespace claire
