// Copyright (c) 2013 The claire-common Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include <claire/common/events/EventLoop.h>

#include <signal.h>
#include <sys/eventfd.h>

#include <algorithm>

#include <boost/bind.hpp>

#include <claire/common/events/Poller.h>
#include <claire/common/events/Channel.h>
#include <claire/common/events/poller/EPollPoller.h>
#include <claire/common/events/TimeoutQueue.h>
#include <claire/common/logging/Logging.h>
#include <claire/common/threading/ThisThread.h>

namespace claire {

namespace {

__thread EventLoop * tLoopInThisThread = NULL;
const int64_t kPollTimeMs = 10;

struct Cmp
{
    bool operator()(const Channel* lhs, const Channel* rhs)
    {
        return static_cast<int>(lhs->priority()) > static_cast<int>(rhs->priority());
    }
};

int CreateEventfd()
{
    int fd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (fd < 0)
    {
        PLOG(FATAL) << "Failed in eventfd";
    }

    return fd;
}

#pragma GCC diagnostic ignored "-Wold-style-cast"
class IgnoreSigPipe
{
public:
    IgnoreSigPipe()
    {
        ::signal(SIGPIPE, SIG_IGN);
        LOG(TRACE) << "Ignore SIGPIPE";
    }
};
#pragma GCC diagnostic error "-Wold-style-cast"

IgnoreSigPipe ignore_sigpipe;

} // namespace

EventLoop::EventLoop()
    : looping_(false),
      quit_(false),
      tid_(ThisThread::tid()),
      poller_(new EPollPoller(this)), // FIXME
      timeouts_(new TimeoutQueue(this)), // since 2.6.28
      wakeup_fd_(CreateEventfd()),
      wakeup_channel_(new Channel(this, wakeup_fd_)),
      current_active_channel_(NULL)
{
    LOG(DEBUG) << "EventLoop create " << this << "in thread " << tid_;
    if (tLoopInThisThread)
    {
        LOG(FATAL) << "Another EventLoop " << tLoopInThisThread
                   << "already exist in this thread" << tid_;
    }
    else
    {
        tLoopInThisThread = this;
    }

    wakeup_channel_->set_read_callback(
        boost::bind(&EventLoop::OnWakeup, this));
    wakeup_channel_->EnableReading();
}

EventLoop::~EventLoop()
{
    LOG(DEBUG) << "EventLoop " << this << " of thread " << tid_
               << " destructs in thread " << ThisThread::tid();
    ::close(wakeup_fd_);

    DCHECK(tLoopInThisThread == this);
    tLoopInThisThread = NULL;
}

void EventLoop::loop()
{
    DCHECK(!looping_);
    AssertInLoopThread();

    looping_ = true;
    LOG(INFO) << "EventLoop " << this << " start looping";

    while(!quit_)
    {
        active_channels_.clear();
        poller_->poll(kPollTimeMs, &active_channels_);

        Cmp cmp;
        sort(active_channels_.begin(), active_channels_.end(), cmp); // FIXME

        for (auto it = active_channels_.begin(); it != active_channels_.end(); ++it)
        {
            current_active_channel_ = *it;
            (*it)->OnEvent();
        }
        current_active_channel_ = NULL;

        RunPendingTasks();
    }

    LOG(INFO) << "EventLoop " << this << " stop looping";
    looping_ = false;
}

void EventLoop::quit()
{
    quit_ = true; // FIXME
    if (!IsInLoopThread())
    {
        Wakeup();
    }
}

void EventLoop::Run(const Task& task)
{
    if (IsInLoopThread())
    {
        task();
    }
    else
    {
        Post(task);
    }
}

void EventLoop::Post(const Task& task)
{
    {
        MutexLock lock(mutex_);
        pending_tasks_.push_back(std::make_pair(ThisThread::GetTraceContext(), task));
    }

    if (!IsInLoopThread() || !current_active_channel_) // FIXME
    {
        Wakeup();
    }
}

void EventLoop::Run(Task&& task)
{
    if (IsInLoopThread())
    {
        task();
    }
    else
    {
        Post(std::move(task));
    }
}

void EventLoop::Post(Task&& task)
{
    {
        MutexLock lock(mutex_);
        pending_tasks_.push_back(std::make_pair(ThisThread::GetTraceContext(), std::move(task))); // emplace_back
    }

    if (!IsInLoopThread() || !current_active_channel_) // FIXME
    {
        Wakeup();
    }
}

TimerId EventLoop::RunAt(const Timestamp& time, const TimeoutCallback& callback)
{
    return timeouts_->Add(time.MicroSecondsSinceEpoch(), callback);
}

TimerId EventLoop::RunAfter(int delay_in_milliseconds, const TimeoutCallback& callback)
{
    return RunAt(AddTime(Timestamp::Now(), delay_in_milliseconds*1000), callback);
}

TimerId EventLoop::RunEvery(int interval_in_milliseconds, const TimeoutCallback& callback)
{
    auto now = Timestamp::Now().MicroSecondsSinceEpoch();
    return timeouts_->AddRepeating(now, interval_in_milliseconds*1000, callback);
}

void EventLoop::Cancel(TimerId id)
{
    timeouts_->Cancel(id.get());
}

void EventLoop::UpdateChannel(Channel* channel)
{
    DCHECK(channel->OwnerLoop() == this);
    AssertInLoopThread();
    poller_->UpdateChannel(channel);
}

void EventLoop::RemoveChannel(Channel* channel)
{
    DCHECK(channel->OwnerLoop() == this);
    AssertInLoopThread();

    if (current_active_channel_)
    {
        DCHECK(current_active_channel_ == channel ||
            std::find(active_channels_.begin(), active_channels_.end(), channel) == active_channels_.end())
            << "Can not remove when processing channel";
    }
    poller_->RemoveChannel(channel);
}

void EventLoop::AbortNotInLoopThread() const
{
    LOG(FATAL) << "EventLoop::AbortNotInLoopThread - EventLoop " << this
               << " was created in thread " << tid_
               << ", current thread id is " <<  ThisThread::tid();
}

void EventLoop::Wakeup()
{
    uint64_t one = 1;
    ssize_t n =  ::write(wakeup_fd_, &one, sizeof(one));
    if (n != sizeof(one))
    {
        LOG(ERROR) << "EventLoop::Wakeup writes " << n << " bytes instead of 8";
    }
}

void EventLoop::OnWakeup()
{
    uint64_t one = 1;
    ssize_t n = ::read(wakeup_fd_, &one, sizeof(one));
    if (n != sizeof(one))
    {
        LOG(ERROR) << "EventLoop::OnWakeup reads " << n << " bytes instead of 8";
    }
}

void EventLoop::RunPendingTasks()
{
    std::vector<Entry> tasks;
    {
        MutexLock lock(mutex_);
        tasks.swap(pending_tasks_);
    }

    for (auto it = tasks.begin(); it != tasks.end(); ++it)
    {
        ThisThread::SetTraceContext((*it).first);
        (*it).second();
        ThisThread::ResetTraceContext();
    }
}

bool EventLoop::IsInLoopThread() const
{
    return tid_ == ThisThread::tid();
}

void EventLoop::AssertInLoopThread() const
{
    assert(IsInLoopThread());
}

EventLoop* EventLoop::CurrentLoopInThisThread()
{
    return tLoopInThisThread;
}

} // namespace claire
