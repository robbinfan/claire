// Copyright (c) 2013 The claire-protorpc Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include <claire/protorpc/RpcChannel.h>

#include "thirdparty/gflags/gflags.h"

#include <string>
#include <unordered_map>

#include <boost/bind.hpp>
#include <boost/atomic.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

#include <claire/common/threading/Mutex.h>
#include <claire/common/events/EventLoop.h>
#include <claire/common/logging/Logging.h>
#include <claire/common/metrics/Counter.h>
#include <claire/common/metrics/Histogram.h>
#include <claire/common/tracing/Tracing.h>

#include <claire/netty/Buffer.h>
#include <claire/netty/InetAddress.h>
#include <claire/netty/http/HttpClient.h>
#include <claire/netty/http/HttpRequest.h>
#include <claire/netty/http/HttpResponse.h>
#include <claire/netty/http/HttpConnection.h>

#include <claire/netty/resolver/Resolver.h>
#include <claire/netty/resolver/ResolverFactory.h>
#include <claire/netty/loadbalancer/LoadBalancer.h>
#include <claire/netty/loadbalancer/LoadBalancerFactory.h>

#include <claire/protorpc/RpcUtil.h>
#include <claire/protorpc/RpcCodec.h>
#include <claire/protorpc/RpcController.h>
#include <claire/protorpc/rpcmessage.pb.h> // protoc-rpc-gen
#include <claire/protorpc/builtin_service.pb.h>

DEFINE_int32(claire_RpcChannel_trace_rate, 1000, "RpcChannel trace per trace_rate");

namespace claire {
namespace protorpc {

namespace {

int64_t GetRequestTimeout(const ::google::protobuf::MethodDescriptor* method)
{
   if (method->options().HasExtension(method_timeout))
   {
        return method->options().GetExtension(method_timeout);
   }
   return method->service()->options().GetExtension(service_timeout);
}

} // namespace

class RpcChannel::Impl : public boost::enable_shared_from_this<RpcChannel::Impl>
{
public:
    Impl(EventLoop* loop, const RpcChannel::Options& options)
        : loop_(loop),
          next_id_(1),
          resolver_(ResolverFactory::instance()->Create(options.resolver_name)),
          loadbalancer_(LoadBalancerFactory::instance()->Create(options.loadbalancer_name)),
          total_request_("protoc.RpcChannel.total_request"),
          timeout_request_("protorpc.RpcChannel.timeout_request"),
          total_response_("protorpc.RpcChannel.total_response"),
          failed_response_("protorpc.RpcChannel.failed_response"),
          connected_(false)
    {
        DCHECK(!!resolver_);
        DCHECK(!!loadbalancer_);

        codec_.set_message_callback(
            boost::bind(&Impl::OnResponse, this, _1, _2));
    }

    void Connect(const std::string& server_address)
    {
        resolver_->Resolve(server_address,
                           boost::bind(&Impl::OnResolveResult, this, _1));
    }

    void Connect(const InetAddress& server_address)
    {
        loop_->Run(
                boost::bind(&Impl::MakeConnection, this, server_address));
    }

    void Shutdown()
    {
        for (auto& client : clients_)
        {
            client.Shutdown();
        }
    }

