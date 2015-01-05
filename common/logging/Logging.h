// Copyright (c) 2013 The claire-common Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

// Copyright (c) 1999, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// This file contains #include information about logging-related stuff.
// Pretty much everybody needs to #include this file so that they can
// log various happenings.

#ifndef _CLAIRE_COMMON_LOGGING_LOGGING_H
#define _CLAIRE_COMMON_LOGGING_LOGGING_H

#include <gflags/gflags.h>

#include <iosfwd>
#include <string>
#include <ostream>
#include <sstream>

#include <claire/common/base/Likely.h>
#include <claire/common/logging/LogMessage.h>

// Make a bunch of macros for logging.  The way to log things is to stream
// things to LOG(<a particular severity level>).  E.g.,
//
//   LOG(INFO) << "Found " << num_cookies << " cookies";
//
// You can also do conditional logging:
//
//   LOG_IF(INFO, num_cookies > 10) << "Got lots of cookies";
//
// You can also do occasional logging (log every n'th occurrence of an
// event):
//
//   LOG_EVERY_N(INFO, 10) << "Got the " << claire::COUNTER << "th cookie";
//
// The above will cause log messages to be output on the 1st, 11th, 21st, ...
// times it is executed.  Note that the special claire::COUNTER value is used
// to identify which repetition is happening.
//
// You can also do occasional conditional logging (log every n'th
// occurrence of an event, when condition is satisfied):
//
//   LOG_IF_EVERY_N(INFO, (size > 1024), 10) << "Got the " << claire::COUNTER
//                                           << "th big cookie";
//
// You can log messages the first N times your code executes a line. E.g.
//
//   LOG_FIRST_N(INFO, 20) << "Got the " << claire::COUNTER << "th cookie";
//
// Outputs log messages for the first 20 times it is executed.
//
// There are also "debug mode" logging macros like the ones above:
//
//   DLOG(INFO) << "Found cookies";
//
//   DLOG_IF(INFO, num_cookies > 10) << "Got lots of cookies";
//
//   DLOG_EVERY_N(INFO, 10) << "Got the " << claire::COUNTER << "th cookie";
//
// All "debug mode" logging is compiled away to nothing for non-debug mode
// compiles.
//
// We also have
//
//   LOG_ASSERT(assertion);
//   DLOG_ASSERT(assertion);
//
// which is syntactic sugar for {,D}LOG_IF(FATAL, assert fails) << assertion;
//
// The supported severity levels for macros that allow you to specify one
// are (in increasing order of severity) TRACE, DEBUG, INFO, WARNING, ERROR, and FATAL.
//
// There is also the special severity of DFATAL, which logs FATAL in
// debug mode, ERROR in normal mode.
//
// Very important: logging a message at the FATAL severity level causes
// the program to terminate (after the message is logged).
//
// Logs will be written to the filename like "<program name>.<date time>.
// <hostname>.<user name>.<pid>.log", and the date time is in UTC.
//

// LOG LINE PREFIX FORMAT
//
// Log lines have this form:
//
//     yyyymmdd hh:mm:ss.uuuuuu threadid severity - file:line : msg...
//
// where the fields are defined as follows:
//
//   yyyy             The year
//   mm               The month (zero padded; ie May is '05')
//   dd               The day (zero padded)
//   hh:mm:ss.uuuuuu  Time in hours, minutes and fractional seconds
//   threadid         The space-padded thread ID as returned by GetTID()
//                    (this matches the PID on Linux)
//   severity         The log severity string
//   file             The file name
//   line             The line number
//   msg              The user-supplied message
//
// Example:
//
//   20121103 11:57:31.739339 24395 INFO - test.cc:2341 : Command line: ./some_prog
//   20131103 11:57:31.739403 24395 INFO - test.cc:2342 : Process id 24395
//
// NOTE: although the microseconds are useful for comparing events on
// a single machine, clocks on different machines may not be well
// synchronized.  Hence, use caution when comparing the low bits of
// timestamps from different machines.

#define LOG_TRACE if (FLAGS_minloglevel <= static_cast<int>(claire::LogSeverity::TRACE)) \
    claire::LogMessage(__FILE__, __LINE__, claire::LogSeverity::TRACE, __func__).stream()
#define LOG_DEBUG if (FLAGS_minloglevel <= static_cast<int>(claire::LogSeverity::DEBUG)) \
    claire::LogMessage(__FILE__, __LINE__, claire::LogSeverity::DEBUG, __func__).stream()
