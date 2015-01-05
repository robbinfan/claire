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
//
// Arghh!  I wish C++ literals were "string".

#ifndef _CLAIRE_COMMON_STRINGS_STRINGPIECE_H_
#define _CLAIRE_COMMON_STRINGS_STRINGPIECE_H_

#include <assert.h>
#include <string.h>
#include <stddef.h>

#include <string>
#include <algorithm>

namespace claire {

class StringPiece
{
public:
    StringPiece() : ptr_(NULL), length_(0) {}

    StringPiece(const char* str)
        : ptr_(str), length_(str == NULL ? 0 : strlen(str)) {}

    StringPiece(const std::string& str)
        : ptr_(str.data()), length_(str.size()) {}

    StringPiece(const char* offset, size_t len)
        : ptr_(offset), length_(len) {}

    StringPiece(const char* left, const char* right)
        : ptr_(left), length_(right - left)
    {
        assert((right - left) >= 0);
    }

    // data() may return a pointer to a buffer with embedded NULs, and the
    // returned buffer may or may not be null terminated.  Therefore it is
    // typically a mistake to pass data() to a routine that expects a NUL
    // terminated string.
    const char* data() const { return ptr_; }
    size_t size() const { return length_; }
    size_t length() const { return length_; }
    bool empty() const { return length_ == 0; }

    void clear() { ptr_ = NULL, length_ = 0; }
    void set(const char* str, size_t len) { ptr_ = str; length_ = len; }
    void set(const char* str)
    {
        ptr_ = str;
        if (str != NULL)
        {
            length_ = strlen(ptr_);
        }
        else
        {
            length_ = 0;
        }
    }

    void set(const void* str, size_t len)
    {
        ptr_ = reinterpret_cast<const char*>(str);
        length_ = len;
    }

    char operator[](size_t n) const { return ptr_[n]; }

    bool starts_with(const StringPiece& x) const
    {
        return ((length_ >= x.length_) &&
                (memcmp(ptr_, x.ptr_, x.length_) == 0));
    }

    bool end_with(const StringPiece& x) const
    {
        return ((length_ >= x.length_) &&
                (memcmp(ptr_ + (length_ - x.length_), x.ptr_, x.length_) == 0));
    }

    void remove_prefix(size_t n)
    {
        assert(n <= length_);
        ptr_ += n;
        length_ -= n;
    }

    void remove_suffix(size_t n)
    {
        assert(n <= length_);
        length_ -= n;
    }

    int compare(const StringPiece& x) const
    {
        int r = memcmp(ptr_, x.ptr_, std::min(length_, x.length_));
        if (r == 0)
        {
            if (length_ < x.length_)
            {
                r = -1;
            }
            else if (length_ > x.length_)
            {
                r += 1;
            }
        }

        return r;
    }

    std::string ToString() const
    {
        return std::string(data(), size());
    }

    void CopyToString(std::string* target) const
    {
        target->assign(ptr_, length_);
    }

    void AppendToString(std::string* target) const
    {
        target->append(ptr_, length_);
    }

    typedef char value_type;

    typedef const char* pointer;
    typedef const char* const_pointer;
    typedef const char& reference;
    typedef const char& const_reference;

    typedef size_t size_type;
    typedef ptrdiff_t diffrencen_type;

    typedef const char* const_iterator;
    typedef const char* iterator;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
    typedef std::reverse_iterator<iterator> reverse_iterator;

    static const size_type npos;

    iterator begin() const { return ptr_; }
    iterator end() const { return ptr_ + length_; }
    const_iterator cbegin() const { return ptr_; }
    const_iterator cend() const { return ptr_ + length_; }

    reverse_iterator rbegin() const
    {
        return reverse_iterator(ptr_ + length_);
    }

    reverse_iterator rend() const
    {
        return reverse_iterator(ptr_);
    }

    const_reverse_iterator crbegin() const
    {
        return const_reverse_iterator(ptr_ + length_);
    }

    const_reverse_iterator crend() const
    {
        return const_reverse_iterator(ptr_);
    }

    size_type max_size() const { return length_; }
    size_type capacity() const { return length_; }

    size_type copy(char* buf, size_type n, size_type pos = 0) const;

    size_type find(const StringPiece& s, size_type pos = 0) const;
    size_type find(char c, size_type pos = 0) const;

    size_type rfind(const StringPiece& s, size_type pos = npos) const;
    size_type rfind(char c, size_type pos = npos) const;

    StringPiece substr(size_type pos, size_type n = npos) const;

    static bool _equal(const StringPiece&, const StringPiece&);

    void advance(size_t offset)
    {
        assert(offset < length_);
        ptr_ += offset;
        return ;
    }

    void assign(const char* start, const char* end__)
    {
        ptr_ = start;
        length_ = end__ - start;
    }

    void reset(const std::string& str)
    {
        reset(str.data(), str.length());
    }

    void reset(const char* start, size_t len)
    {
        ptr_ = start;
        length_ = len;
    }

private:
    const char* ptr_;
    size_t length_;
};

inline bool operator==(const StringPiece& x, const StringPiece& y)
{
    return StringPiece::_equal(x, y);
}

inline bool operator!=(const StringPiece& x, const StringPiece& y)
{
    return !(x == y);
}

inline bool operator<(const StringPiece& x, const StringPiece& y)
{
    const int r = memcmp(x.data(), y.data(),
                         std::min(x.size(), y.size()));
    return ((r < 0) || ((r == 0) && (x.size() < y.size())));
}

inline bool operator>(const StringPiece& x, const StringPiece& y)
{
    return y < x;
}

inline bool operator<=(const StringPiece& x, const StringPiece& y)
{
    return !(x > y);
}

inline bool operator>=(const StringPiece& x, const StringPiece& y)
{
    return !(x < y);
}

} // namespace claire

#endif // _CLAIRE_COMMON_STRINGS_STRINGPIECE_H_
