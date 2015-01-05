// Copyright (c) 2013 The claire-common Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef _CLAIRE_COMMON_EVENTS_CHANNEL_H_
#define _CLAIRE_COMMON_EVENTS_CHANNEL_H_

#include <string>

#include <boost/any.hpp>
#include <boost/function.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>

namespace claire {

class EventLoop;

///
/// A selectable I/O channel.
///
/// This class doesn't own the file descriptor.
/// The file descriptor could be a socket,
/// an eventfd, a timerfd, or a signalfd
class Channel : boost::noncopyable
{
public:
    typedef boost::function<void()> EventCallback;

    enum class Priority : char
    {
        kLow,
        kNormal,
        kHigh
    };

    Channel(EventLoop* loop, int fd);
    ~Channel();

    void Remove();
    void OnEvent();

    void set_read_callback(const EventCallback& callback)
    {
        read_callback_ = callback;
    }

    void set_write_callback(const EventCallback& callback)
    {
        write_callback_ = callback;
    }

    void set_close_callback(const EventCallback& callback)
    {
        close_callback_ = callback;
    }

    void set_error_callback(const EventCallback& callback)
    {
        error_callback_ = callback;
    }

    // Tie this channel to the owner object managed by shared_ptr,
    // prevent the owner object being destroyed in handleEvent.
    void tie(const boost::shared_ptr<void>&);

    int fd() const { return fd_; }
    int events() const { return events_; }

    int revents() const { return revents_; }
    void set_revents(int revents__) { revents_ = revents__; }

    // for Poller
    const boost::any& context() const { return context_; }
    void set_context(const boost::any& context__) { context_ = context__; }

    Priority priority() const { return priority_; }
    void set_priority(Priority priority__) { priority_ = priority__; }

    bool IsNoneEvent() const { return events_ == kNoneEvent; }
    bool IsWriting() const { return (events_ & kWriteEvent); }

    void EnableReading() { events_ |= kReadEvent; Update(); }
    void EnableWriting() { events_ |= kWriteEvent; Update(); }

    void DisableReading() { events_ &= (~kReadEvent); Update(); }
    void DisableWriting() { events_ &= (~kWriteEvent); Update(); }
    void DisableAll() { events_ = kNoneEvent; Update(); }

    EventLoop* OwnerLoop() { return loop_; }

private:
    void Update();
    void OnEventWithGuard();

    EventLoop* loop_;
    const int fd_;
    int events_;
    int revents_;
    boost::any context_; // used by Poller
    Priority priority_;

    boost::weak_ptr<void> tie_;
    bool tied_;
    bool event_handling_;

    const static int kNoneEvent;
    const static int kReadEvent;
    const static int kWriteEvent;

    EventCallback read_callback_;
    EventCallback write_callback_;
    EventCallback close_callback_;
    EventCallback error_callback_;
};

} // namespace claire

#endif // _CLAIRE_COMMON_EVENTS_CHANNEL_H_
