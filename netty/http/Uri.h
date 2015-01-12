// Copyright (c) 2013 The claire-netty Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

// Taken from folly/Uri.h
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

#ifndef _CLAIRE_NETTY_HTTP_URI_H_
#define _CLAIRE_NETTY_HTTP_URI_H_

#include <map>
#include <string>

#include <claire/common/strings/StringPiece.h>

namespace claire {

/**
 * Class representing a URI.
 *
 * Consider http://www.facebook.com/foo/bar?key=foo#anchor
 *
 * The URI is broken down into its parts: scheme ("http"), authority
 * (ie. host and port, in most cases: "www.facebook.com"), path
 * ("/foo/bar"), query ("key=foo") and fragment ("anchor").  The scheme is
 * lower-cased.
 *
 * If this Uri represents a URL, note that, to prevent ambiguity, the component
 * parts are NOT percent-decoded; you should do this yourself with
 * uriUnescape() (for the authority and path) and uriUnescape(...,
 * UriEscapeMode::QUERY) (for the query, but probably only after splitting at
 * '&' to identify the individual parameters).
 */
class Uri
{
public:
    Uri() : port_() {}

    bool Parse(const StringPiece& uri);

    const std::string& scheme() const { return scheme_; }
    const std::string& username() const { return username_; }
    const std::string& password() const { return password_; }
    const std::string& host() const { return host_; }
    uint16_t port() const { return port_; }
    const std::string& path() const { return path_; }
    const std::string& query() const { return query_; }
    const std::string& fragment() const { return fragment_; }

    std::string ToString() const;

    std::string get_parameter(const std::string param) const
    {
        auto it = params_.find(param);
        if (it != params_.end())
        {
            return it->second;
        }

        return std::string();
    }

private:
    std::string scheme_;
    std::string username_;
    std::string password_;
    std::string host_;
    uint16_t port_;
    std::string path_;
    std::string query_;
    std::string fragment_;

    std::map<std::string, std::string> params_;
};

} // namespace claire

#endif // _CLAIRE_NET_HTTP_URI_H_
