// Copyright (c) 2013 The claire-netty Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef _CLAIRE_NETTY_HTTP_HTTPRESPONSE_H_
#define _CLAIRE_NETTY_HTTP_HTTPRESPONSE_H_

#include <string>
#include <algorithm>

#include <boost/lexical_cast.hpp>

#include <claire/netty/http/HttpMessage.h>

namespace claire {

class HttpResponse : public HttpMessage
{
public:
    // RFC 2616: 10 Status Code Definition
    enum StatusCode
    {
        kUnknown,
        k100Continue           = 100,
        k101SwitchingProtocols = 101,

        k200OK                          = 200,
        k201Created                     = 201,
        k202Accepted                    = 202,
        k203NonAuthoritativeInformation = 203,
        k204NoContent                   = 204,
        k205ResetContent                = 205,
        k206PartialContent              = 206,

        k300MultipleChoices   = 300,
        k301MovedPermanently  = 301,
        k302Found             = 302,
        k303SeeOther          = 303,
        k304NotModified       = 304,
        k305UseProxy          = 305,
        k307TemporaryRedirect = 307,

        k400BadRequest                   = 400,
        k401Unauthorized                 = 401,
        k402PaymentRequired              = 402,
        k403Forbidden                    = 403,
        k404NotFound                     = 404,
        k405MethodNotAllowed             = 405,
        k406NotAcceptable                = 406,
        k407ProxyAuthRequired            = 407,
        k408RequestTimeout               = 408,
        k409Conflict                     = 409,
        k410Gone                         = 410,
        k411LengthRequired               = 411,
        k412PreconditionFailed           = 412,
        k413RequestEntityTooLarge        = 413,
        k414RequestURITooLong            = 414,
        k415UnsupportedMediaType         = 415,
        k416RequestedRangeNotSatisfiable = 416,
        k417ExpectationFailed            = 417,

        k500InternalServerError     = 500,
        k501NotImplemented          = 501,
        k502BadGateway              = 502,
        k503ServiceUnavailable      = 503,
        k504GatewayTimeout          = 504,
        k505HTTPVersionNotSupported = 505,
    };

    HttpResponse()
        : status_code_(kUnknown)
    {}

    virtual ~HttpResponse() {}

    StatusCode status() const
    {
        return status_code_;
    }

    bool set_status(const StringPiece& s);

    void set_status(StatusCode code)
    {
        status_code_ = code;
    }

    std::string StatusCodeReasonPhrase() const;

    void swap(HttpResponse& other)
    {
        HttpMessage::swap(other);
        std::swap(status_code_, other.status_code_);
    }

    void Reset();

    virtual std::string GetStartLine() const
    {
        std::string result;
        result.append(get_version_string());
        result.append(" ");
        result.append(boost::lexical_cast<std::string>(status_code_));
        result.append(" ");
        result.append(StatusCodeReasonPhrase());
        result.append("\r\n");
        return result;
    }

    virtual bool ParseStartLine(const std::string& line);
    virtual bool IsComplete(Buffer* buf) const;

private:
    StatusCode status_code_;
};

} // namespace claire

namespace std {

template<>
inline void swap(claire::HttpResponse& lhs, claire::HttpResponse& rhs)
{
    lhs.swap(rhs);
}

} // namespace std

#endif // _CLAIRE_NETTY_HTTP_HTTPRESPONSE_H_
