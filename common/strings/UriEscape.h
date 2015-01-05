// Copyright (c) 2013 The claire-common Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

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

#ifndef _CLAIRE_COMMON_STRINGS_URIESCAPE_H_
#define _CLAIRE_COMMON_STRINGS_URIESCAPE_H_

#include <string>

#include <claire/common/strings/StringPiece.h>

namespace claire {

enum class EscapeMode : char
{
    // The values are meaningful, see generate_escape_tables.py
    ALL = 0,
    QUERY = 1,
    PATH = 2
};

///
/// URI-escape a string.  Appends the result to the output string.
///
/// Alphanumeric characters and other characters marked as "unreserved" in RFC
/// 3986 ( -_.~ ) are left unchanged.  In PATH mode, the forward slash (/) is
/// also left unchanged.  In QUERY mode, spaces are replaced by '+'.  All other
/// characters are percent-encoded.
///
void UriEscape(const StringPiece& input,
               std::string* output,
               EscapeMode mode);

inline std::string UriEscape(const StringPiece& input, EscapeMode mode)
{
    std::string output;
    UriEscape(input, &output, mode);
    return output;
}

///
/// URI-unescape a string.  Appends the result to the output string.
///
/// In QUERY mode, '+' are replaced by space.  %XX sequences are decoded if
/// XX is a valid hex sequence, otherwise we throw invalid_argument.
///
void UriUnescape(const StringPiece& input,
                 std::string* output,
                 EscapeMode mode);

inline std::string UriUnescape(const StringPiece& input, EscapeMode mode)
{
    std::string output;
    UriUnescape(input, &output, mode);
    return output;
}

} // namespace claire

#endif // _CLAIRE_COMMON_STRINGS_URIESCAPE_H_
