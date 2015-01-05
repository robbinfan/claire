// Copyright (c) 2013 The claire-common Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef _CLAIRE_COMMON_FILES_FILEUTIL_H_
#define _CLAIRE_COMMON_FILES_FILEUTIL_H_

#include <stdio.h>
#include <sys/file.h>
#include <sys/types.h>

#include <string>
#include <vector>

#include <boost/noncopyable.hpp>

#include <claire/common/strings/StringPiece.h>
#include <claire/common/logging/Logging.h>

namespace claire {
namespace FileUtil {

enum Status
{
    kOk,
    kIOError
};

class SequentialFile : boost::noncopyable
{
public:
    SequentialFile(const std::string& fname)
        : filename_(fname),
          fp_(::fopen(fname.c_str(), "r"))
    {}

    SequentialFile(const std::string& fname, FILE* f)
        : filename_(fname),
          fp_(f)
    {}

    ~SequentialFile() { ::fclose(fp_); }

    // Read up to "n" bytes from the file.  "scratch[0..n-1]" may be
    // written by this routine.  Sets "*result" to the data that was
    // read (including if fewer than "n" bytes were successfully read).
    // May set "*result" to point at data in "scratch[0..n-1]", so
    // "scratch[0..n-1]" must be live when "*result" is used.
    // If an error was encountered, returns a non-OK status.
    //
    // REQUIRES: External synchronization
    Status Read(size_t n, StringPiece* result, char* scratch);

    // Skip "n" bytes from the file. This is guaranteed to be no
    // slower that reading the same data, but may be faster.
    //
    // If end of file is reached, skipping will stop at the end of the
    // file, and Skip will return OK.
    //
    // REQUIRES: External synchronization
    Status Skip(uint64_t n);

private:
    std::string filename_;
    FILE* fp_;
};

// A file abstraction for randomly reading the contents of a file.
// pread() based random-access
class RandomAccessFile : boost::noncopyable
{
public:
    RandomAccessFile(const std::string& fname)
        : filename_(fname),
          fd_(::open(fname.c_str(), O_RDONLY))
    {}

    RandomAccessFile(const std::string& fname, int fd)
        : filename_(fname),
          fd_(fd)
    {}

    ~RandomAccessFile() { ::close(fd_); }

    // Read up to "n" bytes from the file starting at "offset".
    // "scratch[0..n-1]" may be written by this routine.  Sets "*result"
    // to the data that was read (including if fewer than "n" bytes were
    // successfully read).  May set "*result" to point at data in
    // "scratch[0..n-1]", so "scratch[0..n-1]" must be live when
    // "*result" is used.  If an error was encountered, returns a non-OK
    // status.
    //
    // Safe for concurrent use by multiple threads.
    Status Read(uint64_t offset, size_t n, StringPiece* result, char* scratch) const;

private:
    std::string filename_;
    int fd_;
};

// A file abstraction for sequential writing.  The implementation
// must provide buffering since callers may append small fragments
// at a time to the file.
class WritableFile : boost::noncopyable
{
public:
    WritableFile(const std::string& fname)
        : filename_(fname),
          fp_(::fopen(fname.c_str(), "w+e")),
          written_bytes_(0)
    {
        ::setbuffer(fp_, buffer_, sizeof buffer_);
    }

    WritableFile(const std::string&fname, FILE* fp)
        : filename_(fname),
          fp_(fp),
          written_bytes_(0)
    {
        ::setbuffer(fp_, buffer_, sizeof buffer_);
    }

    virtual ~WritableFile() { ::fclose(fp_); }

    Status Append(const StringPiece& data);
    Status Close();
    Status Flush();

    size_t WrittenBytes() const { return written_bytes_; }
    const std::string filename() const { return filename_; }
    int fd() const { return ::fileno(fp_); }

private:
    std::string filename_;
    FILE* fp_;
    char buffer_[64*1024];
    size_t written_bytes_;
};

// A file abstraction for sequential writing at end of existing file.
class AppendableFile: public WritableFile
{
public:
    AppendableFile(const std::string& fname)
        : WritableFile(fname, fopen(fname.c_str(), "ae"))
    {}

