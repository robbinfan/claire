// Copyright (c) 2013 The claire-netty Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include <claire/netty/TcpServer.h>

#include <boost/bind.hpp>
#include <boost/make_shared.hpp>

#include <claire/common/events/EventLoop.h>
#include <claire/common/events/EventLoopThread.h>
#include <claire/common/events/EventLoopThreadPool.h>
#include <claire/common/base/WeakCallback.h>
#include <claire/common/logging/Logging.h>

#include <claire/netty/Socket.h>
#include <claire/netty/Acceptor.h>
#include <claire/netty/TcpConnection.h>

DEFINE_int32(max_input_connections, 10000, "max input connections");

namespace claire {

class TcpServer::Impl : boost::noncopyable,
                        public boost::enable_shared_from_this<TcpServer::Impl>
{
public:
    Impl(EventLoop* loop__,
         const InetAddress& listen_address,
         const std::string& name__,
         const Option& option)
        : loop_(loop__),
          hostport_(listen_address.ToString()),
          name_(name__),
          acceptor_(loop_, listen_address, option == kReusePort),
          thread_pool_(loop_),
          started_(false),
          next_id_(1)
    {
        acceptor_.SetNewConnectionCallback(
            boost::bind(&Impl::NewConnection, this, _1));
    }

    ~Impl()
    {
        loop_->AssertInLoopThread();
        LOG(TRACE) << "TcpServer::~TcpServer [" << name_ << "] destructing";

        for (auto ite = connections_.begin(); ite != connections_.end(); ++ite)
        {
            auto connection = (*ite).second; // thread safe
            ite->second.reset();

            connection->loop()->Run(
                boost::bind(&TcpConnection::ConnectDestroyed, connection)); // thread safe
            connection.reset();
        }
    }

    const std::string& hostport() const { return hostport_; }
    const std::string& name() const { return name_; }

    void Start()
    {
        if (started_)
        {
            return ;
        }
        started_ = true;

        thread_pool_.Start(thread_init_callback_);
        if (!acceptor_.listenning())
        {
            loop_->Run(
                boost::bind(&Impl::ListenInLoop, shared_from_this()));

        }
    }

    void set_num_threads(int num_threads)
    {
        thread_pool_.set_num_threads(num_threads);
    }

    void set_thread_init_callback(const ThreadInitCallback& callback)
    {
        thread_init_callback_ = callback;
    }

    void set_connection_callback(const ConnectionCallback& callback)
    {
        connection_callback_ = callback;
    }

    void set_message_callback(const MessageCallback& callback)
    {
        message_callback_ = callback;
    }

    void set_write_complete_callback(const WriteCompleteCallback& callback)
    {
        write_complete_callback_ = callback;
    }

    EventLoop* loop() { return loop_; }

private:
    void ListenInLoop()
    {
        loop_->AssertInLoopThread();
        acceptor_.Listen();
    }

    void NewConnection(Socket& socket)
    {
        loop_->AssertInLoopThread();

        if (s_total_connections_ >= FLAGS_max_input_connections)
        {
            LOG(WARNING) << "current connection reach max " << FLAGS_max_input_connections
                         << ", Shutdown from " << socket.peer_address().ToString();
            socket.ShutdownWrite();
            return ;
        }
        s_total_connections_++;

        EventLoop *io_loop = thread_pool_.NextLoop();
        TcpConnectionPtr connection(
            boost::make_shared<TcpConnection>(io_loop,
                                              std::move(socket),
                                              next_id_++));
        connections_[connection->id()] = connection; // thread safe

        connection->set_connection_callback(connection_callback_);
        connection->set_message_callback(message_callback_);
        connection->set_write_complete_callback(write_complete_callback_);

        auto callback = MakeWeakCallback(&Impl::RemoveConnection, shared_from_this());
        connection->set_close_callback(callback);

        io_loop->Run(
            boost::bind(&TcpConnection::ConnectEstablished, connection)); // thread safe
    }

    void RemoveConnection(const TcpConnectionPtr &connection)
    {
        auto callback = WeakCallback<Impl>(
            boost::bind(&Impl::RemoveConnectionInLoop, _1, connection->id()),
            shared_from_this());
        loop_->Run(callback);
    }

    void RemoveConnectionInLoop(TcpConnection::Id connection_id)
    {
        loop_->AssertInLoopThread();

        LOG(INFO) << "TcpServer::RemoveConnectionInLoop [" << name_
              << "] - connection " << connection_id;

        auto it = connections_.find(connection_id);
        if (connections_.find(connection_id) != connections_.end())
        {
            it->second->loop()->Post(
                boost::bind(&TcpConnection::ConnectDestroyed, it->second)); // thread safe
            connections_.erase(it);
        }
        s_total_connections_--;
    }

    typedef std::map<TcpConnection::Id, TcpConnectionPtr> ConnectionMap;

    EventLoop* loop_;
    const std::string hostport_;
    const std::string name_;

    Acceptor acceptor_;
    EventLoopThreadPool thread_pool_;

    ConnectionCallback connection_callback_;
    MessageCallback message_callback_;
    WriteCompleteCallback write_complete_callback_;
    ThreadInitCallback thread_init_callback_;

    bool started_;
    TcpConnection::Id next_id_;
    ConnectionMap connections_;

    static int32_t s_total_connections_;
};

int32_t TcpServer::Impl::s_total_connections_ = 0;

TcpServer::TcpServer(EventLoop* loop__,
                     const InetAddress& listen_address,
                     const std::string& name__,
		     const Option& option)
    : impl_(new Impl(loop__, listen_address, name__, option)) {}

TcpServer::~TcpServer() {}

void TcpServer::set_num_threads(int num_threads)
{
    impl_->set_num_threads(num_threads);
}

void TcpServer::Start()
{
    impl_->Start();
}

const std::string& TcpServer::hostport() const
{
    return impl_->hostport();
}

const std::string& TcpServer::name() const
{
    return impl_->name();
}

void TcpServer::set_thread_init_callback(const ThreadInitCallback& callback)
{
    impl_->set_thread_init_callback(callback);
}

void TcpServer::set_connection_callback(const ConnectionCallback& callback)
{
    impl_->set_connection_callback(callback);
}

void TcpServer::set_message_callback(const MessageCallback& callback)
{
    impl_->set_message_callback(callback);
}

void TcpServer::set_write_complete_callback(const WriteCompleteCallback& callback)
{
    impl_->set_write_complete_callback(callback);
}

EventLoop* TcpServer::loop()
{
    return impl_->loop();
}

} // namespace claire
