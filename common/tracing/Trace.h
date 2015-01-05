// Copyright (c) 2013 The claire-common Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef _CLAIREC_COMMON_TRACING_TRACE_H_
#define _CLAIREC_COMMON_TRACING_TRACE_H_

#include <string>

#include <boost/noncopyable.hpp>

#include <claire/common/time/Timestamp.h>
#include <claire/common/tracing/TraceRecorder.h>

namespace claire {

struct Endpoint
{
    Endpoint()
        : ipv4(0),
          port(-1)
    {}

    Endpoint(int ipv4__, int port__, const std::string& service_name__)
        : ipv4(ipv4__),
          port(port__),
          service_name(service_name__)
    {}

    bool Valid() const
    {
        return ipv4 != 0 && port >= 0;
    }

    void operator=(const Endpoint& other)
    {
        ipv4 = other.ipv4;
        port = other.port;
        service_name = other.service_name;
    }

    int ipv4;
    int port;
    std::string service_name;
};

struct Annotation
{
public:
    Annotation(const std::string& value__)
        : timestamp(Timestamp::Now()),
          value(value__)
    {}

    Annotation(const std::string& value__, const Endpoint& host__)
        : timestamp(Timestamp::Now()),
          value(value__),
          host(host__)
    {}

    static Annotation client_send() { return Annotation("cs"); }
    static Annotation client_recv() { return Annotation("cr"); }
    static Annotation server_send() { return Annotation("ss"); }
    static Annotation server_recv() { return Annotation("sr"); }

    Timestamp timestamp;
    std::string value;
    Endpoint host;
};

struct BinaryAnnotation
{
    BinaryAnnotation(const std::string& name__,
                     const std::string& value__,
                     const std::string& annotation_type__)
        : name(name__),
          value(value__),
          annotation_type(annotation_type__)
    {}

    static BinaryAnnotation message(const std::string& name__, const std::string& value__)
    {
        return BinaryAnnotation(name__, value__, "string");
    }

    std::string name;
    std::string value;
    std::string annotation_type;
    Endpoint host;
};

class Trace : boost::noncopyable
{
public:
    static Trace* FactoryGet(const std::string& name);
    static Trace* FactoryGet(const std::string& name,
                             int64_t trace_id,
                             int64_t span_id);
    static Trace* FactoryGet(const std::string& name,
                             int64_t trace_id,
                             int64_t span_id,
                             int64_t parent_span_id);

    bool has_host() const { return host_.Valid(); }
    const Endpoint& host() const { return host_; }
    void set_host(const Endpoint& host__)
    {
        host_ = host__;
    }

    const std::string& name() const { return name_; }
    int64_t trace_id() const { return trace_id_;}
    int64_t span_id() const { return span_id_; }

    bool has_parent_span_id() const { return parent_span_id_ > 0; }
    int64_t parent_span_id() const { return parent_span_id_; }

    void Record(const Annotation& annotation);
    void Record(const BinaryAnnotation& annotation);

    Trace* MakeChild(const std::string& name);

private:
    Trace(const std::string& name,
          int64_t trace_id,
          int64_t span_id,
          int64_t parent_span_id);
    ~Trace() {}
    friend class TraceRecorder;

    std::string name_;
    int64_t trace_id_;
    int64_t span_id_;
    int64_t parent_span_id_;
    Endpoint host_;
};

} // namespace claire

#endif // _CLAIREC_COMMON_TRACING_TRACE_H_
