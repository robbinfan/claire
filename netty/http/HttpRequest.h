// Copyright (c) 2013 The claire-netty Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef _CLAIRE_NETTY_HTTP_HTTPREQUEST_H_
#define _CLAIRE_NETTY_HTTP_HTTPREQUEST_H_

#include <string>
#include <stdexcept>

#include <claire/netty/http/HttpMessage.h>
#include <claire/netty/http/Uri.h>

namespace claire {

class Buffer;

class HttpRequest : public HttpMessage
{
public:
    enum Method
    {
        kInvalid,
        kGet,
        kPost,
        kHead,
        kPut,
        kDelete
    };

    HttpRequest()
        : method_(kInvalid) {}

    virtual ~HttpRequest() {}

    void set_method(Method m)
    {
        method_ = m;
    }

    bool set_method(const char* start, const char* end)
    {
        std::string m(start, end);
        if (m == "GET")
        {
            method_ = kGet;
        }
        else if (m == "POST")
        {
            method_ = kPost;
        }
        else if (m == "HEAD")
        {
            method_ = kHead;
        }
        else if (m == "PUT")
        {
            method_ = kPut;
        }
        else if (m == "DELETE")
        {
            method_ = kDelete;
        }
        else
        {
            method_ = kInvalid;
        }

        return method_ != kInvalid;
    }

    Method method() const
    {
        return method_;
    }

    const char* MethodString() const
    {
        const char* result = "UNKNOWN";
        switch(method_)
        {
            case kGet:
                result = "GET";
                break;
            case kPost:
                result = "POST";
                break;
            case kHead:
                result = "HEAD";
                break;
            case kPut:
                result = "PUT";
                break;
            case kDelete:
                result = "DELETE";
                break;
            default:
                break;
        }

        return result;
    }

    bool set_uri(const StringPiece& uri__)
    {
        return uri_.Parse(uri__);
    }

    const Uri& uri() const
    {
        return uri_;
    }

    void swap(HttpRequest& other)
    {
        HttpMessage::swap(other);
        std::swap(method_, other.method_);
        std::swap(uri_, other.uri_);
    }

    virtual std::string GetStartLine() const
    {
        std::string result;
        result.append(MethodString());
        result.append(" ");
        result.append(uri_.ToString());
        result.append(" ");
        result.append(get_version_string());
        result.append("\r\n");
        return result;
    }

    virtual bool ParseStartLine(const std::string& line);
    virtual bool IsComplete(Buffer* buffer) const;

    void Reset();

    std::string get_parameter(const std::string& param)
    {
        return uri_.get_parameter(param);
    }

private:
    Method method_;
    Uri uri_;
};

} // namespace claire

namespace std {

template<>
inline void swap(claire::HttpRequest& lhs, claire::HttpRequest& rhs)
{
    lhs.swap(rhs);
}

} // namespace std

#endif // _CLAIRE_NETTY_HTTP_HTTPREQUEST_H_
