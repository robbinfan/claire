// Copyright (c) 2013 The claire-common Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <claire/common/metrics/CounterProvider.h>

#include "thirdparty/gflags/gflags.h"

#include <boost/scoped_array.hpp>

#include <claire/common/threading/Mutex.h>
#include <claire/common/threading/ThisThread.h>

DEFINE_int32(provider_max_counters, 100, "max counters in counter provider");
DEFINE_int32(provider_max_threads, 8, "max threads in counter provider");

namespace claire {

CounterProvider* g_current_provider = NULL;
__thread int t_thread_slot = 0;

class CounterProvider::Impl : boost::noncopyable
{
public:
    static_assert(sizeof(int)==4, "int should be 4 bytes");

    Impl(int max_threads,
         int max_counters)
        : max_threads_(max_threads),
          max_counters_(max_counters),
          data_(new int[max_threads*max_counters])
    {}

    int GetCounterId(const std::string& name)
    {
        MutexLock lock(mutex_);
        auto it = counters_.find(name);
        if (it != counters_.end())
        {
            return it->second;
        }

        if (counters_.size() >= static_cast<size_t>(max_counters_))
        {
            return 0;
        }

        auto result = counters_.insert(std::make_pair(name, counters_.size()+1));
        if (result.second)
        {
            return result.first->second;
        }

        return 0;
    }

    int GetSlotId()
    {
        if (t_thread_slot)
        {
            return t_thread_slot;
        }

        {
            MutexLock lock(mutex_);
            if (t_thread_slot)
            {
                return t_thread_slot;
            }

            if (slots_.size() >= static_cast<size_t>(max_threads_))
            {
                return 0;
            }

            auto result = slots_.insert(std::make_pair(ThisThread::tid(),
                                                       static_cast<int>(slots_.size()+1)));
            if (result.second)
            {
                t_thread_slot = static_cast<int>(slots_.size());
                return t_thread_slot;
            }
        }

        return 0;
    }

    int* GetLocation(int counter_id, int slot_id)
    {
        return (data_.get() + (counter_id-1)*max_threads_+slot_id-1);
    }

    int GetCounterValue(int counter_id)
    {
        int value = 0;
        int row = (counter_id-1)*max_threads_;
        for (int i = 0;i < max_threads_;i++)
        {
            value += data_[row+i];
        }
        return value;
    }

    std::unordered_map<std::string, int> GetSnapshot()
    {
        std::unordered_map<std::string, int> snapshot;
        std::unordered_map<std::string, int> counters;
        {
            MutexLock lock(mutex_);
            counters = counters_;
        }

        for (auto it = counters.begin(); it != counters.end(); ++it)
        {
            snapshot.insert(std::make_pair(it->first, GetCounterValue(it->second)));
        }
        return snapshot;
    }

private:
    int max_threads_;
    int max_counters_;

    mutable Mutex mutex_; // FIXME
    std::unordered_map<std::string, int> counters_;
    std::unordered_map<int, int> slots_; // FIXME
    boost::scoped_array<int> data_;
};

CounterProvider::CounterProvider()
    : impl_(new Impl(FLAGS_provider_max_threads, FLAGS_provider_max_counters))
{}

CounterProvider::~CounterProvider() {}

int CounterProvider::GetCounterId(const std::string& name)
{
    return impl_->GetCounterId(name);
}

int* CounterProvider::GetLocation(int counter_id)
{
    int slot_id = impl_->GetSlotId();
    return impl_->GetLocation(counter_id, slot_id);
}

int CounterProvider::GetCounterValue(const std::string& name)
{
    int counter_id = GetCounterId(name);
    if (counter_id == 0)
    {
        return 0;
    }

    return impl_->GetCounterValue(counter_id);
}

CounterProvider* CounterProvider::instance()
{
    return Singleton<CounterProvider>::instance();
}

std::unordered_map<std::string, int> CounterProvider::GetSnapshot()
{
    return impl_->GetSnapshot();
}

} // namespace claire
