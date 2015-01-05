// Copyright (c) 2013 The claire-common Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <claire/common/tracing/Trace.h>

#include <claire/common/threading/Singleton.h>
#include <claire/common/tracing/Trace.h>
#include <claire/common/logging/Logging.h>

namespace claire {

// static
TraceRecorder* TraceRecorder::instance()
{
    return Singleton<TraceRecorder>::instance();
}

Trace* TraceRecorder::RegisterOrDeleteDuplicate(Trace* trace)
{
    Trace* trace_to_delete = NULL;
    Trace* trace_to_return = NULL;
    auto id(std::make_pair(trace->trace_id(), trace->span_id()));

    {
        MutexLock lock(mutex_);
        auto it = traces_.find(id);
        if (traces_.end() == it)
        {
            traces_[id] = trace;
            trace_to_return = trace;
        }
        else if (trace == it->second)
        {
            // The trace was registered before.
            trace_to_return = trace;
        }
        else
        {
            // We already have one trace with this ids
            trace_to_return = it->second;
            trace_to_delete = trace;
        }
    }
    delete trace_to_delete;
    return trace_to_return;
}

Trace* TraceRecorder::Find(int64_t trace_id, int64_t span_id)
{
    MutexLock lock(mutex_);
    auto it = traces_.find(std::make_pair(trace_id, span_id));
    if (traces_.end() == it)
        return NULL;
    return it->second;
}

void TraceRecorder::Erase(int64_t trace_id, int64_t span_id)
{
    MutexLock lock(mutex_);
    auto it = traces_.find(std::make_pair(trace_id, span_id));
    if (it == traces_.end())
    {
        return ;
    }
    delete it->second;
    traces_.erase(it);
}

} // namespace claire
