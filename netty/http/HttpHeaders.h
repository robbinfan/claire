// Copyright (c) 2013 The claire-netty Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef _CLAIRE_NETTY_HTTP_HTTPHEADERS_H_
#define _CLAIRE_NETTY_HTTP_HTTPHEADERS_H_

#include <vector>
#include <string>

#include <claire/common/strings/StringPiece.h>
#include <claire/netty/Buffer.h>

namespace claire {

// HttpHeaders is Not Thread Safe
class HttpHeaders
{
public:
    void Add(const StringPiece& field, const StringPiece& value)
    {
        headers_.push_back(std::make_pair(field.ToString(), value.ToString()));
    }

    // parse from text
    void Add(const char* start, const char* colon, const char* end)
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

        headers_.push_back(std::make_pair(field, value));
    }

    bool Get(const StringPiece& field, std::string* result) const
    {
        result->clear();
        for (auto& header : headers_)
        {
            if (strncasecmp(field.data(), header.first.c_str(), field.size()) == 0)
            {
                result->assign(header.second.data(), header.second.size());
                return true;
            }
        }

        return false;
    }

    bool Get(const std::string& field, std::vector<std::string>* result) const
    {
        result->clear();
        for (auto& header : headers_)
        {
            if (strncasecmp(field.data(), header.first.c_str(), field.size()) == 0)
            {
                result->push_back(std::string(header.second.data(), header.second.size()));
            }
        }

        return !result->empty();
    }

    bool Unique(const std::string& field) const
    {
        std::vector<std::string> result;
        return Get(field, &result) && result.size() == 1;
    }

    bool empty() const
    {
        return headers_.empty();
    }

    size_t size() const
    {
        return headers_.size();
    }

    void swap(HttpHeaders& other)
    {
        headers_.swap(other.headers_);
    }

    void AppendTo(Buffer* dest) const
    {
        for (auto& header : headers_)
        {
            dest->Append(header.first.c_str());
            dest->Append(": ");
            dest->Append(header.second.c_str());
            dest->Append("\r\n");
        }
        dest->Append("\r\n");
    }

    void AppendTo(std::string* dest) const
    {
        for (auto& header : headers_)
        {
            dest->append(header.first.c_str());
            dest->append(": ");
            dest->append(header.second.c_str());
            dest->append("\r\n");
        }
        dest->append("\r\n");
    }

    std::string ToString() const
    {
        std::string result;
        AppendTo(&result);
        return result;
    }

    void Reset()
    {
        HttpHeaders dummy;
        swap(dummy);
    }

private:
    typedef std::pair<std::string, std::string> Entry;
    std::vector<Entry> headers_;
};

} // namespace claire

namespace std {

template<>
inline void swap(claire::HttpHeaders& lhs, claire::HttpHeaders& rhs)
{
    lhs.swap(rhs);
}

} // namespace std

#endif // _CLAIRE_NETTY_HTTP_HTTPHEADERS_H_
