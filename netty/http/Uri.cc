// Copyright (c) 2013 The claire-netty Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

// Taken from folly/Uri.cpp
//
// Copyright 2013 Facebook, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include <claire/netty/http/Uri.h>

#include <ctype.h>

#include <vector>

#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

#include <claire/common/strings/UriEscape.h>
#include <claire/common/strings/StringPrintf.h>
#include <claire/common/logging/Logging.h>

namespace claire {

namespace {

std::string Submatch(const boost::cmatch& m, size_t index)
{
    auto& sub = m[static_cast<int>(index)];
    return std::string(sub.first, sub.second);
}

void ToLower(std::string* s)
{
    for (size_t i = 0; i < s->length(); ++i)
    {
        (*s)[i] = static_cast<char>(::tolower(static_cast<int>((*s)[i])));
    }
}

}  // namespace

bool Uri::Parse(const StringPiece& uri)
{
    static const boost::regex absolute_regex(
        "([a-zA-Z][a-zA-Z0-9+.-]*):"  // scheme:
        "([^?#]*)"                    // authority and path
        "(?:\\?([^#]*))?"             // ?query
        "(?:#(.*))?");                // #fragment
    static const boost::regex relative_regex(
        "([^?#]*)"                    // authority and path
        "(?:\\?([^#]*))?"             // ?query
        "(?:#(.*))?");                // #fragment

    static const boost::regex authority_and_path_regex("//([^/]*)(/.*)?");

    boost::cmatch match;
    int next_index = 0;
    if (uri[0] != '/' && boost::regex_match(uri.begin(), uri.end(), match, absolute_regex))
    {
        scheme_ = Submatch(match, 1);
        ToLower(&scheme_);
        next_index = 2;
    }
    else if (uri[0] == '/' && boost::regex_match(uri.begin(), uri.end(), match, relative_regex))
    {
        next_index = 1;
    }
    else
    {
        LOG(ERROR) << "invalid uri: " << uri;
        return false;
    }

    StringPiece authority_and_path(match[next_index].first, match[next_index].second);
    next_index++;

    boost::cmatch authority_and_path_match;
    if (!boost::regex_match(authority_and_path.begin(),
                            authority_and_path.end(),
                            authority_and_path_match,
                            authority_and_path_regex))
    {
        // Does not start with //, doesn't have authority
        path_ = authority_and_path.ToString();
    }
    else
    {
        static const boost::regex authority_regex(
            "(?:([^@:]*)(?::([^@]*))?@)?"  // username, password
            "(\\[[^\\]]*\\]|[^\\[:]*)"     // host (IP-literal, dotted-IPv4, or
                                           // named host)
            "(?::(\\d*))?");               // port

        auto authority = authority_and_path_match[1];
        boost::cmatch authority_match;
        if (!boost::regex_match(authority.first,
                                authority.second,
                                authority_match,
                                authority_regex))
        {
            LOG(ERROR) << "invalid uri authority: " << StringPiece(authority.first, authority.second);
            return false;
        }

        StringPiece port__(authority_match[4].first, authority_match[4].second);
        if (!port__.empty())
        {
            port_ = static_cast<uint16_t>(strtoul(port__.data(), NULL, 0));
        }

        username_ = Submatch(authority_match, 1);
        password_ = Submatch(authority_match, 2);
        host_ = Submatch(authority_match, 3);
        path_ = Submatch(authority_and_path_match, 2);
    }

    query_ = Submatch(match, next_index++);
    fragment_ = Submatch(match, next_index++);

    if (path_.length() > 1 && *path_.rbegin() == '/')
    {
        path_.resize(path_.length()-1);
    }

    if (!query_.empty())
    {
        std::vector<std::string> result;
        boost::split(result, query_, boost::is_any_of("&"));
        for (auto it = result.begin(); it != result.end(); ++it)
        {
            auto pos = it->find('=');
            if (pos != std::string::npos)
            {
                auto p = params_.insert(std::make_pair(std::string(*it, 0, pos),
                                                       std::string(*it, pos+1, std::string::npos)));
                if (p.second)
                {
                    std::string tmp;
                    UriUnescape(p.first->second, &tmp, EscapeMode::ALL);
                    p.first->second.swap(tmp);
                }
            }
            else
            {
                params_.insert(std::make_pair(*it, std::string()));
            }
        }
    }
    return true;
}

std::string Uri::ToString() const
{
    std::string uri;
    if (!scheme_.empty())
    {
        StringAppendF(&uri, "%s://", scheme_.c_str());
    }

    if (!password_.empty())
    {
        StringAppendF(&uri, "%s:%s@", username_.c_str(), password_.c_str());
    }
    else if (!username_.empty())
    {
        StringAppendF(&uri, "%s@", username_.c_str());
    }
    uri.append(host_);

    if (port_ != 0)
    {
        StringAppendF(&uri, ":%u", port_);
    }
    uri.append(path_);

    if (!query_.empty())
    {
        StringAppendF(&uri, "?%s", query_.c_str());
    }

    if (!fragment_.empty())
    {
        StringAppendF(&uri, "#%s", fragment_.c_str());
    }

    return uri;
}

} // namespace claire
