// Copyright (c) 2013 The claire-protorpc Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include <claire/protorpc/BuiltinService.h>

#include "thirdparty/gflags/gflags.h"

namespace claire {
namespace protorpc {

static bool g_protorpc_healthy = true;

void BuiltinServiceImpl::HeartBeat(RpcControllerPtr& controller,
                                   const HeartBeatRequestPtr& request,
                                   const HeartBeatResponse* response_prototype,
                                   const RpcDoneCallback& done)
{
    HeartBeatResponse response;
    if (g_protorpc_healthy)
    {
        response.set_status("Ok");
    }
    else
    {
        response.set_status("Bad");
    }
    done(controller, &response);
}

void BuiltinServiceImpl::Services(RpcControllerPtr& controller,
                                  const ServicesRequestPtr& request,
                                  const ServicesResponse* response_prototype,
                                  const RpcDoneCallback& done)
{
    ServicesResponse response;
    for (auto it = services_.cbegin(); it != services_.cend(); ++it)
    {
        if (it->second == this)
        {
            continue;
        }
        response.add_services()->set_name(it->second->GetDescriptor()->full_name());
    }
    done(controller, &response);
}

void BuiltinServiceImpl::GetFileSet(RpcControllerPtr& controller,
                                    const GetFileSetRequestPtr& request,
                                    const GetFileSetResponse* response_prototype,
                                    const RpcDoneCallback& done)
{
    GetFileSetResponse response;
    for (int i = 0; i < request->names_size(); i++)
    {
        auto it = services_.find(request->names(i));
        if (it != services_.end())
        {
            auto file = response.mutable_file_set()->add_file();
            it->second->GetDescriptor()->file()->CopyTo(file);
        }
    }
    done(controller, &response);
}

} // namespace protorpc
} // namespace claire
