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

#include <claire/common/strings/UriEscape.h>

#include <stdexcept>

namespace claire {

// Map from character code to escape mode:
// 0 = pass through
// 1 = unused
// 2 = pass through in PATH mode
// 3 = space, replace with '+' in QUERY mode
// 4 = percent-encode
extern const unsigned char kUriEscapeTable[];

// Map from the character code to the hex value, or 16 if invalid hex char.
extern const unsigned char kHexTable[];

void UriEscape(const StringPiece& input,
               std::string* output,
               EscapeMode mode)
{
    static const char kHexValues[] = "0123456789ABCDEF";
    char esc[3];
    esc[0] = '%';
    // Preallocate assuming that 25% of the input string will be escaped
    output->reserve(output->size() + input.size() + 3 * (input.size() / 4));
    auto p = input.begin();
    auto last = p;  // last regular character
    // We advance over runs of passthrough characters and copy them in one go;
    // this is faster than calling push_back repeatedly.
    unsigned char min_encode = static_cast<unsigned char>(mode);
    while (p != input.end())
    {
        char c = *p;
        unsigned char v = static_cast<unsigned char>(c);
        unsigned char discriminator = kUriEscapeTable[v];
        if (discriminator <= min_encode)
        {
            ++p;
        }
        else if (mode == EscapeMode::QUERY && discriminator == 3)
        {
            output->append(&*last, p - last);
            output->push_back('+');
            ++p;
            last = p;
        }
        else
        {
            output->append(&*last, p - last);
            esc[1] = kHexValues[v >> 4];
            esc[2] = kHexValues[v & 0x0f];
            output->append(esc, 3);
            ++p;
            last = p;
        }
    }
    output->append(&*last, p - last);
}

void UriUnescape(const StringPiece& input,
                 std::string* output,
                 EscapeMode mode)
{
    output->reserve(output->size() + input.size());
    auto p = input.begin();
    auto last = p;
    // We advance over runs of passthrough characters and copy them in one go;
    // this is faster than calling push_back repeatedly.
    while (p != input.end())
    {
        char c = *p;
        unsigned char v = static_cast<unsigned char>(v);
        switch (c) {
            case '%':
                {
                    if (std::distance(p, input.end()) < 3)
                    {
                        throw std::invalid_argument("incomplete percent encode sequence");
                    }
                    auto h1 = kHexTable[static_cast<unsigned char>(p[1])];
                    auto h2 = kHexTable[static_cast<unsigned char>(p[2])];
                    if (h1 == 16 || h2 == 16)
                    {
                         throw std::invalid_argument("invalid percent encode sequence");
                    }
                    output->append(&*last, p - last);
                    output->push_back(static_cast<char>((h1 << 4) | h2));
                    p += 3;
                    last = p;
                    break;
                }
            case '+':
                if (mode == EscapeMode::QUERY)
                {
                    output->append(&*last, p - last);
                    output->push_back(' ');
                    ++p;
                    last = p;
                    break;
                }
                // else fallthrough
            default:
                ++p;
                break;
        }
    }
    output->append(&*last, p - last);
}

} // namespace claire
