// Copyright (c) 2013 The claire-protorpc Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors

#pragma once

#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>

namespace claire {

class EventLoop;
class InetAddress;

namespace protorpc {

class Service;
class RpcServer : boost::noncopyable
{
public:
    struct Options
    {
        bool disable_form = false;
        bool disable_json = false;
        bool disable_flags = false;
        bool disable_pprof = false;
        bool disable_statistics = false;
        bool disable_builtin_service = false;
    };

    RpcServer(EventLoop* loop, const InetAddress& listen_address)
        : RpcServer(loop, listen_address, Options()) {}
    RpcServer(EventLoop* loop, const InetAddress& listen_address, const Options& options);
    ~RpcServer(); // force dtor

    ///
    /// set the num of threads of server, must called before Start
    ///
    void set_num_threads(int num_threads);

    ///
    /// register any service for server, must before Start
    ///
    void RegisterService(Service* service);

    ///
    ///  run the server
    ///
    void Start();

private:
    class Impl;
    boost::shared_ptr<Impl> impl_;
};

} // namespace protorpc
} // namespace claire
