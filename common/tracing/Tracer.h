// Copyright (c) 2013 The claire-common Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef _CLAIRE_COMMON_TRACING_TRACER_H_
#define _CLAIRE_COMMON_TRACING_TRACER_H_

#include <claire/common/tracing/Trace.h>

namespace claire {

class Tracer
{
public:
    virtual ~Tracer() {}

    /// Record one or more annotations.
    virtual void Record(const Trace&, const Annotation&) {}
    virtual void Record(const Trace&, const BinaryAnnotation&) {}
};

} // namespace claire

#endif // _CLAIRE_COMMON_TRACING_TRACER_H_
