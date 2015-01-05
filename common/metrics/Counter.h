// Copyright (c) 2013 The claire-common Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef _CLAIRE_COMMON_METRICS_COUNTER_H_
#define _CLAIRE_COMMON_METRICS_COUNTER_H_

#include <string>

#include <boost/noncopyable.hpp>

namespace claire {

// Counters are dynamically created values which can be tracked in
// the CounterProvider.  They are designed to be lightweight to create and
// easy to use.
//
// Since Counters can be created dynamically by name, there is
// a hash table lookup to find the counter in the table.  A Counter
// object can be created once and used across multiple threads safely.
//
// Example usage:
//    {
//        Counter request_count("RequestCount");
//        request_count.Increment();
//    }
//
// Note that creating counters on the stack does work, however creating
// the counter object requires a hash table lookup.  For inner loops, it
// may be better to create the counter either as a member of another object
// (or otherwise outside of the loop) for maximum performance.
//
// Internally, a counter represents a value in a row of a CounterProvider.
// The row has a 32bit value for each thread in the table and also
// a name (stored in the table metadata).
//
// NOTE: In order to make counters usable in lots of different code,
// avoid any dependencies inside this header file.
//

//------------------------------------------------------------------------------
// Define macros for ease of use. They also allow us to change definitions
// as the implementation varies, or depending on compile options.
//------------------------------------------------------------------------------
// First provide generic macros, which exist in production as well as debug.
#define DEFINE_COUNTER(name, delta) do { \
    ::claire::StatsCounter counter(name); \
    counter.Add(delta); \
} while (0)

#define DEFINE_SIMPLE_COUNTER(name) DEFINE_COUNTER(name, 1)

class Counter : boost::noncopyable
{
public:
    explicit Counter(const std::string& name)
        : name_(name),
          counter_id_(-1)
    {}
    virtual ~Counter() {}

    // Sets the counter to a specific value.
    void Set(int value);

    // Increments the counter.
    void Increment()
    {
        Add(1);
    }

    virtual void Add(int value);

    // Decrements the counter.
    void Decrement()
    {
        Add(-1);
    }

    void Subtract(int value)
    {
        Add(-value);
    }

    // Is this counter valid?
    bool Valid()
    {
        return GetPtr() != NULL;
    }

    int get()
    {
        auto loc = GetPtr();
        if (!loc) return 0;
        return *loc;
    }

protected:
    Counter();

    // Returns the cached address of this counter location.
    int* GetPtr();

    std::string name_;
    // The counter id in the table.  We initialize to -1 (an invalid value)
    // and then cache it once it has been looked up.
    int32_t counter_id_;
};

} // namespace claire

#endif // _CLAIRE_COMMON_METRICS_COUNTER_H_
