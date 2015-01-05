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

#include <claire/common/logging/LogMessage.h>

#include <time.h>
#include <stdio.h> // strerror_r

#include <claire/common/logging/Logging.h> // for CheckOpString
#include <claire/common/threading/ThisThread.h>

namespace claire {
namespace {

enum class LogColor : char
{
    COLOR_DEFAULT,
    COLOR_RED,
    COLOR_GREEN,
    COLOR_YELLOW
};

LogColor SeverityToColor(LogSeverity severity)
{
    LogColor color = LogColor::COLOR_DEFAULT;
    switch (severity)
    {
        case LogSeverity::TRACE:
        case LogSeverity::DEBUG:
        case LogSeverity::INFO:
            color = LogColor::COLOR_DEFAULT;
            break;
        case LogSeverity::WARNING:
            color = LogColor::COLOR_YELLOW;
            break;
        case LogSeverity::ERROR:
        case LogSeverity::FATAL:
            color = LogColor::COLOR_RED;
            break;
        default:
            // should never get here.
            assert(false);
    }
    return color;
}

bool TerminalSupportsColor()
{
    bool term_supports_color = false;
    const char* const term = getenv("TERM");
    if (term != NULL && term[0] != '\0')
    {
        term_supports_color =
            !strcmp(term, "xterm") ||
            !strcmp(term, "xterm-color") ||
            !strcmp(term, "xterm-256color") ||
            !strcmp(term, "screen") ||
            !strcmp(term, "linux") ||
            !strcmp(term, "cygwin");
    }

    return term_supports_color;
}

// Returns the ANSI color code for the given color.
const char* GetAnsiColorCode(LogColor color)
{
    switch (color)
    {
        case LogColor::COLOR_RED:     return "1";
        case LogColor::COLOR_GREEN:   return "2";
        case LogColor::COLOR_YELLOW:  return "3";
        case LogColor::COLOR_DEFAULT: return "";
    };
    return NULL; // stop warning about return type.
}

void ColoredWriteToStderr(LogSeverity severity,
                          const char* message, size_t len)
{
    const auto color = (TerminalSupportsColor() && FLAGS_colorlogtostderr) ?
        SeverityToColor(severity) : LogColor::COLOR_DEFAULT;

    // Avoid using cerr from this module since we may get called during
    // exit code, and cerr may be partially or fully destroyed by then.
    if (LogColor::COLOR_DEFAULT == color)
    {
        fwrite(message, len, 1, stderr);
        return;
    }

    fprintf(stderr, "\033[0;3%sm", GetAnsiColorCode(color));
    fwrite(message, len, 1, stderr);
    fprintf(stderr, "\033[m");  // Resets the terminal to default.
}

__thread char t_time[32];
__thread time_t t_last_second;
__thread char t_errno_buffer[512];

void DefaultOutput(const char* msg, size_t len)
{
    size_t n = ::fwrite(msg, 1, len, stdout);
    (void)n;
}

void DefaultFlush()
{
    ::fflush(stdout);
}

} // namespace

const char* LogSeverityName[] =
{
    "TRACE ",
    "DEBUG ",
    "INFO ",
    "WARNING ",
    "ERROR ",
    "FATAL ",
};

const char * strerror_tl(int saved_errno)
{
    return ::strerror_r(saved_errno, t_errno_buffer, sizeof t_errno_buffer);
}

inline LogStream& operator<<(LogStream& s, const LogMessage::SourceFile& v)
{
    s.Append(v.data_, v.size_);
    return s;
}

LogMessage::OutputFunc g_output = DefaultOutput;
LogMessage::FlushFunc g_flush = DefaultFlush;

LogMessage::Impl::Impl(const SourceFile& file,
                       int line,
                       LogSeverity severity)
    : time_(Timestamp::Now()),
      stream_(),
      severity_(severity)
{
    FormatTime();
    ThisThread::tid();
    stream_ << StringPiece(ThisThread::tid_string(), 6);
    stream_ << StringPiece(LogSeverityName[static_cast<int>(severity)], 5);
    stream_ << "- " << file << ':' << line << " : ";
}

void LogMessage::Impl::FormatTime()
{
    int64_t microseconds_since_epoch = time_.MicroSecondsSinceEpoch();
    time_t seconds = time_.SecondsSinceEpoch();
    int microseconds = static_cast<int>(microseconds_since_epoch % 1000000);
    if (seconds != t_last_second)
    {
        t_last_second = seconds;
        struct tm tm_time;
        ::gmtime_r(&seconds, &tm_time);

        int len = snprintf(t_time, sizeof(t_time), "%4d%02d%02d %02d:%02d:%02d",
            tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
            tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
        assert(len == 17);
        (void)len;
    }

    char buf[32];
    snprintf(buf, sizeof buf, ".%06dZ ", microseconds);

    stream_ << StringPiece(t_time, 17) << buf;
}

void LogMessage::Impl::Finish()
{
    stream_ << '\n';
}

LogMessage::LogMessage(SourceFile file,
                       int line,
                       LogSeverity severity,
                       int ctr)
    : impl_(file, line, severity)
{
    stream().set_ctr(ctr);
}

LogMessage::LogMessage(SourceFile file, int line, LogSeverity severity, const char* func)
    : impl_(file, line, severity)
{
    impl_.stream_ << func << ' ';
}

LogMessage::LogMessage(SourceFile file, int line)
    : impl_(file, line, LogSeverity::INFO) {}

LogMessage::LogMessage(SourceFile file, int line, LogSeverity severity)
    : impl_(file, line, severity) {}

LogMessage::LogMessage(SourceFile file, int line, const CheckOpString& result)
    : impl_(file, line, LogSeverity::FATAL)
{
    stream() << "Check failed: " << *result.str_ << " ";
}

LogMessage::~LogMessage()
{
    impl_.Finish();

    const auto& buf(stream().buffer());

    if (FLAGS_alsologtostderr)
    {
        ColoredWriteToStderr(impl_.severity_, buf.data(), buf.length());
    }

    g_output(buf.data(), buf.length());

    if (impl_.severity_ == LogSeverity::FATAL)
    {
        abort();
    }
}

void LogMessage::SetOutput(OutputFunc out)
{
    g_output = out;
}

void LogMessage::SetFlush(FlushFunc flush)
{
    g_flush = flush;
}

ErrnoLogMessage::~ErrnoLogMessage()
{
    message_.stream() << " : " << strerror_tl(preserved_errno_) << " [" << preserved_errno_ << "] ";
}

} // namespace claire
