// Copyright (c) 2013 The claire-common Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include <claire/common/system/ThisProcess.h>

#include <pwd.h>
#include <stdio.h> // snprintf
#include <sys/types.h>

namespace claire {
namespace ThisProcess {

pid_t pid()
{
    return ::getpid();
}

std::string pid_string()
{
    char pid[32];
    snprintf(pid, sizeof pid, "%d", ::getpid());
    return pid;
}

std::string User()
{
    struct passwd pwd;
    struct passwd* result = NULL;
    char buf[8192];
    const char* user = "unknownuser";

    ::getpwuid_r(::getuid(), &pwd, buf, sizeof buf, &result);
    if (result)
    {
        user = pwd.pw_name;
    }
    return user;
}

std::string Host()
{
    char host[64] = "unknownhost";
    host[sizeof(host)-1] = '\0';
    ::gethostname(host, sizeof host);
    return host;
}

} // namespace ThisProcess
} // namespace claire
