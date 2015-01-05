// Copyright (c) 2013 The claire-common Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include <claire/common/events/poller/EPollPoller.h>

#include <poll.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/epoll.h>

#include <boost/bind.hpp>

#include <claire/common/base/Types.h>
#include <claire/common/events/Channel.h>
#include <claire/common/events/EventLoop.h>
#include <claire/common/logging/Logging.h>

namespace claire {

static_assert(EPOLLIN == POLLIN, "pollin not equal epollin");
static_assert(EPOLLPRI == POLLPRI, "pollpri not equal epollpri");
static_assert(EPOLLOUT == POLLOUT, "pollour not equal epollout");
static_assert(EPOLLRDHUP == POLLRDHUP, "pollrdhup not equal epollrdhup");
static_assert(EPOLLERR == POLLERR, "pollerr not equal epollerr");
static_assert(EPOLLHUP == POLLHUP, "pollhup not equal epollhup");

namespace {

const int kNew = -1;
const int kAdded = 1;
const int kDeleted = 2;

} // namespace

EPollPoller::EPollPoller(EventLoop* loop)
    : loop_(loop),
      epoll_fd_(::epoll_create1(EPOLL_CLOEXEC)),
      events_(64), // initial number of events receivable per poll
      channels_(64)
{}

EPollPoller::~EPollPoller() { ::close(epoll_fd_); }

void EPollPoller::poll(int timeout_in_milliseconds, ChannelList* active_channles)
{
    auto num_events = ::epoll_wait(epoll_fd_,
                                   &events_[0],
                                   static_cast<int>(events_.size()),
                                   timeout_in_milliseconds);
    if (num_events < 0)
    {
        // https://lkml.org/lkml/2013/6/25/336
        if (errno != EINTR)
        {
            LOG(FATAL) << "Exception EPollPoller::poll()";
        }
    }

    for (int i = 0; i < num_events; ++i)
    {
        auto fd = static_cast<int>(events_[i].data.u64);

        // check for spurious notification.
        if (fd >= static_cast<int>(channels_.size()))
        {
            LOG(ERROR) << "receive invalid event of fd " << fd;
            continue;
        }

        auto channel = channels_[fd];
        DCHECK(channel->fd() == fd);
        channel->set_revents(events_[i].events);
        active_channles->push_back(channel);
    }

    if (events_.size() == implicit_cast<size_t>(num_events))
    {
        events_.resize(num_events*2);
    }

    return ;
}

void EPollPoller::UpdateChannel(Channel* channel)
{
    loop_->AssertInLoopThread();

    const int state = boost::any_cast<int>(channel->context());
    if (implicit_cast<size_t>(channel->fd()) >= channels_.size())
    {
        if (state == kNew)
        {
            channels_.resize(channels_.size()*2);
        }
        else
        {
            return ;
        }
    }

    if (state == kNew || state == kDeleted)
    {
        if (state == kNew)
        {
            channels_[channel->fd()] = channel;
        }
        else
        {
            DCHECK(channels_[channel->fd()] == channel);
        }

        channel->set_context(kAdded);
        Update(EPOLL_CTL_ADD, channel);
    }
    else if (state == kAdded)
    {
        DCHECK(channels_[channel->fd()] == channel);

        if (channel->IsNoneEvent())
        {
            Update(EPOLL_CTL_DEL, channel);
            channel->set_context(kDeleted);
        }
        else
        {
            Update(EPOLL_CTL_MOD, channel);
        }
    }
}

void EPollPoller::RemoveChannel(Channel* channel)
{
    loop_->AssertInLoopThread();

    DCHECK(implicit_cast<size_t>(channel->fd()) < channels_.size());
    DCHECK(channels_[channel->fd()] == channel);
    DCHECK(channel->IsNoneEvent());

    const int state = boost::any_cast<int>(channel->context());
    DCHECK(state == kAdded || state == kDeleted);
    (void) state;

    if (state == kAdded)
    {
        Update(EPOLL_CTL_DEL, channel);
    }
    channel->set_context(kNew);

    channels_[channel->fd()] = NULL;
}

void EPollPoller::Update(int operation, Channel* channel)
{
    struct epoll_event event;
    ::bzero(&event, sizeof event);
    event.events = channel->events();
    event.data.u64 = channel->fd();

    if (::epoll_ctl(epoll_fd_, operation, channel->fd(), &event) == 0)
    {
        return ;
    }

    // the given fd is invalid/unusable, so make sure it doesn't hurt us anymore
    PLOG(ERROR) << "epoll_ctl fd " << channel->fd() << " failed";
}

} // namespace claire
