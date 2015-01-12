// Copyright (c) 2013 The claire-netty Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef _CLAIRE_NETTY_LOADBALANCER_RANDOMLOADBALANCER_H_
#define _CLAIRE_NETTY_LOADBALANCER_RANDOMLOADBALANCER_H_

#include <vector>
#include <algorithm>

#include <boost/noncopyable.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>

#include <claire/netty/InetAddress.h>
#include <claire/netty/loadbalancer/LoadBalancer.h>

namespace claire {

class RandomLoadBalancer : public LoadBalancer,
                           boost::noncopyable
{
public:
    virtual ~RandomLoadBalancer() {}

    virtual void AddBackend(const InetAddress& backend, int weight)
    {
        auto it = std::find(backends_.begin(), backends_.end(), backend);
        if (it == backends_.end())
        {
            backends_.push_back(backend);
            std::sort(backends_.begin(), backends_.end());
        }
    }

    virtual void ReleaseBackend(const InetAddress& backend)
    {
        auto it = std::find(backends_.begin(), backends_.end(), backend);
        if (it != backends_.end())
        {
            backends_.erase(it);
        }
    }

    virtual InetAddress NextBackend()
    {
        boost::random::uniform_int_distribution<uint64_t> dist(0, backends_.size()-1);
        return backends_[dist(gen_)];
    }

private:
    std::vector<InetAddress> backends_;
    boost::random::mt19937 gen_;
};

} // namespace claire

#endif // _CLAIRE_NETTY_LOADBALANCER_RANDOMLOADBALANCER_H_
