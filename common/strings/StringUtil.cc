// Copyright (c) 2013 The claire-common Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include <claire/common/strings/StringUtil.h>

#include <stdio.h>
#include <assert.h>

#include <algorithm>

#include <boost/type_traits/is_arithmetic.hpp>

namespace claire {

#pragma GCC diagnostic ignored "-Wtype-limits"
const char digits[] = "9876543210123456789";
const char* zero = digits + 9;
static_assert(sizeof(digits) == 20, "digits size not equal 20");

const char digitsHex[] = "0123456789ABCDEF";
static_assert(sizeof(digitsHex) == 17, "digitsHex size not equal 17");

// Efficient Integer to String Conversions, by Matthew Wilson.
template<typename T>
size_t IntToBuffer(char buffer[], T value)
{
    T i = value;
    char* p = buffer;

    do
    {
        int lsd = static_cast<int>(i % 10);
        i /= 10;
        *p++ = zero[lsd];
    } while (i != 0);

    if (value < 0)
    {
        *p++ = '-';
    }

    *p = '\0';
    std::reverse(buffer, p);

    return p - buffer;
}

size_t HexToBuffer(char buffer[], uintptr_t value)
{
    uintptr_t i = value;
    char* p = buffer;

    do
    {
        int lsd = i % 16;
        i /= 16;
        *p++ = digitsHex[lsd];
    } while (i != 0);

    *p = '\0';
    std::reverse(buffer, p);

    return p - buffer;
}

template size_t IntToBuffer(char [], char);
template size_t IntToBuffer(char [], short);
template size_t IntToBuffer(char [], unsigned short);
template size_t IntToBuffer(char [], int);
template size_t IntToBuffer(char [], unsigned int);
template size_t IntToBuffer(char [], long);
template size_t IntToBuffer(char [], unsigned long);
template size_t IntToBuffer(char [], long long);
template size_t IntToBuffer(char [], unsigned long long);

size_t HexString(char buffer[], size_t buffer_length,
                 const char* data, size_t data_length)
{
    size_t current_length = 0;
    for (size_t i = 0;i < data_length; i++)
    {
        int ret = 0;
        if (i % 16 == 0)
        {
            ret = snprintf(buffer + current_length, buffer_length - current_length, "\n0x%04x:", static_cast<unsigned int>(i));
            if (ret < 0 || ret > static_cast<int>(buffer_length - current_length))
            {
                break;
            }
            current_length += ret;
        }

        if (i % 2 == 0)
        {
            ret = snprintf(buffer + current_length, buffer_length - current_length, " ");
            if (ret < 0 || ret > static_cast<int>(buffer_length - current_length))
            {
                break;
            }
            current_length += ret;
        }

        ret = snprintf(buffer + current_length,  buffer_length - current_length, "%02x", static_cast<uint8_t>(data[i]));
        if (ret < 0 || ret > static_cast<int>(buffer_length - current_length))
        {
            break;
        }
        current_length += ret;
    }

    return current_length;
}

} // namespace claire
