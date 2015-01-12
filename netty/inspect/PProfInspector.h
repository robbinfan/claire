// Copyright (c) 2013 The claire-netty Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#pragma once

#include <set>

#include <boost/noncopyable.hpp>

#include <claire/common/threading/Mutex.h>
#include <claire/netty/http/HttpConnection.h>

namespace claire {

class HttpServer;
class PProfInspector : boost::noncopyable
{
public:
    explicit PProfInspector(HttpServer* server);

private:
    void OnProfile(const HttpConnectionPtr& connection);
    void OnProfileComplete();

    static void OnHeap(const HttpConnectionPtr& connection);
    static void OnGrowth(const HttpConnectionPtr& connection);
    static void OnCmdline(const HttpConnectionPtr& connection);
    static void OnSymbol(const HttpConnectionPtr& connection);

    HttpServer* server_;

    Mutex mutex_;
    std::set<HttpConnection::Id> connections_;
};

} // namespace claire
