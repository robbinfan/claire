// Copyright (c) 2013 The claire-common Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef _CLAIRE_COMMON_BASE_STACKTRACE_H_
#define _CLAIRE_COMMON_BASE_STACKTRACE_H_

#include <string>

namespace claire {

std::string demangle(const char* name);
std::string GetStackTrace(int escape_depth);

} // namespace claire

#endif // _CLAIRE_COMMON_BASE_STACKTRACE_H_