#define LOG_INFO  if (FLAGS_minloglevel <= static_cast<int>(claire::LogSeverity::INFO)) \
    claire::LogMessage(__FILE__, __LINE__).stream()
#define LOG_WARNING \
    claire::LogMessage(__FILE__, __LINE__, claire::LogSeverity::WARNING).stream()
#define LOG_ERROR \
    claire::LogMessage(__FILE__, __LINE__, claire::LogSeverity::ERROR).stream()
#define LOG_FATAL \
    claire::LogMessageFatal(__FILE__, __LINE__).stream()

#ifndef NDEBUG
#define LOG_DFATAL LOG_ERROR
#else
#define LOG_DFATAL LOG_FATAL
#endif

// We use the preprocessor's merging operator, "##", so that, e.g.,
// LOG(INFO) becomes the token LOG_INFO.
#define LOG(severity) LOG_ ## severity

#define LOG_IF(severity, cond) \
    !(cond) ? (void)0 : claire::LogMessageVoidify() & LOG(severity)

#define LOG_ASSERT(condition)  \
    LOG_IF(FATAL, !(condition)) << "Assert failed: " #condition ". "

// CHECK dies with a fatal error if condition is not true.  It is *not*
// controlled by NDEBUG, so the check will be executed regardless of
// compilation mode.  Therefore, it is safe to do things like:
//    CHECK(fp->Write(x) == 4)
#define CHECK(condition)  \
      LOG_IF(FATAL, UNLIKELY(!(condition))) << "Check failed: " #condition " "

namespace claire {

// A container for a string pointer which can be evaluated to a bool -
// true iff the pointer is NULL.
struct CheckOpString
{
    CheckOpString(std::string* str) : str_(str) {}
    // No destructor: if str_ is non-NULL, we're about to LOG(FATAL),
    // so there's no point in cleaning up str_.
    operator bool() const { return UNLIKELY(str_ != NULL); }
    std::string* str_;
};

namespace detail {

// Function is overloaded for integral types to allow static const
// integrals declared in classes and not defined to be used as arguments to
// CHECK* macros. It's not encouraged though.
template <class T>
inline const T&       GetReferenceableValue(const T&           t) { return t; }
inline char           GetReferenceableValue(char               t) { return t; }
inline unsigned char  GetReferenceableValue(unsigned char      t) { return t; }
inline signed char    GetReferenceableValue(signed char        t) { return t; }
inline short          GetReferenceableValue(short              t) { return t; }
inline unsigned short GetReferenceableValue(unsigned short     t) { return t; }
inline int            GetReferenceableValue(int                t) { return t; }
inline unsigned int   GetReferenceableValue(unsigned int       t) { return t; }
inline long           GetReferenceableValue(long               t) { return t; }
inline unsigned long  GetReferenceableValue(unsigned long      t) { return t; }
inline long long      GetReferenceableValue(long long          t) { return t; }
inline unsigned long long GetReferenceableValue(unsigned long long t) { return t; }

// This formats a value for a failing CHECK_XX statement.  Ordinarily,
// it uses the definition for operator<<, with a few special cases below.
template <typename T>
void MakeCheckOpValueString(std::ostream* os, const T& v)
{
    (*os) << v;
}

// Overrides for char types provide readable values for unprintable
// characters.
template <>
void MakeCheckOpValueString(std::ostream* os, const char& v);
template <>
void MakeCheckOpValueString(std::ostream* os, const signed char& v);
template <>
void MakeCheckOpValueString(std::ostream* os, const unsigned char& v);

// Build the error message string. Specify no inlining for code size.
template <typename T1, typename T2>
std::string* MakeCheckOpString(const T1& v1, const T2& v2, const char* exprtext) __attribute__ ((noinline));

// A helper class for formatting "expr (V1 vs. V2)" in a CHECK_XX
// statement.  See MakeCheckOpString for sample usage.
class CheckOpMessageBuilder
{
public:
    // Inserts "exprtext" and " (" to the stream.
    explicit CheckOpMessageBuilder(const char *exprtext);
    // Deletes "stream_".
    ~CheckOpMessageBuilder();
    // For inserting the first variable.
    std::ostream* ForVar1() { return stream_; }
    // For inserting the second variable (adds an intermediate " vs. ").
    std::ostream* ForVar2();
    // Get the result (inserts the closing ")").
    std::string* NewString();

private:
    std::ostringstream* stream_;
};

template <typename T1, typename T2>
std::string* MakeCheckOpString(const T1& v1, const T2& v2, const char* exprtext)
{
    CheckOpMessageBuilder comb(exprtext);
    MakeCheckOpValueString(comb.ForVar1(), v1);
    MakeCheckOpValueString(comb.ForVar2(), v2);
    return comb.NewString();
}

}  // namespace detail
}  // namespace claire

