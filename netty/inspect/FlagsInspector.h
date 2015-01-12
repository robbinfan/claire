// Copyright (c) 2013 The claire-netty Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#pragma once

#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>

namespace claire {

class HttpServer;
class HttpConnection;
typedef boost::shared_ptr<HttpConnection> HttpConnectionPtr;

class FlagsInspector : boost::noncopyable
{
public:
    explicit FlagsInspector(HttpServer* server);

private:
    static void OnFlags(const HttpConnectionPtr& connnection);
};

} // namespace claire
