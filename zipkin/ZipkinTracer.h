// Copyright (c) 2013 The claire-zipkin Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef _CLAIRE_ZIPKIN_ZIPKINTRACER_H_
#define _CLAIRE_ZIPKIN_ZIPKINTRACER_H_

#include <claire/common/tracing/Tracer.h>

#include <boost/noncopyable.hpp>

#include <claire/zipkin/ScribeClient.h>

namespace claire {

class InetAddress;
class Trace;
class Annotation;
class BinaryAnnotation;

class ZipkinTracer : public Tracer,
                     boost::noncopyable
{
public:
    ZipkinTracer(const InetAddress& scribe_address);
    virtual ~ZipkinTracer();

    virtual void Record(const Trace& trace, const Annotation& annotation);
    virtual void Record(const Trace& trace, const BinaryAnnotation& annotation);

private:
    ScribeClient client_;
};

} // namespace claire

#endif // _CLAIRE_ZIPKIN_ZIPKINTRACER_H_
