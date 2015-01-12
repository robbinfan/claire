// Copyright (c) 2013 The claire-netty Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include <claire/netty/http/HttpRequest.h>

#include <vector>
#include <algorithm>

#include <boost/algorithm/string.hpp>

#include <claire/netty/Buffer.h>
#include <claire/common/logging/Logging.h>

using namespace claire;

void HttpRequest::Reset()
{
    HttpRequest dummy;
    swap(dummy);
}

bool HttpRequest::ParseStartLine(const std::string& line)
{
    std::vector<std::string> result;
    boost::algorithm::split(result, line, boost::is_any_of(" "));

    if (result.size() != 2 && result.size() != 3)
    {
        return false;
    }

    if (!set_method(result[0].data(), result[0].data() + result[0].size()))
    {
        return false;
    }

    if (!set_uri(result[1]))
    {
        return false;
    }

    if (result.size() == 3 && !set_version(result[2]))
    {
        return false;
    }

    return true;
}

bool HttpRequest::IsComplete(Buffer* buffer) const
{
    if (method_ == kGet || method_ == kHead)
    {
        return true;
    }

    return IsBodyComplete(buffer);
}

