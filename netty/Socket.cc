// Copyright (c) 2013 The claire-netty Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include <claire/netty/Socket.h>

#include <netinet/in.h>
#include <netinet/tcp.h>

#include <stdio.h>
#include <strings.h>

#include <claire/common/base/Types.h>
#include <claire/common/logging/Logging.h>
#include <claire/netty/Buffer.h>

namespace claire {

namespace {

typedef struct sockaddr SA;
const SA* sockaddr_cast(const struct sockaddr_in* addr)
{
    return static_cast<const SA*>(implicit_cast<const void*>(addr));
}

SA* sockaddr_cast(struct sockaddr_in* addr)
{
    return static_cast<SA*>(implicit_cast<void*>(addr));
}

template <int Type = SOCK_STREAM, int Protocol = IPPROTO_TCP>
int CreateNonBlockingOrDie()
{
    int fd = ::socket(AF_INET, Type | SOCK_NONBLOCK | SOCK_CLOEXEC, Protocol);
    if (fd < 0)
    {
        LOG(FATAL) << "socket failed";
    }

    return fd;
}

} // namespace

// static
std::unique_ptr<Socket> Socket::NewNonBlockingSocket(bool is_tcp)
{
    if (is_tcp)
    {
        return std::unique_ptr<Socket>(new Socket(CreateNonBlockingOrDie()));
    }
    else
    {
        return std::unique_ptr<Socket>(new Socket(CreateNonBlockingOrDie<SOCK_DGRAM, IPPROTO_UDP>()));
    }
}

Socket::~Socket()
{
    if (fd_ > 0)
    {
        if (::close(fd_) < 0)
        {
            PLOG(ERROR) << "close " << fd_ << " failed ";
        }
    }
}

void Socket::BindOrDie(const InetAddress& local_address__)
{
    int ret = ::bind(fd_, sockaddr_cast(&local_address__.sockaddr()), sizeof local_address__);
    if (ret < 0)
    {
        PLOG(FATAL) << "bind failed ";
    }
}

void Socket::ListenOrDie()
{
    int ret = ::listen(fd_, SOMAXCONN);
    if (ret < 0)
    {
        PLOG(FATAL) << "listen failed";
    }
}

int Socket::AcceptOrDie(InetAddress *peer_address__)
{
    auto length = static_cast<socklen_t>(sizeof *peer_address__);
    int nfd = ::accept4(fd_,
                        sockaddr_cast(peer_address__->mutable_sockaddr()),
                        &length,
                        SOCK_NONBLOCK | SOCK_CLOEXEC);
    if (nfd < 0)
    {
        int saved_errno = errno;
        PLOG(ERROR) << "accept failed ";

        switch (saved_errno)
        {
            case EAGAIN:
            case ECONNABORTED:
            case EINTR:
            case EPROTO:
            case EPERM:
            case EMFILE:
                errno = saved_errno;
                break;
            default:
                LOG(FATAL) << "unknown error of ::accept" << saved_errno;
                break;
        }
    }

    return nfd;
}

void Socket::ShutdownWrite()
{
    if (::shutdown(fd_, SHUT_WR) < 0)
    {
        PLOG(ERROR) << "shutdown failed ";
    }
}

const InetAddress Socket::local_address() const
{
    struct sockaddr_in address;
    ::bzero(&address, sizeof address);

    socklen_t length = sizeof address;
    if (::getsockname(fd_, sockaddr_cast(&address), &length) < 0)
    {
        PLOG(ERROR) << "getsockname failed ";
    }

    return address;
}

const InetAddress Socket::peer_address() const
{
    struct sockaddr_in address;
    ::bzero(&address, sizeof address);

    socklen_t length = sizeof address;
    if (::getpeername(fd_, sockaddr_cast(&address), &length) < 0)
    {
        PLOG(ERROR) << "getpeername failed ";
    }

    return address;
}

int Socket::Connect(const InetAddress& server_address)
{
    return ::connect(fd_,
                     sockaddr_cast(&server_address.sockaddr()),
                     static_cast<socklen_t>(sizeof server_address));
}

ssize_t Socket::Read(void* buffer, size_t length)
{
    return ::read(fd_, buffer, length);
}

ssize_t Socket::Read(Buffer* buffer, InetAddress* peer_address__)
{
    char extra[65536];
    size_t writable = buffer->WritableBytes();

    struct iovec vec[2];
    vec[0].iov_base = buffer->BeginWrite();
    vec[0].iov_len = writable;
    vec[1].iov_base = extra;
    vec[1].iov_len = sizeof extra;

    struct msghdr hdr;
    ::bzero(&hdr, sizeof hdr);

    hdr.msg_name = peer_address__->mutable_sockaddr();
    hdr.msg_namelen = peer_address__ ? static_cast<socklen_t>(sizeof *peer_address__) : 0;

    hdr.msg_iov = vec;
    hdr.msg_iovlen = (writable < sizeof extra) ? 2 : 1;

    ssize_t n = ::recvmsg(fd_, &hdr, 0);
    if (n < 0)
    {
        PLOG(ERROR) << "recvmsg failed ";
        return n;
    }

    if (implicit_cast<size_t>(n) <= writable)
    {
        buffer->HasWritten(static_cast<size_t>(n));
    }
    else
    {
        buffer->HasWritten(writable);
        buffer->Append(extra, static_cast<size_t>(n) - writable);
    }

    return n;
}

ssize_t Socket::Write(const void* buffer, size_t length)
{
    return ::write(fd_, buffer, length);
}

ssize_t Socket::Write(Buffer* buffer)
{
    return ::write(fd_, buffer->Peek(), buffer->ReadableBytes());
}

ssize_t Socket::sendto(const void * data, size_t length, const InetAddress& server_address)
{
    return ::sendto(fd_, data, length, 0, sockaddr_cast(&server_address.sockaddr()), sizeof(server_address.sockaddr()));
}

int Socket::ErrorCode() const
{
    int option = 0;
    auto length = static_cast<socklen_t>(sizeof option);

    if (::getsockopt(fd_, SOL_SOCKET, SO_ERROR, &option, &length) < 0)
    {
        return errno;
    }
    else
    {
        return option;
    }
}

void Socket::SetTcpNoDelay(bool on)
{
    int option = on ? 1 : 0;
    ::setsockopt(fd_, IPPROTO_TCP, TCP_NODELAY,
                 &option, static_cast<socklen_t>(sizeof option));
}

void Socket::SetReuseAddr(bool on)
{
    int option = on ? 1 : 0;
    ::setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR,
                 &option, static_cast<socklen_t>(sizeof option));
}

