// Copyright (c) 2013 The claire-common Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include <claire/common/threading/Thread.h>

#include <boost/bind.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include <claire/common/base/Exception.h>
#include <claire/common/logging/Logging.h>
#include <claire/common/threading/ThisThread.h>

namespace claire {

struct Thread::Priv
{
    Thread::ThreadEntry entry_;
    boost::weak_ptr<pid_t> weak_tid_;
    const std::string name_;

    Priv(const Thread::ThreadEntry& entry,
         const boost::shared_ptr<pid_t>& tid,
         const std::string& name)
        : entry_(entry),
          weak_tid_(tid),
          name_(name)
    {}

    void RunInThread()
    {
        auto tid = weak_tid_.lock();
        if (tid)
        {
            *tid = ThisThread::tid();
            tid.reset();
        }

        ThisThread::set_thread_name(name_.c_str());
        try
        {
            entry_();
            claire::ThisThread::set_thread_name("finished");
        }
        catch (const Exception& e)
        {
            ThisThread::set_thread_name("crashed");
            fprintf(stderr, "exception caught in Thread %s\n", name_.c_str());
            fprintf(stderr, "reason: %s\n", e.what());
            fprintf(stderr, "stack trace: %s\n", e.stack_trace());
            abort();
        }
        catch (const std::exception& e)
        {
            ThisThread::set_thread_name("crashed");
            fprintf(stderr, "exception caught in Thread %s\n", name_.c_str());
            fprintf(stderr, "reason: %s\n", e.what());
            abort();
        }
        catch (...)
        {
            claire::ThisThread::set_thread_name("crashed");
            fprintf(stderr, "unknown exception caught in Thread %s\n", name_.c_str());
            throw; // rethrow
        }
    }

    static void* StartThread(void* object)
    {
        auto priv = static_cast<Priv*>(object);
        priv->RunInThread();
        delete priv;
        return NULL;
    }
};

Thread::Thread(const ThreadEntry& entry, const std::string& name__)
    : started_(false),
      joined_(false),
      pthread_id_(0),
      tid_(new pid_t(0)),
      entry_(entry),
      name_(name__)
{}

Thread::Thread(ThreadEntry&& entry, const std::string& name__)
    : started_(false),
      joined_(false),
      pthread_id_(0),
      tid_(new pid_t(0)),
      entry_(entry),
      name_(name__)
{}

Thread::~Thread()
{
    if (started_ && !joined_)
    {
        pthread_detach(pthread_id_);
    }
}

void Thread::Start()
{
    if (started_)
    {
        return ;
    }
    started_ = true;

    auto priv = new Priv(entry_, tid_, name_);
    if (pthread_create(&pthread_id_, NULL, &Priv::StartThread, priv))
    {
        started_ = false;
        delete priv; // use scopeguard FIXME
        LOG(FATAL) << "pthread_create failed";
    }
}

int Thread::Join()
{
    DCHECK(started_);
    DCHECK(!joined_);
    joined_ = true;
    return pthread_join(pthread_id_, NULL);
}

} // namespace claire
