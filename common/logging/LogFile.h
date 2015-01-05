// Copyright (c) 2013 The claire-common Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef _CLAIRE_COMMON_LOGGING_LOGFILE_H_
#define _CLAIRE_COMMON_LOGGING_LOGFILE_H_

#include <time.h>

#include <string>

#include <boost/scoped_ptr.hpp>
#include <boost/noncopyable.hpp>

#include <claire/common/files/FileUtil.h>
#include <claire/common/threading/Mutex.h>

namespace claire {

class LogFile : boost::noncopyable
{
public:
    LogFile(bool thread_safe);
    ~LogFile();

    void Append(const char* data, size_t length);
    void Flush();

private:
    void RollFile();
    void DropFileMemory();
    std::string GetLogFileName()  const;
    std::string GetLinkLogFileName()  const;
    void AppendUnLocked(const char* data, size_t length);

    const std::string base_name_;
    int count_;
    boost::scoped_ptr<Mutex> mutex_;
    boost::scoped_ptr<FileUtil::WritableFile> file_;
    time_t last_period_;
    time_t last_roll_;
    time_t last_flush_;
};

} // namespace claire

#endif // _CLAIRE_COMMON_LOGGING_LOGFILE_H_
