// Copyright (c) 2013 The claire-netty Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef _CLAIRE_NETTY_HTTP_HTTPSERVER_H_
#define _CLAIRE_NETTY_HTTP_HTTPSERVER_H_

#include <set>
#include <map>
#include <algorithm>

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>

#include <claire/common/threading/Mutex.h>
#include <claire/netty/TcpServer.h>
#include <claire/netty/http/HttpConnection.h>

namespace claire {

class HttpServer : boost::noncopyable
{
public:
    typedef HttpConnection::HeadersCallback HeadersCallback;
    typedef boost::function<void(const HttpConnectionPtr&)> RequestCallback;
    typedef boost::function<void(const HttpConnectionPtr&)> ConnectionCallback;

    HttpServer(EventLoop* loop,
               const InetAddress& listen_address,
               const std::string& name);

    /// Callback be registered before calling start().
    /// Not thread safe
    void set_headers_callback(const HeadersCallback& callback)
    {
        headers_callback_ = callback;
    }

    void Register(const std::string& path, const RequestCallback& callback, bool silent)
    {
        auto it = request_callbacks_.find(path);
        if (it != request_callbacks_.end())
        {
            return ;
        }

        auto n = request_callbacks_.insert(std::make_pair(path, callback));
        DCHECK(n.second);

        if (!silent)
        {
            show_paths_.insert(path);
        }
    }

    void set_connection_callback(const ConnectionCallback& callback)
    {
        connection_callback_ = callback;
    }

    template<size_t N>
    void RegisterAsset(const std::string& path, const char (&s)[N])
    {
        RegisterAsset(path, s, N);
    }

    void RegisterAsset(const std::string& path, const char* data, size_t length);

    void set_num_threads(int num_threads)
    {
        server_.set_num_threads(num_threads);
    }

    void Start();

    bool IsConnectionExist(HttpConnection::Id id) const
    {
        MutexLock lock(mutex_);
        return connections_.find(id) != connections_.end();
    }

    template<typename T>
    void SendByHttpConnectionId(HttpConnection::Id id, T data) const
    {
        MutexLock lock(mutex_);
        auto it = connections_.find(id);
        if (it != connections_.end())
        {
            it->second->Send(data);
        }
    }

    void Shutdown(HttpConnection::Id id)
    {
        MutexLock lock(mutex_);
        auto it = connections_.find(id);
        if (it != connections_.end())
        {
            it->second->Shutdown();
        }
    }

    void OnError(HttpConnection::Id id, HttpResponse::StatusCode status, const std::string& reason)
    {
        MutexLock lock(mutex_);
        auto it = connections_.find(id);
        if (it != connections_.end())
        {
            it->second->OnError(status, reason);
        }
    }

    EventLoop* loop() { return server_.loop(); }

private:
    void OnConnection(const TcpConnectionPtr& connection);
    void OnMessage(const TcpConnectionPtr& connection, Buffer* buffer);

    void OnAsset(const HttpConnectionPtr& connection);
    void OnIndexPage(const HttpConnectionPtr& connection);

    TcpServer server_;
    HeadersCallback headers_callback_;
    ConnectionCallback connection_callback_;
    std::map<std::string, RequestCallback> request_callbacks_;

    mutable Mutex mutex_;
    std::map<HttpConnection::Id, HttpConnectionPtr> connections_;
    std::map<std::string, StringPiece> assets_;

    std::set<std::string> show_paths_;
};

} // namespace claire

#endif // _CLAIRE_NETTY_HTTP_HTTPSERVER_H_
