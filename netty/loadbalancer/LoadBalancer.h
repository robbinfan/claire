// Copyright (c) 2013 The claire-netty Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef _CLAIRE_NETTY_LOADBALANCER_LOADBALANCER_H_
#define _CLAIRE_NETTY_LOADBALANCER_LOADBALANCER_H_

namespace claire {

class InetAddress;

enum class ConnectResult
{
    kSuccess,
    kFailed,
    kTimeout
};

enum class RequestResult
{
    kSuccess,
    kFailed,
    kTimeout
};

class LoadBalancer
{
public:
    virtual ~LoadBalancer() {}

    virtual void AddBackend(const InetAddress& backend, int weight) = 0;

    virtual void ReleaseBackend(const InetAddress& backend)
    {
        // No-op
    }

    virtual InetAddress NextBackend() = 0;

    virtual void AddConnectResult(const InetAddress& key,  ConnectResult result, int64_t connect_time)
    {
        // No-op
    }

    virtual void AddRequestResult(const InetAddress& key, RequestResult result, int64_t request_time)
    {
        // No-op
    }
};

} // namespace claire

#endif // _CLAIRE_NETTY_LOADBALANCER_LOADBALANCER_H_
