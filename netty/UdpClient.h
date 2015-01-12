// Copyright (c) 2013 The claire-common Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef _CLAIRE_NETTY_UDPCLIENT_H_
#define _CLAIRE_NETTY_UDPCLIENT_H_

#include <map>
#include <memory>

#include <boost/atomic.hpp>
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>

#include <claire/netty/Buffer.h>
#include <claire/netty/InetAddress.h>
#include <claire/common/threading/Mutex.h>

namespace claire {

class Socket;
class Channel;
class EventLoop;

class UdpClient : boost::noncopyable
{
public:
    typedef boost::function<void(Buffer*, const InetAddress&)> MessageCallback;
    typedef boost::function<void(Buffer*, const InetAddress&)> TimeoutCallback;
    typedef boost::function<int64_t(const void*, size_t)> HashCodeFunction;

    UdpClient(EventLoop* loop, const std::string& name__);
    ~UdpClient();

    const std::string name() const { return name_; }

    void Start();
    void Send(Buffer* buffer, const InetAddress& server_address);
    void Send(const void* data, size_t length, const InetAddress& server_address);

    void set_message_callback(const MessageCallback& callback)
    {
        message_callback_ = callback;
    }

    void set_timeout_callback(const TimeoutCallback& callback)
    {
        timeout_callback_ = callback;
    }

    void set_hash_code_function(const HashCodeFunction& function)
    {
        hash_code_function_ = function;
    }

    // timeout in milisecond, default is 0
    void set_timeout(int64_t timeout) { timeout_ = timeout; }

private:
    void StartInLoop();

    void OnRead();
    void OnTimeout(int64_t hash_code);

    struct OutstandingCall;

    EventLoop* loop_;
    const std::string name_;

    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel> channel_;
    Buffer input_buffer_;
    boost::atomic<bool> started_;
    int64_t timeout_;

    MessageCallback message_callback_;
    TimeoutCallback timeout_callback_;
    HashCodeFunction hash_code_function_;

    Mutex mutex_;
    std::map<int64_t, OutstandingCall> outstanding_calls_; //@GUARDBY mutex_
};

} // namespace claire

#endif // _CLAIRE_NETTY_UDPCLIENT_H_
