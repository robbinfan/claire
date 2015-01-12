// Copyright (c) 2013 The claire-netty Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#pragma once

#include <string.h>

#include <vector>
#include <string>
#include <algorithm>

#include <claire/netty/Endian.h>
#include <claire/common/strings/StringPiece.h>
#include <claire/common/logging/Logging.h>

namespace claire {

/// A buffer class modeled after org.jboss.netty.buffer.ChannelBuffer
///
/// @code
/// +-------------------+------------------+------------------+
/// | prependable bytes |  readable bytes  |  writable bytes  |
/// |                   |     (CONTENT)    |                  |
/// +-------------------+------------------+------------------+
/// |                   |                  |                  |
/// 0      <=      reader_index   <=   writer_index    <=     size
/// @endcode
class Buffer
{
public:
    static const size_t kCheapPrepend = 8;
    static const size_t kInitialSize = 1024;

    Buffer()
        : buffer_(kCheapPrepend+kInitialSize),
          reader_index_(kCheapPrepend),
          writer_index_(kCheapPrepend)
    {
        DCHECK(ReadableBytes() == 0);
        DCHECK(WritableBytes() == kInitialSize);
        DCHECK(PrependableBytes() == kCheapPrepend);
    }

    Buffer(const void* data, size_t length)
        : buffer_(kCheapPrepend),
          reader_index_(kCheapPrepend),
          writer_index_(kCheapPrepend)
    {
        DCHECK(ReadableBytes() == 0);
        DCHECK(WritableBytes() == 0);
        DCHECK(PrependableBytes() == kCheapPrepend);
        Append(data, length);
    }

    void swap(Buffer& rhs)
    {
        buffer_.swap(rhs.buffer_);
        std::swap(reader_index_, rhs.reader_index_);
        std::swap(writer_index_, rhs.writer_index_);
    }

    size_t ReadableBytes() const { return writer_index_ - reader_index_; }
    size_t WritableBytes() const { return buffer_.size() - writer_index_; }
    size_t PrependableBytes() const { return reader_index_; }

    const char* FindCRLF() const
    {
        const char* crlf = std::search(Peek(), BeginWrite(), kCRLF, kCRLF+2);
        return crlf == BeginWrite() ? NULL : crlf;
    }

    const char* FindCRLF(const char* start) const
    {
        DCHECK(Peek() <= start);
        DCHECK(start <= BeginWrite());
        const char* crlf = std::search(start, BeginWrite(), kCRLF, kCRLF+2);
        return crlf == BeginWrite() ? NULL : crlf;
    }

    void Consume(size_t length)
    {
        CHECK(length <= ReadableBytes());
        if (length < ReadableBytes())
        {
            reader_index_ += length;
        }
        else
        {
            ConsumeAll();
        }
    }

    void ConsumeUntil(const char* end)
    {
        DCHECK(end >= Peek());
        DCHECK(end <= BeginWrite());
        Consume(end - Peek());
    }

    void ConsumeAll()
    {
        writer_index_ = kCheapPrepend;
        reader_index_ = kCheapPrepend;
    }

    std::string ConsumeAllAsString()
    {
        std::string result(Peek(), ReadableBytes());
        ConsumeAll();
        return result;
    }

    std::string ConsumeAsString(size_t length)
    {
        DCHECK(length <= ReadableBytes());
        std::string result(Peek(), length);
        Consume(length);
        return result;
    }

    StringPiece ToStringPiece() const
    {
        return StringPiece(Peek(), static_cast<int>(ReadableBytes()));
    }

    void Append(const void* data, size_t length)
    {
        EnsureWritableBytes(length);
        std::copy(static_cast<const char*>(data),
                  static_cast<const char*>(data)+length,
                  BeginWrite());
        HasWritten(length);
    }

    void Append(const StringPiece& s)
    {
        Append(s.data(), s.size());
    }

    void AppendInt64(int64_t x)
    {
        int64_t be64 = HostToNetwork64(x);
        Append(&be64, sizeof be64);
    }

    void AppendInt32(int32_t x)
    {
        int32_t be32 = HostToNetwork32(x);
        Append(&be32, sizeof be32);
    }

    void AppendInt16(int16_t x)
    {
        int16_t be16 = HostToNetwork16(x);
        Append(&be16, sizeof be16);
    }

    void AppendInt8(int8_t x)
    {
        Append(&x, sizeof x);
    }

    size_t Read(void* data, size_t length)
    {
        size_t readable_length = std::min(ReadableBytes(), length);
        ::memcpy(data, Peek(), readable_length);
        Consume(readable_length);
        return readable_length;
    }

    int32_t ReadInt32()
    {
        int32_t result = PeekInt32();
        Consume(sizeof result);
        return result;
    }

    int16_t ReadInt16()
    {
        int16_t result = PeekInt16();
        Consume(sizeof result);
        return result;
    }

    int8_t ReadInt8()
    {
        int8_t result = PeekInt8();
        Consume(sizeof result);
        return result;
    }

    const char* Peek() const { return begin() + reader_index_; }

    int32_t PeekInt32() const
    {
        CHECK(ReadableBytes() >= sizeof(int32_t));
        int32_t be32 = 0;
        ::memcpy(&be32, Peek(), sizeof be32);
        return NetworkToHost32(be32);
    }

    int16_t PeekInt16() const
    {
        CHECK(ReadableBytes() >= sizeof(int16_t));
        int16_t be16 = 0;
        ::memcpy(&be16, Peek(), sizeof be16);
        return NetworkToHost16(be16);
    }

    int8_t PeekInt8() const
    {
        CHECK(ReadableBytes() >= sizeof(int8_t));
        return *Peek();
    }

    void Prepend(const void* data, size_t length)
    {
        CHECK(length <= PrependableBytes());
        reader_index_ -= length;
        std::copy(static_cast<const char*>(data),
                  static_cast<const char*>(data)+length,
                  begin()+reader_index_);
    }

    void PrependInt32(int32_t x)
    {
        auto be32 = HostToNetwork32(static_cast<uint32_t>(x));
        Prepend(&be32, sizeof be32);
    }

    void PrependInt16(int16_t x)
    {
        auto be16 = HostToNetwork16(static_cast<uint16_t>(x));
        Prepend(&be16, sizeof be16);
    }

    void PrependInt8(int8_t x)
    {
        Prepend(&x, sizeof x);
    }

    char* BeginWrite() { return begin() + writer_index_; }
    const char* BeginWrite() const { return begin() + writer_index_; }

    void HasWritten(size_t length)
    {
        writer_index_ += length;
    }

    void EnsureWritableBytes(size_t length)
    {
        if (WritableBytes() < length)
        {
            Reserve(length);
        }
        DCHECK(WritableBytes() >= length);
    }

private:
    char* begin() { return &buffer_[0]; }
    const char* begin() const { return &buffer_[0]; }

    void Reserve(size_t length)
    {
        if ((WritableBytes()+PrependableBytes()) < (length+kCheapPrepend))
        {
            buffer_.resize(writer_index_+length);
        }
        else
        {
            DCHECK(kCheapPrepend < reader_index_);
            size_t readable = ReadableBytes();
            std::copy(begin()+reader_index_,
                      begin()+writer_index_,
                      begin()+kCheapPrepend);
            reader_index_ = kCheapPrepend;
            writer_index_ = reader_index_ + readable;
            DCHECK(readable == ReadableBytes());
        }
    }

    std::vector<char> buffer_;
    size_t reader_index_;
    size_t writer_index_;

    static const char kCRLF[];
};

} // namespace claire
