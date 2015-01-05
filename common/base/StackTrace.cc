// Copyright (c) 2013 The claire-common Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include <claire/common/base/StackTrace.h>

#include <stdlib.h>
#include <execinfo.h>

#undef CLAIRE_DEMANGLE
#if defined(__GNUG__) && __GNUG__ >= 4
#include <cxxabi.h>
#include <string.h>
#define CLAIRE_DEMANGLE 1
#endif

namespace claire {

#ifdef CLAIRE_DEMANGLE
std::string demangle(const char* name)
{
    int status;
    size_t length = 0;
    // malloc() memory for the demangled type name
    char* demangled = abi::__cxa_demangle(name, NULL, &length, &status);
    if (status != 0)
    {
        return name;
    }
    // len is the length of the buffer (including NUL terminator and maybe
    // other junk)

    std::string result(demangled, strlen(demangled));
    free(demangled);
    return result;
}

#else

std::string demangle(const char* name)
{
    return name;
}

#endif // CLAIRE_DEMANGLE
#undef CLAIRE_DEMANGLE

std::string GetStackTrace(int escape_depth)
{
    std::string stack;

    const int length = 200;
    void* buffer[length];
    int nptrs = ::backtrace(buffer, length);
    char** strings = ::backtrace_symbols(buffer, nptrs);
    if (strings)
    {
        for (int i = escape_depth; i < nptrs; ++i)
        {
            stack.append(demangle(strings[i]));
            stack.append("\r\n");
        }
        free(strings);
    }
    return stack;
}

} // namespace claire
