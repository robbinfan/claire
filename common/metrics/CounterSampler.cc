// Copyright (c) 2013 The claire-common Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <claire/common/metrics/CounterSampler.h>

#include <gflags/gflags.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/prettywriter.h>

#include <boost/bind.hpp>

#include <claire/common/metrics/CounterProvider.h>

DEFINE_int32(sample_duration, 60, "sample stat duration, unit is minute, default is 60");
DEFINE_int32(sample_rate, 1, "sample rate, unit is second, default is 1 second");

namespace claire {

CounterSampler::CounterSampler(EventLoop* loop)
    : loop_(loop),
      samples_(FLAGS_sample_duration*60/FLAGS_sample_rate)
{
    loop_->RunEvery(FLAGS_sample_rate*1000,
                    boost::bind(&CounterSampler::DoSample, this));
}

void CounterSampler::WriteJson(std::string* output)
{
    rapidjson::Document doc;
    SampleEntry entry;
    {
        MutexLock lock(mutex_);
        if (samples_.empty())
        {
            return ;
        }
        entry = samples_.rbegin()->second;
    }

    doc.SetArray();
    for (auto it = entry.cbegin(); it != entry.cend(); ++it)
    {
        rapidjson::Value value;
        value.SetString(it->first.c_str(), doc.GetAllocator());
        doc.PushBack(value, doc.GetAllocator());
    }

    rapidjson::StringBuffer buffer;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);
    output->assign(buffer.GetString());
}

void CounterSampler::WriteJson(std::string* output,
                               const std::vector<std::string>& metrics,
                               int64_t since)
{
    rapidjson::Document doc;
    doc.SetObject();

    rapidjson::Value names(rapidjson::kArrayType);
    names.PushBack("time", doc.GetAllocator());
    for (auto it = metrics.begin(); it != metrics.end(); ++it)
    {
        rapidjson::Value value((*it).c_str(),
                               static_cast<uint32_t>((*it).length()),
                               doc.GetAllocator());
        names.PushBack(value, doc.GetAllocator());
    }
    doc.AddMember("names", names, doc.GetAllocator());

    boost::circular_buffer<Sample> samples;
    {
        MutexLock lock(mutex_);
        samples = samples_;
    }

    rapidjson::Value data(rapidjson::kArrayType);
    for (auto sample_iterator = samples.begin(); sample_iterator != samples.end(); ++sample_iterator)
    {
        auto& sample = *sample_iterator;
        if (1000*sample.first <= since)
        {
            continue;
        }

        rapidjson::Value item(rapidjson::kArrayType);
        item.PushBack(sample.first*1000, doc.GetAllocator());

        auto entry = sample.second;
        for (auto metric_iterator = metrics.begin(); metric_iterator != metrics.end(); ++metric_iterator)
        {
            auto it = entry.find(*metric_iterator);
            if (it != entry.end()) // FIXME
            {
                item.PushBack(it->second, doc.GetAllocator());
            }
            else
            {
                item.PushBack(0, doc.GetAllocator());
            }
        }
        data.PushBack(item, doc.GetAllocator());
    }
    doc.AddMember("data", data, doc.GetAllocator());

    rapidjson::StringBuffer buffer;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);
    output->assign(buffer.GetString());
}

void CounterSampler::DoSample()
{
    SampleEntry entry;
    auto snapshot = CounterProvider::instance()->GetSnapshot();
    if (!last_snapshot_.empty())
    {
        entry = DoSnapshotDiff(snapshot, last_snapshot_);
    }
    last_snapshot_.swap(snapshot);

    if (!entry.empty())
    {
        MutexLock lock(mutex_);
        samples_.push_back({Timestamp::Now().SecondsSinceEpoch(), std::move(entry)});
    }
}

CounterSampler::SampleEntry CounterSampler::DoSnapshotDiff(const Snapshot& lhs, const Snapshot& rhs)
{
    SampleEntry entry;
    for (auto it = lhs.begin(); it != lhs.end(); ++it)
    {
        auto pos = rhs.find(it->first);
        if (pos == rhs.end())
        {
            continue;
        }

        if (it->second < pos->second)
        {
            entry.insert({it->first, INT_MAX - pos->second - it->second}); // FIXME: overflow?
        }
        else
        {
            entry.insert({it->first, it->second - pos->second});
        }
    }
    return entry;
}

} // namespace claire
