// Copyright (c) 2013 The claire-netty Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef _CLAIRE_NETTY_HTTP_HTTPMESSAGE_H_
#define _CLAIRE_NETTY_HTTP_HTTPMESSAGE_H_

#include <stdlib.h>

#include <string>
#include <algorithm>

#include <claire/netty/http/HttpHeaders.h>
#include <claire/netty/Buffer.h>

namespace claire {

// Base class of HttpRequest & HttpResponse
// HttpMessage is Not Thread Safe
class HttpMessage
{
public:
    enum HttpVersion
    {
        kUnknown,
        kHttp10,
        kHttp11
    };

    HttpMessage()
        : version_(kUnknown) {}

    virtual ~HttpMessage() {}

    HttpVersion version() const
    {
        return version_;
    }

    std::string get_version_string() const
    {
        if (version_ == kHttp10)
        {
            return "HTTP/1.0";
        }
        else if (version_ == kHttp11)
        {
            return "HTTP/1.1";
        }
        else
        {
            return "";
        }
    }

    void set_version(HttpVersion v)
    {
        version_ = v;
    }

    bool set_version(const StringPiece& s)
    {
        bool succeed = std::equal(s.data(), s.data() + s.size() - 1,"HTTP/1.");
        if (succeed)
        {
            auto v = s[s.size()-1];
            if (v == '1')
            {
                version_ = kHttp11;
            }
            else if (v == '0')
            {
                version_ = kHttp10;
            }
            else
            {
                succeed = false;
            }
        }

        return succeed;
    }

    const HttpHeaders& headers() const { return headers_; }
    HttpHeaders* mutable_headers() { return &headers_; }

    bool HasHeader(const std::string& field) const
    {
        std::string result;
        return headers_.Get(field, &result);
    }

    std::string get_header(const std::string& field) const
    {
        std::string result;
        headers_.Get(field, &result);
        return result;
    }

    bool IsUniqueHeader(const std::string& field) const
    {
        return headers_.Unique(field);
    }

    void AddHeader(const StringPiece& field, const StringPiece& value)
    {
        headers_.Add(field, value);
    }

    void AddHeader(const char* start, const char* colon, const char* end)
    {
        std::string field(start, colon);

        ++colon;
        while (colon < end && isspace(*colon))
        {
            ++colon;
        }

        std::string value(colon, end);
        while (!value.empty() && isspace(value[value.size()-1]))
        {
            value.resize(value.size()-1);
        }

        headers_.Add(field, value);
    }

    const std::string& body() const { return body_; }
    std::string* mutable_body() { return &body_; }

    // here we want copy the data, not need reference count string's cow
    void set_body(const StringPiece& body__)
    {
        body_.assign(body__.data(), body__.size());
    }

    void swap(HttpMessage& other)
    {
        std::swap(version_, other.version_);
        std::swap(headers_, other.headers_);
        std::swap(body_, other.body_);
    }

    void AppendTo(std::string* result) const
    {
        auto sl = GetStartLine();
        result->append(sl);
        headers_.AppendTo(result);
        result->append(body_);
    }

    void AppendTo(Buffer* result) const
    {
        auto sl = GetStartLine();
        result->Append(sl);
        headers_.AppendTo(result);
        result->Append(body_);
    }

    std::string ToString() const
    {
        std::string result;
        AppendTo(&result);
        return result;
    }

    int get_content_length() const
    {
        std::string v;
        auto has = headers_.Get("Content-Length", &v);
        if (!has)
        {
            return -1;
        }

        // FIXME
        return static_cast<uint32_t>(strtoul(v.c_str(), NULL, 0));
    }

    bool IsKeepAlive() const
    {
        std::string alive;
        if (!headers_.Get("Connection", &alive))
        {
            if (version_  == kHttp10)
            {
                return false;
            }
            return true;
        }
        return strcasecmp(alive.c_str(), "keep-alive") == 0;
    }

    bool IsBodyComplete(Buffer* buf) const
    {
        std::string value;
        if (headers_.Get("Transfer-Encoding", &value))
        {
            if (strcasecmp(value.c_str(), "chunked") == 0)
            {
                const char kChunkedEnd[] = "0\r\n\r\n";
                auto pos = std::search(buf->Peek(),
                                       buf->Peek()+buf->ReadableBytes(),
                                       kChunkedEnd+0,
                                       kChunkedEnd+sizeof(kChunkedEnd));
                if (pos)
                {
                    return true;
                }
            }
        }
        else if (headers_.Get("Content-Length", &value))
        {
            auto len = ::strtoul(value.c_str(), NULL, 0);
            if (len <= buf->ReadableBytes())
            {
                return true;
            }
        }

        return false;
    }

    virtual std::string GetStartLine() const = 0;
    virtual bool ParseStartLine(const std::string& line) = 0;
    virtual bool IsComplete(Buffer* buf) const = 0;
    virtual void Reset() = 0;

    size_t GetBodyLength(Buffer* buf) const
    {
        std::string value;
        if (headers_.Get("Content-Length", &value))
        {
            return ::strtoul(value.c_str(), NULL, 0);
        }
        else if (headers_.Get("Transfer-Encoding", &value))
        {
            const static char kChunkedEnd[] = "0\r\n\r\n";
            auto pos = std::search(buf->Peek(),
                                   buf->Peek()+buf->ReadableBytes(),
                                   kChunkedEnd+0,
                                   kChunkedEnd+sizeof(kChunkedEnd));
            if (pos)
            {
                return pos+5-buf->Peek();
            }
        }
        return 0;
    }

private:
    HttpVersion version_;
    HttpHeaders headers_;
    std::string body_;
};

} // namespace claire

#endif // _CLAIRE_NETTY_HTTP_HTTPMESSAGE_H_
