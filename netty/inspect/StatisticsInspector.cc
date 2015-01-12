// Copyright (c) 2013 The claire-netty Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include <claire/netty/inspect/StatisticsInspector.h>

#include <string>

#include <boost/bind.hpp>
#include <boost/algorithm/string.hpp>

#include <claire/netty/http/HttpServer.h>
#include <claire/netty/http/HttpRequest.h>
#include <claire/netty/http/HttpResponse.h>
#include <claire/netty/http/HttpConnection.h>

#include <claire/common/metrics/CounterSampler.h>
#include <claire/common/metrics/HistogramRecorder.h>

#include <claire/netty/static_resource.h>

namespace claire {

namespace {

int GetSinceParameter(const std::string& parameter)
{
    if (parameter.empty())
    {
        return -1;
    }

    try
    {
        auto since = std::stoi(parameter);
        return since > 0 ? since : 0;
    }
    catch (...)
    {
        LOG(ERROR) << "Invalid parameter " << parameter;
    }

    return -1;
}

} // namespace

StatisticsInspector::StatisticsInspector(HttpServer* server)
    : sampler_(new CounterSampler(server->loop()))
{
    if (!server)
    {
        return ;
    }

    server->Register("/histograms",
                     boost::bind(&StatisticsInspector::OnHistograms, _1),
                     false);
    server->Register("/counters",
                     boost::bind(&StatisticsInspector::OnCounters, _1),
                     false);
    server->Register("/counterdata",
                     boost::bind(&StatisticsInspector::OnCounterData, this, _1),
                     true);

    server->RegisterAsset("/static/grapher.js",
                          RESOURCE_claire_netty_inspect_assets_grapher_js);
    server->RegisterAsset("/static/parser.js",
                          RESOURCE_claire_netty_inspect_assets_parser_js);
    server->RegisterAsset("/static/dygraph-extra.js",
                          RESOURCE_claire_netty_inspect_assets_dygraph_extra_js);
    server->RegisterAsset("/static/dygraph-combined.js",
                          RESOURCE_claire_netty_inspect_assets_dygraph_combined_js);
}

StatisticsInspector::~StatisticsInspector() {}

void StatisticsInspector::OnHistograms(const HttpConnectionPtr& connection)
{
    HttpResponse response;
    response.set_status(HttpResponse::k200OK);
    HistogramRecorder::instance()->WriteHTMLGraph("", response.mutable_body());
    response.AddHeader("Content-Type", "text/html");
    connection->Send(&response);
}

void StatisticsInspector::OnCounters(const HttpConnectionPtr& connection)
{
    HttpResponse response;
    response.set_status(HttpResponse::k200OK);
    response.mutable_body()->assign(RESOURCE_claire_netty_inspect_assets_graphview_html);
    connection->Send(&response);
}

void StatisticsInspector::OnCounterData(const HttpConnectionPtr& connection)
{
    std::string output;
    auto request = connection->mutable_request();
    if (request->uri().query().empty())
    {
        sampler_->WriteJson(&output);
    }
    else
    {
        auto metrics_query = request->uri().get_parameter("metrics");
        auto since_query = request->uri().get_parameter("since");

        std::vector<std::string> metrics;
        boost::split(metrics, metrics_query, boost::is_any_of(","));

        int64_t since = GetSinceParameter(request->uri().get_parameter("since"));
        if (since < 0)
        {
            connection->OnError(HttpResponse::k400BadRequest,
                                "Invalid Since Parameter");
            return ;
        }

        sampler_->WriteJson(&output, metrics, since);
    }

    HttpResponse response;
    response.set_status(HttpResponse::k200OK);
    response.AddHeader("Content-Type", "application/json");
    response.mutable_body()->swap(output);
    connection->Send(&response);
}

} // namespace claire
