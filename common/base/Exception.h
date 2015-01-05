// Copyright (c) 2013 The claire-common Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef _CLAIRE_COMMON_BASE_EXCEPTION_H_
#define _CLAIRE_COMMON_BASE_EXCEPTION_H_

#include <string>
#include <exception>

namespace claire {

class Exception : public std::exception
{
public:
    explicit Exception(const char* what);
    explicit Exception(const std::string& what);

    virtual ~Exception() throw() {}

    virtual const char* what() const throw()
    {
        return what_.c_str();
    }

    const char* stack_trace() const throw()
    {
        return stack_trace_.c_str();
    }

private:
    std::string what_;
    std::string stack_trace_;
};

} // namespace claire

#endif // _CLAIRE_COMMON_BASE_EXCEPTION_H_
