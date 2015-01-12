// Copyright (c) 2013 The claire-netty Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#pragma once

#include <map>
#include <string>

#include <boost/noncopyable.hpp>

#include <claire/common/threading/Mutex.h>
#include <claire/common/threading/Singleton.h>
#include <claire/netty/resolver/Resolver.h>

namespace claire {

template<typename NewResolver>
Resolver* ResolverCreator()
{
    return new NewResolver();
}

class ResolverFactory : boost::noncopyable
{
public:
    typedef Resolver* (*Creator)();

    static ResolverFactory* instance();

    ///
    /// Create a specific type resolver by resolver's name
    ///
    Resolver* Create(const std::string& name) const;

    ///
    /// Register a resolver creator by name to factory
    ///
    void Register(const std::string& name, Creator creator);

private:
    ResolverFactory();
    ~ResolverFactory() {}
    friend class Singleton<ResolverFactory>;

    mutable Mutex mutex_;
    std::map<std::string, Creator> creators_;
};

} // namespace claire
