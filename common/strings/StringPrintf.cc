// Copyright (c) 2013 The claire-common Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <claire/common/strings/StringPrintf.h>

#include <stdio.h>
#include <errno.h>

#include <claire/common/logging/Logging.h>

namespace claire {

namespace {

// Templatized backend for StringPrintF/StringAppendF. This does not finalize
// the va_list, the caller is expected to do that.
template <class StringType>
static void StringAppendVT(StringType* dst,
                           const typename StringType::value_type* format,
                           va_list ap)
{
    // First try with a small fixed size buffer.
    // This buffer size should be kept in sync with StringUtilTest.GrowBoundary
    // and StringUtilTest.StringPrintfBounds.
    typename StringType::value_type stack_buf[1024];

    va_list ap_copy;
    va_copy(ap_copy, ap);

    int result = vsnprintf(stack_buf, sizeof(stack_buf)/sizeof(stack_buf[0]), format, ap_copy);
    va_end(ap_copy);

    if (result >= 0 && result < static_cast<int>(sizeof(stack_buf)/sizeof(stack_buf[0])))
    {
        // It fit.
        dst->append(stack_buf, result);
        return;
    }

    // Repeatedly increase buffer size until it fits.
    int mem_length = static_cast<int>(sizeof(stack_buf)/sizeof(stack_buf[0]));
    while (true)
    {
        if (result < 0) {
            if (errno != 0 && errno != EOVERFLOW)
            {
                // If an error other than overflow occurred, it's never going to work.
                DLOG(WARNING) << "Unable to printf the requested string due to error.";
                return;
            }
            // Try doubling the buffer size.
            mem_length *= 2;
        }
        else
        {
            // We need exactly "result + 1" characters.
            mem_length = result + 1;
        }

        if (mem_length > 32 * 1024 * 1024)
        {
            // That should be plenty, don't try anything larger.  This protects
            // against huge allocations when using vsnprintfT implementations that
            // return -1 for reasons other than overflow without setting errno.
            DLOG(WARNING) << "Unable to printf the requested string due to size.";
            return;
        }

        std::vector<typename StringType::value_type> mem_buf(mem_length);

        // NOTE: You can only use a va_list once.  Since we're in a while loop, we
        // need to make a new copy each time so we don't use up the original.
        va_copy(ap_copy, ap);
        result = vsnprintf(&mem_buf[0], mem_length, format, ap_copy);
        va_end(ap_copy);

        if ((result >= 0) && (result < mem_length))
        {
            // It fit.
            dst->append(&mem_buf[0], result);
            return;
        }
    }
}

}  // namespace

std::string StringPrintf(const char* format, ...)
{
    va_list ap;
    va_start(ap, format);
    std::string result;
    StringAppendV(&result, format, ap);
    va_end(ap);
    return result;
}

std::string StringPrintV(const char* format, va_list ap)
{
    std::string result;
    StringAppendV(&result, format, ap);
    return result;
}

const std::string& SStringPrintf(std::string* dst, const char* format, ...)
{
    va_list ap;
    va_start(ap, format);
    dst->clear();
    StringAppendV(dst, format, ap);
    va_end(ap);
    return *dst;
}

void StringAppendF(std::string* dst, const char* format, ...)
{
    va_list ap;
    va_start(ap, format);
    StringAppendV(dst, format, ap);
    va_end(ap);
}

void StringAppendV(std::string* dst, const char* format, va_list ap)
{
    StringAppendVT(dst, format, ap);
}

}  // namespace common
