// Copyright (c) 2013 The claire-common Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include <claire/common/tracing/Tracing.h>
#include <claire/common/tracing/Tracer.h>

namespace claire {

static Tracer g_null_tracer;
static Tracer* g_tracer = &g_null_tracer;

void InstallClaireTracer(Tracer* tracer)
{
    g_tracer = tracer;
}

void OutputTrace(const Trace& trace, const Annotation& annotation)
{
    g_tracer->Record(trace, annotation);
}

void OutputTrace(const Trace& trace, const BinaryAnnotation& annotation)
{
    g_tracer->Record(trace, annotation);
}

} // namespace claire
