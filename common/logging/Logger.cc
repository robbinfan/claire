// Copyright (c) 2013 The Claire Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include <claire/common/logging/Logger.h>

#include <stdio.h>
#include <signal.h>

#include <boost/bind.hpp>

#include <claire/common/logging/LogFile.h>

namespace claire {

Logger::Logger()
    : running_(false),
      thread_(boost::bind(&Logger::ThreadMain, this), "Logger"),
      mutex_(),
      condition_(mutex_),
      latch_(1),
      current_buffer_(new Buffer),
      next_buffer_(new Buffer),
      buffers_()
{
    current_buffer_->bzero();
    next_buffer_->bzero();
    buffers_.reserve(16);
}

void Logger::Append(const char* data, size_t length)
{
    MutexLock lock(mutex_);
    if (current_buffer_->avail() > length)
    {
        current_buffer_->Append(data, length);
    }
    else
    {
        buffers_.push_back(current_buffer_.release());
        if (next_buffer_)
        {
            current_buffer_ = boost::ptr_container::move(next_buffer_);
        }
        else
        {
            current_buffer_.reset(new Buffer);
        }

        current_buffer_->Append(data, length);
        condition_.Notify();
    }
}

void Logger::ThreadMain()
{
    // first ignore unuse signals
    {
        sigset_t mask;
        sigemptyset(&mask);
        sigaddset(&mask, SIGHUP);
        sigaddset(&mask, SIGPIPE);
        sigaddset(&mask, SIGUSR1);
        sigaddset(&mask, SIGUSR2);
        sigprocmask(SIG_BLOCK, &mask, NULL);
    }

    assert(running_ = true);
    latch_.CountDown();

    LogFile output(false);

    BufferPtr buffer1(new Buffer);
    BufferPtr buffer2(new Buffer);

    buffer1->bzero();
    buffer2->bzero();

    decltype(buffers_) buffers_to_write;
    buffers_to_write.reserve(16);

    while (running_)
    {
        assert(buffer1 && buffer1->length() == 0);
        assert(buffer2 && buffer2->length() == 0);
        assert(buffers_to_write.empty());

        {
            MutexLock lock(mutex_);
            if (buffers_.empty())
            {
                condition_.WaitForSeconds(FLAGS_logbufsecs);
            }
            buffers_.push_back(current_buffer_.release());
            buffers_to_write.swap(buffers_);

            current_buffer_ = boost::ptr_container::move(buffer1);
            if (!next_buffer_)
            {
                next_buffer_ = boost::ptr_container::move(buffer2);
            }
        }

        assert(!buffers_to_write.empty());

        if (buffers_to_write.size() > 25)
        {
            char buf[256];
            snprintf(buf, sizeof buf, "Dropped log messages at %s, %zd larger buffers\n",
                     Timestamp::Now().ToFormattedString().c_str(),
                     buffers_to_write.size()-2);
            fputs(buf, stderr);
            output.Append(buf, static_cast<int>(strlen(buf)));
            buffers_to_write.erase(buffers_to_write.begin()+2, buffers_to_write.end());
            //FIXME: add over load callback?
        }

        for (size_t i = 0;i < buffers_to_write.size();i++)
        {
            // here, use writev can not make it fast!
            output.Append(buffers_to_write[i].data(), buffers_to_write[i].length());
        }

        if (buffers_to_write.size() > 2)
        {
            // drop non-bzero-ed buffers, avoid trashing
            buffers_to_write.resize(2);
        }

        if (!buffer1)
        {
            assert(!buffers_to_write.empty());
            buffer1 = buffers_to_write.pop_back();
            buffer1->Reset();
        }

        if (!buffer2)
        {
            assert(!buffers_to_write.empty());
            buffer2 = buffers_to_write.pop_back();
            buffer2->Reset();
        }

        buffers_to_write.clear();
        output.Flush();
    }

    output.Flush();
}

} // namespace claire
