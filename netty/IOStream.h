// Copyright (c) 2013 The claire-netty Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef _CLAIRE_NETTY_IOSTREAM_H_
#define _CLAIRE_NETTY_IOSTREAM_H_

#include <stddef.h>

#include <boost/noncopyable.hpp>

#include <claire/netty/Buffer.h>

namespace claire {

class IOStream : boost::noncopyable
{
public:
    enum Status
    {
        kOk,
        kReadPastEnd,
        kNoEnoughSpace,
        kInvalidData
    };

    IOStream(Buffer* buffer)
        : buffer_(buffer),
          status_(kOk)
    {}

    Status status() const { return status_; }

    void set_status(Status s)
    {
        if (status_ == kOk)
            status_ = s;
    }

    template<typename T, size_t N>
    inline size_t Read(T (&s)[N], size_t length)
    {
        static_assert(sizeof(T) == 1, "T must be char like size");
        if (N < length)
        {
            set_status(kReadPastEnd);
            return 0;
        }
        return buffer_->Read(&s[0], length);
    }

    template<typename T, size_t N>
    inline void Append(const T (&s)[N], size_t length)
    {
        static_assert(sizeof(T) == 1, "T must be char like size");
        if (N < length)
        {
            set_status(kNoEnoughSpace);
            return ;
        }
        buffer_->Append(&s[0], length);
    }

    void Skip(size_t length)
    {
        buffer_->Consume(length);
    }

    template<typename T>
    IOStream& operator<<(T i);

    template<typename T>
    IOStream& operator>>(T& i);

    operator bool() const { return status() == kOk; }

private:
    Buffer* buffer_;
    Status status_;
};

} // namespace claire

#endif // _CLAIRE_NETTY_IOSTREAM_H_