    AppendableFile(const std::string& fname, FILE* fp)
        : WritableFile(fname, fp)
    {}

    virtual ~AppendableFile()
    {}
};

// Identifies a locked file.
class FileLock : boost::noncopyable
{
public:
    FileLock() {}
    virtual ~FileLock() {}

    int fd_;
};

// Create a brand new sequentially-readable file with the specified name.
// On success, stores a pointer to the new file in *result and returns OK.
// On failure stores NULL in *result and returns non-OK.  If the file does
// not exist, returns a non-OK status.
//
// The returned file will only be accessed by one thread at a time.
Status NewSequentialFile(const std::string& fname, SequentialFile** result);

// Create a brand new random access read-only file with the
// specified name.  On success, stores a pointer to the new file in
// *result and returns OK.  On failure stores NULL in *result and
// returns non-OK.  If the file does not exist, returns a non-OK
// status.
//
// The returned file may be concurrently accessed by multiple threads.
Status NewRandomAccessFile(const std::string& fname, RandomAccessFile** result);

// Create an object that writes to a new file with the specified
// name.  Deletes any existing file with the same name and creates a
// new file.  On success, stores a pointer to the new file in
// *result and returns OK.  On failure stores NULL in *result and
// returns non-OK.
//
// The returned file will only be accessed by one thread at a time.
Status NewWritableFile(const std::string& fname, WritableFile** result);

// Derived from NewWritableFile.  One change: if the file exists,
// move to the end of the file and continue writing.
// new file.  On success, stores a pointer to the open file in
// *result and returns OK.  On failure stores NULL in *result and
// returns non-OK.
//
// The returned file will only be accessed by one thread at a time.
Status NewAppendableFile(const std::string& fname, WritableFile** result);

// Returns true iff the named file exists.
bool FileExists(const std::string& fname);

// Store in *result the names of the children of the specified directory.
// The names are relative to "dir".
// Original contents of *results are dropped.
Status GetChildren(const std::string& dir, std::vector<std::string>* result);

// Delete the named file.
Status DeleteFile(const std::string& fname);

// Create the specified directory.
Status CreateDir(const std::string& dirname);

// Delete the specified directory.
Status DeleteDir(const std::string& dirname);

// Store the size of fname in *file_size.
Status GetFileSize(const std::string& fname, uint64_t* file_size);

// Rename file src to target.
Status RenameFile(const std::string& src, const std::string& target);

// Lock the specified file.  Used to prevent concurrent access to
// the same db by multiple processes.  On failure, stores NULL in
// *lock and returns non-OK.
//
// On success, stores a pointer to the object that represents the
// acquired lock in *lock and returns OK.  The caller should call
// UnlockFile(*lock) to release the lock.  If the process exits,
// the lock will be automatically released.
//
// If somebody else already holds the lock, finishes immediately
// with a failure.  I.e., this call does not wait for existing locks
// to go away.
//
// May create the named file if it does not already exist.
Status LockFile(const std::string& fname, FileLock** lock);

// Release the lock acquired by a previous successful call to LockFile.
// REQUIRES: lock was returned by a successful LockFile() call
// REQUIRES: lock has not already been unlocked.
Status UnlockFile(FileLock* lock);

// A utility routine: write "data" to the named file.
Status WriteStringToFile(const StringPiece& data, const std::string& fname);

// A utility routine: read contents of named file into *data
Status ReadFileToString(const std::string& fname, std::string* data);

// A utility routine: create a symbolic link point to oldname named newname
Status SymLink(const std::string& oldname, const std::string& newname);

// A utility routine: determin filename is symbolic link or not
bool IsSymLink(const std::string& fname);

// A utility routine: read real name of symbolic link point to
Status ReadLink(const std::string& fname, std::string* data);

} // namespace FileUtil
} // namespace claire

#endif // _CLAIRE_COMMON_FILES_FILEUTIL_H_
