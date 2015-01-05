// Copyright (c) 2013 The claire-common Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

// Copyright 2001-2010 The RE2 Authors.  All Rights Reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// A string-like object that points to a sized piece of memory.
//
// Functions or methods may use const StringPiece& parameters to accept either
// a "const char*" or a "string" value that will be implicitly converted to
// a StringPiece.  The implicit conversion means that it is often appropriate
// to include this .h file in other files rather than forward-declaring
// StringPiece as would be appropriate for most other Google classes.
//
// Systematic usage of StringPiece is encouraged as it will reduce unnecessary
// conversions from "const char*" to "string" and back again.
//

#include <claire/common/strings/StringPiece.h>
#include <claire/common/logging/Logging.h>

namespace claire {

bool StringPiece::_equal(const StringPiece& x, const StringPiece& y)
{
    size_t len = x.size();
    if (len != y.size())
    {
        return false;
    }
    const char* p = x.data();
    const char* p2 = y.data();
    // Test last byte in case strings share large common prefix
    if ((len > 0) && (p[len-1] != p2[len-1])) return false;
    const char* p_limit = p + len;
    for (; p < p_limit; p++, p2++)
    {
        if (*p != *p2)
            return false;
    }
    return true;
}

StringPiece::size_type StringPiece::copy(char* buf, size_type n, size_type pos) const
{
    size_t ret = std::min(length_ - pos, n);
    ::memcpy(buf, ptr_ + pos, ret);
    return ret;
}

StringPiece::size_type StringPiece::find(const StringPiece& s, size_type pos) const
{
    if (pos > length_)
    {
        return npos;
    }

    const char* result = std::search(ptr_ + pos, ptr_ + length_,
                                     s.ptr_, s.ptr_ + s.length_);
    const size_type xpos = result - ptr_;
    return xpos + s.length_ <= length_ ? xpos : npos;
}

StringPiece::size_type StringPiece::find(char c, size_type pos) const
{
    if (length_ <= 0 || pos >= length_)
    {
        return npos;
    }
    const char* result = std::find(ptr_ + pos, ptr_ + length_, c);
    return result != ptr_ + length_ ? result - ptr_ : npos;
}

StringPiece::size_type StringPiece::rfind(const StringPiece& s, size_type pos) const
{
    if (length_ < s.length_) return npos;
    const size_t ulen = length_;
    if (s.length_ == 0) return std::min(ulen, pos);

    const char* last = ptr_ + std::min(ulen - s.length_, pos) + s.length_;
    const char* result = std::find_end(ptr_, last, s.ptr_, s.ptr_ + s.length_);
    return result != last ? result - ptr_ : npos;
}

StringPiece::size_type StringPiece::rfind(char c, size_type pos) const
{
    if (length_ <= 0) return npos;
    for (int i = static_cast<int>(std::min(pos, length_ - 1)); i >= 0; --i)
    {
        if (ptr_[i] == c)
        {
            return static_cast<size_type>(i);
        }
    }
    return npos;
}

StringPiece StringPiece::substr(size_type pos, size_type n) const
{
    if (pos > length_) pos = length_;
    if (n > length_ - pos) n = length_ - pos;
    return StringPiece(ptr_ + pos, n);
}

const StringPiece::size_type StringPiece::npos = size_type(-1);

} // namespace claire
