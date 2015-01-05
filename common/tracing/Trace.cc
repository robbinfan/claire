// Copyright (c) 2013 The claire-common Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include <claire/common/tracing/Trace.h>

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>

#include <claire/common/tracing/Tracing.h>
#include <claire/common/logging/Logging.h>

namespace claire {

namespace {

int64_t UniqueId()
{
    // thread safe
    static boost::random::mt19937 generator;
    static boost::random::uniform_int_distribution<int64_t> distribution(1, 2L<<56);
    return distribution(generator);
}

} // namespace

//static
Trace* Trace::FactoryGet(const std::string& name)
{
    return FactoryGet(name, UniqueId(), UniqueId(), 0);
}

Trace* Trace::FactoryGet(const std::string& name,
                         int64_t trace_id,
                         int64_t span_id)
{
    return FactoryGet(name, trace_id, span_id, 0);
}

Trace* Trace::FactoryGet(const std::string& name,
                         int64_t trace_id,
                         int64_t span_id,
                         int64_t parent_span_id)
{
    auto trace = new Trace(name, trace_id, span_id, parent_span_id);
    return TraceRecorder::instance()->RegisterOrDeleteDuplicate(trace);
}

Trace::Trace(const std::string& name__,
             int64_t trace_id__,
             int64_t span_id__,
             int64_t parent_span_id__)
    : name_(name__),
      trace_id_(trace_id__),
      span_id_(span_id__),
      parent_span_id_(parent_span_id__)
{}

void Trace::Record(const Annotation& annotation)
{
    LOG(DEBUG) << "Trace " << trace_id_ << ", " << span_id_ << " " << annotation.value << " at " << annotation.timestamp.MicroSecondsSinceEpoch()
               << " for " << host_.service_name;
    OutputTrace(*this, annotation);
}

void Trace::Record(const BinaryAnnotation& annotation)
{
    LOG(DEBUG) << "Trace " << trace_id_ << ", " << span_id_ << " " << annotation.value
               << " for " << host_.service_name;
    OutputTrace(*this, annotation);
}

Trace* Trace::MakeChild(const std::string& name__)
{
    return FactoryGet(name__, trace_id_, span_id_+1, span_id_);
}

} // namespace claire
