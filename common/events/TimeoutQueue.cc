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

#define __STDC_LIMIT_MACROS
#include <claire/common/events/TimeoutQueue.h>

#include <sys/timerfd.h>

#include <vector>
#include <algorithm>

#include <boost/bind.hpp>

#include <claire/common/logging/Logging.h>
#include <claire/common/events/EventLoop.h>
#include <claire/common/events/Channel.h>

namespace claire {

namespace {

int CreateTimerfd()
{
    int fd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if (fd < 0)
    {
        PLOG(FATAL) << "Failed in timerfd_create ";
    }

    return fd;
}

struct timespec HowMuchTimeFromNow(int64_t when, int64_t now)
{
    auto delta = when - now;
    if (delta < 100)
    {
        delta = 100;
    }

    struct timespec ts;
    ts.tv_sec = static_cast<time_t>(delta / 1000000);
    ts.tv_nsec = static_cast<long>((delta % Timestamp::kMicroSecondsPerSecond) * 1000); // FIXME
    return ts;
}

void ReadTimerfd(int fd)
{
    uint64_t howmany;
    ssize_t n = ::read(fd, &howmany, sizeof howmany);
    if (n != sizeof howmany)
    {
        PLOG(ERROR) << "ResetTimerfd reads " << n << " bytes instead of 8";
    }
}

void ResetTimerfd(int fd, int64_t expiration)
{
    // wake up loop by timerfd_settime()
    struct itimerspec spec_new;
    struct itimerspec spec_old;
    ::bzero(&spec_new, sizeof spec_new);
    ::bzero(&spec_old, sizeof spec_old);

    spec_new.it_value = HowMuchTimeFromNow(expiration,
                                           Timestamp::Now().MicroSecondsSinceEpoch());
    int ret = ::timerfd_settime(fd, 0, &spec_new, &spec_old);
    if (ret)
    {
        PLOG(ERROR) << "timerfd_settime failed";
    }
}

} // namespace

TimeoutQueue::TimeoutQueue(EventLoop* loop)
    : loop_(loop),
      timer_fd_(CreateTimerfd()),
      timer_channel_(new Channel(loop, timer_fd_)),
      next_(1)
{
    timer_channel_->set_read_callback(
        boost::bind(&TimeoutQueue::OnTimer, this));
    timer_channel_->EnableReading();
    timer_channel_->set_priority(Channel::Priority::kHigh); // FIXME
}

TimeoutQueue::~TimeoutQueue()
{
    ::close(timer_fd_);
}

void TimeoutQueue::OnTimer()
{
    ReadTimerfd(timer_fd_);
    ResetTimerfd(timer_fd_, Run(Timestamp::Now().MicroSecondsSinceEpoch()));
}

TimeoutQueue::Id TimeoutQueue::Add(int64_t expiration, const Callback& callback)
{
    auto id = next_++;
    Event event({id, expiration, -1, callback});
    loop_->Run(boost::bind(&TimeoutQueue::AddInLoop, this, event));
    return id;
}

TimeoutQueue::Id TimeoutQueue::Add(int64_t expiration, Callback&& callback)
{
    auto id = next_++;
    Event event({id, expiration, -1, std::move(callback)});
    loop_->Run(boost::bind(&TimeoutQueue::AddInLoop, this, event));
    return id;
}

TimeoutQueue::Id TimeoutQueue::AddRepeating(int64_t now, int64_t interval, const Callback& callback)
{
    auto id = next_++;
    Event event({id, now + interval, interval, callback});
    loop_->Run(boost::bind(&TimeoutQueue::AddInLoop, this, event));
    return id;
}

TimeoutQueue::Id TimeoutQueue::AddRepeating(int64_t now, int64_t interval, Callback&& callback)
{
    auto id = next_++;
    Event event({id, now + interval, interval, std::move(callback)});
    loop_->Run(boost::bind(&TimeoutQueue::AddInLoop, this, event));
    return id;
}

void TimeoutQueue::AddInLoop(Event& event)
{
    loop_->AssertInLoopThread();
    bool reset = event.expiration < NextExpiration();
    timeouts_.insert(std::move(event));
    if (reset)
    {
        ResetTimerfd(timer_fd_, NextExpiration());
    }
    return ;
}

void TimeoutQueue::Cancel(Id id)
{
    loop_->Run(boost::bind(&TimeoutQueue::CancelInLoop, this, id));
}

void TimeoutQueue::CancelInLoop(Id id)
{
    loop_->AssertInLoopThread();
    timeouts_.get<kById>().erase(id);
}

int64_t TimeoutQueue::NextExpiration() const
{
    return (timeouts_.empty() ? std::numeric_limits<int64_t>::max() :
        timeouts_.get<kByExpiration>().begin()->expiration);
}

int64_t TimeoutQueue::Run(int64_t now)
{
    std::vector<Event> expired;
    auto& events = timeouts_.get<kByExpiration>();
    auto end = events.upper_bound(now);
    std::move(events.begin(), end, std::back_inserter(expired));
    events.erase(events.begin(), end);

    for (auto it = expired.begin(); it != expired.end(); ++it)
    {
        auto& event = *it;
        // Reinsert if repeating, do this before executing callbacks
        // so the callbacks have a chance to call erase
        if (event.repeat_interval >= 0)
        {
            event.expiration = now + event.repeat_interval; // FIXME
            timeouts_.insert(event);
        }
    }

    // if delete expire timer in callback, no realy action
    // if delete repeat in callback, it is safe
    for (auto it = expired.begin(); it != expired.end(); ++it)
    {
        (*it).callback();
    }
    return NextExpiration();
}

} // namespace claire
