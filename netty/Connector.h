// Copyright (c) 2013 The claire-netty Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef _CLAIRE_NETTY_CONNECTOR_H_
#define _CLAIRE_NETTY_CONNECTOR_H_

#include <boost/atomic.hpp>
#include <boost/function.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/random/mersenne_twister.hpp>

#include <claire/netty/Callbacks.h>
#include <claire/netty/InetAddress.h>
#include <claire/common/events/TimerId.h>

namespace claire {

class Socket;
class Channel;
class EventLoop;

class Connector : boost::noncopyable,
                  public boost::enable_shared_from_this<Connector>
{
public:
    Connector(EventLoop* loop, const InetAddress& server_address);
    ~Connector();

    void set_new_connection_callback(const NewConnectionCallback& callback)
    {
        new_connection_callback_ = callback;
    }

    void Connect();
    void Stop();
    void Restart();

private:
    enum States
    {
        kDisconnected, // Init state or connection down
        kConnecting,   // Before fd writeable
        kConnected     // After fd writeable
    };

    void set_state(States s)
    {
        state_ = s;
    }

    void ConnectInLoop();
    void StopInLoop();

    void OnWrite();
    void OnError();
    void OnConnectTimeout();

    void ResetAndRetry();
    void RemoveAndResetChannel();
    void ResetChannel();

    EventLoop* loop_;
    const InetAddress server_address_;

    std::unique_ptr<Channel> channel_;
    NewConnectionCallback new_connection_callback_;

    boost::atomic<bool> connect_;
    boost::atomic<States> state_;

    boost::random::mt19937 gen_;
    TimerId retry_timer_;
    TimerId connect_timer_;
};

typedef boost::shared_ptr<Connector> ConnectorPtr;

} // namespace claire

#endif // _CLAIRE_NETTY_CONNECTOR_H_
