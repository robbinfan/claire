// Copyright (c) 2013 The claire-netty Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include <claire/netty/http/HttpClient.h>

#include <boost/bind.hpp>

#include <claire/netty/http/HttpResponse.h>
#include <claire/common/logging/Logging.h>

namespace claire {

HttpClient::~HttpClient() {}

HttpClient::HttpClient(EventLoop* loop, const std::string& name)
    : loop_(loop),
      client_(loop, name)
{
    client_.set_connection_callback(
            boost::bind(&HttpClient::OnConnection, this, _1));
    client_.set_message_callback(
            boost::bind(&HttpClient::OnMessage, this, _1, _2));
}

void HttpClient::Connect(const InetAddress& server_address)
{
    client_.Connect(server_address);
}

void HttpClient::Shutdown()
{
    client_.Shutdown();
}

void HttpClient::OnConnection(const TcpConnectionPtr& connection)
{
   if (connection->connected())
    {
        connection_.reset(new HttpConnection(connection));
        if (headers_callback_)
        {
            connection_->set_headers_callback(headers_callback_);
        }

        if (connection_callback_)
        {
            connection_callback_(connection_);
        }
    }
    else
    {
        if (connection_callback_)
        {
            connection_callback_(connection_);
        }
        connection_.reset();
    }
}

void HttpClient::OnMessage(const TcpConnectionPtr& connection, Buffer* buffer)
{
    DCHECK(connection_->id() == connection->id());

    if (!connection_->Parse<HttpResponse>(buffer))
    {
        connection_->Shutdown();
        return ;
    }

    if (connection_->GotAll())
    {
        if (message_callback_)
        {
            message_callback_(connection_);
        }
        connection_->Reset();
    }
}

} // namespace claire
