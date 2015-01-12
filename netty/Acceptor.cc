// Copyright (c) 2013 The claire-netty Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include <claire/netty/Acceptor.h>

#include <boost/bind.hpp>

#include <claire/common/events/Channel.h>
#include <claire/common/events/EventLoop.h>
#include <claire/common/logging/Logging.h>
#include <claire/netty/Socket.h>
#include <claire/netty/InetAddress.h>

namespace claire {

Acceptor::Acceptor(EventLoop* loop,
                   const InetAddress& listen_address,
                   bool reuse_port)
    : loop_(loop),
      accept_socket_(Socket::NewNonBlockingSocket(true)),
      accept_channel_(new Channel(loop, accept_socket_->fd())),
      listenning_(false)
{
    accept_socket_->SetReuseAddr(true);
    accept_socket_->SetReusePort(reuse_port);
    accept_socket_->BindOrDie(listen_address);

    accept_channel_->set_read_callback(
        boost::bind(&Acceptor::OnRead, this));
}

Acceptor::~Acceptor()
{
    accept_channel_->DisableAll();
    accept_channel_->Remove();
}

void Acceptor::Listen()
{
    loop_->AssertInLoopThread();

    if (!listenning_)
    {
        listenning_ = true;
        accept_socket_->ListenOrDie();
        accept_channel_->EnableReading();
    }
}

void Acceptor::OnRead()
{
    loop_->AssertInLoopThread();

    InetAddress from;
    Socket socket(accept_socket_->AcceptOrDie(&from));
    if (socket.fd() >= 0)
    {
        if (new_connection_callback_)
        {
            new_connection_callback_(socket);
        }
    }
    else
    {
        if (errno == EMFILE)
        {
            // In multi-thread env, no good solution
            PLOG(ERROR) << "accept overload";
        }
    }
}

} // namespace claire
