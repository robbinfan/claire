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

// ELF file parser

#ifndef _CLAIRE_COMMON_SYMBOLIZER_ELF_H_
#define _CLAIRE_COMMON_SYMBOLIZER_ELF_H_

#include <stdio.h>
#include <elf.h>
#include <link.h>  // For ElfW()

#include <claire/common/base/Likely.h>
#include <claire/common/strings/StringPiece.h>
#include <claire/common/logging/Logging.h>

namespace claire {

///
/// ELF file parser.
///
/// We handle native files only (32-bit files on a 32-bit platform, 64-bit files
/// on a 64-bit platform), and only executables (ET_EXEC) and shared objects
/// (ET_DYN).
///
class ElfFile : boost::noncopyable
{
public:
    ElfFile();
    explicit ElfFile(const char* name);
    ~ElfFile();

    ElfFile(ElfFile&& other);
    ElfFile& operator=(ElfFile&& other);

    /// Retrieve the ELF header
    const ElfW(Ehdr)& GetElfHeader() const
    {
        return At<ElfW(Ehdr)>(0);
    }

    ///
    /// Get the base address, the address where the file should be loaded if
    /// no relocations happened.
    ///
    uintptr_t GetBaseAddress() const
    {
        return base_address_;
    }

    /// Find a section given its name
    const ElfW(Shdr)* GetSectionByName(const char* name) const;

    /// Find a section given its index in the section header table
    const ElfW(Shdr)* GetSectionByIndex(size_t idx) const;

    /// Retrieve the name of a section
    const char* GetSectionName(const ElfW(Shdr)& section) const;

    /// Get the actual section body
    StringPiece GetSectionBody(const ElfW(Shdr)& section) const;

    /// Retrieve a string from a string table section
    const char* GetString(const ElfW(Shdr)& string_table, size_t offset) const;

    ///
    /// Find symbol definition by address.
    /// Note that this is the file virtual address, so you need to undo
    /// any relocation that might have happened.
    ///
    typedef std::pair<const ElfW(Shdr)*, const ElfW(Sym)*> Symbol;
    Symbol GetDefinitionByAddress(uintptr_t address) const;

    ///
    /// Find symbol definition by name.
    ///
    /// If a symbol with this name cannot be found, a <nullptr, nullptr> Symbol
    /// will be returned. This is O(N) in the number of symbols in the file.
    ///
    Symbol GetSymbolByName(const char* name) const;

    /// Get the value of a symbol.
    template <class T>
    const T& GetSymbolValue(const ElfW(Sym)* symbol) const
    {
        const ElfW(Shdr)* section = GetSectionByIndex(symbol->st_shndx);
        DCHECK(section) <<  "Symbol's section index is invalid";

        return ValueAt<T>(*section, symbol->st_value);
    }

    ///
    /// Get the value of the object stored at the given address.
    ///
    /// This is the function that you want to use in conjunction with
    /// getSymbolValue() to follow pointers. For example, to get the value of
    /// a char* symbol, you'd do something like this:
    ///
    ///  auto sym = GetSymbolByName("someGlobalValue");
    ///  auto addr = GetSymbolValue<ElfW(Addr)>(sym.second);
    ///  const char* str = &GetSymbolValue<const char>(addr);
    ///
    template <class T>
    const T& GetAddressValue(const ElfW(Addr) addr) const
    {
        const ElfW(Shdr)* section = GetSectionContainingAddress(addr);
        DCHECK(section) << "Address does not refer to existing section";

        return ValueAt<T>(*section, addr);
    }

    /// Retrieve symbol name.
    const char* GetSymbolName(Symbol symbol) const;

    /// Find the section containing the given address
    const ElfW(Shdr)* GetSectionContainingAddress(ElfW(Addr) addr) const;

private:
    void Init();
    void Destroy();

    void ValidateStringTable(const ElfW(Shdr)& string_table) const;

    template <class T>
    const typename std::enable_if<std::is_pod<T>::value, T>::type& At(ElfW(Off) offset) const
    {
        DCHECK(offset + sizeof(T) <= length_)
            << "Offset is not contained within our mmapped file";
        return *reinterpret_cast<T*>(file_ + offset);
    }

    template <class T>
    const T& ValueAt(const ElfW(Shdr)& section, const ElfW(Addr) addr) const
    {
        // For exectuables and shared objects, st_value holds a virtual address
        // that refers to the memory owned by sections. Since we didn't map the
        // sections into the addresses that they're expecting (sh_addr), but
        // instead just mmapped the entire file directly, we need to translate
        // between addresses and offsets into the file.
        //
        // TODO: For other file types, st_value holds a file offset directly. Since
        //       I don't have a use-case for that right now, just assert that
        //       nobody wants this. We can always add it later.
        DCHECK(GetElfHeader().e_type == ET_EXEC || GetElfHeader().e_type == ET_DYN)
            << "Only exectuables and shared objects are supported";
        DCHECK(addr >= section.sh_addr &&
            (addr + sizeof(T)) <= (section.sh_addr + section.sh_size))
            << "Address is not contained within the provided segment";

        return At<T>(section.sh_offset + (addr - section.sh_addr));
    }

    int fd_;
    char* file_;     // mmap() location
    size_t length_;  // mmap() length

    uintptr_t base_address_;
};

}  // namespace claire

#endif //  _CLAIRE_COMMON_SYMBOLIZER_ELF_H_
