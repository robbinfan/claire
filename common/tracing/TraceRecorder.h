// Copyright (c) 2013 The claire-common Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef _CLAIRE_COMMON_TRACING_TRACERECORDER_H_
#define _CLAIRE_COMMON_TRACING_TRACERECORDER_H_

#include <map>
#include <string>
#include <utility>

#include <boost/noncopyable.hpp>

#include <claire/common/threading/Mutex.h>
#include <claire/common/threading/Singleton.h>

namespace claire {

class Trace;

class TraceRecorder : boost::noncopyable
{
public:
    static TraceRecorder* instance();

    // Register, or add a new trace to the collection of traces. If an
    // identically named traces is already registered, then the argument
    // |trace| will deleted.  The returned value is always the registered
    // trace (either the argument, or the pre-existing registered trace).
    Trace* RegisterOrDeleteDuplicate(Trace* trace);

    // Find a trace by trace id and span id. This method is thread
    // safe.  It returns NULL if a matching histogram is not found.
    Trace* Find(int64_t trace_id, int64_t span_id);
    Trace* Find(const std::pair<int64_t, int64_t>& context)
    {
        return Find(context.first, context.second);
    }

    // Erase trace from registered collections and release memory
    void Erase(int64_t trace_id, int64_t span_id);
    void Erase(const std::pair<int64_t, int64_t>& context)
    {
        return Erase(context.first, context.second);
    }

private:
    // We keep all registered traces in a map, from id to trace
    typedef std::pair<int64_t, int64_t> TraceId;
    typedef std::map<TraceId, Trace*> TraceMap;

    TraceRecorder() {}
    ~TraceRecorder() {}
    friend class Singleton<TraceRecorder>;

    Mutex mutex_;
    TraceMap traces_;
};

} // namespace claire

#endif // _CLAIRE_COMMON_TRACING_TRACERECORDER_H_
