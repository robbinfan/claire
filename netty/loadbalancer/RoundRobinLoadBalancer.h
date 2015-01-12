// Copyright (c) 2014 The claire-netty Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef _CLAIRE_NETTY_LOADBALANCER_ROUNDROBINBALANCER_H_
#define _CLAIRE_NETTY_LOADBALANCER_ROUNDROBINBALANCER_H_

#include <vector>
#include <algorithm>

#include <boost/noncopyable.hpp>

#include <claire/netty/InetAddress.h>
#include <claire/netty/loadbalancer/LoadBalancer.h>

#include <claire/common/logging/Logging.h>

namespace claire {

// RoundRobinLoadBalancer need external synchronization
class RoundRobinLoadBalancer : public LoadBalancer,
                               boost::noncopyable
{
public:
    RoundRobinLoadBalancer() : current_(0) {}

    virtual ~RoundRobinLoadBalancer() {}

    virtual void AddBackend(const InetAddress& backend, int weight)
    {
        auto it = std::find(backends_.begin(), backends_.end(), backend);
        if (it == backends_.end())
        {
            backends_.push_back(backend);
        }
    }

    virtual void ReleaseBackend(const InetAddress& backend)
    {
        backends_.erase(std::remove(backends_.begin(), backends_.end(), backend),
                        backends_.end());
    }

    virtual InetAddress NextBackend()
    {
        CHECK(!backends_.empty());
        if (current_ >= backends_.size())
        {
            current_ = 0;
        }
        return backends_[current_++];
    }

private:
    std::vector<InetAddress> backends_;
    size_t current_;
};

} // namespace claire

#endif // _CLAIRE_NETTY_LOADBALANCER_ROUNDROBINBALANCER_H_
