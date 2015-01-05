// Copyright (c) 2013 The claire-common Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef _CLAIRE_COMMON_EVENTS_EVENTLOOP_H_
#define _CLAIRE_COMMON_EVENTS_EVENTLOOP_H_

#include <map>
#include <vector>

#include <boost/atomic.hpp>
#include <boost/function.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/noncopyable.hpp>

#include <claire/common/threading/Mutex.h>
#include <claire/common/time/Timestamp.h>
#include <claire/common/events/TimerId.h>
#include <claire/common/tracing/TraceContext.h>

namespace claire {

class Poller;
class Channel;
class TimeoutQueue;

/// Reactor, at most one per thread.
class EventLoop : boost::noncopyable
{
public:
    typedef boost::function<void()> Task;
    typedef boost::function<void()> TimeoutCallback;

    EventLoop();
    ~EventLoop();

    ///
    /// Loops forever.
    ///
    /// Must be called in the same thread as creation of the object.
    ///
    void loop();

    /// Quits loop.
    ///
    /// This is not 100% thread safe, if you call through a raw pointer,
    /// better to call through shared_ptr<EventLoop> for 100% safety.
    void quit();

    /// Runs task immediately in the loop thread.
    /// It wakes up the loop, and run the task.
    /// If in the same loop thread, task is run within the function.
    /// Safe to call from other threads.
    void Run(const Task& task);
    void Run(Task&& task);

    /// Queues callback in the loop thread.
    /// Runs after finish pooling.
    /// Safe to call from other threads.
    void Post(const Task& task);
    void Post(Task&& task);

    ///
    /// Runs callback at 'time'.
    /// Safe to call from other threads.
    ///
    TimerId RunAt(const Timestamp& time,
                  const TimeoutCallback& callback);

    ///
    /// Runs callback after @c delay microseconds
    /// Safe to call from other threads.
    ///
    TimerId RunAfter(int delay, const TimeoutCallback& callback);

    ///
    /// Runs callback every @c interval milliseconds.
    /// Safe to call from other threads.
    ///
    TimerId RunEvery(int interval, const TimeoutCallback& callback);

    // Cancels the timer.
    // Safe to call from other threads.
    void Cancel(TimerId id);

    // for channel
    void UpdateChannel(Channel* channel);
    void RemoveChannel(Channel* channel);

    void AssertInLoopThread() const;
    bool IsInLoopThread() const;

    static EventLoop* CurrentLoopInThisThread();

private:
    typedef std::pair<TraceContext, Task> Entry;

    void AbortNotInLoopThread() const;
    void Wakeup();
    void OnWakeup();
    void OnTimer();
    void RunPendingTasks();

    typedef std::vector<Channel*> ChannelList;

    boost::atomic<bool> looping_;
    boost::atomic<bool> quit_;
    pid_t tid_;

    boost::scoped_ptr<Poller> poller_;
    boost::scoped_ptr<TimeoutQueue> timeouts_;

    int wakeup_fd_;
    boost::scoped_ptr<Channel> wakeup_channel_;

    ChannelList active_channels_;
    Channel* current_active_channel_;

    Mutex mutex_;
    std::vector<Entry> pending_tasks_; // @GUARDBY mutex_
};

} // namespace claire

#endif // _CLAIRE_COMMON_EVENTS_EVENTLOOP_H_
