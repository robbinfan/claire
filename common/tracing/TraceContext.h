// Copyright (c) 2013 The claire-common Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef _CLAIRE_COMMON_TRACING_TRACECONTEXT_H_
#define _CLAIRE_COMMON_TRACING_TRACECONTEXT_H_

#include <stdint.h>

#include <utility>

namespace claire {

typedef std::pair<int64_t, int64_t> TraceContext;

namespace ThisThread {

void SetTraceContext(int64_t trace_id, int64_t span_id);
void SetTraceContext(const TraceContext& context);
TraceContext GetTraceContext();
void ResetTraceContext();

} // namespace claire

class TraceContextGuard
{
public:
    TraceContextGuard() {} // context has been set

    TraceContextGuard(int64_t trace_id, int64_t span_id)
    {
        ThisThread::SetTraceContext(trace_id, span_id);
    }

    explicit TraceContextGuard(const TraceContext& context)
    {
        ThisThread::SetTraceContext(context);
    }

    ~TraceContextGuard()
    {
        ThisThread::ResetTraceContext();
    }
};

} // namespace claire

#endif // _CLAIRE_COMMON_TRACING_TRACECONTEXT_H_
