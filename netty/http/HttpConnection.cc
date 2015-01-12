// Copyright (c) 2013 The claire-netty Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include <claire/netty/http/HttpConnection.h>

#include <boost/lexical_cast.hpp>

#include <claire/netty/http/HttpRequest.h>
#include <claire/netty/http/HttpResponse.h>
#include <claire/netty/TcpConnection.h>
#include <claire/netty/Buffer.h>
#include <claire/common/logging/Logging.h>

namespace claire {

HttpConnection::~HttpConnection() {}

void HttpConnection::Shutdown()
{
    connection_->Shutdown();
}

void HttpConnection::Send(Buffer* buffer)
{
    connection_->Send(buffer);
}

void HttpConnection::Send(const StringPiece& data)
{
    connection_->Send(data);
}

void HttpConnection::Send(HttpRequest* request)
{
    if (request->version() == HttpMessage::kUnknown)
    {
        request->set_version(HttpMessage::kHttp11);
    }

    if (!request->HasHeader("Content-Type"))
    {
        request->AddHeader("Content-Type", "text/html");
    }

    if (!request->HasHeader("Transfer-Encoding")
        && !request->HasHeader("Content-Length"))
    {
        request->AddHeader("Content-Length", boost::lexical_cast<std::string>(request->mutable_body()->length()));
    }
    request->AddHeader("Connection", "Keep-Alive");

    Buffer buffer;
    request->AppendTo(&buffer);
    connection_->Send(&buffer);

    last_request_uri_ = request->uri();
}

void HttpConnection::Send(HttpResponse* response)
{
    if (response->version() == HttpMessage::kUnknown)
    {
        response->set_version(message_->version());
    }

    if (response->status() == HttpResponse::kUnknown)
    {
        response->set_status(HttpResponse::k200OK);
    }

    if (!response->HasHeader("Content-Type"))
    {
        response->AddHeader("Content-Type", "text/html");
    }

    if (!response->HasHeader("Transfer-Encoding")
        && !response->HasHeader("Content-Length"))
    {
        response->AddHeader("Content-Length", boost::lexical_cast<std::string>(response->mutable_body()->length()));
    }

    if (message_->IsKeepAlive() && message_->version() == HttpMessage::kHttp10)
    {
        response->AddHeader("Connection", "Keep-Alive");
    }

    Buffer buffer;
    response->AppendTo(&buffer);
    connection_->Send(&buffer);

    if (!message_->IsKeepAlive() && !response->IsKeepAlive())
    {
        Shutdown();
    }
}

bool HttpConnection::ParseMessage(Buffer* buffer)
{
    bool ok = true;
    bool more = true;
    while (more)
    {
        if (state_ == kExpectStartLine)
        {
            auto crlf = buffer->FindCRLF();
            if (crlf)
            {
                std::string line(buffer->Peek(), crlf);
                ok = message_->ParseStartLine(line);
                if (ok)
                {
                    state_ = kExpectHeaders;
                    buffer->ConsumeUntil(crlf+2);
                }
                else
                {
                    more = false;
                }
            }
            else
            {
                more = false;
            }
        }
        else if (state_ == kExpectHeaders)
        {
            auto crlf = buffer->FindCRLF();
            if (crlf)
            {
                auto colon = std::find(buffer->Peek(), crlf, ':');
                if (colon != crlf)
                {
                    message_->AddHeader(buffer->Peek(), colon, crlf);
                }
                else
                {
                    if (crlf == buffer->Peek())
                    {
                        // empty line
                        state_ = kExpectBody;

                        if (headers_callback_)
                        {
                            headers_callback_(shared_from_this());
                        }
                    }
                }
                buffer->ConsumeUntil(crlf+2);
            }
            else
            {
                more = false;
            }
        }
        else if (state_ == kExpectBody)
        {
            if (body_callback_)
            {
                body_callback_(shared_from_this(), buffer);
            }

            if (message_->IsComplete(buffer))
            {
                auto length = message_->GetBodyLength(buffer);
                message_->mutable_body()->append(buffer->Peek(), length);
                buffer->Consume(length);

                state_ = kGotAll;
            }
            more = false;
        }
    }

    return ok;
}

template<typename T>
bool HttpConnection::Parse(Buffer* buffer)
{
    if (state_ == kGotAll)
    {
        return true;
    }

    if (!message_)
    {
        message_.reset(new T);
    }
    assert(typeid(T) == typeid(*message_));

    return ParseMessage(buffer);
}

template bool HttpConnection::Parse<HttpRequest>(Buffer*);
template bool HttpConnection::Parse<HttpResponse>(Buffer*);

void HttpConnection::OnError(HttpResponse::StatusCode status, const std::string& reason)
{
    HttpResponse response;
    response.set_status(status);
    response.mutable_body()->assign(reason);
    this->Send(&response);
    this->Shutdown();
}

} // namespace claire
