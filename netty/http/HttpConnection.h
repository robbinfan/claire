// Copyright (c) 2013 The claire-netty Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef _CLAIRE_NETTY_HTTT_HTTPCONNECTION_H_
#define _CLAIRE_NETTY_HTTT_HTTPCONNECTION_H_

#include <string>

#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

#include <claire/netty/TcpConnection.h>
#include <claire/netty/http/HttpMessage.h>
#include <claire/netty/http/HttpRequest.h>
#include <claire/netty/http/HttpResponse.h>
#include <claire/common/time/Timestamp.h>
#include <claire/common/base/Types.h>

namespace claire {

class Buffer;

class HttpConnection;
typedef boost::shared_ptr<HttpConnection> HttpConnectionPtr;

class HttpConnection : boost::noncopyable,
                       public boost::enable_shared_from_this<HttpConnection>
{
public:
    typedef boost::function<void (const HttpConnectionPtr&)> HeadersCallback;
    typedef boost::function<void (const HttpConnectionPtr&, Buffer*)> BodyCallback;
    typedef TcpConnection::Id Id;

    enum State
    {
        kExpectStartLine,
        kExpectHeaders,
        kExpectBody,
        kGotAll,
    };

    HttpConnection(const TcpConnectionPtr& connection)
        : connection_(connection),
          state_(kExpectStartLine)
    {}

    ~HttpConnection();

    void Reset()
    {
        message_->Reset();
        state_ = kExpectStartLine;
    }

    void Shutdown();

    template<typename T>
    bool Parse(Buffer* buffer);

    void Send(Buffer* buffer);
    void Send(HttpRequest* request);
    void Send(HttpResponse* response);
    void Send(const StringPiece& data);

    void set_headers_callback(const HeadersCallback& callback)
    {
        headers_callback_ = callback;
    }

    void set_body_callback(const BodyCallback& callback)
    {
        body_callback_ = callback;
    }

    bool GotHeaders() const
    {
        return state_ == kExpectBody;
    }

    bool GotAll() const
    {
        return state_ == kGotAll;
    }

    bool connected() const
    {
        if (connection_)
        {
            return connection_->connected();
        }
        return false;
    }

    Id id() const
    {
        return connection_->id();
    }

    const boost::shared_ptr<HttpRequest> mutable_request() const
    {
        return down_pointer_cast<HttpRequest>(message_);
    }

    const boost::shared_ptr<HttpResponse> mutable_response() const
    {
        return down_pointer_cast<HttpResponse>(message_);
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

    void OnError(HttpResponse::StatusCode status, const std::string& reason);

private:
    bool ParseMessage(Buffer* buf);

    const TcpConnectionPtr connection_;
    boost::shared_ptr<HttpMessage> message_;
    State state_;
    Uri last_request_uri_;

    HeadersCallback headers_callback_;
    BodyCallback body_callback_;
};

} // namespace claire

#endif // _CLAIRE_NETTY_HTTT_HTTPCONNECTION_H_
