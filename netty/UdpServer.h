// Copyright (c) 2013 The claire-netty Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef _CLAIRE_NETTY_UDPSERVER_H_
#define _CLAIRE_NETTY_UDPSERVER_H_

#include <memory>
#include <string>

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>

#include <claire/netty/Buffer.h>
#include <claire/common/metrics/Counter.h>

namespace claire {

class Socket;
class Channel;
class EventLoop;
class InetAddress;

class UdpServer : boost::noncopyable
{
public:
    typedef boost::function<void(Buffer*, const InetAddress&)> MessageCallback;

    UdpServer(EventLoop* loop,
              const InetAddress& listen_address,
              const std::string& name);
    ~UdpServer();

    const std::string name() const
    {
        return name_;
    }

    const std::string local_address() const
    {
        return local_address_;
    }

    void Start();
    void Send(Buffer* buffer, const InetAddress& server_address);
    void Send(const void* data, size_t length, const InetAddress& server_address);

    void set_message_callback(const MessageCallback& callback)
    {
        message_callback_ = callback;
    }

private:
    void OnRead();
    void StartInLoop();

    EventLoop* loop_;
    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel> channel_;
    Buffer input_buffer_;

    const std::string name_;
    const std::string local_address_;
    MessageCallback message_callback_;
    bool started_;

    Counter received_bytes_counter_;
    Counter sent_bytes_counter_;
};

} // namespace claire

#endif // _CLAIRE_NETTY_UDPSERVER_H_
