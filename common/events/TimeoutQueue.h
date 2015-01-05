// Copyright (c) 2013 The claire-common Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

//
// Copyright 2013 Facebook, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

//
// Simple timeout queue.  Call user-specified callbacks when their timeouts
// expire.
//
// This class assumes that "time" is an int64_t and doesn't care about time
// units (seconds, milliseconds, etc).  You call Run() using
// the same time units that you use to specify callbacks.
//
// @author Tudor Bosman (tudorb@fb.com)
//

#ifndef _CLAIRE_COMMON_EVENTS_TIMEOUTQUEUE_H_
#define _CLAIRE_COMMON_EVENTS_TIMEOUTQUEUE_H_

#include <stdint.h>

#include <boost/atomic.hpp>
#include <boost/function.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/noncopyable.hpp>

#include <boost/multi_index/member.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/indexed_by.hpp>
#include <boost/multi_index/ordered_index.hpp>

namespace claire {

class EventLoop;
class Channel;

class TimeoutQueue : boost::noncopyable
{
public:
    typedef int64_t Id;
    typedef boost::function<void()> Callback;

    TimeoutQueue(EventLoop* loop);
    ~TimeoutQueue();

    ///
    /// Add a one-time timeout event that will fire "expire" time
    /// (that is, the first time that Run() is called with a time value >= expiration)
    ///
    Id Add(int64_t expiration, const Callback& callback);
    Id Add(int64_t expiration, Callback&& callback);

    ///
    /// Add a repeating timeout event that will fire every "interval" time units
    /// (it will first fire when Run() is called with a time value >=
    /// now + interval).
    ///
    Id AddRepeating(int64_t now, int64_t interval, const Callback& callback);
    Id AddRepeating(int64_t now, int64_t interval, Callback&& callback);

    ///
    /// Erase a given timeout event.
    ///
    void Cancel(Id id);

private:
    struct Event
    {
        Id id;
        int64_t expiration;
        int64_t repeat_interval;
        Callback callback;
    };

    typedef boost::multi_index_container<
        Event,
        boost::multi_index::indexed_by<
            boost::multi_index::ordered_unique<boost::multi_index::member<
                Event, Id, &Event::id
            >>,
            boost::multi_index::ordered_non_unique<boost::multi_index::member<
                Event, int64_t, &Event::expiration
            >>
        >
    > Set;

    enum
    {
        kById = 0,
        kByExpiration = 1
    };

    //
    // Process all events that are due at times <= "now" by calling their
    // callbacks.
    //
    // Callbacks are allowed to call back into the queue and add / erase events;
    // they might create more events that are already due.  In this case,
    // Run will only go through the queue once, and return a "next
    // expiration" time in the past or present (<= now).
    //
    // Return the time that the next event will be due (same as
    // NextExpiration(), below)
    //
    int64_t Run(int64_t now);

    //
    // Return the time that the next event will be due.
    //
    int64_t NextExpiration() const;

    void AddInLoop(Event& event);
    void CancelInLoop(Id id);
    void OnTimer();

    EventLoop* loop_;
    int timer_fd_;
    boost::scoped_ptr<Channel> timer_channel_;

    Set timeouts_;
    boost::atomic<Id> next_;
};

} // namespace claire

#endif // _CLAIRE_COMMON_EVENTS_TIMEOUTQUEUE_H_
