// Copyright (c) 2013 The claire-netty Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifdef HAVE_CARES
#include <claire/netty/resolver/DnsResolver.h>

#include <ares.h>
#include <netdb.h>

#include <gflags/gflags.h>

#include <boost/bind.hpp>
#include <boost/ptr_container/ptr_map.hpp>

#include <claire/common/events/Channel.h>
#include <claire/common/events/EventLoop.h>
#include <claire/common/logging/Logging.h>

DEFINE_bool(cares_only_dns, false, "c-ares only use dns without host");

namespace claire {

namespace {

int GetTimeout(ares_channel ctx)
{
    struct timeval tv;
    auto tp = ares_timeout(ctx, NULL, &tv);

    int timeout(-1);
    if (tp)
    {
        timeout = static_cast<int>(tp->tv_sec*1000 + tp->tv_usec/1000);
    }
    return timeout;
}

} // namespace

class DnsResolver::Impl : boost::noncopyable
{
public:
    Impl() : loop_(NULL), context_(NULL) {}

    void Init(EventLoop* loop)
    {
        CHECK(!loop_);
        loop_ = loop;

        auto ret = ares_library_init(ARES_LIB_INIT_ALL);
        CHECK_EQ(ret, ARES_SUCCESS) << "ares_init_options failed " << ares_strerror(ret);

        static char lookups[] = "b";

        int option_mask = ARES_OPT_FLAGS;
        struct ares_options options;
        ::bzero(&options, sizeof options);

        options.flags = ARES_FLAG_NOCHECKRESP;
        options.flags |= ARES_FLAG_STAYOPEN;
        options.flags |= ARES_FLAG_IGNTC; // only UDP

        option_mask |= ARES_OPT_SOCK_STATE_CB;
        options.sock_state_cb = &Impl::AresSocketStateCallback;
        options.sock_state_cb_data = this;

        option_mask |= ARES_OPT_TIMEOUTMS;
        options.timeout = 2000;

        if (FLAGS_cares_only_dns)
        {
            option_mask |= ARES_OPT_LOOKUPS;
            options.lookups = lookups;
        }

        ret = ares_init_options(&context_, &options, option_mask);
        CHECK_EQ(ret, ARES_SUCCESS) << "ares_init_options failed " << ares_strerror(ret);

        ares_set_socket_callback(context_, &Impl::AresSocketCreateCallback, this);
    }

    ~Impl()
    {
        ares_destroy(context_);
    }

    void Resolve(const std::string& host, const Resolver::ResolveCallback& callback)
    {
        loop_->AssertInLoopThread();

        auto data(new QueryData(this, callback));
        ares_gethostbyname(context_, host.data(), AF_INET, &Impl::AresHostCallback, data);

        if (!timer_id_.Valid())
        {
            timer_id_ = loop_->RunAfter(GetTimeout(context_),
                                        boost::bind(&Impl::OnTimeout, this));
        }
    }

private:
    void OnTimeout()
    {
        ares_process_fd(context_, ARES_SOCKET_BAD, ARES_SOCKET_BAD);

        auto timeout = GetTimeout(context_);
        LOG(DEBUG) << "DnsResolver " << this << " next timeout " << timeout;

        timer_id_.Reset();
        if (timeout >= 0)
        {
            timer_id_ = loop_->RunAfter(timeout, boost::bind(&Impl::OnTimeout, this));
        }
    }

    void OnRead(int sockfd)
    {
        DCHECK(channels_.find(sockfd) != channels_.end());
        LOG(DEBUG) << "DnsResolver::OnRead " << sockfd;

        ares_process_fd(context_, sockfd, ARES_SOCKET_BAD);
    }

    void OnQueryResult(int status, struct hostent* result, const Resolver::ResolveCallback& callback)
    {
        LOG(DEBUG) << "DnsResolver::OnQueryResult " << status;

        std::vector<InetAddress> addresses;
        if (result != NULL)
        {
            for (auto alias = result->h_aliases; *alias != NULL; ++alias)
            {
                LOG(DEBUG) << "alias: " << *alias;
            }

            for (auto haddr = result->h_addr_list; *haddr != NULL; ++haddr)
            {
                struct sockaddr_in address;
                ::bzero(&address, sizeof address);
                address.sin_family = AF_INET;
                address.sin_port = 0;
                address.sin_addr = *(reinterpret_cast<in_addr*>(*haddr));

                addresses.push_back(InetAddress(address));
            }
        }
        callback(addresses);
    }

    void OnAresSocketCreate(int sockfd, int type)
    {
        loop_->AssertInLoopThread();

        auto channel = new Channel(loop_, sockfd);
        channel->SetReadCallback(boost::bind(&Impl::OnRead, this, sockfd));
        channel->EnableReading();
        channels_.insert(sockfd, channel);
    }

    void OnAresSocketStateChange(int sockfd, bool readable, bool writable)
    {
        loop_->AssertInLoopThread();

        auto it = channels_.find(sockfd);
        if (it == channels_.end())
        {
            LOG(ERROR) << "receive invalid fd " << sockfd << " event";
            return ;
        }

        if (!readable && !writable)
        {
            it->second->DisableAll();
            it->second->Remove();
            channels_.erase(it);
        }
    }

    static void AresHostCallback(void* data, int status, int timeouts, struct hostent* hostent)
    {
        auto query = static_cast<QueryData*>(data);
        query->resolver->OnQueryResult(status, hostent, query->callback);
        delete query;
    }

    static int AresSocketCreateCallback(int sockfd, int type, void* data)
    {
        static_cast<Impl*>(data)->OnAresSocketCreate(sockfd, type);
        return 0;
    }

    static void AresSocketStateCallback(void* data, int sockfd, int read, int write)
    {
        static_cast<Impl*>(data)->OnAresSocketStateChange(sockfd, read, write);
    }

    struct QueryData
    {
        Impl* resolver;
        Resolver::ResolveCallback callback;

        QueryData(Impl* r__, const Resolver::ResolveCallback& callback__)
            : resolver(r__),
              callback(callback__)
        {}
    };

    EventLoop* loop_;
    ares_channel context_;
    TimerId timer_id_;
    boost::ptr_map<int, Channel> channels_;
};

DnsResolver::DnsResolver() : impl_(new Impl()) {}
DnsResolver::~DnsResolver() {}

void DnsResolver::Init(EventLoop* loop)
{
    impl_->Init(loop);
}

void DnsResolver::Resolve(const std::string& address, const Resolver::ResolveCallback& callback)
{
    impl_->Resolve(address, callback);
}

} // namespace claire

#endif