    void CallMethod(const ::google::protobuf::MethodDescriptor* method,
                    RpcControllerPtr& controller,
                    const ::google::protobuf::Message& request,
                    const ::google::protobuf::Message* response_prototype,
                    const RpcChannel::Callback& done)
    {
        RpcMessage message;
        MakeRequest(method, request, &message);
        message.set_compress_type(controller->compress_type());

        if (message.request().empty())
        {
            controller->SetFailed(RPC_ERROR_INVALID_REQUEST);
            return ;
        }
        RegisterRequest(method,
                        controller,
                        response_prototype,
                        done,
                        message.id());

        ThisThread::ResetTraceContext();
        if ((controller->parent() && controller->parent()->has_trace_id())
            || FLAGS_claire_RpcChannel_trace_rate == 0 
	        || (FLAGS_claire_RpcChannel_trace_rate > 0 && message.id() % FLAGS_claire_RpcChannel_trace_rate == 0))
        {
            Trace* trace = nullptr;
            Trace* parent_trace = nullptr;
            if (controller->parent())
            {
                parent_trace = GET_TRACE_BY_TRACEID(controller->parent()->trace_id().trace_id(), controller->parent()->trace_id().span_id());
                DCHECK(parent_trace);
                trace = parent_trace->MakeChild(method->name());
            }
            else
            {
                trace = Trace::FactoryGet(method->name());
            }

            message.mutable_trace_id()->set_trace_id(trace->trace_id());
            message.mutable_trace_id()->set_span_id(trace->span_id());
            if (parent_trace)
            {
                message.mutable_trace_id()->set_parent_span_id(parent_trace->span_id());
            }
            ThisThread::SetTraceContext(trace->trace_id(), trace->span_id());
        }
        TraceContextGuard trace_guard;

        if (!connected())
        {
            AddPendingRequest(message);
            return ;
        }

        if (loop_->IsInLoopThread())
        {
            SendInLoop(message);
        }
        else
        {
            loop_->Post(boost::bind(&Impl::SendInLoop, shared_from_this(), message));
        }
    }

private:
    bool connected() const { return connected_; }

    void MakeRequest(const ::google::protobuf::MethodDescriptor* method,
                     const ::google::protobuf::Message& request,
                     RpcMessage* message)
    {
        message->set_type(REQUEST);
        message->set_id(next_id_++);
        message->set_service(method->service()->full_name());
        message->set_method(method->name());
        message->set_request(request.SerializeAsString());
    }

    void RegisterRequest(const ::google::protobuf::MethodDescriptor* method,
                         RpcControllerPtr& controller,
                         const ::google::protobuf::Message* response_prototype,
                         const RpcChannel::Callback& done,
                         int64_t id)
    {
        auto timeout = static_cast<int>(GetRequestTimeout(method));
        DCHECK(timeout > 0);
        auto timeout_timer = loop_->RunAfter(timeout,
                                             boost::bind(&Impl::OnTimeout, this, id));
        {
            MutexLock lock(mutex_);
            outstandings_[id] = {response_prototype, done, controller, timeout_timer};
        }
        total_request_.Increment();
    }

    bool HasPendingRequest() const
    {
        MutexLock lock(mutex_);
        return !pending_requests_.empty();
    }

    void AddPendingRequest(RpcMessage& message)
    {
        MutexLock lock(mutex_);
        pending_requests_.insert({message.id(), std::move(message)});
    }

    void DispatchPendingRequest()
    {
        std::unordered_map<int64_t, RpcMessage> requests;
        {
            MutexLock lock(mutex_);
            requests.swap(pending_requests_);
        }

        for (auto& request: requests)
        {
            SendInLoop(request.second); // FIXME
        }
    }

    void SendInLoop(RpcMessage& message)
    {
        loop_->AssertInLoopThread();
        auto server_address = loadbalancer_->NextBackend();
        DCHECK(connections_.find(server_address) != connections_.end());
        auto& connection = connections_[server_address];

        if (message.has_trace_id())
        {
            TraceContextGuard guard(message.trace_id().trace_id(), message.trace_id().span_id());  
            TRACE_SET_HOST(connection->local_address().IpAsInt(), connection->local_address().port(), message.service());
            TRACE_ANNOTATION(Annotation::client_send());
        }

        Buffer buffer;
        codec_.SerializeToBuffer(message, &buffer);
        connection->Send(&buffer);
    }

    void OnResolveResult(const std::vector<InetAddress>& server_addresses)
    {
        for (auto& address : server_addresses)
        {
            MakeConnection(address);
        }
    }

    void MakeConnection(const InetAddress& server_address)
    {
        HttpClient* client = new HttpClient(loop_, "RpcChannel");
        clients_.push_back(client);
        client->set_connection_callback(
                boost::bind(&Impl::OnConnection, this, _1));
        client->Connect(server_address);
        client->set_retry(true);
    }