// Helper functions for CHECK_OP macro.
// The (int, int) specialization works around the issue that the compiler
// will not instantiate the template version of the function on values of
// unnamed enum type - see comment below.
#define DEFINE_CHECK_OP_IMPL(name, op) \
    template <typename T1, typename T2> \
    inline std::string* name##Impl(const T1& v1, const T2& v2, const char* exprtext) \
    { \
        if (LIKELY(v1 op v2)) return NULL; \
        else return claire::detail::MakeCheckOpString(v1, v2, exprtext); \
    } \
    inline std::string* name##Impl(int v1, int v2, const char* exprtext) \
    { \
        return name##Impl<int, int>(v1, v2, exprtext); \
    }

// We use the full name Check_EQ, Check_NE, etc. in case the file including
// Logging.h provides its own #defines for the simpler names EQ, NE, etc.
// This happens if, for example, those are used as token names in a
// yacc grammar.
DEFINE_CHECK_OP_IMPL(Check_EQ, ==)  // Compilation error with CHECK_EQ(NULL, x)?
DEFINE_CHECK_OP_IMPL(Check_NE, !=)  // Use CHECK(x == NULL) instead.
DEFINE_CHECK_OP_IMPL(Check_LE, <=)
DEFINE_CHECK_OP_IMPL(Check_LT, < )
DEFINE_CHECK_OP_IMPL(Check_GE, >=)
DEFINE_CHECK_OP_IMPL(Check_GT, > )
#undef DEFINE_CHECK_OP_IMPL

// Helper macro for binary operators.
// Don't use this macro directly in your code, use CHECK_EQ et al below.

#if !defined(NDEBUG)
// In debug mode, avoid constructing CheckOpStrings if possible,
// to reduce the overhead of CHECK statments by 2x.
// Real DCHECK-heavy tests have seen 1.5x speedups.

