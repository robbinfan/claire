// Copyright (c) 2013 The claire-netty Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include <claire/netty/inspect/PProfInspector.h>

#ifdef HAVE_PPROF
#include <gperftools/profiler.h>
#include <gperftools/malloc_extension.h>
#endif

#include <boost/bind.hpp>
#include <boost/algorithm/string.hpp>

#include <claire/netty/http/HttpServer.h>
#include <claire/netty/http/HttpRequest.h>
#include <claire/netty/http/HttpResponse.h>

#include <claire/common/files/FileUtil.h>
#include <claire/common/logging/Logging.h>
#include <claire/common/events/EventLoop.h>
#include <claire/common/symbolizer/Symbolizer.h>

static const char kProfileFile[] = "profile.dat";
static const int kDefaultProfileSeconds = 30;
static const int kMaxProfileSeconds = 600;

namespace claire {

namespace {

int GetProfileSeconds(const std::string& parameter)
{
    if (parameter.empty())
    {
        return kDefaultProfileSeconds;
    }

    try
    {
        auto s =  std::stoi(parameter);
        if (s < 0 || s > kMaxProfileSeconds)
        {
            LOG(ERROR) << "Invalid profile seconds parameter: " << parameter;
        }
        else
        {
            return s;
        }
    }
    catch (const std::out_of_range& e)
    {
        LOG(ERROR) << "Invalid profile seconds parameter: " << parameter
                   << ", out of range";
    }
    catch (const std::invalid_argument& e)
    {
        LOG(ERROR) << "Invalid profile seconds parameter: " << parameter
                   << ", invalid argument";
    }

    return -1;
}

} // namespace

PProfInspector::PProfInspector(HttpServer* server)
    : server_(server)
{
    if (!server_)
    {
        return ;
    }

#ifdef HAVE_PPROF
    server_->Register("/pprof/profile",
                      boost::bind(&PProfInspector::OnProfile, this, _1),
                      false);
    server_->Register("/pprof/heap",
                      boost::bind(&PProfInspector::OnHeap, _1),
                      false);
    server_->Register("/pprof/growth",
                      boost::bind(&PProfInspector::OnGrowth, _1),
                      false);
    server_->Register("/pprof/cmdline",
                      boost::bind(&PProfInspector::OnCmdline, _1),
                      false);
    server_->Register("/pprof/symbol",
                      boost::bind(&PProfInspector::OnSymbol, _1),
                      false);
#endif
}

void PProfInspector::OnProfile(const HttpConnectionPtr& connection)
{
#ifdef HAVE_PPROF
    auto request = connection->mutable_request();
    if (request->method() != HttpRequest::kGet)
    {
        connection->OnError(HttpResponse::k400BadRequest,
                            "Only accept Get method");
        return ;
    }

    int seconds = GetProfileSeconds(request->get_parameter("seconds"));
    if (seconds < 0)
    {
        connection->OnError(HttpResponse::k400BadRequest,
                            "Invalid Profile Seconds Parameter");
        return ;
    }

    {
        MutexLock lock(mutex_);
        if (connections_.empty())
        {
            ProfilerStart(kProfileFile);
            CHECK(!EventLoop::CurrentLoopInThisThread());
            EventLoop::CurrentLoopInThisThread()->RunAfter(seconds*1000,
                                                           boost::bind(&PProfInspector::OnProfileComplete, this));
        }
        connections_.insert(connection->id());
    }
#endif
}

void PProfInspector::OnProfileComplete()
{
#ifdef HAVE_PPROF
    ProfilerFlush();
    ProfilerStop();

    std::set<HttpConnection::Id> connections;
    {
        MutexLock lock(mutex_);
        connections.swap(connections_);
    }

    std::string output;
    FileUtil::ReadFileToString(kProfileFile, &output);
    for (auto& connection : connections)
    {
        server_->SendByHttpConnectionId(connection, output);
        server_->Shutdown(connection);
    }
#endif
}

void PProfInspector::OnHeap(const HttpConnectionPtr& connection)
{
#ifdef HAVE_PPROF
    std::string output;
    MallocExtension::instance()->GetHeapSample(&output);
    connection->Send(output);
    connection->Shutdown();
#endif
}

void PProfInspector::OnGrowth(const HttpConnectionPtr& connection)
{
#ifdef HAVE_PPROF
    std::string output;
    MallocExtension::instance()->GetHeapGrowthStacks(&output);
    connection->Send(output);
    connection->Shutdown();
#endif
}

void PProfInspector::OnCmdline(const HttpConnectionPtr& connection)
{
    std::string output;
    FileUtil::ReadFileToString("/proc/self/cmdline", &output);
    std::replace(output.begin(), output.end(), '\0', '\n');

    connection->Send(output);
    connection->Shutdown();
}

void PProfInspector::OnSymbol(const HttpConnectionPtr& connection)
{
    auto request = connection->mutable_request();
    if (request->method() == HttpRequest::kGet)
    {
        const char s[] = "num_symbols: 1\n";
        connection->Send(StringPiece(s, sizeof(s)-1));
        connection->Shutdown();
        return ;
    }

    if (request->method() != HttpRequest::kPost)
    {
        connection->OnError(HttpResponse::k400BadRequest,
                            "Only accept Post method");
        return ;
    }

    std::vector<std::string> addresses;
    boost::algorithm::split(addresses, request->body(), boost::is_any_of("+"));

    HttpResponse response;
    for (auto& address : addresses)
    {
        StringPiece name;
        Symbolizer symbolizer;
        Dwarf::LocationInfo location;

        response.mutable_body()->append(address);
        response.mutable_body()->append("\t");
        if (symbolizer.Symbolize(static_cast<uintptr_t>(std::stoull(address)), &name, &location))
        {
            response.mutable_body()->append(name.ToString());
            response.mutable_body()->append("\n");
        }
        else
        {
            response.mutable_body()->append("unknown\n");
        }
    }

    connection->Send(&response);
}

} // namespace claire