void Socket::SetKeepAlive(bool on)
{
    int option = on ? 1 : 0;
    ::setsockopt(fd_, SOL_SOCKET, SO_KEEPALIVE,
                 &option, static_cast<socklen_t>(sizeof option));
}

void Socket::SetReusePort(bool on)
{
#ifdef SO_REUSEPORT
    int option = on ? 1: 0;
    ::setsockopt(fd_, SOL_SOCKET, SO_KEEPALIVE,
                 &option, static_cast<socklen_t>(sizeof option));
#else
    LOG(ERROR) << "Not Support SO_REUSEPORT";
#endif
}

bool Socket::GetTcpInfo(struct tcp_info* tcpi) const
{
    socklen_t len = sizeof(*tcpi);
    bzero(tcpi, len);
    return ::getsockopt(fd_, SOL_TCP, TCP_INFO, tcpi, &len) == 0;
}

bool Socket::GetTcpInfoString(char* buffer, int length) const
{
    struct tcp_info tcpi;
    bool ok = GetTcpInfo(&tcpi);
    if (ok)
    {
        snprintf(buffer, length, "unrecovered=%u "
                 "rto=%u ato=%u snd_mss=%u rcv_mss=%u "
                 "lost=%u retrans=%u rtt=%u snd_rttvar=%u "
                 "snd_sshthresh=%u snd_cwnd=%u total_retrans=%u",
                 tcpi.tcpi_retransmits,     // Number of unrecovered [RTO] timeouts
                 tcpi.tcpi_rto,             // Retransmit timeout in usec
                 tcpi.tcpi_ato,             // Predicted tick of soft clock in usec
                 tcpi.tcpi_snd_mss,
                 tcpi.tcpi_rcv_mss,
                 tcpi.tcpi_lost,            // Lost packets
                 tcpi.tcpi_retrans,         // Retransmitted packets out
                 tcpi.tcpi_rtt,             // Smoothed round trip time in usec
                 tcpi.tcpi_rttvar,          // Medium deviation
                 tcpi.tcpi_snd_ssthresh,
                 tcpi.tcpi_snd_cwnd,
                 tcpi.tcpi_total_retrans);  // Total retransmits for entire connection
    }
    return ok;
}

} // namespace claire
