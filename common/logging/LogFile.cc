// Copyright (c) 2013 The claire-common Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include <claire/common/logging/LogFile.h>

#include <assert.h>
#include <string.h>
#include <unistd.h>

#include "thirdparty/gflags/gflags.h"

#include <claire/common/files/FileUtil.h>
#include <claire/common/time/Timestamp.h>
#include <claire/common/threading/Mutex.h>
#include <claire/common/system/ThisProcess.h>

DEFINE_bool(drop_log_memory, true, "Drop in-memory buffers of log contents. "
            "Logs can grow very quickly and they are rarely read before they "
            "need to be evicted from memory. Instead, drop them from memory "
            "as soon as they are flushed to disk.");

namespace claire {

const static int kCheckTimeRoll = 1024;
const static int kSecondsPerDay = 60*60*24;

LogFile::~LogFile() {}

LogFile::LogFile(bool thread_safe)
    : base_name_(::gflags::ProgramInvocationShortName()),
      count_(0),
      mutex_(thread_safe ? new Mutex : NULL),
      last_period_(0),
      last_roll_(0),
      last_flush_(0)
{
    assert(base_name_.find('/') == std::string::npos);
    RollFile();
}

void LogFile::Append(const char* data, size_t length)
{
    if(mutex_)
    {
        MutexLock lock(*mutex_);
        AppendUnLocked(data, length);
    }
    else
    {
        AppendUnLocked(data, length);
    }
}

void LogFile::Flush()
{
    if (mutex_)
    {
        MutexLock lock(*mutex_);
        file_->Flush();
        DropFileMemory();
    }
    else
    {
        file_->Flush();
        DropFileMemory();
    }
}

void LogFile::AppendUnLocked(const char* data, size_t length)
{
    file_->Append(StringPiece(data, length));
    if (file_->WrittenBytes() > static_cast<uint32_t>(FLAGS_max_log_size)*1024*1024)
    {
        RollFile();
    }
    else
    {
        if (count_ > kCheckTimeRoll)
        {
            count_ = 0;
            auto now = Timestamp::Now().SecondsSinceEpoch();
            auto this_period = now / kSecondsPerDay * kSecondsPerDay;
            if (this_period != last_period_)
            {
                RollFile();
            }
            else if (now - last_flush_ > FLAGS_logbufsecs)
            {
                last_flush_ = now;
                file_->Flush();
            }
        }
        else
        {
            ++count_;
        }
    }
}

void LogFile::RollFile()
{
    auto now = Timestamp::Now().SecondsSinceEpoch();
    auto name = GetLogFileName();
    auto start = now / kSecondsPerDay * kSecondsPerDay;

    if (now > last_roll_)
    {
        last_roll_ = now;
        last_flush_ = now;
        last_period_ = start;

        file_.reset(new FileUtil::AppendableFile(name));

        std::string link_name = GetLinkLogFileName();;
        if (FileUtil::FileExists(link_name))
        {
            std::string fname;
            uint64_t length;
            if ((FileUtil::ReadLink(link_name, &fname) == FileUtil::kOk)
                && (FileUtil::GetFileSize(fname, &length) == FileUtil::kOk)
                && (length == 0))
            {
                FileUtil::DeleteFile(fname);
            }

            FileUtil::DeleteFile(link_name);
        }
        FileUtil::SymLink(name, link_name);
    }
}

void LogFile::DropFileMemory()
{
    auto length = file_->WrittenBytes();
    if (FLAGS_drop_log_memory && static_cast<int>(length) >= ::getpagesize())
    {
        // don't evict the most recent page
        length &= ~(::getpagesize() - 1);
        ::posix_fadvise(file_->fd(), 0, length, POSIX_FADV_DONTNEED);
    }
}

std::string LogFile::GetLogFileName() const
{
    auto fname = FLAGS_log_dir;
    if (!fname.empty() && *fname.rbegin() != '/')
    {
        fname += "/";
    }

    fname += base_name_;
    fname += ".";
    fname += Timestamp::Now().ToFormattedString();
    fname += ".";
    fname += ThisProcess::Host();
    fname += ".";
    fname += ThisProcess::pid_string();
    fname += ".";
    fname += ThisProcess::User();
    fname += ".log";
    return fname;
}

std::string LogFile::GetLinkLogFileName() const
{
    auto fname = FLAGS_log_dir;
    if (!fname.empty() && *fname.rbegin() != '/')
    {
        fname += "/";
    }

    fname += base_name_;
    fname += ".log";
    return fname;
}

} // namespace claire
