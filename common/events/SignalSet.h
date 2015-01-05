// Copyright (c) 2013 The claire-common Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef _CLAIRE_COMMON_EVENTS_SIGNALSET_H_
#define _CLAIRE_COMMON_EVENTS_SIGNALSET_H_

#include <set>
#include <vector>

#include <boost/function.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/noncopyable.hpp>

namespace claire {

class Channel;
class EventLoop;

class SignalSet : boost::noncopyable
{
public:
    typedef boost::function<void(int)> SignalHandler;

    SignalSet(EventLoop* loop);
    SignalSet(EventLoop* loop,
              int signal_number_1);
    SignalSet(EventLoop* loop,
              int signal_number_1,
              int signal_number_2);
    SignalSet(EventLoop* loop,
              int signal_number_1,
              int signal_number_2,
              int signal_number_3);
    ~SignalSet();

    /// Add a signal_number to signalset, it will mask the signal_number in current thread
    void Add(int signal_number);

    /// Add a handler to all signals, called when receive any signal in signalset
    void Wait(const SignalHandler& handler);

    /// Remove all signals
    void Clear();

    /// Remove all handlers
    void Cancel();

    /// Remove a signal_number from signalset
    void Remove(int signal_number);

private:
    void OnSignal();

    EventLoop* loop_;
    std::set<int> signals_;
    std::vector<SignalHandler> handlers_;

    int fd_;
    boost::scoped_ptr<Channel> channel_;
};

} // namespace claire

#endif // _CLAIRE_COMMON_EVENTS_SIGNALSET_H_
