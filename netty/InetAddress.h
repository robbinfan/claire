// Copyright (c) 2013 The claire-netty Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef _CLAIRE_NETTY_INETADDRESS_H_
#define _CLAIRE_NETTY_INETADDRESS_H_

#include <netinet/in.h>

#include <algorithm>
#include <string>

#include <claire/common/strings/StringPiece.h>

namespace claire {

/// Wrapper of sockaddr_in
class InetAddress
{
public:
    InetAddress();

    /// Constructs an endpoint with given port number.
    /// Mostly used in TcpServer listening.
    explicit InetAddress(uint16_t port);

    /// Constructs an endpoint with given ip and port.
    /// @c ip should be "1.2.3.4"
    InetAddress(const StringPiece& ip, uint16_t port);

    /// Constructs an endpoint with given struct @c sockaddr_in
    /// Mostly used when accepting new connections
    InetAddress(const struct sockaddr_in& address)
        : address_(address)
    {}

    /// @c address should be "1.2.3.4:56"
    InetAddress(const StringPiece& address);

    std::string ip() const;
    int IpAsInt() const;
    uint16_t port() const;

    std::string ToString() const;

    const struct sockaddr_in& sockaddr() const
    {
        return address_;
    }

    struct sockaddr_in* mutable_sockaddr()
    {
        return &address_;
    }

    void swap(InetAddress& other)
    {
        std::swap(address_, other.address_);
    }

private:
    struct sockaddr_in address_;
};

inline bool operator==(const InetAddress& lhs, const InetAddress& rhs)
{
    return memcmp(&lhs, &rhs, sizeof lhs) == 0;
}

inline bool operator<(const InetAddress& lhs, const InetAddress& rhs)
{
    return memcmp(&lhs, &rhs, sizeof lhs) < 0;
}

} // namespace claire

#endif // _CLAIRE_NETTY_INETADDRESS_H_
