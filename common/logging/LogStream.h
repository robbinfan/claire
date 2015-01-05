// Copyright (c) 2013 The claire-common Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef _CLAIRE_COMMON_LOGGING_LOGSTREAM_H_
#define _CLAIRE_COMMON_LOGGING_LOGSTREAM_H_

#include <string>

#include <claire/common/logging/LogBuffer.h>
#include <claire/common/strings/StringUtil.h>
#include <claire/common/strings/StringPiece.h>

namespace claire {

enum class LogFormat : char
{
    kNormal = 0,
    kHex = 1
};

class LogStream
{
public:
    typedef LogStream self;
    typedef LogBuffer<kSmallBuffer> Buffer;

    LogStream()
        : format_(LogFormat::kNormal)
    {}

    self& operator<<(bool v)
    {
        buffer_.Append(v ? "1" : "0", 1);
        return *this;
    }

    self& operator<<(char v)
    {
        buffer_.Append(&v, 1);
        return *this;
    }

    self& operator<<(short);
    self& operator<<(unsigned short);
    self& operator<<(int);
    self& operator<<(unsigned int);
    self& operator<<(long);
    self& operator<<(unsigned long);
    self& operator<<(long long);
    self& operator<<(unsigned long long);

    self& operator<<(const void *);

    self& operator<<(float v)
    {
        *this<< static_cast<double>(v);
        return *this;
    }

    self& operator<<(double v);

    self& operator<<(const char* v)
    {
        Append(v, strlen(v));
        return *this;
    }

    self& operator<<(const StringPiece& s)
    {
        Append(s.data(), s.size());
        return *this;
    }

    self& operator<<(StringPiece&& s) //move
    {
        Append(s.data(), s.size());
        return *this;
    }

    self& operator<<(const std::string& s)
    {
        Append(s.data(), s.size());
        return *this;
    }

    self& operator<<(std::string&& s)
    {
        Append(s.data(), s.size());
        return *this;
    }

    self& operator<<(const LogFormat& format)
    {
        format_ = format;
        return *this;
    }

    void set_ctr(int v)
    {
        ctr_ = v;
    }

    int ctr() const
    {
        return ctr_;
    }

    const Buffer& buffer() const
    {
        return buffer_;
    }

    void ResetBuffer()
    {
        format_ = LogFormat::kNormal;
        buffer_.Reset();
    }

    void Append(const char* msg, size_t len)
    {
        if (format_ == LogFormat::kNormal) // likely
        {
            buffer_.Append(msg, len);
        }
        else if (format_ == LogFormat::kHex)
        {
            buffer_.Add(HexString(buffer_.current(), buffer_.avail(), msg, len));
        }
    }

private:
    template<typename T>
    void FormatInteger(T);

    Buffer buffer_;
    LogFormat format_;
    int ctr_;
};

} // namespace claire

#endif // _CLAIRE_COMMON_LOGGING_LOGSTREAM_H_
