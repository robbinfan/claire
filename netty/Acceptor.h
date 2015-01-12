// Copyright (c) 2013 The claire-netty Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#pragma once

#include <boost/noncopyable.hpp>

#include <claire/netty/Callbacks.h>

namespace claire {

class Channel;
class EventLoop;
class InetAddress;

///
/// Acceptor of incoming TCP connections.
///
class Acceptor : boost::noncopyable
{
public:
    ///
    /// Default ctor, do not use reuse port
    ///
    Acceptor(EventLoop* loop, const InetAddress& listen_address)
        : Acceptor(loop, listen_address, false) {}

    ///
    /// User can enable reuse port by 3rd params
    ///
    Acceptor(EventLoop* loop,
             const InetAddress& listen_address,
             bool reuse_port);

    ~Acceptor();

    ///
    /// Must call before Listen(), set NewConnectionCallback for notify new connection event
    ///
    void SetNewConnectionCallback(const NewConnectionCallback& callback)
    {
        new_connection_callback_ = callback;
    }

    ///
    /// Is acceptor listening
    ///
    bool listenning() const { return listenning_; }

    ///
    /// Begin Listen
    ///
    void Listen();

private:
    ///
    /// Process events which channel notified
    ///
    void OnRead();

    EventLoop* loop_;
    std::unique_ptr<Socket> accept_socket_;
    std::unique_ptr<Channel> accept_channel_;
    NewConnectionCallback new_connection_callback_;
    bool listenning_;
};

} // namespace claire
