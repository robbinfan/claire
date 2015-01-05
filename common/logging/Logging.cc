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

#include <claire/common/logging/Logging.h>

#include <stdlib.h>

#include <claire/common/logging/Logger.h>

DEFINE_bool(alsologtostderr, false,
            "log messages go to stderr in addition to logfiles");

DEFINE_bool(colorlogtostderr, false,
            "color messages logged to stderr (if supported by terminal)");

DEFINE_int32(stderrthreshold,
             2, // INFO
             "log messages at or above this level are copied to stderr in "
             "addition to logfiles.  This flag obsoletes --alsologtostderr.");

DEFINE_int32(minloglevel, 0, "Messages logged at a lower level than this don't "
             "actually get logged anywhere");

DEFINE_int32(logbufsecs, 3,
             "Buffer log messages for at most this many seconds");

DEFINE_string(log_dir, "",
              "If specified, logfiles are written into this directory instead "
              "of the default logging directory.");

DEFINE_int32(max_log_size, 1800,
             "approx. maximum log file size (in MB). A value of 0 will "
             "be silently overridden to 1.");

DEFINE_bool(stop_logging_if_full_disk, false,
            "Stop attempting to log to disk if the disk is full.");

namespace claire {
namespace detail {

// Helper functions for string comparisons.
#define DEFINE_CHECK_STROP_IMPL(name, func, expected)                               \
    std::string* Check##func##expected##Impl(const char* s1, const char* s2,        \
                                             const char* names)                     \
    {                                                                               \
        bool equal = s1 == s2 || (s1 && s2 && !func(s1, s2));                       \
        if (equal == expected) return NULL;                                         \
        else                                                                        \
        {                                                                           \
            std::ostringstream ss;                                                  \
            if (!s1) s1 = "";                                                       \
            if (!s2) s2 = "";                                                       \
            ss << #name " failed: " << names << " (" << s1 << " vs. " << s2 << ")"; \
            return new std::string(ss.str());                                       \
        }                                                                           \
    }

DEFINE_CHECK_STROP_IMPL(CHECK_STREQ, strcmp, true)
DEFINE_CHECK_STROP_IMPL(CHECK_STRNE, strcmp, false)
DEFINE_CHECK_STROP_IMPL(CHECK_STRCASEEQ, strcasecmp, true)
DEFINE_CHECK_STROP_IMPL(CHECK_STRCASENE, strcasecmp, false)
#undef DEFINE_CHECK_STROP_IMPL

CheckOpMessageBuilder::CheckOpMessageBuilder(const char *exprtext)
    : stream_(new std::ostringstream)
{
    *stream_ << exprtext << " (";
}

CheckOpMessageBuilder::~CheckOpMessageBuilder()
{
    delete stream_;
}

std::ostream* CheckOpMessageBuilder::ForVar2()
{
    *stream_ << " vs. ";
    return stream_;
}

std::string* CheckOpMessageBuilder::NewString()
{
    *stream_ << ")";
    return new std::string(stream_->str());
}

template <>
void MakeCheckOpValueString(std::ostream* os, const char& v)
{
    if (v >= 32 && v <= 126)
    {
        (*os) << "'" << v << "'";
    }
    else
    {
        (*os) << "char value " << static_cast<short>(v);
    }
}

template <>
void MakeCheckOpValueString(std::ostream* os, const signed char& v)
{
    if (v >= 32 && v <= 126)
    {
        (*os) << "'" << v << "'";
    }
    else
    {
        (*os) << "signed char value " << static_cast<short>(v);
    }
}

template <>
void MakeCheckOpValueString(std::ostream* os, const unsigned char& v)
{
    if (v >= 32 && v <= 126)
    {
        (*os) << "'" << v << "'";
    }
    else
    {
        (*os) << "unsigned char value " << static_cast<unsigned short>(v);
    }
}

claire::LogStream& operator<<(claire::LogStream& os, const PRIVATE_Counter&)
{
    return os << os.ctr();
}

} // namespace detail

static Logger *g_logger = NULL;

void OutputLog(const char* msg, size_t len)
{
    g_logger->Append(msg, len);
}

bool IsClaireLoggingInitialized()
{
    return g_logger != NULL;
}

void InitClaireLogging(const char* argv0)
{
    CHECK(!IsClaireLoggingInitialized()) << "You called InitClaireLogging() twice!";

    g_logger = new Logger();
    g_logger->Start();

    LogMessage::SetOutput(OutputLog);
    atexit(ShutdownClaireLogging);
}

void ShutdownClaireLogging()
{
    delete g_logger;
    g_logger = NULL;
}

} // namespace claire
