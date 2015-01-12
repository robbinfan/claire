// Copyright (c) 2013 The claire-netty Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include <claire/netty/TcpConnection.h>

#include <errno.h>
#include <stdio.h>

#include <boost/bind.hpp>

#include <claire/netty/Socket.h>
#include <claire/common/events/EventLoop.h>
#include <claire/common/events/Channel.h>
#include <claire/common/logging/Logging.h>
#include <claire/common/metrics/Histogram.h>

DEFINE_int32(connection_watermark, 64*1024*1024, "tcp connection high watermark");

namespace claire {

namespace {

size_t OutputBufferSize(const boost::ptr_vector<Buffer>& v)
{
    size_t n = 0;
    for (auto it = v.begin(); it != v.end(); ++it)
    {
        n += (*it).ReadableBytes();
    }
    return n;
}

} // namespace

TcpConnection::TcpConnection(EventLoop *loop__,
                             Socket&& socket,
                             Id id__)
    : loop_(loop__),
      id_(id__),
      state_(kConnecting),
      socket_(new Socket(std::forward<Socket>(socket))),
      channel_(new Channel(loop_, socket_->fd())),
      local_address_(socket_->local_address()),
      peer_address_(socket_->peer_address()),
      received_bytes_(0),
      sent_bytes_(0),
      received_bytes_counter_("claire.TcpConnection.ReceivedBytes"),
      sent_bytes_counter_("claire.TcpConnection.SentBytes")
{
    channel_->set_read_callback(
        boost::bind(&TcpConnection::OnRead, this));
    channel_->set_write_callback(
        boost::bind(&TcpConnection::OnWrite, this));
    channel_->set_close_callback(
        boost::bind(&TcpConnection::OnClose, this));
    channel_->set_error_callback(
        boost::bind(&TcpConnection::OnError, this));

    LOG(DEBUG) << "TcpConnection::TcpConnection " << peer_address_.ToString()
               << " -> " << local_address_.ToString() << " : id=" << id_
               << " fd=" << socket_->fd();

    socket_->SetKeepAlive(true);

    Counter("claire.TcpConnection.connected").Increment();
}

TcpConnection::~TcpConnection()
{
    HISTOGRAM_MEMORY_KB("claire.TcpConnection.SentBytes", sent_bytes_);
    HISTOGRAM_MEMORY_KB("claire.TcpConnection.ReceivedBytes", received_bytes_);
    Counter("claire.TcpConnection.disconnected").Increment();

    LOG(DEBUG) << "TcpConnection::TcpConnection " << peer_address_.ToString()
               << " -> " << local_address_.ToString() << " : id=" << id_
               << " fd=" << channel_->fd();
}

void TcpConnection::Send(Buffer* buffer)
{
    if (loop_->IsInLoopThread())
    {
        SendInLoop(*buffer);
    }
    else
    {
        loop_->Run(
            boost::bind(static_cast<void (TcpConnection::*) (Buffer&)>(&TcpConnection::SendInLoop),
                        shared_from_this(),
                        Buffer(std::forward<Buffer>(*buffer))));
    }
}

void TcpConnection::Send(const StringPiece& s)
{
    if (state_ != kConnected)
    {
        return ;
    }

    if (loop_->IsInLoopThread())
    {
        SendInLoop(s.data(), s.size());
    }
    else
    {
        loop_->Run(
            boost::bind(static_cast<void (TcpConnection::*) (Buffer&)>(&TcpConnection::SendInLoop),
                        shared_from_this(),
                        Buffer(s.data(), s.size())));
    }
}

void TcpConnection::SendInLoop(Buffer& buffer)
{
    if (channel_->IsWriting())
    {
        CHECK(!output_buffers_.empty());
        output_buffers_.push_back(new Buffer(std::forward<Buffer>(buffer)));
    }
    else
    {
        SendInLoop(buffer.Peek(), buffer.ReadableBytes());
        buffer.ConsumeAll();
    }
}

void TcpConnection::SendInLoop(const void* data, size_t length)
{
    loop_->AssertInLoopThread();

    if (state_ == kDisconnected)
    {
        LOG(ERROR) << "disconnected, give up writing";
        return ;
    }

    bool error = false;
    ssize_t nwrote = 0;
    size_t remaining = length;
    if (!channel_->IsWriting() && output_buffers_.empty())
    {
        nwrote = socket_->Write(data, length);
        if (nwrote >= 0)
        {
            sent_bytes_ += static_cast<int>(nwrote);
            sent_bytes_counter_.Add(static_cast<int>(nwrote));
            remaining -= nwrote;
            if (remaining == 0 && write_complete_callback_)
            {
                loop_->Run(
                    boost::bind(write_complete_callback_, shared_from_this()));
            }
        }
        else
        {
            if (errno != EWOULDBLOCK)
            {
                PLOG(ERROR) << "TcpConnection::SendInLoop";
                if (errno == EPIPE || errno == ECONNRESET)
                {
                    error = true;
                }
            }
        }
    }

    DCHECK(remaining <= length);
    if (!error && remaining > 0)
    {
        LOG(TRACE) << "writing more data";

        size_t left = OutputBufferSize(output_buffers_);
        if (left + remaining > static_cast<size_t>(FLAGS_connection_watermark)
            && left < static_cast<size_t>(FLAGS_connection_watermark)
            && high_watermark_callback_)
        {
            loop_->Post(
                boost::bind(high_watermark_callback_,
                            shared_from_this(),
                            left + remaining));
        }

        if (output_buffers_.empty())
        {
            output_buffers_.push_back(new Buffer());
        }
        output_buffers_.back().Append(static_cast<const char*>(data)+nwrote, remaining);

        if (!channel_->IsWriting())
        {
            channel_->EnableWriting();
        }
    }
}

void TcpConnection::Shutdown()
{
    auto s = kConnected;
    if (state_.compare_exchange_strong(s, kDisconnecting))
    {
        loop_->Run(
            boost::bind(&TcpConnection::ShutdownInLoop, shared_from_this()));
    }
}

void TcpConnection::ShutdownInLoop()
{
    loop_->AssertInLoopThread();

    if (!channel_->IsWriting())
    {
        socket_->ShutdownWrite();
    }
}

void TcpConnection::SetTcpNoDelay(bool on)
{
    socket_->SetTcpNoDelay(on);
}

void TcpConnection::ConnectEstablished()
{
    loop_->AssertInLoopThread();

    DCHECK(state_ == kConnecting);
    set_state(kConnected);

    channel_->tie(shared_from_this());
    channel_->EnableReading();

    if (connection_callback_)
    {
        connection_callback_(shared_from_this());
    }
}

void TcpConnection::ConnectDestroyed()
{
    loop_->AssertInLoopThread();

    if (state_ == kConnected) // call by TcpClient/TcpServer
    {
        set_state(kDisconnected);
        channel_->DisableAll();

        if (connection_callback_)
        {
            connection_callback_(shared_from_this());
        }
    }

    channel_->Remove();
}

void TcpConnection::OnRead()
{
    auto n = socket_->Read(&input_buffer_, NULL);
    if (n > 0)
    {
        received_bytes_ += static_cast<int>(n);
        received_bytes_counter_.Add(static_cast<int>(n));
        if (message_callback_)
        {
            message_callback_(shared_from_this(), &input_buffer_);
        }
        else
        {
            input_buffer_.ConsumeAll();
        }
    }
    else if (n == 0)
    {
        OnClose();
    }
    else
    {
        OnError();
    }
}

void TcpConnection::OnWrite()
{
    if (channel_->IsWriting())
    {
        while (!output_buffers_.empty())
        {
            auto n = socket_->Write(&output_buffers_[0]);
            if (n > 0)
            {
                sent_bytes_ += static_cast<int>(n);
                sent_bytes_counter_.Add(static_cast<int>(n));
                output_buffers_[0].Consume(n);
                if (output_buffers_[0].ReadableBytes() == 0)
                {
                    output_buffers_.erase(output_buffers_.begin());
                }
                else
                {
                    LOG(TRACE) << "need write more data";
                    break;
                }
            }
        }

        if (output_buffers_.empty())
        {
            channel_->DisableWriting();
            if (write_complete_callback_)
            {
                loop_->Run(
                            boost::bind(write_complete_callback_, shared_from_this()));
            }

            if (state_ == kDisconnecting)
            {
                ShutdownInLoop();
            }
        }
    }
    else
    {
        LOG(ERROR) << "Connection is down, no more writing";
    }
}

void TcpConnection::OnClose()
{
    LOG(TRACE) << "TcpConnection::OnClose " << peer_address_.ToString()
               << " -> " << local_address_.ToString() << " id=" << id_
               << " state = " << state_;

    DCHECK(state_ != kDisconnected);
    set_state(kDisconnected);
    channel_->DisableAll();

    auto guard(shared_from_this());
    if (connection_callback_)
    {
        connection_callback_(guard);
    }

    if (close_callback_)
    {
        close_callback_(guard);
    }
}

void TcpConnection::OnError()
{
    LOG(TRACE) << "TcpConnection::OnError " << peer_address_.ToString()
               << " -> " << local_address_.ToString() << " id=" << id_
               << "] - SO_ERROR = " << strerror_tl(socket_->ErrorCode());
}

std::string TcpConnection::GetTcpInfoString() const
{
    char buffer[1024];
    buffer[0] = '\0';
    socket_->GetTcpInfoString(buffer, sizeof buffer);
    return buffer;
}

} // namespace claire
