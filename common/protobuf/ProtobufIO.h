// Copyright (c) 2013 The claire-common Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef _CLAIRE_COMMON_PROTOBUF_PROTOBUFIO_H_
#define _CLAIRE_COMMON_PROTOBUF_PROTOBUFIO_H_

#include <string>

namespace google {
namespace protobuf {

class Message;
} // namespace protobuf
} // namespace google

namespace claire {

class StringPiece;

bool SerializeToJson(const ::google::protobuf::Message& message,
                     std::string* output);

bool ParseFromJson(const StringPiece& json,
                   ::google::protobuf::Message* message);


} // namespace claire

#endif // _CLAIRE_COMMON_PROTOBUF_PROTOBUFIO_H_