#define CHECK_OP_LOG(name, op, val1, val2, log)                  \
    while (auto _result =                                        \
         Check##name##Impl(                                      \
             claire::detail::GetReferenceableValue(val1),        \
             claire::detail::GetReferenceableValue(val2),        \
             #val1 " " #op " " #val2))                           \
    log(__FILE__, __LINE__, claire::CheckOpString(_result)).stream()
#else
// In optimized mode, use CheckOpString to hint to compiler that
// the while condition is unlikely.
#define CHECK_OP_LOG(name, op, val1, val2, log)                  \
  while (claire::CheckOpString _result =                         \
         Check##name##Impl(                                      \
             claire::detail::GetReferenceableValue(val1),        \
             claire::detail::GetReferenceableValue(val2),        \
             #val1 " " #op " " #val2))                           \
    log(__FILE__, __LINE__, _result).stream()
#endif  // !NDEBUG

#define CHECK_OP(name, op, val1, val2) \
    CHECK_OP_LOG(name, op, val1, val2, claire::LogMessageFatal)

// Equality/Inequality checks - compare two values, and log a FATAL message
// including the two values when the result is not as expected.  The values
// must have operator<<(LogStream&, ...) defined.
//
// You may append to the error message like so:
//   CHECK_NE(1, 2) << ": The world must be ending!";
//
// We are very careful to ensure that each argument is evaluated exactly
// once, and that anything which is legal to pass as a function argument is
// legal here.  In particular, the arguments may be temporary expressions
// which will end up being destroyed at the end of the apparent statement,
// for example:
//   CHECK_EQ(std::string("abc")[1], 'b');
//
// WARNING: These don't compile correctly if one of the arguments is a pointer
// and the other is NULL. To work around this, simply static_cast NULL to the
// type of the desired pointer.

#define CHECK_EQ(val1, val2) CHECK_OP(_EQ, ==, val1, val2)
#define CHECK_NE(val1, val2) CHECK_OP(_NE, !=, val1, val2)
#define CHECK_LE(val1, val2) CHECK_OP(_LE, <=, val1, val2)
#define CHECK_LT(val1, val2) CHECK_OP(_LT, < , val1, val2)
#define CHECK_GE(val1, val2) CHECK_OP(_GE, >=, val1, val2)
#define CHECK_GT(val1, val2) CHECK_OP(_GT, > , val1, val2)

// Check that the input is non NULL.  This very useful in constructor
// initializer lists.

#define CHECK_NOTNULL(val) \
    claire::CheckNotNull(__FILE__, __LINE__, "'" #val "' Must be non NULL", (val))

// Helper functions for string comparisons.
// To avoid bloat, the definitions are in Logging.cc.
#define DECLARE_CHECK_STROP_IMPL(func, expected) \
    namespace claire { \
    namespace detail { \
        std::string* Check##func##expected##Impl(const char* s1, const char* s2, const char* names); \
    } \
    }

DECLARE_CHECK_STROP_IMPL(strcmp, true)
DECLARE_CHECK_STROP_IMPL(strcmp, false)
DECLARE_CHECK_STROP_IMPL(strcasecmp, true)
DECLARE_CHECK_STROP_IMPL(strcasecmp, false)
#undef DECLARE_CHECK_STROP_IMPL

// Helper macro for string comparisons.
// Don't use this macro directly in your code, use CHECK_STREQ et al below.
#define CHECK_STROP(func, op, expected, s1, s2) \
    while (claire::CheckOpString _result = \
           claire::detail::Check##func##expected##Impl((s1), (s2), #s1 " " #op " " #s2)) \
        LOG(FATAL) << _result

// String (char*) equality/inequality checks.
// CASE versions are case-insensitive.
//
// Note that "s1" and "s2" may be temporary strings which are destroyed
// by the compiler at the end of the current "full expression"
// (e.g. CHECK_STREQ(Foo().c_str(), Bar().c_str())).

#define CHECK_STREQ(s1, s2) CHECK_STROP(strcmp, ==, true, s1, s2)
#define CHECK_STRNE(s1, s2) CHECK_STROP(strcmp, !=, false, s1, s2)
#define CHECK_STRCASEEQ(s1, s2) CHECK_STROP(strcasecmp, ==, true, s1, s2)
#define CHECK_STRCASENE(s1, s2) CHECK_STROP(strcasecmp, !=, false, s1, s2)

#define CHECK_INDEX(I,A) CHECK(I < (sizeof(A)/sizeof(A[0])))
#define CHECK_BOUND(B,A) CHECK(B <= (sizeof(A)/sizeof(A[0])))

#define CHECK_DOUBLE_EQ(val1, val2)                  \
    do {                                             \
        CHECK_LE((val1), (val2)+0.000000000000001L); \
        CHECK_GE((val1), (val2)-0.000000000000001L); \
    } while (0)

#define CHECK_NEAR(val1, val2, margin)           \
    do {                                         \
        CHECK_LE((val1), (val2)+(margin));       \
        CHECK_GE((val1), (val2)-(margin));       \
    } while (0)


// PLOG() and PLOG_IF() and PCHECK() behave exactly like their LOG* and
// CHECK equivalents with the addition that they postpend a description
// of the current state of errno to their output lines.

#define PLOG(severity) \
    claire::ErrnoLogMessage( \
        __FILE__, __LINE__, claire::LogSeverity::severity, 0).stream()

#define PLOG_IF(severity, condition) \
    !(condition) ? (void) 0 : claire::LogMessageVoidify() & PLOG(severity)

// A CHECK() macro that postpends errno if the condition is false. E.g.
//
// if (poll(fds, nfds, timeout) == -1) { PCHECK(errno == EINTR); ... }
#define PCHECK(condition)  \
    PLOG_IF(FATAL, UNLIKELY(!(condition))) << "Check failed: " #condition " "

// A CHECK() macro that lets you assert the success of a function that
// returns -1 and sets errno in case of an error. E.g.
//
// CHECK_ERR(mkdir(path, 0700));
//
// or
//
// int fd = open(filename, flags); CHECK_ERR(fd) << ": open " << filename;
#define CHECK_ERR(invocation)                                          \
    PLOG_IF(FATAL, UNLIKELY((invocation) == -1)) << #invocation

// Use macro expansion to create, for each use of LOG_EVERY_N(), static
// variables with the __LINE__ expansion as part of the variable name.
#define LOG_EVERY_N_VARNAME(base, line) LOG_EVERY_N_VARNAME_CONCAT(base, line)
#define LOG_EVERY_N_VARNAME_CONCAT(base, line) base ## line

#define LOG_OCCURRENCES LOG_EVERY_N_VARNAME(occurrences_, __LINE__)
#define LOG_OCCURRENCES_MOD_N LOG_EVERY_N_VARNAME(occurrences_mod_n_, __LINE__)

#define SOME_KIND_OF_LOG_EVERY_N(severity, n) \
    static int LOG_OCCURRENCES = 0, LOG_OCCURRENCES_MOD_N = 0; \
    ++LOG_OCCURRENCES; \
    if (++LOG_OCCURRENCES_MOD_N > n) LOG_OCCURRENCES_MOD_N -= n; \
        if (LOG_OCCURRENCES_MOD_N == 1) \
            claire::LogMessage( \
                __FILE__, __LINE__, claire::LogSeverity::severity, LOG_OCCURRENCES).stream()

#define SOME_KIND_OF_LOG_IF_EVERY_N(severity, condition, n) \
    static int LOG_OCCURRENCES = 0, LOG_OCCURRENCES_MOD_N = 0; \
    ++LOG_OCCURRENCES; \
    if (condition && \
        ((LOG_OCCURRENCES_MOD_N=(LOG_OCCURRENCES_MOD_N + 1) % n) == (1 % n))) \
            claire::LogMessage( \
                __FILE__, __LINE__, claire::LogSeverity::severity, LOG_OCCURRENCES).stream()

#define SOME_KIND_OF_PLOG_EVERY_N(severity, n, what_to_do) \
    static int LOG_OCCURRENCES = 0, LOG_OCCURRENCES_MOD_N = 0; \
    ++LOG_OCCURRENCES; \
    if (++LOG_OCCURRENCES_MOD_N > n) LOG_OCCURRENCES_MOD_N -= n; \
        if (LOG_OCCURRENCES_MOD_N == 1) \
            claire::ErrnoLogMessage( \
                __FILE__, __LINE__, claire::LogSeverity::severity, LOG_OCCURRENCES).stream()

#define SOME_KIND_OF_LOG_FIRST_N(severity, n) \
    static int LOG_OCCURRENCES = 0; \
    if (LOG_OCCURRENCES <= n) \
        ++LOG_OCCURRENCES; \
    if (LOG_OCCURRENCES <= n) \
        claire::LogMessage( \
            __FILE__, __LINE__, claire::LogSeverity::severity, LOG_OCCURRENCES).stream()

#define LOG_EVERY_N(severity, n) \
    SOME_KIND_OF_LOG_EVERY_N(severity, (n))

#define PLOG_EVERY_N(severity, n) \
    SOME_KIND_OF_PLOG_EVERY_N(severity, (n))

#define LOG_FIRST_N(severity, n) \
    SOME_KIND_OF_LOG_FIRST_N(severity, (n))

#define LOG_IF_EVERY_N(severity, condition, n) \
    SOME_KIND_OF_LOG_IF_EVERY_N(severity, (condition), (n))

// Plus some debug-logging macros that get compiled to nothing for production

#ifndef NDEBUG

#define DLOG(severity) LOG(severity)
#define DLOG_IF(severity, condition) LOG_IF(severity, condition)
#define DLOG_EVERY_N(severity, n) LOG_EVERY_N(severity, n)
#define DLOG_IF_EVERY_N(severity, condition, n) LOG_IF_EVERY_N(severity, condition, n)
#define DLOG_ASSERT(condition) LOG_ASSERT(condition)

// debug-only checking.  not executed in NDEBUG mode.
#define DCHECK(condition) CHECK(condition)
#define DCHECK_EQ(val1, val2) CHECK_EQ(val1, val2)
#define DCHECK_NE(val1, val2) CHECK_NE(val1, val2)
#define DCHECK_LE(val1, val2) CHECK_LE(val1, val2)
#define DCHECK_LT(val1, val2) CHECK_LT(val1, val2)
#define DCHECK_GE(val1, val2) CHECK_GE(val1, val2)
#define DCHECK_GT(val1, val2) CHECK_GT(val1, val2)
#define DCHECK_NOTNULL(val) CHECK_NOTNULL(val)
#define DCHECK_STREQ(str1, str2) CHECK_STREQ(str1, str2)
#define DCHECK_STRCASEEQ(str1, str2) CHECK_STRCASEEQ(str1, str2)
#define DCHECK_STRNE(str1, str2) CHECK_STRNE(str1, str2)
#define DCHECK_STRCASENE(str1, str2) CHECK_STRCASENE(str1, str2)

#define DCHECK_ERR(invocation) CHECK_ERR(invocation)

#else  // NDEBUG

#define DLOG(severity) \
    true ? (void) 0 : claire::LogMessageVoidify() & LOG(severity)

#define DLOG_IF(severity, condition) \
    (true || !(condition)) ? (void) 0 : claire::LogMessageVoidify() & LOG(severity)

#define DLOG_EVERY_N(severity, n) \
    true ? (void) 0 : claire::LogMessageVoidify() & LOG(severity)

#define DLOG_IF_EVERY_N(severity, condition, n) \
    (true || !(condition))? (void) 0 : claire::LogMessageVoidify() & LOG(severity)

#define DLOG_ASSERT(condition) \
    true ? (void) 0 : LOG_ASSERT(condition)

#define DCHECK(condition) \
    while (false) CHECK(condition)

#define DCHECK_EQ(val1, val2) \
    while (false) CHECK_EQ(val1, val2)

#define DCHECK_NE(val1, val2) \
    while (false) CHECK_NE(val1, val2)

#define DCHECK_LE(val1, val2) \
    while (false) CHECK_LE(val1, val2)

#define DCHECK_LT(val1, val2) \
    while (false) CHECK_LT(val1, val2)

#define DCHECK_GE(val1, val2) \
    while (false) CHECK_GE(val1, val2)

#define DCHECK_GT(val1, val2) \
    while (false) CHECK_GT(val1, val2)

// You may see warnings in release mode if you don't use the return
// value of DCHECK_NOTNULL. Please just use DCHECK for such cases.
#define DCHECK_NOTNULL(val) (val)

#define DCHECK_STREQ(str1, str2) \
    while (false) CHECK_STREQ(str1, str2)

#define DCHECK_STRCASEEQ(str1, str2) \
    while (false) CHECK_STRCASEEQ(str1, str2)

#define DCHECK_STRNE(str1, str2) \
    while (false) CHECK_STRNE(str1, str2)

#define DCHECK_STRCASENE(str1, str2) \
    while (false) CHECK_STRCASENE(str1, str2)

#define DCHECK_ERR(invocation) \
    auto ret = (invocation); \
    while (false) CHECK_ERR(ret)

#endif  // NDEBUG

#define NOTREACHED() DCHECK(false)

#define HEXDUMP(severity) \
    LOG(severity) << claire::LogFormat::kHex

// A small helper for CHECK_NOTNULL().
template <typename T>
T* CheckNotNull(const char *file, int line, const char *names, T* t)
{
    if (t == NULL)
    {
        claire::LogMessageFatal(file, line, new std::string(names));
    }
    return t;
}

enum class PRIVATE_Counter : char {COUNTER};

// Allow folks to put a counter in the LOG_EVERY_X()'ed messages. This
// only works if ostream is a LogStream. If the ostream is not a
// LogStream you'll get an assert saying as much at runtime.
claire::LogStream& operator<<(claire::LogStream& os, const PRIVATE_Counter&);

// Set whether log messages go to stderr in addition to logfiles.
DECLARE_bool(alsologtostderr);

// Set color messages logged to stderr (if supported by terminal).
DECLARE_bool(colorlogtostderr);

// Log messages at a level >= this flag are automatically sent to
// stderr in addition to log files.
DECLARE_int32(stderrthreshold);

// Sets the maximum number of seconds which logs may be buffered for.
DECLARE_int32(logbufsecs);

// Log suppression level: messages logged at a lower level than this
// are suppressed.
DECLARE_int32(minloglevel);

// If specified, logfiles are written into this directory instead of the
// default logging directory.
DECLARE_string(log_dir);

// Sets the maximum log file size (in MB).
DECLARE_int32(max_log_size);

// Sets whether to avoid logging to the disk if the disk is full.
DECLARE_bool(stop_logging_if_full_disk);

namespace claire {

void InitClaireLogging(const char* argv0);
void ShutdownClaireLogging();

} // namespace claire

#endif // _CLAIRE_COMMON_LOGGING_LOGGING_H
