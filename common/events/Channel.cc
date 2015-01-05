// Copyright (c) 2013 The claire-common Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include <claire/common/events/Channel.h>

#include <poll.h>

#include <claire/common/events/EventLoop.h>
#include <claire/common/logging/Logging.h>

namespace claire {

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = POLLIN | POLLPRI;
const int Channel::kWriteEvent = POLLOUT;

Channel::Channel(EventLoop* loop, int fd__)
    : loop_(loop),
      fd_(fd__),
      events_(0),
      revents_(0),
      context_(-1),
      priority_(Priority::kNormal),
      tied_(false),
      event_handling_(false)
{}

Channel::~Channel() { CHECK(!event_handling_); }

void Channel::tie(const boost::shared_ptr<void>& ptr)
{
    tie_ = ptr;
    tied_ = true;
}

void Channel::Update()
{
    loop_->UpdateChannel(this);
}

void Channel::Remove()
{
    loop_->RemoveChannel(this);
}

void Channel::OnEvent()
{
    if (tied_)
    {
        auto guard = tie_.lock();
        if (guard)
        {
            OnEventWithGuard();
        }
    }
    else
    {
        OnEventWithGuard();
    }
}

void Channel::OnEventWithGuard()
{
    event_handling_ = true;

    if ((revents_ & POLLHUP) && !(revents_ & POLLIN))
    {
        LOG(WARNING) << "Channel::Handle_event() POLLHUP";

        if (close_callback_)
        {
            close_callback_();
        }
    }

    if (revents_ & POLLNVAL)
    {
        LOG(WARNING) << "Channel::OnEvent POLLNVAL";
    }

    if (revents_ & (POLLERR | POLLNVAL))
    {
        if (error_callback_)
        {
            error_callback_();
        }
    }

    if (revents_ & (POLLIN | POLLPRI | POLLRDHUP))
    {
        if (read_callback_)
        {
            read_callback_();
        }
    }

    if (revents_ & POLLOUT)
    {
        if (write_callback_)
        {
            write_callback_();
        }
    }

    event_handling_ = false;
}

} // namespace claire
