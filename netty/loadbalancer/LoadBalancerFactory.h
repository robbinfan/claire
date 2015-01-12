// Copyright (c) 2013 The claire-netty Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef _CLAIRE_NETTY_LOADBALANCER_LOADBALANCERFACTORY_H_
#define _CLAIRE_NETTY_LOADBALANCER_LOADBALANCERFACTORY_H_

#include <map>
#include <string>

#include <boost/noncopyable.hpp>

#include <claire/common/threading/Singleton.h>
#include <claire/common/threading/Mutex.h>
#include <claire/netty/loadbalancer/LoadBalancer.h>

namespace claire {

template <typename NewLoadBalancer>
LoadBalancer* LoadBalancerCreator()
{
    return new NewLoadBalancer();
}

class LoadBalancerFactory : boost::noncopyable
{
public:
    typedef LoadBalancer* (*Creator)();

    static LoadBalancerFactory* instance();

    LoadBalancer* Create(const std::string& name) const;
    void Register(const std::string& name, Creator creator);

private:
    LoadBalancerFactory();
    ~LoadBalancerFactory();

    friend class Singleton<LoadBalancerFactory>;

    mutable Mutex mutex_;
    std::map<std::string, Creator> creators_;
};

} // namespace claire

#endif // _CLAIRE_NETTY_LOADBALANCER_LOADBALANCERFACTORY_H_
