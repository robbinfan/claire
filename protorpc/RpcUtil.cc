// Copyright (c) 2013 The claire-protorpc Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors

#include <claire/protorpc/RpcUtil.h>

#include <string>

#include <claire/protorpc/rpcmessage.pb.h>

namespace claire {
namespace protorpc {

const std::string& ErrorCodeToString(int error)
{
    auto descriptor = ErrorCode_descriptor()->FindValueByNumber(error);
    if (!descriptor)
    {
        return ErrorCode_descriptor()->FindValueByNumber(RPC_ERROR_UNKNOWN_ERROR)->name();
    }

    return descriptor->name();
}

} // namespace protorpc
} // namespace claire
