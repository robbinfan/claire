// Copyright (c) 2013 The claire-protorpc Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors

#pragma once

#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>

namespace claire {

class Buffer;
class HttpConnection;
typedef boost::shared_ptr<HttpConnection> HttpConnectionPtr;

namespace protorpc {

class RpcMessage;
class RpcCodec : boost::noncopyable
{
public:
    typedef boost::function<void(const HttpConnectionPtr&,
                                 const RpcMessage&) > MessageCallback;

    RpcCodec();

    void set_message_callback(const MessageCallback& callback)
    {
        message_callback_ = callback;
    }

    void ParseFromBuffer(const HttpConnectionPtr& connection, Buffer* buffer) const;
    void SerializeToBuffer(RpcMessage& message, Buffer* buffer) const;

private:
    MessageCallback message_callback_;
};

} // namespace protorpc
} // namespace claire
