// Copyright (c) 2013 The claire-netty Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include <claire/netty/loadbalancer/LoadBalancerFactory.h>

#include <claire/common/logging/Logging.h>
#include <claire/netty/loadbalancer/RandomLoadBalancer.h>
#include <claire/netty/loadbalancer/RoundRobinLoadBalancer.h>

namespace claire {

LoadBalancerFactory::LoadBalancerFactory()
{
    creators_.insert(std::make_pair("random", &LoadBalancerCreator<RandomLoadBalancer>));
    creators_.insert(std::make_pair("roundrobin", &LoadBalancerCreator<RoundRobinLoadBalancer>));
}

LoadBalancerFactory::~LoadBalancerFactory() {}

LoadBalancerFactory* LoadBalancerFactory::instance()
{
    return Singleton<LoadBalancerFactory>::instance();
}

LoadBalancer* LoadBalancerFactory::Create(const std::string& name) const
{
    MutexLock lock(mutex_);
    auto it = creators_.find(name);
    if (it == creators_.end())
    {
        return NULL;
    }
    return (*(it->second))();
}

void LoadBalancerFactory::Register(const std::string& name, Creator creator)
{
    MutexLock lock(mutex_);
    auto it = creators_.find(name);
    if (it != creators_.end())
    {
        LOG(FATAL) << "LoadBalancerFactory: " << name << " already registered.";
    }
    else
    {
        creators_.insert(std::make_pair(name, creator));
    }
}

} // namespace claire
