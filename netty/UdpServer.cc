// Copyright (c) 2013 The claire-netty Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include <claire/netty/UdpServer.h>

#include <boost/bind.hpp>

#include <claire/common/events/EventLoop.h>
#include <claire/common/events/Channel.h>
#include <claire/common/logging/Logging.h>
#include <claire/netty/InetAddress.h>
#include <claire/netty/Socket.h>

namespace claire {

UdpServer::UdpServer(EventLoop* loop,
                     const InetAddress& listen_address,
                     const std::string& name__)
    : loop_(loop),
      socket_(Socket::NewNonBlockingSocket(false)),
      channel_(new Channel(loop, socket_->fd())),
      name_(name__),
      local_address_(listen_address.ToString()),
      started_(false),
      received_bytes_counter_("claire.UdpServer.received_bytes"),
      sent_bytes_counter_("claire.UdpServer.sent_bytes")
{
    socket_->SetReuseAddr(true);
    socket_->BindOrDie(listen_address);
    channel_->set_read_callback(
        boost::bind(&UdpServer::OnRead, this));
}

UdpServer::~UdpServer()
{
    LOG(TRACE) << "Udperver::~UdpServer [" << name_ << "] destructing";
}

void UdpServer::Start()
{
    if (started_)
    {
        return ;
    }
    started_ = true;

    loop_->Run(
        boost::bind(&UdpServer::StartInLoop, this)); //FIXME
}

void UdpServer::StartInLoop()
{
    loop_->AssertInLoopThread();
    channel_->EnableReading();
}

void UdpServer::Send(Buffer* buffer, const InetAddress& server_address)
{
    Send(buffer->Peek(), buffer->ReadableBytes(), server_address);
    buffer->ConsumeAll();
}

void UdpServer::Send(const void* data,
                     size_t length,
                     const InetAddress& server_address)
{
    auto n = socket_->sendto(data, length, server_address);
    DCHECK(n == static_cast<int>(length));
    sent_bytes_counter_.Add(static_cast<int>(n));
}

void UdpServer::OnRead()
{
    loop_->AssertInLoopThread();

    InetAddress peer_address;
    auto length = socket_->Read(&input_buffer_, &peer_address);
    if (length > 0)
    {
        received_bytes_counter_.Add(static_cast<int>(length));
        if (message_callback_)
        {
            message_callback_(&input_buffer_, peer_address);
        }
        input_buffer_.ConsumeAll();
    }
    else
    {
        if (errno != EAGAIN)
        {
            LOG(ERROR) << "UdpServer::OnRead [" << name_
                       << "] SO_ERROR = " <<  socket_->ErrorCode() << " " << strerror_tl(socket_->ErrorCode());
        }
    }
}

} // namespace claire
