// Copyright (c) 2013 The claire-common Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include <claire/common/tracing/TraceContext.h>

namespace claire {
namespace ThisThread {

__thread int64_t tTraceId = 0;
__thread int64_t tSpanId = 0;

void SetTraceContext(int64_t trace_id, int64_t span_id)
{
    tTraceId = trace_id;
    tSpanId = span_id;
}

void SetTraceContext(const TraceContext& context)
{
    tTraceId = context.first;
    tSpanId = context.second;
}

TraceContext GetTraceContext()
{
    return std::make_pair(tTraceId, tSpanId);
}
void ResetTraceContext()
{
    tTraceId = 0;
    tSpanId = 0;
}

} // namespace ThisThread
} // namespace claire