    void OnConnection(const HttpConnectionPtr& connection)
    {
        if (connection->connected())
        {
            const char meta[] = ("POST /__protorpc__ HTTP/1.1\r\nConnection: Keep-Alive\r\n\r\n");
            connection->Send(StringPiece(meta, sizeof(meta)-1));
            connection->set_headers_callback(
                    boost::bind(&Impl::OnHeaders, this, _1));
        }
        else
        {
            connections_.erase(connection->peer_address());
            loadbalancer_->ReleaseBackend(connection->peer_address());
            connected_ = !(connections_.empty());
        }
    }

    void OnHeaders(const HttpConnectionPtr& connection)
    {
        if (connection->mutable_response()->status() != HttpResponse::k200OK)
        {
            LOG(ERROR) << "connection to " << connection->peer_address().ToString() << " failed, "
                       << connection->mutable_response()->StatusCodeReasonPhrase();
            connection->Shutdown();
            return ;
        }

        connections_.insert(std::make_pair(connection->peer_address(), connection));
        loadbalancer_->AddBackend(connection->peer_address(), 1); //FIXME

        connection->set_body_callback(
            boost::bind(&Impl::OnMessage, this, _1, _2));

        connected_ = true;
        if (HasPendingRequest())
        {
            DispatchPendingRequest();
        }
    }

    void OnMessage(const HttpConnectionPtr& connection, Buffer* buffer)
    {
        codec_.ParseFromBuffer(connection, buffer);
    }

    void OnResponse(const HttpConnectionPtr& connection, const RpcMessage& message)
    {
        ThisThread::ResetTraceContext();
        if (message.has_trace_id())
        {
            ThisThread::SetTraceContext(message.trace_id().trace_id(), message.trace_id().span_id());
        }
        TraceContextGuard trace_context_guard;

        if (message.type() != RESPONSE)
        {
            LOG(ERROR) << "Invalid message, not request type";
            connection->Shutdown();
            return ;
        }

        if (!message.has_response())
        {
            LOG(ERROR) << "Invalid message, without response field";
            connection->Shutdown();
            return ;
        }

        OutstandingCall out;
        {
            MutexLock lock(mutex_);
            auto it = outstandings_.find(message.id());
            if (it != outstandings_.end())
            {
                out.swap(it->second);
                outstandings_.erase(it);
            }
            else
            {
                return ;
            }
        }
        loop_->Cancel(out.timer);

        TRACE_ANNOTATION(Annotation::client_recv());
        total_response_.Increment();
        if (message.has_error() && message.error() != RPC_SUCCESS)
        {
            if (message.has_reason())
            {
                out.controller->SetFailed(message.error(), message.reason());
            }
            else
            {
                out.controller->SetFailed(message.error());
            }
        }

        if (out.response_prototype)
        {
            boost::shared_ptr< ::google::protobuf::Message> response(out.response_prototype->New());
            if (!response->ParseFromString(message.response()))
            {
                out.controller->SetFailed(RPC_ERROR_PARSE_FAIL);
            }

            if (out.controller->Failed())
            {
                failed_response_.Increment();
            }
            out.callback(out.controller, response);

            HISTOGRAM_CUSTOM_TIMES("claire.RpcChannel.request_duration",
                                   static_cast<int>(TimeDifference(Timestamp::Now(), out.sent_time)/1000),
                                   1,
                                   10000,
                                   100);
            loadbalancer_->AddRequestResult(connection->peer_address(),
                                            out.controller->Failed() ? RequestResult::kFailed : RequestResult::kSuccess,
                                            0);
        }
        else
        {
            LOG(ERROR) << "No Response prototype, id " << message.id();
            connection->Shutdown();
        }

        ERASE_TRACE();
    }

