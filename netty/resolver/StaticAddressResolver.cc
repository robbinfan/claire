// Copyright (c) 2013 The claire-netty Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include <claire/netty/resolver/StaticAddressResolver.h>

#include <algorithm>

#include <boost/bind.hpp>
#include <boost/algorithm/string.hpp>

namespace claire {

void StaticAddressResolver::Resolve(const std::string& address, const Resolver::ResolveCallback& callback)
{
    std::vector<std::string> v;
    boost::algorithm::split(v, address, boost::is_any_of(",;"));

    std::vector<InetAddress> result;
    for (auto& item : v)
    {
        result.push_back(InetAddress(item));
    }

    std::sort(result.begin(), result.end());
    result.erase(std::unique(result.begin(), result.end()), result.end());

    callback(result);
}

} // namespace claire
