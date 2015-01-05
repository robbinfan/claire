// Copyright (c) 2013 The claire-common Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef _CLAIRE_COMMON_EVENTS_POLLER_H_
#define _CLAIRE_COMMON_EVENTS_POLLER_H_

#include <claire/common/events/Channel.h>

#include <vector>

#include <boost/noncopyable.hpp>

namespace claire {

class Poller : boost::noncopyable
{
public:
    typedef std::vector<Channel*> ChannelList;

    virtual ~Poller() {}

    /// Polls the I/O events.
    /// Must be called in the loop thread.
    virtual void poll(int timeout_in_milliseconds, ChannelList* active_channels) = 0;

    /// Changes the interested I/O events.
    /// Must be called in the loop thread.
    virtual void UpdateChannel(Channel* channel) = 0;

    /// Remove the channel, when it destructs.
    /// Must be called in the loop thread.
    virtual void RemoveChannel(Channel* channel) = 0;
};

} // namespace claire

#endif // _CLAIRE_COMMON_EVENTS_POLLER_H_
