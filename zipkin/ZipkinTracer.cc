// Copyright (c) 2013 The claire-zipkin Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include <claire/zipkin/ZipkinTracer.h>

#include <sstream>

#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/insert_linebreaks.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/archive/iterators/ostream_iterator.hpp>

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TBufferTransports.h>

#include <claire/common/tracing/Trace.h>
#include <claire/netty/InetAddress.h>

#include <claire/zipkin/zipkinCore_types.h>

namespace claire {

namespace {

std::string base64_encode(const std::string& text)
{
    using namespace boost::archive::iterators;
    std::stringstream os;
    typedef 
        insert_linebreaks<         // insert line breaks every 72 characters
            base64_from_binary<    // convert binary values ot base64 characters
                transform_width<   // retrieve 6 bit integers from a sequence of 8 bit bytes
                    const char *,
                    6,
                    8
                >
            > 
            ,72
        > base64_text; // compose all the above operations in to a new iterator
    
    std::copy(
        base64_text(text.c_str()),
        base64_text(text.c_str() + text.size()),
        ostream_iterator<char>(os)
    );
    return os.str();
}

std::string FormatTraceMessage(const Trace& trace, const Annotation& origin_annotation)
{
    using apache::thrift::transport::TMemoryBuffer;
    using apache::thrift::protocol::TBinaryProtocol;

    zipkin::thrift::Span span;
    span.__set_trace_id(trace.trace_id());
    span.__set_name(trace.name());
    span.__set_id(trace.span_id());
    if (trace.has_parent_span_id())
    {
        span.__set_parent_id(trace.parent_span_id());
    }

    std::vector<zipkin::thrift::Annotation> annotations;
    zipkin::thrift::Annotation annotation;
    annotation.__set_timestamp(origin_annotation.timestamp.MicroSecondsSinceEpoch());
    annotation.__set_value(origin_annotation.value);

    if (origin_annotation.host.Valid() || trace.has_host())
    {
        const claire::Endpoint* origin_host = &origin_annotation.host;
        if (!origin_annotation.host.Valid())
        {
            origin_host = &trace.host();
        }
        zipkin::thrift::Endpoint host;
        host.__set_ipv4(origin_host->ipv4);
        host.__set_port(static_cast<int16_t>(origin_host->port));
        host.__set_service_name(origin_host->service_name);
        annotation.__set_host(host);
    }

    annotations.push_back(annotation);
    span.__set_annotations(annotations);

    boost::shared_ptr<TMemoryBuffer> buffer(new TMemoryBuffer());
    boost::shared_ptr<TBinaryProtocol> protocol(new TBinaryProtocol(buffer));

    span.write(protocol.get());
    return base64_encode(buffer->getBufferAsString());
}

std::string FormatTraceMessage(const Trace& trace, const BinaryAnnotation& origin_annotation)
{
    using apache::thrift::transport::TMemoryBuffer;
    using apache::thrift::protocol::TBinaryProtocol;

    zipkin::thrift::Span span;
    span.__set_trace_id(trace.trace_id());
    span.__set_name(trace.name());
    span.__set_id(trace.span_id());
    if (trace.has_parent_span_id())
    {
        span.__set_parent_id(trace.parent_span_id());
    }

    std::vector<zipkin::thrift::BinaryAnnotation> annotations;
    zipkin::thrift::BinaryAnnotation annotation;
    annotation.__set_key(origin_annotation.name);
    annotation.__set_value(origin_annotation.value);
    if (origin_annotation.annotation_type == "string")
    {
        annotation.__set_annotation_type(zipkin::thrift::AnnotationType::STRING);
    }

    if (origin_annotation.host.Valid() || trace.has_host())
    {
        const claire::Endpoint* origin_host = &origin_annotation.host;
        if (!origin_annotation.host.Valid())
        {
            origin_host = &trace.host();
        }
        zipkin::thrift::Endpoint host;
        host.__set_ipv4(origin_host->ipv4);
        host.__set_port(static_cast<int16_t>(origin_host->port));
        host.__set_service_name(origin_host->service_name);
        annotation.__set_host(host);
    }

    annotations.push_back(annotation);
    span.__set_binary_annotations(annotations);

    boost::shared_ptr<TMemoryBuffer> buffer(new TMemoryBuffer());
    boost::shared_ptr<TBinaryProtocol> protocol(new TBinaryProtocol(buffer));

    span.write(protocol.get());
    return base64_encode(buffer->getBufferAsString());
}

} // namespace

ZipkinTracer::ZipkinTracer(const InetAddress& scribe_address)
    : client_(scribe_address)
{}

ZipkinTracer::~ZipkinTracer() {}

void ZipkinTracer::Record(const Trace& trace, const Annotation& annotation)
{
    auto message = FormatTraceMessage(trace, annotation);
    client_.Log("zipkin", message);
}

void ZipkinTracer::Record(const Trace& trace, const BinaryAnnotation& annotation)
{
    auto message = FormatTraceMessage(trace, annotation);
    client_.Log("zipkin", message);
}

} // namespace claire
