// Copyright (c) 2013 The claire-common Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef _CLAIRE_COMMON_THREADING_THREAD_H_
#define _CLAIRE_COMMON_THREADING_THREAD_H_

#include <pthread.h>

#include <string>

#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>

namespace claire {

class Thread : boost::noncopyable
{
public:
    typedef boost::function<void()> ThreadEntry;

    explicit Thread(const ThreadEntry&, const std::string&);
    explicit Thread(ThreadEntry&&, const std::string&);
    ~Thread();

    void Start();
    int Join();

    bool started() const { return started_; }
    pid_t tid() const { return *tid_; }
    const std::string& name() const { return name_; }

private:
    struct Priv;

    bool started_;
    bool joined_;
    pthread_t pthread_id_;
    boost::shared_ptr<pid_t> tid_;
    ThreadEntry entry_;
    const std::string name_;
};

} // namespace claire

#endif // _CLAIRE_COMMON_THREAD_THREAD_H_
