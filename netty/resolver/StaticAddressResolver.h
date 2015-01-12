// Copyright (c) 2013 The claire-netty Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#pragma once

#include <boost/noncopyable.hpp>

#include <claire/netty/resolver/Resolver.h>

namespace claire {

class StaticAddressResolver : public Resolver,
                              boost::noncopyable
{
public:
    virtual ~StaticAddressResolver() {}

    virtual void Resolve(const std::string& address, const Resolver::ResolveCallback& callback);
};

} // namespace claire
