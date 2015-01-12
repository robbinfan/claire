// Copyright (c) 2013 The claire-netty Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include <claire/netty/http/HttpResponse.h>

#include <stdio.h>

#include <vector>

#include <boost/algorithm/string.hpp>

#include <claire/netty/Buffer.h>
#include <claire/common/logging/Logging.h>

namespace claire {
namespace {

static const struct {
    int status;
    const char* reason_phrase;
} kStatusInfo[] =
{
    { 100, "Continue" },
    { 101, "Switching Protocols" },
    { 200, "OK" },
    { 201, "Created" },
    { 202, "Accepted" },
    { 203, "Non-Authoritative Information"},
    { 204, "No Content" },
    { 205, "Reset Content" },
    { 206, "Partial Content" },
    { 300, "Multiple Choices" },
    { 301, "Moved Permanently" },
    { 302, "Found" },
    { 303, "See Other" },
    { 304, "Not Modified" },
    { 305, "Use Proxy" },
    { 307, "Temporary Redirect" },
    { 400, "Bad Request" },
    { 401, "Unauthorized" },
    { 402, "Payment Required" },
    { 403, "Forbidden" },
    { 404, "Not Found" },
    { 405, "Method Not Allowed" },
    { 406, "Not Acceptable" },
    { 407, "Proxy Authentication Required" },
    { 408, "Request Timeout" },
    { 409, "Conflict" },
    { 410, "Gone" },
    { 411, "Length Required" },
    { 412, "Precondition Failed" },
    { 413, "Request Entity Too Large" },
    { 414, "Request-URI Too Long" },
    { 415, "Unsupported Media Type" },
    { 416, "Requested Range Not Satisfiable" },
    { 417, "Expectation Failed" },
    { 500, "Internal Server Error" },
    { 501, "Not Implemented" },
    { 502, "Bad Gateway" },
    { 503, "Service Unavailable" },
    { 504, "Gateway Timeout" },
    { 505, "HTTP Version Not Supported" },
    { 0,   NULL },
};

} // namespace

void HttpResponse::Reset()
{
    HttpResponse dummy;
    swap(dummy);
}

std::string HttpResponse::StatusCodeReasonPhrase() const
{
    int i = 0;
    while (static_cast<StatusCode>(kStatusInfo[i].status) != kUnknown)
    {
        if (kStatusInfo[i].status == status_code_)
        {
            return kStatusInfo[i].reason_phrase;
        }
        i++;
    }
    return "";
}

bool HttpResponse::set_status(const StringPiece& s)
{
    DCHECK(status_code_ == kUnknown);

    // FIXME
    auto sc = boost::lexical_cast<int>(s.ToString());

    int i = 0;
    while (kStatusInfo[i].status != static_cast<int>(kUnknown))
    {
        if (kStatusInfo[i].status == sc)
        {
            status_code_ = static_cast<StatusCode>(sc);
            return true;
        }
        i++;
    }
    return false;
}

bool HttpResponse::ParseStartLine(const std::string& line)
{
    std::vector<std::string> result;
    boost::algorithm::split(result, line, boost::is_any_of(" "));

    if (result.size() < 2)
    {
        return false;
    }

    if (!set_version(result[0]))
    {
        return false;
    }

    if (!set_status(result[1]))
    {
        return false;
    }

    return true;
}

bool HttpResponse::IsComplete(Buffer* buffer) const
{
    if (status_code_ < 200 || status_code_ == 204 || status_code_ == 304)
    {
        return true;
    }

    return IsBodyComplete(buffer);
}

} // namespace claire
