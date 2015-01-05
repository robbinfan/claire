// Copyright (c) 2013 The claire-common Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef _CLAIRE_COMMON_EVENTS_POLLER_EPOLLPOLLER_H_
#define _CLAIRE_COMMON_EVENTS_POLLER_EPOLLPOLLER_H_

#include <claire/common/events/Poller.h>

#include <set>
#include <vector>

struct epoll_event;
namespace claire {

class EPollPoller : public Poller
{
public:
    EPollPoller(EventLoop* loop);
    virtual ~EPollPoller();

    virtual void poll(int timeout_in_milliseconds, ChannelList* active_channles);
    virtual void UpdateChannel(Channel* channel);
    virtual void RemoveChannel(Channel* channel);

private:
    void Update(int operation, Channel* channel);

    EventLoop* loop_;
    int epoll_fd_;
    std::vector<struct epoll_event> events_;
    std::vector<Channel*> channels_;
};

} // namespace claire

#endif // _CLAIRE_COMMON_EVENTS_POLLER_EPOLLPOLLER_H_
