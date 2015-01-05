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

#ifndef _CLAIRE_COMMON_SYMBOLIZER_SYMBOLIZER_H_
#define _CLAIRE_COMMON_SYMBOLIZER_SYMBOLIZER_H_

#include <stdint.h>

#include <string>

#include <boost/noncopyable.hpp>
#include <boost/ptr_container/ptr_map.hpp>

#include <claire/common/symbolizer/Elf.h>
#include <claire/common/symbolizer/Dwarf.h>
#include <claire/common/strings/StringPiece.h>

namespace claire {

/// Convert an address to symbol name and source location.
class Symbolizer : boost::noncopyable
{
public:
    /// Symbolize an instruction pointer address, returning the symbol name
    /// and file/line number information.
    ///
    /// The returned StringPiece objects are valid for the lifetime of
    /// this Symbolizer object.
    bool Symbolize(uintptr_t address,
                   StringPiece* symbol_name,
                   Dwarf::LocationInfo* location);

    static void Write(std::string* out,
                      uintptr_t address,
                      const StringPiece& symbol_name,
                      const Dwarf::LocationInfo& location);

private:
    ElfFile* GetFile(const std::string& name);
    // cache open ELF files
    boost::ptr_map<const std::string, ElfFile> elf_files_;
};

}  // namespace claire

#endif // _CLAIRE_COMMON_SYMBOLIZER_SYMBOLIZER_H_
