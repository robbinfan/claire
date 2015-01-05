// Copyright (c) 2013 The claire-common Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef _CLAIRE_COMMON_METRICS_COUNTERPROVIDER_H_
#define _CLAIRE_COMMON_METRICS_COUNTERPROVIDER_H_

#include <string>
#include <unordered_map>

#include <boost/scoped_ptr.hpp>
#include <boost/noncopyable.hpp>

#include <claire/common/threading/Singleton.h>

namespace claire {

class CounterProvider : boost::noncopyable
{
public:
    // For convenience, we create a singleton provider.  This is generally
    // used automatically by the counters.
    static CounterProvider* instance();

    // Find a counter in the CounterProvider
    //
    // Returns an id for the counter which can be used to call GetLocation().
    // If the counter does not exist, attempts to create a row for the new
    // counter.  If there is no space in the provider for the new counter,
    // returns 0.
    int GetCounterId(const std::string& name);

    // Gets the location of a particular counter of calling Thread.
    int* GetLocation(int counter_id);

    // Gets the sum of the values for a particular counter.  If the counter
    // does not exist, creates the counter.
    int GetCounterValue(const std::string& name);

    // Convenience function to lookup a counter location for a
    // counter by name for the calling thread.  Will register
    // the thread if it is not already registered.
    static int* GetLocation(const std::string& name);

    std::unordered_map<std::string, int> GetSnapshot();

private:
    CounterProvider();
    ~CounterProvider();

    friend class Singleton<CounterProvider>;

    class Impl;
    boost::scoped_ptr<Impl> impl_;
};

} // namespace claire

#endif // _CLAIRE_COMMON_METRICS_COUNTERPROVIDER_H_
