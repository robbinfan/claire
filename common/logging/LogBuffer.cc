// Copyright (c) 2013 The claire-common Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include <claire/common/logging/LogBuffer.h>

namespace claire {

template<int SIZE>
const char* LogBuffer<SIZE>::DebugString()
{
    *current_ = '\0';
    return data_;
}

template<int SIZE>
void LogBuffer<SIZE>::CookieStart() {}
template<int SIZE> void LogBuffer<SIZE>::CookieEnd() {}

template class LogBuffer<kSmallBuffer>;
template class LogBuffer<kLargeBuffer>;

} // namespace claire
