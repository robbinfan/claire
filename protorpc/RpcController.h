// Copyright (c) 2013 The claire-protorpc Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#pragma once

#include <string>

#include <boost/any.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/noncopyable.hpp>

#include <claire/protorpc/rpcmessage.pb.h>

// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://code.google.com/p/protobuf/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

namespace claire {
namespace protorpc {

class RpcController;
typedef boost::shared_ptr<RpcController> RpcControllerPtr;

// An RpcController mediates a single method call.  The primary purpose of
// the controller is to provide a way to manipulate settings specific to the
// RPC implementation and to find out about RPC-level errors.
//
// The methods provided by the RpcController interface are intended to be a
// "least common denominator" set of features which we expect all
// implementations to support.  Specific implementations may provide more
// advanced features (e.g. deadline propagation).
class RpcController : boost::noncopyable
{
public:
    typedef boost::function<void()> CancelCallback;

    RpcController();

    // Client-side methods ---------------------------------------------
    // These calls may be made from the client side only.  Their results
    // are undefined on the server side (may crash).

    // Resets the RpcController to its initial state so that it may be reused in
    // a new call.  Must not be called while an RPC is in progress.
    void Reset();

    // After a call has finished, returns true if the call failed.  The possible
    // reasons for failure depend on the RPC implementation.  Failed() must not
    // be called before a call has finished.  If Failed() returns true, the
    // contents of the response message are undefined.
    bool Failed() const;

    // If Failed() is true, returns a human-readable description of the error.
    std::string ErrorText() const;

    // Server-side methods ---------------------------------------------
    // These calls may be made from the server side only.  Their results
    // are undefined on the client side (may crash).

    // Causes Failed() to return true on the client side.  "reason" will be
    // incorporated into the message returned by ErrorText().  If you find
    // you need to return machine-readable information about failures, you
    // should incorporate it into your response protocol buffer and should
    // NOT call SetFailed().
    void SetFailed(const std::string& reason);

    int ErrorCode() const { return error_; }

    void SetFailed(int error) { error_ = error; }
    void SetFailed(int error, const std::string& reason)
    {
        error_ = error;
        reason_ = reason;
    }

    void set_context(const boost::any& context__)
    {
        context_ = context__;
    }

    boost::any& context() { return context_; }
    const boost::any& context() const { return context_; }

    void set_compress_type(CompressType compress_type)
    {
        compress_type_ = compress_type;
    }
    CompressType compress_type() const { return compress_type_; }

    RpcControllerPtr parent() { return parent_.lock(); }
    void set_parent(RpcControllerPtr& p)
    {
        parent_ = p;
    }

    bool has_trace_id() const { return trace_id_.IsInitialized(); }
    void set_trace_id(const TraceId& trace_id__)
    {
        if (trace_id__.IsInitialized())
        {
            trace_id_.CopyFrom(trace_id__);
        }
    }

    const TraceId& trace_id() const { return trace_id_; }

private:
    int error_;
    std::string reason_;
    CompressType compress_type_;
    boost::weak_ptr<RpcController> parent_;
    TraceId trace_id_;
    boost::any context_;
};

} // namespace protorpc
} // namespace claire
