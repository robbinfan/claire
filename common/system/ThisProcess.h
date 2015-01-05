// Copyright (c) 2013 The claire-common Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef _CLAIRE_COMMON_SYSTEM_THISPROCESS_H_
#define _CLAIRE_COMMON_SYSTEM_THISPROCESS_H_

#include <unistd.h>

#include <string>

namespace claire {
namespace ThisProcess {

pid_t pid();
std::string pid_string();

std::string User();
std::string Host();

} // namespace ThisProcess
} // namespace claire

#endif // _CLAIRE_COMMON_SYSTEM_THISPROCESS_H_
