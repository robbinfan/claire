#include <claire/netty/TcpClient.h>

#include <stdio.h>

#include <boost/bind.hpp>
#include <boost/make_shared.hpp>

#include <claire/netty/Connector.h>
#include <claire/netty/Socket.h>
#include <claire/netty/TcpConnection.h>
#include <claire/common/base/WeakCallback.h>
#include <claire/common/events/EventLoop.h>
#include <claire/common/threading/Mutex.h>
#include <claire/common/logging/Logging.h>

namespace claire {

class TcpClient::Impl : boost::noncopyable,
                        public boost::enable_shared_from_this<TcpClient::Impl>
{
public:
    Impl(EventLoop* loop__, const std::string& name__)
        : loop_(loop__),
          name_(name__),
          retry_(false),
          connect_(false),
          next_id_(1)
    {}

    ~Impl()
    {
        MutexLock lock(mutex_);
        if (!connection_ && connector_)
        {
            connector_->Stop();
        }
    }

    void Connect(const InetAddress& server_address)
    {
        LOG(INFO) << "TcpClient::Connect[" << name_ << "] - connecting to "
                  << server_address.ToString();

        connect_ = true;
        connector_.reset(new Connector(loop_, server_address));
        connector_->set_new_connection_callback(
            boost::bind(&Impl::NewConnection, this, _1));
        connector_->Connect();
    }

    void Shutdown()
    {
        connect_ = false;
        {
            MutexLock lock(mutex_);
            if (connection_)
            {
                connection_->Shutdown();
            }
        }
    }

    void Stop()
    {
        connect_ = false;
        if (connector_)
        {
            connector_->Stop();
        }
    }

    bool retry() const { return retry_; }
    void set_retry(bool on) { retry_ = on; }

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

    TcpConnectionPtr connection() const
    {
        MutexLock lock(mutex_);
        return connection_;
    }

    EventLoop* loop() { return loop_; }

private:
    void NewConnection(Socket& socket)
    {
        loop_->AssertInLoopThread();

        // FIXME poll with zero timeout to double confirm the new connection
        TcpConnectionPtr connection__(
            boost::make_shared<TcpConnection>(loop_,
                                              std::move(socket),
                                              next_id_++));
        connection__->set_connection_callback(connection_callback_);
        connection__->set_message_callback(message_callback_);
        connection__->set_write_complete_callback(write_complete_callback_);

        auto callback = MakeWeakCallback(&Impl::RemoveConnection, shared_from_this());
        connection__->set_close_callback(callback);

        {
            MutexLock lock(mutex_);
            connection_ = connection__;
        }
        connection_->ConnectEstablished();
    }

    void RemoveConnection(const TcpConnectionPtr& connection__)
    {
        loop_->AssertInLoopThread();
        DCHECK(loop_ == connection__->loop());

        {
            MutexLock lock(mutex_);
            DCHECK(connection__ == connection_);
            connection_.reset();
        }

        loop_->Post(
            boost::bind(&TcpConnection::ConnectDestroyed, connection__)); // thread safe
        if (retry_ && connect_)
        {
            LOG(INFO) << "TcpClient::RemoveConnection[" << name_ << "] - Reconnecting to "
                      << connection__->peer_address().ToString();
            connector_->Restart();
        }
    }

    EventLoop* loop_;
    ConnectorPtr connector_;
    const std::string name_;

    ConnectionCallback connection_callback_;
    MessageCallback message_callback_;
    WriteCompleteCallback write_complete_callback_;

    boost::atomic<bool> retry_;
    boost::atomic<bool> connect_;

    TcpConnection::Id next_id_; // overflow is ok

    mutable Mutex mutex_;
    TcpConnectionPtr connection_; //@GUARD_BY mutex_
};

TcpClient::TcpClient(EventLoop* loop__,
                     const std::string& name__)
    : impl_(new Impl(loop__, name__)) {}

TcpClient::~TcpClient() {}

void TcpClient::Connect(const InetAddress& server_address)
{
    impl_->Connect(server_address);
}

void TcpClient::Shutdown()
{
    impl_->Shutdown();
}

void TcpClient::Stop()
{
    impl_->Stop();
}

bool TcpClient::retry() const
{
    return impl_->retry();
}

void TcpClient::set_retry(bool on)
{
    impl_->set_retry(on);
}

void TcpClient::set_connection_callback(const ConnectionCallback& callback)
{
    impl_->set_connection_callback(callback);
}

void TcpClient::set_message_callback(const MessageCallback& callback)
{
    impl_->set_message_callback(callback);
}

void TcpClient::set_write_complete_callback(const WriteCompleteCallback& callback)
{
    impl_->set_write_complete_callback(callback);
}

TcpConnectionPtr TcpClient::connection() const
{
    return impl_->connection();
}

EventLoop* TcpClient::loop()
{
    return impl_->loop();
}

} // namespace claire
