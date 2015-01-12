// Copyright (c) 2013 The claire-common Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include <claire/netty/UdpClient.h>
#include <claire/netty/Socket.h>
#include <claire/common/events/EventLoop.h>
#include <claire/common/events/Channel.h>
#include <claire/common/events/TimerId.h>
#include <claire/common/logging/Logging.h>

#include <boost/bind.hpp>

namespace claire {

struct UdpClient::OutstandingCall
{
    OutstandingCall() {}
    OutstandingCall(Buffer && buffer__,
                    const InetAddress& server_address__,
                    TimerId timer_id__)
        : server_address(server_address__),
          timer_id(timer_id__)
    {
        buffer.swap(buffer__);
    }

    void swap(OutstandingCall& other)
    {
        buffer.swap(other.buffer);
        server_address.swap(other.server_address);
        timer_id.swap(other.timer_id);
    }

    // implicit copy-ctor, move-ctor, dtor and assignment are fine
    // NOTE: implicit move-ctor is added in g++ 4.6

    Buffer buffer;
    InetAddress server_address;
    TimerId timer_id;
};

UdpClient::UdpClient(EventLoop* loop, const std::string& name__)
    : loop_(loop),
      name_(name__),
      socket_(Socket::NewNonBlockingSocket(false)),
      channel_(new Channel(loop, socket_->fd())),
      started_(false),
      timeout_(0)
{
    socket_->BindOrDie(InetAddress());
    channel_->set_read_callback(
        boost::bind(&UdpClient::OnRead, this));
    LOG(DEBUG) << name_ << "bind to local address " << socket_->local_address().ToString();
}

UdpClient::~UdpClient()
{
    LOG(TRACE) << "UdpClient::~UdpClient [" << name_ << "] destructing";

    started_ = false;
    std::map<int64_t, OutstandingCall> calls;
    {
        MutexLock lock(mutex_);
        calls.swap(outstanding_calls_);
    }

    for (auto it = calls.begin(); it != calls.end(); ++it)
    {
        LOG(DEBUG) << "message hash code " << it->first << " canceled";
        loop_->Cancel(it->second.timer_id);
    }
}

void UdpClient::Start()
{
    if (started_)
    {
        return ;
    }
    started_ = true;

    loop_->Run(
        boost::bind(&UdpClient::StartInLoop, this)); //FIXME
}

void UdpClient::StartInLoop()
{
    loop_->AssertInLoopThread();
    channel_->EnableReading();
}

void UdpClient::Send(Buffer* buffer, const InetAddress& server_address)
{
    if (!started_)
    {
        return ;
    }

    socket_->sendto(buffer->Peek(), buffer->ReadableBytes(), server_address);
    auto hash_code = hash_code_function_(buffer->Peek(), buffer->ReadableBytes());
    LOG(DEBUG) << name_ << " sendto " << server_address.ToString() <<", hash code " << hash_code;

    if (timeout_ > 0)
    {
        auto timer_id = loop_->RunAfter(
            static_cast<int>(timeout_), boost::bind(&UdpClient::OnTimeout, this, hash_code));
        {
            MutexLock lock(mutex_);
            if (!started_)
            {
                return ;
            }
            DCHECK(outstanding_calls_.find(hash_code) == outstanding_calls_.end());

            //FIXME: g++ 4.8 support emplace
            auto ret = outstanding_calls_.insert(
                std::make_pair(hash_code, OutstandingCall(std::move(*buffer), server_address, timer_id)));
            DCHECK(ret.second);
        }
    }

    buffer->ConsumeAll();
    return ;
}

void UdpClient::Send(const void* data, size_t length, const InetAddress& server_address)
{
    if (!started_)
    {
        return ;
    }
    socket_->sendto(data, length, server_address);
    auto hash_code = hash_code_function_(data, length);
    LOG(DEBUG) << name_ << " sendto " << server_address.ToString() <<", hash code " << hash_code;

    if (timeout_ > 0)
    {
        auto timer_id = loop_->RunAfter(
            static_cast<int>(timeout_), boost::bind(&UdpClient::OnTimeout, this, hash_code));
        {
            MutexLock lock(mutex_);
            if (!started_)
            {
                return ;
            }
            DCHECK(outstanding_calls_.find(hash_code) == outstanding_calls_.end());

            //FIXME: g++ 4.8 support emplace
            auto ret = outstanding_calls_.insert(
                std::make_pair(hash_code, OutstandingCall(Buffer(data, length), server_address, timer_id)));
            DCHECK(ret.second);
        }
    }

    return ;
}

void UdpClient::OnRead()
{
    loop_->AssertInLoopThread();

    InetAddress peer_address;
    auto length = socket_->Read(&input_buffer_, &peer_address);
    if (length > 0)
    {
        bool match = true;
        auto hash_code = hash_code_function_(input_buffer_.Peek(), input_buffer_.ReadableBytes());
        {
            MutexLock lock(mutex_);
            auto it = outstanding_calls_.find(hash_code);
            if (it != outstanding_calls_.end())
            {
                loop_->Cancel(it->second.timer_id);
                outstanding_calls_.erase(it);
            }
            else
            {
                match = false;
            }
        }

        if (match)
        {
            LOG(DEBUG) << name_ << " receive message from " << peer_address.ToString() << ", hash code " << hash_code
                       << " match existing outstanding call";

            if (message_callback_)
            {
                message_callback_(&input_buffer_, peer_address);
            }
        }
        else
        {
            LOG(DEBUG) << name_ << " receive message from " << peer_address.ToString() << ", hash code " << hash_code
                       << " not match any outstanding call";
        }
        input_buffer_.ConsumeAll();
    }
    else
    {
        if (errno != EAGAIN)
        {
            LOG(ERROR) << "UdpClient::OnRead [" << name_
                       << "] SO_ERROR = " << socket_->ErrorCode() << " " << strerror_tl(socket_->ErrorCode());
        }
    }
}

void UdpClient::OnTimeout(int64_t hash_code)
{
    loop_->AssertInLoopThread();

    OutstandingCall call;
    {
        MutexLock lock(mutex_);
        auto it = outstanding_calls_.find(hash_code);
        if  (it == outstanding_calls_.end())
        {
            DCHECK(!started_);
            return;
        }

        call.swap(it->second);
        outstanding_calls_.erase(it);
    }

    LOG(ERROR) << name_ <<  " message hash code " << hash_code << " timeout";
    if (timeout_callback_)
    {
        timeout_callback_(&call.buffer, call.server_address);
    }
}

} // namespace claire
