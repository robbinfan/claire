// Copyright (c) 2013 The claire-protorpc Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#pragma once

#include <map>
#include <string>

#include <claire/protorpc/builtin_service.pb.h> // generate by protoc

namespace claire {
namespace protorpc {

class BuiltinServiceImpl : public BuiltinService {
public:
    typedef std::map<std::string, Service*> ServiceMap;

    void set_services(const ServiceMap& services)
    {
        services_ = services;
    }

    virtual void HeartBeat(RpcControllerPtr& controller,
                           const HeartBeatRequestPtr& request,
                           const HeartBeatResponse* response_prototype,
                           const RpcDoneCallback& done);

    virtual void Services(RpcControllerPtr& controller,
                          const ServicesRequestPtr& request,
                          const ServicesResponse* response_prototype,
                          const RpcDoneCallback& done);

    virtual void GetFileSet(RpcControllerPtr& controller,
                            const GetFileSetRequestPtr& request,
                            const GetFileSetResponse* response_prototype,
                            const RpcDoneCallback& done);

private:
    ServiceMap services_;
};

} // namespace protorpc
} // namespace claire
