// Copyright (c) 2013 The claire-common Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef _CLAIRE_COMMON_METRICS_COUNTERSAMPLER_H_
#define _CLAIRE_COMMON_METRICS_COUNTERSAMPLER_H_

#include <time.h>
#include <stdint.h>

#include <vector>
#include <string>
#include <unordered_map>

#include <boost/noncopyable.hpp>
#include <boost/circular_buffer.hpp>

#include <claire/common/threading/Mutex.h>
#include <claire/common/events/EventLoop.h>

namespace claire {

class CounterSampler : boost::noncopyable
{
public:
    CounterSampler(EventLoop* loop);
    void WriteJson(std::string* output);
    void WriteJson(std::string* output,
                   const std::vector<std::string>& metrics,
                   int64_t since);
private:
    typedef std::unordered_map<std::string, int> SampleEntry;
    typedef std::pair<time_t, SampleEntry> Sample;
    typedef std::unordered_map<std::string, int> Snapshot;

    void DoSample();
    SampleEntry DoSnapshotDiff(const Snapshot& lhs, const Snapshot& rhs);

    EventLoop* loop_;
    Mutex mutex_;
    boost::circular_buffer<Sample> samples_; // @GUARDBY mutex_
    Snapshot last_snapshot_;
};

} // namespace claire

#endif // _CLAIRE_COMMON_METRICS_COUNTERSAMPLER_H_
