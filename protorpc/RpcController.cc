// Copyright (c) 2013 The claire-protorpc Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include <claire/protorpc/RpcController.h>
#include <claire/protorpc/RpcUtil.h>

namespace claire {
namespace protorpc {

RpcController::RpcController()
    : error_(RPC_SUCCESS),
      compress_type_(Compress_None) {}

void RpcController::Reset()
{
    error_ = RPC_SUCCESS;
    reason_.clear();
    compress_type_ = Compress_None;
    parent_.reset();
    trace_id_.Clear();
    context_ = boost::any();
}

bool RpcController::Failed() const
{
    return error_ != RPC_SUCCESS;
}

void RpcController::SetFailed(const std::string& reason)
{
    SetFailed(RPC_ERROR_INTERNAL_ERROR, reason);
}

std::string RpcController::ErrorText() const
{
    std::string output;
    if (Failed())
    {
        output = ErrorCodeToString(error_);

        if (!reason_.empty())
        {
            output += ": " + reason_;
        }
    }
    return output;
}

} // namespace protorpc
} // namespace claire
