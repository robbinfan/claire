// Copyright (c) 2013 The claire-netty Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef _CLAIRE_NETTY_HTTP_HTTPCLIENT_H_
#define _CLAIRE_NETTY_HTTP_HTTPCLIENT_H_

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>

#include <claire/netty/TcpClient.h>
#include <claire/netty/http/HttpConnection.h>

namespace claire {

class HttpClient : boost::noncopyable
{
public:
    typedef HttpConnection::HeadersCallback HeadersCallback;
    typedef boost::function<void(const HttpConnectionPtr&)> MessageCallback;
    typedef boost::function<void(const HttpConnectionPtr&)> ConnectionCallback;

    HttpClient(EventLoop* loop, const std::string& name);
    ~HttpClient();

    void set_headers_callback(const HeadersCallback& callback)
    {
        headers_callback_ = callback;
    }

    void set_message_callback(const MessageCallback& callback)
    {
        message_callback_ = callback;
    }

    void set_connection_callback(const ConnectionCallback& callback)
    {
        connection_callback_ = callback;
    }

    void Connect(const InetAddress& server_address);
    void Shutdown();

    void Send(Buffer* buffer)
    {
        if (connection_)
        {
            connection_->Send(buffer);
        }
    }

    bool connected() const
    {
        if (connection_)
        {
            return connection_->connected();
        }
        return false;
    }

    InetAddress peer_address() const
    {
        if (connection_)
        {
            return connection_->peer_address();
        }
        return InetAddress();
    }

    InetAddress local_address() const
    {
        if (connection_)
        {
            return connection_->local_address();
        }
        return InetAddress();
    }

    bool retry() const { return client_.retry(); }
    void set_retry(bool on)
    {
        client_.set_retry(on);
    }

private:
    void OnConnection(const TcpConnectionPtr& connection);
    void OnMessage(const TcpConnectionPtr& connection, Buffer* buffer);

    EventLoop* loop_;
    TcpClient client_;
    HttpConnectionPtr connection_;
    MessageCallback message_callback_;
    HeadersCallback headers_callback_;
    ConnectionCallback connection_callback_;
};

} // namespace claire

#endif // _CLAIRE_NETTY_HTTP_HTTPCLIENT_H_
