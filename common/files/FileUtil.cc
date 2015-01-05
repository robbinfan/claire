// Copyright (c) 2013 The claire-common Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include <claire/common/files/FileUtil.h>

#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/file.h>

#include <claire/common/logging/Logging.h>

namespace claire {
namespace FileUtil {

Status SequentialFile::Read(size_t n, StringPiece* result, char* scratch)
{
    Status s = kOk;
    size_t r = ::fread_unlocked(scratch, 1, n, fp_);
    if (r < n)
    {
        if (::feof(fp_))
        {
            // We leave status as ok if we hit the end of the file
        }
        else
        {
            // A partial read with an error: return a non-ok status
            s = kIOError;
            PLOG(ERROR) << "SequentialFile::Read " << filename_ << " failed :";
        }
    }

    if (result)
    {
        *result = StringPiece(scratch, r);
    }
    return s;
}

Status SequentialFile::Skip(uint64_t n)
{
    if (::fseek(fp_, n, SEEK_CUR))
    {
        PLOG(ERROR) << "SequentialFile::Skip" << filename_ << " failed: ";
        return kIOError;
    }
    return kOk;
}

Status RandomAccessFile::Read(uint64_t offset, size_t n, StringPiece* result, char* scratch) const
{
    Status s = kOk;
    ssize_t r = ::pread(fd_, scratch, n, static_cast<off_t>(offset));
    if (r < 0)
    {
        s = kIOError;
        PLOG(ERROR) << "RandomAccessFile::Read " << filename_ << " failed: ";
    }

    if (result)
    {
        *result = StringPiece(scratch, (r < 0) ? 0 : r);
    }
    return s;
}

Status WritableFile::Append(const StringPiece& data)
{
    size_t n = ::fwrite_unlocked(data.data(), 1, data.length(), fp_);
    while (n < data.length())
    {
        size_t x = ::fwrite_unlocked(data.data() + n, 1, data.length() - n, fp_);
        if (x == 0)
        {
            int err = ::ferror(fp_);
            if (err)
            {
                PLOG(ERROR) << "WritableFile::Append " << filename_ << " failed :";
            }

            written_bytes_ += n;
            return kIOError;
        }
        n += x;
    }

    written_bytes_ += n;
    return kOk;
}

Status WritableFile::Close()
{
    if (::fclose(fp_))
    {
        PLOG(ERROR) << "WritableFile Close " << filename_ << " failed: ";
        return kIOError;
    }
    return kOk;
}

Status WritableFile::Flush()
{
    if (::fflush(fp_))
    {
        PLOG(ERROR) << "WritableFile::Flush " << filename_ << " failed: ";
        return kIOError;
    }
    return kOk;
}

Status NewSequentialFile(const std::string& fname, SequentialFile** result)
{
    *result = NULL;
    Status s = kOk;

    FILE* fp = ::fopen(fname.c_str(), "r");
    if (fp == NULL)
    {
        s = kIOError;
        PLOG(ERROR) << "NewSequentialFile fopen " << fname << " failed: ";
    }
    else
    {
        *result = new SequentialFile(fname, fp);
    }
    return s;
}

Status NewRandomAccessFile(const std::string& fname, RandomAccessFile** result)
{
    *result = NULL;
    Status s = kOk;

    int fd = ::open(fname.c_str(), O_RDONLY);
    if (fd < 0)
    {
        s = kIOError;
        PLOG(ERROR) << "NewRandomAccessFile open " << fname << " failed: ";
    }
    else
    {
        *result = new RandomAccessFile(fname, fd);
    }
    return s;
}

Status NewWritableFile(const std::string& fname, WritableFile** result)
{
    *result = NULL;
    Status s = kOk;

    FILE* fp = ::fopen(fname.c_str(), "w+e");
    if (fp == NULL)
    {
        s = kIOError;
        PLOG(ERROR) << "NewWritableFile fopen " << fname << " failed: ";
    }
    else
    {
        *result = new WritableFile(fname, fp);
    }

    return s;
}

Status NewAppendableFile(const std::string& fname, WritableFile** result)
{
    *result = NULL;
    Status s = kOk;

    FILE* fp = ::fopen(fname.c_str(), "ae");
    if (fp == NULL)
    {
        s = kIOError;
        PLOG(ERROR) << "NewAppendableFile fopen " << fname << " failed: ";
    }
    else
    {
        uint64_t size;
        s = GetFileSize(fname, &size);
        if (s)
        {
            ::fclose(fp);
        }
        else
        {
            *result = new AppendableFile(fname.c_str(), fp);
        }
    }

    return s;
}

bool FileExists(const std::string& fname)
{
    return ::access(fname.c_str(), F_OK) == 0;
}

Status GetChildren(const std::string& dir, std::vector<std::string>* result)
{
    result->clear();

    DIR* d = ::opendir(dir.c_str());
    if (d == NULL)
    {
        PLOG(ERROR) << "GetChildren opendir " << dir << " failed: ";
        return kIOError;
    }

    struct dirent* entry;
    while ((entry = ::readdir(d)) != NULL)
    {
        result->push_back(entry->d_name);
    }

    ::closedir(d);
    return kOk;
}

Status DeleteFile(const std::string& fname)
{
    if (::unlink(fname.c_str()) != 0)
    {
        PLOG(ERROR) << "DeleteFile unlink " << fname << " failed: ";
        return kIOError;
    }
    return kOk;
}

Status CreateDir(const std::string& name)
{
    if (::mkdir(name.c_str(), 0755) != 0)
    {
        PLOG(ERROR) << "CreateDir mkdir " << name << " failed: ";
        return kIOError;
    }
    return kOk;
}

Status DeleteDir(const std::string& name)
{
    if (::rmdir(name.c_str()) != 0)
    {
        PLOG(ERROR) << "DeleteDir rmdir " << name << " failed: ";
        return kIOError;
    }
    return kOk;
}

Status GetFileSize(const std::string& fname, uint64_t* size)
{
    struct stat sbuf;
    if (::stat(fname.c_str(), &sbuf) != 0)
    {
        *size = 0;
        PLOG(ERROR) << "GetFileSize stat " << fname << " failed: ";
        return kIOError;
    }
    else
    {
        *size = sbuf.st_size;
        return kOk;
    }
}

Status RenameFile(const std::string& src, const std::string& target)
{
    if (::rename(src.c_str(), target.c_str()) != 0)
    {
        PLOG(ERROR) << "RenameFile rename " << src << " to " << target << " failed: ";
        return kIOError;
    }
    return kOk;
}

// matthewv July 17, 2012 ... riak was overlapping activity on the
// same database directory due to the incorrect assumption that the
// code below worked within the riak process space.  The fix leads to a choice:
// fcntl() only locks against external processes, not multiple locks from
// same process.  But it has worked great with NFS forever
// flock() locks against both external processes and multiple locks from
// same process.  It does not with NFS until Linux 2.6.12 ... other OS may vary.
// SmartOS/Solaris do not appear to support flock() though there is a man page.
// Pick the fcntl() or flock() below as appropriate for your environment / needs.
static int LockOrUnlock(int fd, bool lock)
{
#ifndef LOCK_UN
    // works with NFS, but fails if same process attempts second access to
    //  db, i.e. makes second DB object to same directory
    errno = 0;
    struct flock f;
    memset(&f, 0, sizeof(f));
    f.l_type = (lock ? F_WRLCK : F_UNLCK);
    f.l_whence = SEEK_SET;
    f.l_start = 0;
    f.l_len = 0;        // Lock/unlock entire file
    return ::fcntl(fd, F_SETLK, &f);
#else
    // does NOT work with NFS, but DOES work within same process
    return ::flock(fd, (lock ? LOCK_EX : LOCK_UN) | LOCK_NB);
#endif
}

Status LockFile(const std::string& fname, FileLock** lock)
{
    *lock = NULL;
    Status s = kOk;

    int fd = ::open(fname.c_str(), O_RDWR | O_CREAT, 0644);
    if (fd < 0)
    {
        PLOG(ERROR) << "LockFile open " << fname << " failed: ";
        s = kIOError;
    }
    else if (LockOrUnlock(fd, true) == -1)
    {
        PLOG(ERROR) << "lock " << fname << " failed: ";
        ::close(fd);
        s = kIOError;
    }
    else
    {
        FileLock* my_lock = new FileLock;
        my_lock->fd_ = fd;
        *lock = my_lock;
    }
    return s;
}

Status UnlockFile(FileLock* lock)
{
    Status s = kOk;
    if (LockOrUnlock(lock->fd_, false) == -1)
    {
        s = kIOError;
        PLOG(ERROR) << "unlock failed: ";
    }

    ::close(lock->fd_);
    delete lock;
    return s;
}

Status WriteStringToFile(const StringPiece& data, const std::string& fname)
{
    WritableFile* file;
    Status s = NewWritableFile(fname, &file);
    if (s)
    {
        return s;
    }
    s = file->Append(data);
    if (s)
    {
        s = file->Close();
    }
    delete file;  // Will auto-close if we did not close above
    if (s)
    {
        DeleteFile(fname);
    }
    return s;
}

Status ReadFileToString(const std::string& fname, std::string* data)
{
    data->clear();

    SequentialFile* file;
    Status s = NewSequentialFile(fname, &file);
    if (s)
    {
        return s;
    }

    char space[8192];
    while (true)
    {
        StringPiece fragment;
        s = file->Read(sizeof space, &fragment, space);
        if (s)
        {
            break;
        }

        if (fragment.empty())
        {
            break;
        }
        data->append(fragment.data(), fragment.size());
    }

    delete file;
    return kOk;
}

Status SymLink(const std::string& oldname, const std::string& newname)
{
    if (::symlink(oldname.c_str(), newname.c_str()))
    {
        PLOG(ERROR) << "SymLink " << oldname << " failed :";
        return kIOError;
    }
    return kOk;
}

bool IsSymLink(const std::string& fname)
{
    struct stat sbuf;
    if (::lstat(fname.c_str(), &sbuf))
    {
        PLOG(ERROR) << "isSymLink lstat " << fname << " failed: ";
        return false;
    }

    if (S_ISLNK(sbuf.st_mode))
    {
        return true;
    }
    return false;
}

Status ReadLink(const std::string& fname, std::string* data)
{
    data->clear();

    char name[1024];
    ssize_t n = ::readlink(fname.c_str(), name, sizeof name);
    if (n == -1)
    {
        PLOG(ERROR) << "ReadLink " << fname << " failed: ";
        return kIOError;
    }
    else
    {
        DCHECK(n < static_cast<ssize_t>(sizeof(name)));
    }

    data->append(name, n);
    return kOk;
}

} // namespace FileUtil
} // namespace claire
