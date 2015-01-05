// Copyright (c) 2013 The claire-common Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

// Copyright (c) 1999, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef _CLAIRE_COMMON_LOGGING_LOGMESSAGE_H_
#define _CLAIRE_COMMON_LOGGING_LOGMESSAGE_H_

#include <boost/noncopyable.hpp>

#include <claire/common/time/Timestamp.h>
#include <claire/common/logging/LogStream.h>

namespace claire {

enum class LogSeverity : char
{
    TRACE = 0,
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    FATAL,
    NUM_LOG_SEVERITIES,
};

// Forward Declaire
struct CheckOpString;

class LogMessage : boost::noncopyable
{
public:
    struct SourceFile
    {
        template<int N>
        inline SourceFile(const char (&arr)[N])
            : data_(arr),
              size_(N-1)
        {
            const auto slash = strrchr(data_, '/');
            if (slash)
            {
                data_ = slash + 1;
                size_ -= static_cast<int>(data_ - arr);
            }
        }

        explicit SourceFile(const char* file)
            : data_(file)
        {
            const auto slash = strrchr(file, '/');
            if (slash)
            {
                data_ = slash + 1;
            }
            size_ = static_cast<int>(strlen(data_));
        }

        const char* data_;
        int size_;
    };

    LogMessage(SourceFile file, int line, LogSeverity severity, int ctr);
    LogMessage(SourceFile file, int line, LogSeverity severity, const char* func);

    // Two special constructors that generate reduced amounts of code at
    // LOG call sites for common cases.

    // Used for LOG(INFO): Implied are: severity = INFO, ctr = 0
    LogMessage(SourceFile file, int line);

    // Used for LOG(severity) where severity != INFO.  Implied are: ctr = 0
    LogMessage(SourceFile file, int line, LogSeverity severity);

    // A special constructor used for check failures
    LogMessage(SourceFile file, int line, const CheckOpString& result);

    ~LogMessage();

    LogStream& stream()
    {
        return impl_.stream_;
    }

    typedef void (*OutputFunc)(const char* msg, size_t len);
    typedef void (*FlushFunc)();

    static void SetOutput(OutputFunc);
    static void SetFlush(FlushFunc);

private:
    class Impl
    {
    public:
        Impl(const SourceFile& file, int line, LogSeverity);

        void FormatTime();
        void Finish();

        Timestamp time_;
        LogStream stream_;
        LogSeverity severity_;
    };

    Impl impl_;
};

// This class is used to explicitly ignore values in the conditional
// logging macros.  This avoids compiler warnings like "value computed
// is not used" and "statement has no effect".

class LogMessageVoidify
{
public:
    LogMessageVoidify() {}
    // This has to be an operator with a precedence lower than << but
    // higher than ?:
    void operator&(LogStream&) {}
};

// This class happens to be thread-hostile because all instances share
// a single data buffer, but since it can only be created just before
// the process dies, we don't worry so much.
class LogMessageFatal : boost::noncopyable
{
public:
    LogMessageFatal(const char* file, int line)
        : message_(LogMessage::SourceFile(file), line, LogSeverity::FATAL) {}

    LogMessageFatal(const char* file, int line, const CheckOpString& result)
        : message_(LogMessage::SourceFile(file), line, result) {}

    LogStream& stream() { return message_.stream(); }

private:
    LogMessage message_;
};

class ErrnoLogMessage : boost::noncopyable
{
public:
    ErrnoLogMessage(const char* file, int line, LogSeverity severity, int ctr)
        : message_(LogMessage::SourceFile(file), line, severity, ctr),
          preserved_errno_(errno)
    {}

    // Postpends ": strerror_tl(errno) [errno]".
    ~ErrnoLogMessage();

    LogStream& stream() { return message_.stream(); }

private:
    LogMessage message_;
    int preserved_errno_;
};

const char * strerror_tl(int saved_errno);

} // namespace claire

#endif // _CLAIRE_COMMON_LOGGING_LOGMESSAGE_H_
