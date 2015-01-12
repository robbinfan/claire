// Copyright (c) 2013 The claire-netty Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include <claire/netty/InetAddress.h>

#include <stdlib.h>
#include <strings.h>
#include <arpa/inet.h>

#include <string>
#include <vector>

#include <boost/algorithm/string.hpp>

#include <claire/netty/Endian.h>
#include <claire/common/logging/Logging.h>

// INADDR_ANY use (type)value casting.
#pragma GCC diagnostic ignored "-Wold-style-cast"
static const in_addr_t kInaddrAny = INADDR_ANY;
#pragma GCC diagnostic error "-Wold-style-cast"

namespace claire {
namespace detail {

void FromIpPort(const StringPiece& ip, uint16_t port, sockaddr_in* addr)
{
    ::bzero(addr, sizeof *addr);

    addr->sin_family = AF_INET;
    addr->sin_port = HostToNetwork16(port);
    if (::inet_pton(AF_INET, ip.data(), &addr->sin_addr) <= 0)
    {
        PLOG(ERROR) << "inet_pton failed";
    }
}

} // namespace detail

static_assert(sizeof(InetAddress) == sizeof(struct sockaddr_in),
              "InetAddress should equal sockaddr_in size");

InetAddress::InetAddress()
{
    ::bzero(&address_, sizeof address_);
    address_.sin_family = AF_INET;
    address_.sin_addr.s_addr = HostToNetwork32(kInaddrAny);
}

InetAddress::InetAddress(uint16_t port__)
{
    ::bzero(&address_, sizeof address_);
    address_.sin_family = AF_INET;
    address_.sin_addr.s_addr = HostToNetwork32(kInaddrAny);
    address_.sin_port = HostToNetwork16(port__);
}

InetAddress::InetAddress(const StringPiece& ip__, uint16_t port__)
{
    detail::FromIpPort(ip__, port__, &address_);
}

InetAddress::InetAddress(const StringPiece& address)
{
    std::vector<std::string> result;
    boost::split(result, address, boost::is_any_of(":"));
    CHECK_EQ(result.size(), 2);

    auto saved_errno = errno;
    errno = 0;

    uint16_t port__ = static_cast<uint16_t>(::strtoul(result[1].c_str(), NULL, 0));
    CHECK_NE(errno, ERANGE);
    CHECK_NE(errno, EINVAL);

    errno = saved_errno;

    detail::FromIpPort(result[0], port__, &address_);
}

std::string InetAddress::ip() const
{
    char buf[32];
    ::inet_ntop(AF_INET, &address_.sin_addr, buf, sizeof buf);
    return buf;
}

int InetAddress::IpAsInt() const
{
    return NetworkToHost32(address_.sin_addr.s_addr);
}

uint16_t InetAddress::port() const
{
    return NetworkToHost16(address_.sin_port);
}

std::string InetAddress::ToString() const
{
    char buf[32];
    snprintf(buf, sizeof buf, "%s:%u", ip().c_str(), port());
    return buf;
}

} // namespace claire
