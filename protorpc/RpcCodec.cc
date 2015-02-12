// Copyright (c) 2013 The claire-protorpc Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include <claire/protorpc/RpcCodec.h>

#include <zlib.h>
#include "thirdparty/snappy/snappy.h"
#include "thirdparty/google/protobuf/message.h"

#include <string>

#include <claire/common/logging/Logging.h>

#include <claire/netty/Buffer.h>
#include <claire/netty/http/HttpConnection.h>

#include <claire/protorpc/RpcUtil.h>
#include <claire/protorpc/rpcmessage.pb.h> // protoc-rpc-gen

namespace claire {
namespace protorpc {

namespace {

const static int kMinMessageLength = sizeof(int32_t); // checksum
const static int kMaxMessageLength = 64*1024*1024;    // same as codec_stream.h kDefaultTotalBytesLimit
const static int kChecksumLength   = sizeof(int32_t);

// ByteSizeConsistencyError and InitializationErrorMessage are
// copied from google/protobuf/message_lite.cc

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

// Authors: wink@google.com (Wink Saville),
//          kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

// When serializing, we first compute the byte size, then serialize the message.
// If serialization produces a different number of bytes than expected, we
// call this function, which crashes.  The problem could be due to a bug in the
// protobuf implementation but is more likely caused by concurrent modification
// of the message.  This function attempts to distinguish between the two and
// provide a useful error message.
void ByteSizeConsistencyError(int byte_size_before_serialization,
                              int byte_size_after_serialization,
                              int bytes_produced_by_serialization)
{
    CHECK_EQ(byte_size_before_serialization, byte_size_after_serialization)
        << "Protocol message was modified concurrently during serialization.";
    CHECK_EQ(bytes_produced_by_serialization, byte_size_before_serialization)
        << "Byte size calculation and serialization were inconsistent.  This "
           "may indicate a bug in protocol buffers or it may be caused by "
           "concurrent modification of the message.";
    LOG(FATAL) << "This shouldn't be called if all the sizes are equal.";
}

std::string InitializationErrorMessage(const char* action,
                                       const ::google::protobuf::MessageLite& message)
{
    // Note:  We want to avoid depending on strutil in the lite library, otherwise
    //   we'd use:
    //
    // return strings::Substitute(
    //   "Can't $0 message of type \"$1\" because it is missing required "
    //   "fields: $2",
    //   action, message.GetTypeName(),
    //   message.InitializationErrorString());

    std::string result;
    result += "Can't ";
    result += action;
    result += " message of type \"";
    result += message.GetTypeName();
    result += "\" because it is missing required fields: ";
    result += message.InitializationErrorString();
    return result;
}

int32_t BytesChecksum(const char* buffer, size_t length)
{
    return static_cast<int32_t>(::adler32(1,
                                          reinterpret_cast<const Bytef*>(buffer),
                                          static_cast<int>(length)));
}

ErrorCode Parse(Buffer* buffer, RpcMessage* message)
{
    auto length = buffer->PeekInt32();
    buffer->Consume(sizeof(int32_t));

    auto expected_checksum = buffer->PeekInt32();
    buffer->Consume(kChecksumLength);

    auto checksum = BytesChecksum(buffer->Peek(),
                                  static_cast<int>(length - kChecksumLength));
    if (checksum != expected_checksum)
    {
        return RPC_ERROR_INVALID_CHECKSUM;
    }

    if (!message->ParseFromArray(buffer->Peek(), length - kChecksumLength))
    {
        return RPC_ERROR_PARSE_FAIL;
    }

    buffer->Consume(length - kChecksumLength);

    if (message->has_compress_type())
    {
        switch (message->compress_type())
        {
            case Compress_Snappy:
            {
                auto compressed_message
                    = message->has_request() ? message->mutable_request() : message->mutable_response();
                std::string uncompressed_message;
                if (snappy::Uncompress(compressed_message->data(),
                                      compressed_message->size(),
                                      &uncompressed_message))
                {
                    compressed_message->swap(uncompressed_message);
                }
                break;
            }
            default:
                message->set_compress_type(Compress_None);
                break;
        }
    }
    return RPC_SUCCESS;
}

} // namespace

RpcCodec::RpcCodec()
{
    GOOGLE_PROTOBUF_VERIFY_VERSION;
}

void RpcCodec::ParseFromBuffer(const HttpConnectionPtr& connection, Buffer* buffer) const
{
    while (buffer->ReadableBytes() >= static_cast<size_t>(kMinMessageLength))
    {
        auto length = buffer->PeekInt32();
        if (length > kMaxMessageLength || length < kMinMessageLength)
        {
            connection->OnError(HttpResponse::k400BadRequest,
                                "Invalid message length");
            break;
        }

        if (buffer->ReadableBytes() >= implicit_cast<size_t>(length + sizeof(int32_t)))
        {
            RpcMessage message;
            auto error = Parse(buffer, &message);
            if (error != RPC_SUCCESS)
            {
                connection->OnError(HttpResponse::k400BadRequest,
                                    ErrorCodeToString(error));
                break;
            }
            else
            {
                message_callback_(connection, message);
            }
        }
        else
        {
            break;
        }
    }
}

void RpcCodec::SerializeToBuffer(RpcMessage& message, Buffer* buffer) const
{
    DCHECK(message.IsInitialized())
        << InitializationErrorMessage("Serialize", message);

    if (message.has_compress_type())
    {
        switch (message.compress_type())
        {
            case Compress_Snappy:
            {
                auto uncompressed_message =
                    message.has_request() ? message.mutable_request() : message.mutable_response();
                std::string compressed_message;
                if (snappy::Compress(uncompressed_message->data(),
                                     uncompressed_message->size(),
                                     &compressed_message))

                {
                    uncompressed_message->swap(compressed_message);
                }
                break;
            }
            default:
                message.set_compress_type(Compress_None);
                break;
        }
    }

    auto bytes = message.ByteSize();
    DCHECK(bytes < kMaxMessageLength)
        << "message " << message.id() << " length " << bytes << " too big.";
    buffer->EnsureWritableBytes(bytes + kChecksumLength + sizeof(int32_t));

    auto start = reinterpret_cast<uint8_t*>(buffer->BeginWrite());
    auto end = message.SerializeWithCachedSizesToArray(start);
    if ((end - start) != bytes)
    {
        ByteSizeConsistencyError(bytes,
                                 message.ByteSize(),
                                 static_cast<int>(end - start));
    }
    buffer->HasWritten(bytes);

    buffer->PrependInt32(BytesChecksum(buffer->Peek(), buffer->ReadableBytes()));
    buffer->PrependInt32(static_cast<int32_t>(buffer->ReadableBytes()));
}

} // namespace protorpc
} // namespace claire
