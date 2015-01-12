// Copyright (c) 2013 The claire-netty Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include <claire/netty/IOStream.h>

#include <algorithm>

namespace claire {

namespace  {

template<typename T>
T Swap(T v)
{
    T ret = v;
    std::reverse(reinterpret_cast<char*>(&ret),
                 reinterpret_cast<char*>(&ret)+sizeof(v));
    return ret;
}

} // namespace

template<typename T>
IOStream& IOStream::operator<<(T i)
{
    T v = Swap(i);
    buffer_->Append(&v, sizeof v);
    return *this;
}

template IOStream& IOStream::operator<<(uint8_t);
template IOStream& IOStream::operator<<(int8_t);
template IOStream& IOStream::operator<<(uint16_t);
template IOStream& IOStream::operator<<(int16_t);
template IOStream& IOStream::operator<<(uint32_t);
template IOStream& IOStream::operator<<(int32_t);
template IOStream& IOStream::operator<<(uint64_t);
template IOStream& IOStream::operator<<(int64_t);
template IOStream& IOStream::operator<<(float);
template IOStream& IOStream::operator<<(double);

template<typename T>
IOStream& IOStream::operator>>(T& i)
{
    if (buffer_->Read(&i, sizeof i) < sizeof i)
    {
        set_status(kReadPastEnd);
    }
    i = Swap(i);
    return *this;
}

template IOStream& IOStream::operator>>(uint8_t&);
template IOStream& IOStream::operator>>(int8_t&);
template IOStream& IOStream::operator>>(uint16_t&);
template IOStream& IOStream::operator>>(int16_t&);
template IOStream& IOStream::operator>>(uint32_t&);
template IOStream& IOStream::operator>>(int32_t&);
template IOStream& IOStream::operator>>(uint64_t&);
template IOStream& IOStream::operator>>(int64_t&);
template IOStream& IOStream::operator>>(float&);
template IOStream& IOStream::operator>>(double&);

} // namespace claire
