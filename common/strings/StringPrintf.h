// Copyright (c) 2013 The claire-common Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef _CLAIRE_COMMON_STRINGS_STRINGPRINTF_H_
#define _CLAIRE_COMMON_STRINGS_STRINGPRINTF_H_

#include <stdarg.h>   // va_list

#include <string>

namespace claire {

// Return a C++ string given printf-like input.
std::string StringPrintf(const char* format, ...)
    __attribute__((format(printf, 1, 2)));

// Return a C++ string given vprintf-like input.
std::string StringPrintV(const char* format, va_list ap)
    __attribute__((format(printf, 1, 0)));

// Store result into a supplied string and return it.
const std::string& SStringPrintf(std::string* dst,
                                 const char* format, ...)
    __attribute__((format(printf, 2, 3)));

// Append result to a supplied string.
void StringAppendF(std::string* dst, const char* format, ...)
    __attribute__((format(printf, 2, 3)));

// Lower-level routine that takes a va_list and appends to a specified
// string.  All other routines are just convenience wrappers around it.
void StringAppendV(std::string* dst, const char* format, va_list ap)
    __attribute__((format(printf, 2, 0)));

}  // namespace claire

#endif // _CLAIRE_COMMON_STRINGS_STRINGPRINTF_H_
