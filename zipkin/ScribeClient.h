// Copyright (c) 2013 The claire-zipkin Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef _CLAIRE_ZIPKIN_SCRIBECLIENT_H_
#define _CLAIRE_ZIPKIN_SCRIBECLIENT_H_

#include <string>

#include <boost/scoped_ptr.hpp>
#include <boost/noncopyable.hpp>

namespace claire {

class InetAddress;
class ScribeClient : boost::noncopyable
{
public:
    ScribeClient(const InetAddress& scribe_address);
    ~ScribeClient();

    void Log(const std::string& category, const std::string& message);

private:
    class Impl;
    boost::scoped_ptr<Impl> impl_;
};

} // namespace claire
#endif // _CLAIRE_ZIPKIN_SCRIBECLIENT_H_
