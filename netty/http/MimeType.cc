// Copyright (c) 2013 The claire-netty Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include <claire/netty/http/MimeType.h>

#include <map>
#include <vector>
#include <algorithm>

#include <boost/bind.hpp>
#include <boost/algorithm/string.hpp>

#include <claire/common/files/FileUtil.h>
#include <claire/common/logging/Logging.h>

namespace claire {

namespace {

void UpdateMimeTypeMap(std::map<std::string, std::string>* m,
                       const std::string& k,
                       std::string& v)
{
    m->insert(std::make_pair(k, v));
}

std::map<std::string, std::string> InitMimeTypeMapping()
{
    std::map<std::string, std::string> m;
    m.insert(std::make_pair("gif", "image/gif"));
    m.insert(std::make_pair("jpg", "image/jpeg"));
    m.insert(std::make_pair("png", "image/png"));
    m.insert(std::make_pair("ico", "image/x-icon"));
    m.insert(std::make_pair("htm", "text/html"));
    m.insert(std::make_pair("html", "text/html"));
    m.insert(std::make_pair("txt", "text/x"));
    m.insert(std::make_pair("xml", "text/xml"));
    m.insert(std::make_pair("js", "application/x-javascript"));

    std::string fdata;
    if (FileUtil::ReadFileToString("/etc/mime.types", &fdata))
    {
        return m;
    }

    std::vector<std::string> lines;
    boost::algorithm::split(lines, fdata, boost::is_any_of("\r\n"));

    for (auto& line : lines)
    {
        if (line.empty() || line[0] == '#')
        {
            continue;
        }

        std::vector<std::string> result;
        boost::algorithm::split(result, line, boost::is_any_of("\t"));
        std::for_each(result.begin()+1,
                      result.end(),
                      boost::bind(&UpdateMimeTypeMap, &m, result[0], _1));
    }

    return m;
}

} // namespace

MimeType::MimeType(const std::string& mime)
{
    auto pos = mime.find('/');
    if (pos != std::string::npos)
    {
        type_ = mime.substr(0, pos);
        subtype_ = mime.substr(pos + 1, mime.size() + 1);
    }
    // throw ?
}

MimeType MimeType::ExtensionToMimeType(const std::string& extension)
{
    const static std::map<std::string, std::string> mapping = InitMimeTypeMapping();

    auto it = mapping.find(extension);
    if (it != mapping.end())
    {
        return MimeType(it->second);
    }

    LOG(ERROR) << "extension " << extension << " not found match mime";
    return MimeType();
}

} // namespace claire
