// Copyright (c) 2013 The claire-common Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef _CLAIRE_COMMON_THREADING_THREADPOOL_H_
#define _CLAIRE_COMMON_THREADING_THREADPOOL_H_

#include <deque>
#include <string>

#include <boost/atomic.hpp>
#include <boost/noncopyable.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

#include <claire/common/threading/Mutex.h>
#include <claire/common/threading/Thread.h>
#include <claire/common/threading/Condition.h>
#include <claire/common/tracing/TraceContext.h>

namespace claire {

class ThreadPool : boost::noncopyable
{
public:
    typedef boost::function<void()> Task;

    explicit ThreadPool(const std::string& name);
    ~ThreadPool();

    // must called before Start
    void set_max_queue_size(int max_size) { max_queue_size_ = max_size; }

    void Start(int num_threads);
    void Stop();

    void Run(const Task& task);
    void Run(Task&& task);

private:
    typedef std::pair<TraceContext, Task> Entry;

    bool IsFull() const;
    Entry Take();
    void RunInThread();

    Mutex mutex_;
    Condition not_empty_;
    Condition not_full_;

    const std::string name_;
    boost::ptr_vector<Thread> threads_;
    std::deque<Entry> queue_;
    size_t max_queue_size_;
    boost::atomic<bool> running_;
};

} // namespace claire

#endif // _CLAIRE_COMMON_THREADING_THREADPOOL_H_
