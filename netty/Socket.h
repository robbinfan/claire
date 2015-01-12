// Copyright (c) 2013 The claire-netty Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef _CLAIRE_NETTY_SOCKET_H_
#define _CLAIRE_NETTY_SOCKET_H_

#include <memory>
#include <algorithm>

#include <boost/noncopyable.hpp>

#include <claire/netty/InetAddress.h>

struct tcp_info;

namespace claire {

class Buffer;

/// Wrapper of socket file descriptor.
/// It closes the sockfd when desctructs.
/// It's thread safe, all operations are delagated to OS.
class Socket : boost::noncopyable
{
public:
    static std::unique_ptr<Socket> NewNonBlockingSocket(bool is_tcp);

    Socket()
        : fd_(-1)
    {}

    explicit Socket(int fd__)
        : fd_(fd__)
    {}

    Socket(Socket&& other)
        : fd_(-1)
    {
        std::swap(fd_, other.fd_);
    }

    ~Socket();

    int fd() const { return fd_; }
    void Reset() { fd_ = -1; }

    /// abort if address in use
    void BindOrDie(const InetAddress& local_address);

    /// abort if address in use
    void ListenOrDie();

    /// On success, returns a non-negative integer that is
    /// a descriptor for the accepted socket, which has been
    /// set to non-blocking and close-on-exec. *from is assigned.
    /// On error, -1 is returned, and *from is untouched.
    int AcceptOrDie(InetAddress* peer_address);

    int Connect(const InetAddress& server_address);

    /// half close the socket
    void ShutdownWrite();

    ssize_t Read(void* buffer, size_t length);
    ssize_t Read(Buffer* buffer, InetAddress* peer_address);
    ssize_t Write(const void* buffer, size_t length);
    ssize_t Write(Buffer* buffer);
    ssize_t sendto(const void* buffer, size_t length, const InetAddress& server_address);

    /// Get local InetAddress
    const InetAddress local_address() const;

    /// Get peer InetAddress
    const InetAddress peer_address() const;

    bool IsSelfConnect() const { return local_address() == peer_address(); }

    /// Get the errno of socket fd_
    int ErrorCode() const;

    void SetTcpNoDelay(bool on);
    void SetReuseAddr(bool on);
    void SetKeepAlive(bool on);

    void SetReusePort(bool on);

    bool GetTcpInfo(struct tcp_info*) const;
    bool GetTcpInfoString(char* buffer, int length) const;
private:
    int fd_;
};

} // namespace claire

#endif // _CLAIRE_NETTY_SOCKET_H_
