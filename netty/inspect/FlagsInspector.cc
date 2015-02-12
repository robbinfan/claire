// Copyright (c) 2013 The claire-netty Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include <claire/netty/inspect/FlagsInspector.h>

#include "thirdparty/ctemplate/template.h"
#include "thirdparty/rapidjson/document.h"
#include "thirdparty/rapidjson/stringbuffer.h"
#include "thirdparty/rapidjson/prettywriter.h"

#include <vector>

#include <boost/bind.hpp>

#include <claire/netty/http/HttpServer.h>
#include <claire/netty/http/HttpRequest.h>
#include <claire/netty/http/HttpResponse.h>
#include <claire/netty/http/HttpConnection.h>

#include <claire/netty/static_resource.h>

namespace claire {

namespace  {

void OnFlagsPage(const HttpConnectionPtr& connection, HttpResponse* response)
{
    std::vector< ::gflags::CommandLineFlagInfo> flags;
    GetAllFlags(&flags);

    ctemplate::TemplateDictionary dictionary("data");
    dictionary.SetValue("HOST_NAME", connection->mutable_request()->get_header("HOST"));

    auto it = flags.begin();
    while (it != flags.end())
    {
        if (it->filename[0] != '/')
        {
            auto discard_name = it->filename;
            while ((it != flags.end()) && (discard_name == it->filename))
            {
                ++it;
            }
            continue;
        }

        const auto filename = it->filename;
        auto table = dictionary.AddSectionDictionary("FLAG_TABLE");
        table->SetValue("FILE_NAME", filename);

        while ((it != flags.end()) && (filename == it->filename))
        {
            auto row = table->AddSectionDictionary("FLAG_ROW");
            if (!it->is_default)
            {
                row->SetValue("FLAG_STYLE", "color:green");
            }

            row->SetValue("FLAG_NAME", it->name);
            row->SetValue("FLAG_TYPE", it->type);
            row->SetValue("FLAG_DESCRIPTION", it->description);
            row->SetValue("FLAG_DEFAULT_VALUE", it->default_value);
            row->SetValue("FLAG_CURRENT_VALUE", it->current_value);
            ++it;
        }
    }

    ctemplate::StringToTemplateCache("flags.html",
                                     RESOURCE_claire_netty_inspect_assets_flags_html,
                                     sizeof(RESOURCE_claire_netty_inspect_assets_flags_html),
                                     ctemplate::STRIP_WHITESPACE);
    ctemplate::ExpandTemplate("flags.html",
                              ctemplate::STRIP_WHITESPACE,
                              &dictionary,
                              response->mutable_body());
    response->AddHeader("Content-Type", "text/html");
}

void ModifyFlags(const HttpConnectionPtr& connection, HttpResponse* response)
{
    rapidjson::Document doc;
    doc.Parse<0>(connection->mutable_request()->mutable_body()->c_str());

    rapidjson::Document result;
    result.SetObject();
    for (auto it = doc.MemberBegin(); it != doc.MemberEnd(); ++it)
    {
        if (::gflags::SetCommandLineOption(it->name.GetString(),
                                           it->value.GetString()).empty())
        {
            result.AddMember(it->name, it->value, doc.GetAllocator());
        }
    }

    rapidjson::StringBuffer buffer;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
    result.Accept(writer);

    response->set_status(HttpResponse::k200OK);
    response->AddHeader("Content-Type", "application/json");
    response->mutable_body()->assign(buffer.GetString());
}

} // namespace

FlagsInspector::FlagsInspector(HttpServer* server)
{
    if (!server)
    {
        return ;
    }

    server->Register("/flags",
                     boost::bind(&FlagsInspector::OnFlags, _1),
                     false);

    server->RegisterAsset("/static/flags.html",
                          RESOURCE_claire_netty_inspect_assets_flags_html,
                          sizeof(RESOURCE_claire_netty_inspect_assets_flags_html));
    server->RegisterAsset("/static/flags.js",
                          RESOURCE_claire_netty_inspect_assets_flags_js,
                          sizeof(RESOURCE_claire_netty_inspect_assets_flags_js));
}

void FlagsInspector::OnFlags(const HttpConnectionPtr& connection)
{
    HttpResponse response;
    switch (connection->mutable_request()->method())
    {
        case HttpRequest::kGet:
            OnFlagsPage(connection, &response);
            break;
        case HttpRequest::kPost:
            ModifyFlags(connection, &response);
            break;
        default:
            connection->OnError(HttpResponse::k400BadRequest,
                                "Only accept Get/Post method");
            return ;
    }
    connection->Send(&response);
}

} // namespace claire
