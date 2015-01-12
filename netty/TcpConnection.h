// Copyright (c) 2013 The claire-netty Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef _CLAIRE_NETTY_TCPCONNECTION_H_
#define _CLAIRE_NETTY_TCPCONNECTION_H_

#include <string>

#include <boost/any.hpp>
#include <boost/atomic.hpp>
#include <boost/function.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

#include <claire/netty/Buffer.h>
#include <claire/netty/Callbacks.h>
#include <claire/netty/InetAddress.h>
#include <claire/common/strings/StringPiece.h>
#include <claire/common/metrics/Counter.h>

namespace claire {

class Socket;
class Channel;
class EventLoop;

/// TCP connection, for both client and server usage.
class TcpConnection: boost::noncopyable,
                     public boost::enable_shared_from_this<TcpConnection>
{
public:
    typedef int32_t Id;

    /// User should not create this object.
    TcpConnection(EventLoop* loop,
                  Socket&& socket,
                  Id id);
    ~TcpConnection();

    EventLoop* loop() const { return loop_; }
    Id id() const { return id_; }

    const InetAddress& local_address() const { return local_address_; }
    const InetAddress& peer_address() const { return peer_address_; }
    bool connected() const { return state_ == kConnected; }

    void SetTcpNoDelay(bool on);

    void set_context(const boost::any& context__)
    {
        context_ = context__;
    }

    boost::any& context() { return context_; }
    const boost::any& context() const { return context_; }
    boost::any* mutable_context() { return &context_; }

    void set_connection_callback(const ConnectionCallback& callback)
    {
        connection_callback_ = callback;
    }

    void set_message_callback(const MessageCallback& callback)
    {
        message_callback_ = callback;
    }

    void set_write_complete_callback(const WriteCompleteCallback& callback)
    {
        write_complete_callback_ = callback;
    }

    void set_close_callback(const CloseCallback& callback)
    {
        close_callback_ = callback;
    }

    void set_high_watermark_callback(const HighWaterMarkCallback& callback)
    {
        high_watermark_callback_ = callback;
    }

    void Send(Buffer* buffer);
    void Send(const StringPiece& message);

    // NOT thread safe, no simultaneous calling
    void Shutdown();

    void ConnectEstablished();
    void ConnectDestroyed();

    std::string GetTcpInfoString() const;
private:
    enum States
    {
        kDisconnected,
        kDisconnecting,
        kConnecting,
        kConnected
    };

    void OnRead();
    void OnWrite();
    void OnClose();
    void OnError();

    void SendInLoop(Buffer& buffer);
    void SendInLoop(const void* data, size_t length);
    void ShutdownInLoop();
    void set_state(States s) { state_ = s; }

    EventLoop* loop_;
    const Id id_;

    boost::atomic<States> state_;

    boost::scoped_ptr<Socket> socket_;
    boost::scoped_ptr<Channel> channel_;

    InetAddress local_address_;
    InetAddress peer_address_;

    ConnectionCallback connection_callback_;
    MessageCallback    message_callback_;
    WriteCompleteCallback write_complete_callback_;
    HighWaterMarkCallback high_watermark_callback_;
    CloseCallback close_callback_;

    Buffer input_buffer_;
    boost::ptr_vector<Buffer> output_buffers_;

    boost::any context_;

    int received_bytes_;
    int sent_bytes_;

    Counter received_bytes_counter_;
    Counter sent_bytes_counter_;
};

} // namespace claire

#endif // _CLAIRE_NETTY_TCPCONNECTION_H_
