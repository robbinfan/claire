// Copyright (c) 2013 The claire-zipkin Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include <claire/zipkin/ScribeClient.h>

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/protocol/TProtocol.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TTransport.h>
#include <thrift/transport/TTransportUtils.h>

#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>

#include <claire/common/base/BlockingQueue.h>
#include <claire/common/threading/Thread.h>
#include <claire/common/time/Timestamp.h>
#include <claire/netty/InetAddress.h>

#include <claire/zipkin/scribe.h>

namespace claire {

class ScribeClient::Impl
{
public:
    Impl(const InetAddress& scribe_address)
        : socket_(new apache::thrift::transport::TSocket(scribe_address.ip(), scribe_address.port())),
          transport_(new apache::thrift::transport::TFramedTransport(socket_)),
          protocol_(new apache::thrift::protocol::TBinaryProtocol(transport_)),
          client_(protocol_, protocol_),
          io_thread_(boost::bind(&Impl::ThreadEntry, this), "ScribeClient"),
          running_(false)
    {
        TryConnect();
        if (running_)
        {
            io_thread_.Start();
        }
    }

    ~Impl()
    {
        if (running_)
        {
            running_ = false;
            io_thread_.Join();
            transport_->close();
        }
    }

    void Log(const std::string& category, const std::string& message)
    {
        scribe::thrift::LogEntry e;
        e.__set_category(category);
        e.__set_message(message);
        queue_.Put(e);

        TryConnect();
    }

private:
    void TryConnect()
    {
        if (running_)
        {
            return ;
        }

        if (last_connect_timestamp_.Valid() && TimeDifference(Timestamp::Now(), last_connect_timestamp_) < 100000)
        {
            return ;
        }

        try
        {
            transport_->open();
            running_ = true;
        }
        catch (const std::exception& e)
        {
            LOG(DEBUG) << "TryConnect Fail: " << e.what(); 
        }
    }

    void ThreadEntry()
    {
        while (running_)
        {
            std::vector<scribe::thrift::LogEntry> logs;
            logs.push_back(queue_.Take());
            client_.Log(logs);
        }
    }

    boost::shared_ptr<apache::thrift::transport::TSocket> socket_;
    boost::shared_ptr<apache::thrift::transport::TTransport> transport_;
    boost::shared_ptr<apache::thrift::protocol::TProtocol> protocol_;
    scribe::thrift::scribeClient client_;
    Thread io_thread_;
    BlockingQueue<scribe::thrift::LogEntry> queue_;
    bool running_;
    Timestamp last_connect_timestamp_;
};

ScribeClient::ScribeClient(const InetAddress& scribe_address)
    : impl_(new Impl(scribe_address))
{}

ScribeClient::~ScribeClient() {}

void ScribeClient::Log(const std::string& category, const std::string& message)
{
    impl_->Log(category, message);
}

} // namespace claire
