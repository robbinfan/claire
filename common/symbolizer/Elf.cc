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

#include <claire/common/symbolizer/Elf.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

#include <string>

#include <claire/common/logging/Logging.h>

namespace claire {

ElfFile::ElfFile()
    : fd_(-1),
      file_(reinterpret_cast<char*>(-1)), // MAP_FAILED
      length_(0),
      base_address_(static_cast<uintptr_t>(0))
{}

ElfFile::ElfFile(const char* name)
    : fd_(open(name, O_RDONLY)),
      file_(reinterpret_cast<char*>(-1)), // MAP_FAILED
      length_(0),
      base_address_(static_cast<uintptr_t>(0))
{
    DCHECK(fd_ >= 0);

    struct stat st;
    int r = fstat(fd_, &st);
    DCHECK (r != -1);
    length_ = st.st_size;

    file_ = static_cast<char*>(mmap(NULL, length_, PROT_READ, MAP_SHARED, fd_, 0));
    DCHECK(file_ != reinterpret_cast<char*>(-1));

    Init();
}

ElfFile::~ElfFile()
{
    Destroy();
}

ElfFile::ElfFile(ElfFile&& other)
  : fd_(other.fd_),
    file_(other.file_),
    length_(other.length_),
    base_address_(other.base_address_)
{
    other.fd_ = -1;
    other.file_ = reinterpret_cast<char*>(-1);
    other.length_ = 0;
    other.base_address_ = 0;
}

ElfFile& ElfFile::operator=(ElfFile&& other)
{
    DCHECK(this != &other);
    Destroy();

    fd_ = other.fd_;
    file_ = other.file_;
    length_ = other.length_;
    base_address_ = other.base_address_;

    other.fd_ = -1;
    other.file_ = reinterpret_cast<char*>(-1);
    other.length_ = 0;
    other.base_address_ = 0;

    return *this;
}

void ElfFile::Destroy()
{
    if (file_ != reinterpret_cast<char*>(-1))
    {
        ::munmap(file_, length_);
    }

    if (fd_ != -1)
    {
        ::close(fd_);
    }
}

void ElfFile::Init()
{
  auto& elf_header = this->GetElfHeader();

  // Validate ELF magic numbers
  CHECK(elf_header.e_ident[EI_MAG0] == ELFMAG0 &&
        elf_header.e_ident[EI_MAG1] == ELFMAG1 &&
        elf_header.e_ident[EI_MAG2] == ELFMAG2 &&
        elf_header.e_ident[EI_MAG3] == ELFMAG3) << "invalid ELF magic";

// Validate ELF class (32/64 bits)
#define EXPECTED_CLASS P1(ELFCLASS, __ELF_NATIVE_CLASS)
#define P1(a, b) P2(a, b)
#define P2(a, b) a ## b
    CHECK(elf_header.e_ident[EI_CLASS] == EXPECTED_CLASS) << "invalid ELF class";
#undef P1
#undef P2
#undef EXPECTED_CLASS

// Validate ELF data encoding (LSB/MSB)
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
# define EXPECTED_ENCODING ELFDATA2LSB
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
# define EXPECTED_ENCODING ELFDATA2MSB
#else
# error Unsupported byte order
#endif
    CHECK(elf_header.e_ident[EI_DATA] == EXPECTED_ENCODING) << "invalid ELF encoding";
#undef EXPECTED_ENCODING

    // Validate ELF version (1)
    DCHECK(elf_header.e_ident[EI_VERSION] == EV_CURRENT &&
           elf_header.e_version == EV_CURRENT) << "invalid ELF version";

    // We only support executable and shared object files
    DCHECK(elf_header.e_type == ET_EXEC || elf_header.e_type == ET_DYN)
           << "invalid ELF file type";

    DCHECK(elf_header.e_phnum != 0) << "no program header!";
    DCHECK(elf_header.e_phentsize == sizeof(ElfW(Phdr)))
           << "invalid program header entry size";
    DCHECK(elf_header.e_shentsize == sizeof(ElfW(Shdr)))
           << "invalid section header entry size";

    const ElfW(Phdr)* program_header = &At<ElfW(Phdr)>(elf_header.e_phoff);
    bool found_base = false;
    for (size_t i = 0; i < elf_header.e_phnum; program_header++, i++)
    {
        // Program headers are sorted by load address, so the first PT_LOAD
        // header gives us the base address.
        if (program_header->p_type == PT_LOAD)
        {
            base_address_ = program_header->p_vaddr;
            found_base = true;
            break;
        }
    }

    DCHECK(found_base) << "could not find base address";
}

const ElfW(Shdr)* ElfFile::GetSectionByIndex(size_t idx) const
{
    DCHECK(idx < GetElfHeader().e_shnum) <<  "invalid section index";
    return &At<ElfW(Shdr)>(GetElfHeader().e_shoff + idx * sizeof(ElfW(Shdr)));
}

StringPiece ElfFile::GetSectionBody(const ElfW(Shdr)& section) const
{
    return StringPiece(file_ + section.sh_offset, section.sh_size);
}

void ElfFile::ValidateStringTable(const ElfW(Shdr)& string_table) const
{
    DCHECK(string_table.sh_type == SHT_STRTAB) << "invalid type for string table";

    const char* start = file_ + string_table.sh_offset;
    // First and last bytes must be 0
    DCHECK(string_table.sh_size == 0 ||
           (start[0] == '\0' && start[string_table.sh_size - 1] == '\0'))
        << "invalid string table";
}

const char* ElfFile::GetString(const ElfW(Shdr)& string_table, size_t offset) const
{
    ValidateStringTable(string_table);
    DCHECK(offset < string_table.sh_size) << "invalid offset in string table";

    return file_ + string_table.sh_offset + offset;
}

const char* ElfFile::GetSectionName(const ElfW(Shdr)& section) const
{
    if (GetElfHeader().e_shstrndx == SHN_UNDEF)
    {
        return NULL;  // no section name string table
    }

    const ElfW(Shdr)& section_names = *GetSectionByIndex(GetElfHeader().e_shstrndx);
    return GetString(section_names, section.sh_name);
}

const ElfW(Shdr)* ElfFile::GetSectionByName(const char* name) const
{
    if (GetElfHeader().e_shstrndx == SHN_UNDEF)
    {
        return NULL;  // no section name string table
    }

    // Find offset in the section name string table of the requested name
    const ElfW(Shdr)& section_names = *GetSectionByIndex(GetElfHeader().e_shstrndx);

    ValidateStringTable(section_names);
    const auto start = file_ + section_names.sh_offset;
    const auto end = start + section_names.sh_size;
    auto cur = start;

    while (cur != end && strcmp(name, cur))
    {
        cur += strlen(cur) + 1;
    }

    if (cur == end)
    {
        return NULL;
    }
    const auto found_name = cur;
    size_t offset = found_name - (file_ + section_names.sh_offset);

    // Find section with the appropriate sh_name offset
    auto ptr = &At<ElfW(Shdr)>(GetElfHeader().e_shoff);
    for (size_t i = 0; i < GetElfHeader().e_shnum; i++, ptr++)
    {
        if (ptr->sh_name == offset)
        {
            return ptr;
        }
    }
    return NULL;
}

ElfFile::Symbol ElfFile::GetDefinitionByAddress(uintptr_t address) const
{
    Symbol found_symbol;
    found_symbol.first = NULL;
    found_symbol.second = NULL;

    // Try the .dynsym section first if it exists, it's smaller.
    auto ptr = &At<ElfW(Shdr)>(GetElfHeader().e_shoff);
    for (size_t i = 0; i < GetElfHeader().e_shnum; i++, ptr++)
    {
        auto sh = *ptr;
        if (sh.sh_type != SHT_DYNSYM && sh.sh_type != SHT_SYMTAB)
        {
            continue;
        }

        auto sym = &At<ElfW(Sym)>(sh.sh_offset);
        const auto end = sym + (sh.sh_size / sh.sh_entsize);
        while (sym < end)
        {
            auto type = ELF32_ST_TYPE(sym->st_info);
            if ((type == STT_OBJECT || type == STT_FUNC) && sym->st_shndx != SHN_UNDEF)
            {
                if (address >= sym->st_value && address < sym->st_value + sym->st_size)
                {
                    found_symbol.first = ptr;
                    found_symbol.second = sym;
                    return found_symbol;
                }
            }
            ++sym;
        }
    }

    return found_symbol;
}

ElfFile::Symbol ElfFile::GetSymbolByName(const char* name) const
{
    Symbol found_symbol;
    found_symbol.first = NULL;
    found_symbol.second = NULL;

    // Try the .dynsym section first if it exists, it's smaller.
    auto ptr = &At<ElfW(Shdr)>(GetElfHeader().e_shoff);
    for (size_t i = 0; i < GetElfHeader().e_shnum; i++, ptr++)
    {
        if (ptr->sh_link == SHN_UNDEF)
        {
            continue;
        }

        if (ptr->sh_type != SHT_DYNSYM && ptr->sh_type != SHT_DYNSYM)
        {
            continue;
        }

        auto sym = &At<ElfW(Sym)>(ptr->sh_offset);
        const auto end = sym + (ptr->sh_size / ptr->sh_entsize);
        while (sym < end)
        {
            if (sym->st_shndx != SHN_UNDEF && sym->st_name != 0)
            {
                auto sym_name = GetString(*GetSectionByIndex(ptr->sh_link), sym->st_name);
                if (strcmp(sym_name, name) == 0)
                {
                    found_symbol.first = ptr;
                    found_symbol.second = sym;
                    return found_symbol;
                }
            }
            ++sym;
        }
    }

    return found_symbol;
}

const ElfW(Shdr)* ElfFile::GetSectionContainingAddress(ElfW(Addr) addr) const
{
    auto ptr = &At<ElfW(Shdr)>(GetElfHeader().e_shoff);
    for (size_t i = 0; i < GetElfHeader().e_shnum; i++, ptr++)
    {
        if ((addr >= ptr->sh_addr) && (addr < (ptr->sh_addr + ptr->sh_size)))
        {
            return ptr;
        }
    }

    return NULL;
}

const char* ElfFile::GetSymbolName(Symbol symbol) const
{
    if (!symbol.first || !symbol.second)
    {
        return NULL;
    }

    if (symbol.second->st_name == 0)
    {
        return NULL;  // symbol has no name
    }

    if (symbol.first->sh_link == SHN_UNDEF)
    {
        return NULL;  // symbol table has no strings
    }

    return GetString(*GetSectionByIndex(symbol.first->sh_link),
                     symbol.second->st_name);
}

}  // namespace claire
