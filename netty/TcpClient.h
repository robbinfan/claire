// Copyright (c) 2013 The claire-netty Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef _CLAIRE_NETTY_TCPCLIENT_H_
#define _CLAIRE_NETTY_TCPCLIENT_H_

#include <string>

#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>

#include <claire/netty/Callbacks.h>

namespace claire {

class EventLoop;
class InetAddress;
class TcpConnection;
typedef boost::shared_ptr<TcpConnection> TcpConnectionPtr;

class TcpClient : boost::noncopyable
{
public:
    TcpClient(EventLoop* loop, const std::string& name);
    ~TcpClient();

    /// Start client to connect server_address
    /// It's harmless to call it multiple times.
    /// Thread safe
    void Connect(const InetAddress& server_address);

    /// Shutdown connection to server_address
    /// It's harmless to call it multiple times.
    /// Thread safe
    void Shutdown();

    /// Stop client, it will not shutdown the up connection
    /// It's harmless to call it multiple times.
    /// Thread safe
    void Stop();

    /// Is enable retry
    /// Thread safe
    bool retry() const;

    /// enable/disable retry
    /// Thread safe
    void set_retry(bool on);

    /// ConnectionCallback to TcpConnection, called when connection up/down
    /// Not thread safe
    void set_connection_callback(const ConnectionCallback& callback);

    /// MessageCallback to TcpConnection, called when message received
    /// Not thread safe
    void set_message_callback(const MessageCallback& callback);

    /// WriteCompleteCallback to TcpConnection, called when write success(to OS!)
    /// Not thread safe
    void set_write_complete_callback(const WriteCompleteCallback& callback);

    TcpConnectionPtr connection() const;

    EventLoop* loop();

private:
    class Impl;
    boost::shared_ptr<Impl> impl_;
};

} // namespace claire

#endif // _CLAIRE_NETTY_TCPCLIENT_H_
