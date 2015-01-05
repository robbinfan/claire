// Copyright (c) 2013 The claire-common Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef _CLAIRE_COMMON_LOGGING_LOGGER_H_
#define _CLAIRE_COMMON_LOGGING_LOGGER_H_

#include <boost/atomic.hpp>
#include <boost/noncopyable.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

#include <claire/common/threading/Thread.h>
#include <claire/common/threading/Mutex.h>
#include <claire/common/threading/Condition.h>
#include <claire/common/threading/CountDownLatch.h>
#include <claire/common/logging/LogBuffer.h>

namespace claire {

/// A Logger is the interface used by logging modules to emit entries
/// to a log.  A typical implementation will dump formatted data to a
/// sequence of files.  We also provide interfaces that will forward
/// the data to another thread so that the invoker never blocks.
/// Implementations is thread-safe since the logging system
/// may write to them from multiple threads.
class Logger : boost::noncopyable
{
public:
    Logger();

    ~Logger()
    {
        Stop();
    }

    // Writes "message[0,message_len-1]" corresponding to an event that
    // occurred at "timestamp".
    //
    // The input message has already been formatted as deemed
    // appropriate by the higher level logging facility.  For example,
    // textual log messages already contain timestamps, and the
    // file:linenumber header.
    void Append(const char* data, size_t length);

    void Start()
    {
        if (!running_)
        {
            running_ = true;
            thread_.Start();
            latch_.Wait();
        }
    }

    void Stop()
    {
        if (running_)
        {
            running_ = false;
            condition_.Notify();
            thread_.Join();
        }
    }

private:
    void ThreadMain();

    // for compiler
    Logger(const Logger&) = delete;
    void operator=(const Logger&) = delete;

    typedef LogBuffer<kLargeBuffer> Buffer;
    typedef boost::ptr_vector<Buffer> BufferVector;
    typedef BufferVector::auto_type BufferPtr;

    boost::atomic<bool> running_;

    Thread thread_;
    Mutex mutex_;
    Condition condition_;
    CountDownLatch latch_;

    BufferPtr current_buffer_;
    BufferPtr next_buffer_;
    BufferVector buffers_;
};

} // namespace claire

#endif // _CLAIRE_COMMON_LOGGIN_LOGGER_H_
