// Copyright (c) 2013 The claire-common Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include <claire/common/time/Timestamp.h>

#include <stdio.h>
#include <sys/time.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#undef __STDC_FORMAT_MACROS

namespace claire {

static_assert(sizeof(Timestamp) == sizeof(int64_t), "Timestamp size not equal int64_t");

std::string Timestamp::ToString() const
{
    auto seconds = microseconds_since_epoch_ / kMicroSecondsPerSecond;
    auto microseconds = microseconds_since_epoch_ % kMicroSecondsPerSecond;

    char time[32];
    snprintf(time, sizeof(time)-1, "%" PRId64 ".%06" PRId64 "", seconds, microseconds);
    return time;
}

std::string Timestamp::ToFormattedString() const
{
    auto seconds = static_cast<time_t>(microseconds_since_epoch_ / kMicroSecondsPerSecond);
    auto microseconds = static_cast<int>(microseconds_since_epoch_ % kMicroSecondsPerSecond);
    struct tm tm_time;
    ::gmtime_r(&seconds, &tm_time);

    char time[32];
    snprintf(time, sizeof(time), "%4d%02d%02d %02d:%02d:%02d.%06d",
        tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
        tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec,
        microseconds);
    return time;
}

Timestamp Timestamp::Now()
{
    struct timeval tv;
    ::gettimeofday(&tv, NULL);
    return Timestamp(tv.tv_sec * kMicroSecondsPerSecond + tv.tv_usec);
}

} // namespace claire
