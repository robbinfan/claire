// Copyright (c) 2013 The claire-netty Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include <claire/netty/Connector.h>

#include <boost/bind.hpp>
#include <boost/random/uniform_int_distribution.hpp>

#include <claire/common/events/Channel.h>
#include <claire/common/events/EventLoop.h>
#include <claire/common/logging/Logging.h>
#include <claire/common/base/WeakCallback.h>
#include <claire/netty/Socket.h>

namespace claire {

Connector::Connector(EventLoop* loop, const InetAddress& server_address)
    : loop_(loop),
      server_address_(server_address),
      connect_(false),
      state_(kDisconnected)
{}

Connector::~Connector()
{
    CHECK(!channel_) << "Connector destruct in use ";
}

void Connector::Connect()
{
    if (connect_)
    {
        return ;
    }
    connect_ = true;

    loop_->Run(
        boost::bind<void>(MakeWeakCallback(&Connector::ConnectInLoop, shared_from_this())));
}

void Connector::ConnectInLoop()
{
    loop_->AssertInLoopThread();
    CHECK(state_ == kDisconnected);

    if (!connect_)
    {
        LOG(DEBUG) << "do not connnect";
        return ;
    }

    // Connector not own the socket fd all its lifecycle
    auto socket(Socket::NewNonBlockingSocket(true));
    int ret = socket->Connect(server_address_);
    int saved_errno = (ret == 0) ? 0 : errno;
    switch (saved_errno)
    {
        case 0:
        case EINPROGRESS:
        case EINTR:
        case EISCONN:
            {
                set_state(kConnecting);
                channel_.reset(new Channel(loop_, socket->fd()));
                channel_->set_write_callback(
                    boost::bind<void>(MakeWeakCallback(&Connector::OnWrite, shared_from_this())));
                channel_->set_error_callback(
                    boost::bind<void>(MakeWeakCallback(&Connector::OnError, shared_from_this())));
                channel_->EnableWriting();
                socket->Reset(); // now socket release the ownership of fd

                if (saved_errno)
                {
                    connect_timer_ = loop_->RunAfter(3000, // FIXME
                                                     boost::bind(&Connector::OnConnectTimeout, this));
                }
            }
            break;
        case EAGAIN:
        case EADDRINUSE:
        case EADDRNOTAVAIL:
        case ECONNREFUSED:
        case ENETUNREACH:
            ResetAndRetry();
            break;
        default:
            PLOG(ERROR) << "Unexpected error in Connector::ConnectInLoop ";
            break;
    }
}

void Connector::Stop()
{
    if (!connect_)
    {
        return ;
    }
    connect_ = false;

    loop_->Run(
        boost::bind(&Connector::StopInLoop, shared_from_this()));
}

void Connector::StopInLoop()
{
    loop_->AssertInLoopThread();

    if (state_ == kConnecting)
    {
        set_state(kDisconnected);
        ::close(channel_->fd());
        ResetAndRetry();
    }
}

void Connector::Restart()
{
    loop_->AssertInLoopThread();

    set_state(kDisconnected);
    connect_ = true;
    ConnectInLoop();
}

void Connector::OnWrite()
{
    DCHECK(!!channel_) << "channel must not be NULL";
    LOG(DEBUG) << "Connector::OnWrite fd = " << channel_->fd();

    if (connect_timer_.Valid())
    {
        loop_->Cancel(connect_timer_);
        connect_timer_.Reset();
    }

    if (state_ == kConnecting)
    {
        Socket socket(channel_->fd());
        RemoveAndResetChannel();
        if (socket.ErrorCode())
        {
            LOG(WARNING) << "Connector::OnWrite - SO_ERROR = "
                         << strerror_tl(socket.ErrorCode());
            ResetAndRetry();
        }
        else if (socket.IsSelfConnect())
        {
            LOG(WARNING) << "Connector::OnWrite - Self connect";
            ResetAndRetry();
        }
        else
        {
            set_state(kConnected);
            if (connect_ && new_connection_callback_)
            {
                new_connection_callback_(socket);
            }
        }
    }
    else
    {
        // what happened?
        DCHECK(state_ == kDisconnected);
    }
}

void Connector::OnError()
{
    DCHECK(!!channel_) << "channel must not be NULL";
    DCHECK_EQ(state_, kConnecting) << "state not kConnecting: " << state_;
    LOG(ERROR) << "Connector::OnError fd = " << channel_->fd();

    {
        Socket socket(channel_->fd());
        LOG(TRACE) << "SO_ERROR = " << strerror_tl(socket.ErrorCode());
    }
    ResetAndRetry();
}

void Connector::OnConnectTimeout()
{
    connect_timer_.Reset();
    OnError();
}

void Connector::RemoveAndResetChannel()
{
    channel_->DisableAll();
    channel_->Remove();
    loop_->Post(boost::bind(&Connector::ResetChannel, shared_from_this()));
}

void Connector::ResetAndRetry()
{
    RemoveAndResetChannel();
    set_state(kDisconnected);
    if (!connect_)
    {
        return ;
    }

    boost::random::uniform_int_distribution<> dist(1, 3000);
    auto retry_delay = dist(gen_);
    LOG(INFO) << "Connector::ResetAndRetry - Retry connecting to " << server_address_.ToString()
              << " in " << retry_delay << " milliseconds. ";

    if (retry_timer_.Valid())
    {
        loop_->Cancel(retry_timer_);
        retry_timer_.Reset();
    }
    retry_timer_ = loop_->RunAfter(retry_delay,
                                   boost::bind(&Connector::ConnectInLoop, this));
}

void Connector::ResetChannel()
{
    channel_.reset();
}

} // namespace claire
