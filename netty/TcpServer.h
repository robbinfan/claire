// Copyright (c) 2013 The claire-netty Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef _CLAIRE_NETTY_TCPSERVER_H_
#define _CLAIRE_NETTY_TCPSERVER_H_

#include <string>

#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>

#include <claire/netty/Callbacks.h>

namespace claire {

class EventLoop;
class InetAddress;

class TcpServer : boost::noncopyable
{
public:
    enum Option
    {
        kNoReusePort,
        kReusePort
    };

    TcpServer(EventLoop* loop__,
              const InetAddress& listen_address,
              const std::string& name__)
        : TcpServer(loop__, listen_address, name__, kNoReusePort) {}
    TcpServer(EventLoop* loop,
              const InetAddress& listen_address,
              const std::string& name,
              const Option& option);
    ~TcpServer();

    const std::string& hostport() const;
    const std::string& name() const;

    /// Starts the server if it's not listenning.
    ///
    /// It's harmless to call it multiple times.
    /// Thread safe.
    void Start();

    /// Set the number of threads for handling input.
    ///
    /// Always accepts new connection in loop's thread.
    /// Must be called before @c Start
    /// @param num_threads
    /// - 0 means all I/O in loop's thread, no thread will created.
    ///   this is the default value.
    /// - 1 means all I/O in another thread.
    /// - N means a thread pool with N threads, new connections
    ///   are assigned on a round-robin basis.
    /// Please only set before @c Start!
    void set_num_threads(int num_threads);

    /// ThreadInitCallback to threadpool
    /// Not thread safe
    void set_thread_init_callback(const ThreadInitCallback& callback);

    /// ConnectionCallback to TcpConnection, called when connections up/down
    /// Not thread safe
    void set_connection_callback(const ConnectionCallback& callback);

    /// MessageCallback to TcpConnection, called when message arriving
    /// Not thread safe
    void set_message_callback(const MessageCallback& callback);

    /// WriteCompleteCallback to TcpConnection, called when message write success(to OS!)
    /// Not thread safe
    void set_write_complete_callback(const WriteCompleteCallback& callback);

    EventLoop* loop();
private:
    class Impl;
    boost::shared_ptr<Impl> impl_;
};

} // namespace claire

#endif // _CLAIRE_NET_TCPSERVER_H_
