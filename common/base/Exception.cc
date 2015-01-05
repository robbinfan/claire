// Copyright (c) 2013 The claire-common Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include <claire/common/base/Exception.h>
#include <claire/common/base/StackTrace.h>

namespace claire {

Exception::Exception(const char* what__)
    : what_(what__),
      stack_trace_(GetStackTrace(2))
{}

Exception::Exception(const std::string& what__)
    : what_(what__),
      stack_trace_(GetStackTrace(2))
{}

} // namespace claire

