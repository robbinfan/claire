// Copyright (c) 2013 The claire-common Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef _CLAIRE_COMMON_TRACING_TRACING_H_
#define _CLAIRE_COMMON_TRACING_TRACING_H_

#include <claire/common/strings/StringPrintf.h>
#include <claire/common/tracing/TraceRecorder.h>
#include <claire/common/tracing/TraceContext.h>
#include <claire/common/tracing/Trace.h>

#define TRACE_PRINTF(fmt, args...) \
    do { \
        auto trace = claire::TraceRecorder::instance()->Find(claire::ThisThread::GetTraceContext()); \
        if (trace) trace->Record(Annotation(claire::StringPrintf(fmt, ## args))); \
    } while (0)

#define GET_TRACE_BY_TRACEID(trace_id, span_id) \
    (claire::TraceRecorder::instance()->Find(trace_id, span_id))

#define THISTHREAD_TRACE() \
    (claire::TraceRecorder::instance()->Find(claire::ThisThread::GetTraceContext()))

#define TRACE_ANNOTATION(annotation) \
    do { \
        auto trace = claire::TraceRecorder::instance()->Find(claire::ThisThread::GetTraceContext()); \
        if (trace) trace->Record(annotation); \
    } while (0)

#define TRACE_SET_HOST(ip, port, service_name) \
    do { \
        auto trace = claire::TraceRecorder::instance()->Find(claire::ThisThread::GetTraceContext()); \
        if (trace) trace->set_host(claire::Endpoint(ip, port, service_name)); \
    } while (0)

#define ERASE_TRACE() \
    claire::TraceRecorder::instance()->Erase(claire::ThisThread::GetTraceContext()) \

namespace claire {

class Tracer;
void InstallClaireTracer(Tracer* tracer);
void OutputTrace(const Trace&, const Annotation&);
void OutputTrace(const Trace&, const BinaryAnnotation&);

} // namespace claire

#endif // _CLAIRE_COMMON_TRACING_TRACING_H_