    void OnTimeout(int64_t id)
    {
        timeout_request_.Increment();

        OutstandingCall out;
        {
            MutexLock lock(mutex_);
            auto it = outstandings_.find(id);
            if (it != outstandings_.end())
            {
                out.swap(it->second);
                outstandings_.erase(it);

                pending_requests_.erase(id);
            }
            else
            {
                NOTREACHED();
                return ;
            }
        }

        out.controller->SetFailed(RPC_ERROR_REQUEST_TIMEOUT);

        ::google::protobuf::MessagePtr response; //FIXME
        out.callback(out.controller, response);
    }

    void SendHeartBeat()
    {
        auto method = BuiltinService::descriptor()->FindMethodByName("HeartBeat");
        HeartBeatRequest request;

        for (auto& client : clients_)
        {
            if (client.connected())
            {
                RpcMessage message;
                MakeRequest(method, request, &message);

                RpcControllerPtr controller(new RpcController());
                controller->set_context(client.peer_address());
                RegisterRequest(method,
                                controller,
                                &(HeartBeatResponse::default_instance()),
                                boost::bind(&Impl::OnHeartBeatResponse, this, _1, _2),
                                message.id());

                Buffer buffer;
                codec_.SerializeToBuffer(message, &buffer);
                client.Send(&buffer);
            }
        }
    }

    void OnHeartBeatResponse(RpcControllerPtr& controller, const ::google::protobuf::MessagePtr& message)
    {
        auto server_address = boost::any_cast<InetAddress>(controller->context());
        auto response = ::google::protobuf::down_pointer_cast<HeartBeatResponse>(message);
        if (controller->Failed() || response->status() != "Ok")
        {
            loadbalancer_->ReleaseBackend(server_address);
        }
    }

    struct OutstandingCall
    {
        OutstandingCall()
            : response_prototype(nullptr)
        {}

        OutstandingCall(const ::google::protobuf::Message* response_prototype__,
                        const RpcChannel::Callback& callback__,
                        RpcControllerPtr& controller__,
                        TimerId timer__)
            : response_prototype(response_prototype__),
              callback(callback__),
              controller(controller__),
              timer(timer__),
              sent_time(Timestamp::Now())
        {}

        void swap(OutstandingCall& other)
        {
            std::swap(response_prototype, other.response_prototype);
            std::swap(callback, other.callback);
            std::swap(controller, other.controller);
            std::swap(timer, other.timer);
            std::swap(sent_time, other.sent_time);
        }

        const ::google::protobuf::Message* response_prototype;
        RpcChannel::Callback callback;
        RpcControllerPtr controller;
        TimerId timer;
        Timestamp sent_time;
    };

    EventLoop* loop_;
    boost::atomic<int64_t> next_id_;
    RpcCodec codec_;

    boost::scoped_ptr<Resolver> resolver_;
    boost::scoped_ptr<LoadBalancer> loadbalancer_;

    boost::ptr_vector<HttpClient> clients_;
    std::map<InetAddress, HttpConnectionPtr> connections_;

    mutable Mutex mutex_;
    std::unordered_map<int64_t, OutstandingCall> outstandings_;
    std::unordered_map<int64_t, RpcMessage> pending_requests_;

    Counter total_request_;
    Counter timeout_request_;
    Counter total_response_;
    Counter failed_response_;

    bool connected_;
};

RpcChannel::RpcChannel(EventLoop* loop, const Options& options)
    : impl_(new Impl(loop, options)) {}

RpcChannel::~RpcChannel() {}

void RpcChannel::CallMethod(const ::google::protobuf::MethodDescriptor* method,
                            RpcControllerPtr& controller,
                            const ::google::protobuf::Message& request,
                            const ::google::protobuf::Message* response_prototype,
                            const Callback& done)
{
    impl_->CallMethod(method,
                      controller,
                      request,
                      response_prototype,
                      done);
}

void RpcChannel::Connect(const std::string& server_address)
{
    impl_->Connect(server_address);
}

void RpcChannel::Connect(const InetAddress& server_address)
{
    impl_->Connect(server_address);
}

void RpcChannel::Shutdown()
{
    impl_->Shutdown();
}

} // namespace protorpc
} // namespace claire
