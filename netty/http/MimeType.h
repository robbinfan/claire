// Copyright (c) 2013 The claire-netty Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef _CLAIRE_NETTY_HTTP_MIMETYPE_H_
#define _CLAIRE_NETTY_HTTP_MIMETYPE_H_

#include <string>

namespace claire {

class MimeType
{
public:
    MimeType() {}
    MimeType(const std::string& mime);
    MimeType(const std::string& type, const std::string& subtype)
        : type_(type), subtype_(subtype) {}

    void swap(MimeType& other)
    {
        type_.swap(other.type_);
        subtype_.swap(other.subtype_);
    }

    static MimeType ExtensionToMimeType(const std::string& extension);

    bool match(const MimeType& mime) const
    {
        return ((mime.type_ == type_) || (mime.type_ == "*") || (type_ == "*"))
                && ((mime.subtype_ == subtype_) || (mime.subtype_ == "*") || (subtype_ == "*"));
    }

    bool Match(const std::string& m) const
    {
        return match(MimeType(m));
    }

    const std::string& get_type() const
    {
        return type_;
    }

    const std::string& get_subtype() const
    {
        return subtype_;
    }

    const std::string ToString() const
    {
        return type_ + "/" + subtype_;
    }

    bool empty() const
    {
        return type_.empty() && subtype_.empty();
    }

private:
    std::string type_;
    std::string subtype_;
};

} // namespace claire

#endif // _CLAIRE_NETTY_HTTP_MIMETYPE_H_
