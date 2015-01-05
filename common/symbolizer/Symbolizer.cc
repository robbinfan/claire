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

#include <claire/common/symbolizer/Symbolizer.h>

#include <vector>

#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>

#include <claire/common/symbolizer/Elf.h>
#include <claire/common/symbolizer/Dwarf.h>
#include <claire/common/strings/StringPiece.h>
#include <claire/common/logging/Logging.h>
#include <claire/common/files/FileUtil.h>
#include <claire/common/base/StackTrace.h>

namespace claire {
namespace {

StringPiece ToStringPiece(const boost::csub_match& m)
{
    return StringPiece(m.first, m.second);
}

uint64_t FromHex(StringPiece s)
{
    // Make a copy; we need a null-terminated string for strtoull
    std::string str(s.data(), s.size());
    const char* p = str.c_str();
    char* end;
    uint64_t val = strtoull(p, &end, 16);
    CHECK(*p != '\0' && *end == '\0');
    return val;
}

std::vector<std::string> GetMapsLine()
{
    std::string data;
    FileUtil::ReadFileToString("/proc/self/maps", &data);

    std::vector<std::string> result;
    boost::algorithm::split(result, data, boost::is_any_of("\n"));

    return result;
}

struct MappedFile
{
    uintptr_t begin;
    uintptr_t end;
    std::string name;
};

}  // namespace

bool Symbolizer::Symbolize(uintptr_t address,
                           StringPiece* symbol_name,
                           Dwarf::LocationInfo* location)
{
    symbol_name->clear();
    *location = Dwarf::LocationInfo();

    // Entry in /proc/self/maps
    static const boost::regex map_line_regex(
        "([[:xdigit:]]+)-([[:xdigit:]]+)"  // from-to
        "\\s+"
        "[\\w-]+"  // permissions
        "\\s+"
        "([[:xdigit:]]+)"  // offset
        "\\s+"
        "[[:xdigit:]]+:[[:xdigit:]]+"  // device, minor:major
        "\\s+"
        "\\d+"  // inode
        "\\s*"
        "(.*)");  // file name

    boost::cmatch match;

    MappedFile found_file;
    bool error = true;
    auto lines = GetMapsLine();
    for (auto it = lines.begin(); it != lines.end(); ++it)
    {
        StringPiece line(*it);
        if (line.empty())
        {
            continue;
        }

        CHECK(boost::regex_match(line.begin(), line.end(), match, map_line_regex));
        uint64_t begin = FromHex(ToStringPiece(match[1]));
        uint64_t end = FromHex(ToStringPiece(match[2]));
        uint64_t file_offset = FromHex(ToStringPiece(match[3]));
        if (file_offset != 0)
        {
            error = true;  // main mapping starts at 0
            continue;
        }

        if (begin <= address && address < end)
        {
            found_file.begin = begin;
            found_file.end = end;
            found_file.name.assign(match[4].first, match[4].second);
            error = false;
            break;
        }
        continue;
    }

    if (error)
    {
        return false;
    }

    auto elf_file = GetFile(found_file.name);
    if (!elf_file)
    {
        return false;
    }

    // Undo relocation
    uintptr_t orig_address = address - found_file.begin + elf_file->GetBaseAddress();

    auto sym = elf_file->GetDefinitionByAddress(orig_address);
    if (!sym.first)
    {
        return false;
    }

    auto name = elf_file->GetSymbolName(sym);
    if (name)
    {
        *symbol_name = name;
    }

    Dwarf(elf_file).FindAddress(orig_address, location);
    return true;
}

ElfFile* Symbolizer::GetFile(const std::string& name)
{
    if (name[0] == '[' || name[name.size()-1] == ')')
    {
        return NULL;
    }

    auto pos = elf_files_.find(name);
    if (pos == elf_files_.end())
    {
        pos = elf_files_.insert(name, new ElfFile(name.c_str())).first;
    }

    return pos->second;
}

void Symbolizer::Write(std::string* out,
                       uintptr_t address,
                       const StringPiece& symbol_name,
                       const Dwarf::LocationInfo& location)
{
    char buf[20];
    sprintf(buf, "%#18jx", address);
    out->append("    @ ");
    out->append(buf);

    if (!symbol_name.empty())
    {
        out->append(" ");
        out->append(demangle(symbol_name.data())); // FIXME

        if (location.has_file_and_line)
        {
            out->append("   ");
            out->append(location.file.ToString());
            out->append(":");
            snprintf(buf, sizeof buf, "%llu", static_cast<unsigned long long>(location.line));
            out->append(buf);
        }

        if (location.has_main_file)
        {
            if (!location.has_file_and_line || location.file.ToString() != location.main_file.ToString())
            {
                out->append("\n                         (compiling ");
                out->append(location.main_file.ToString());
                out->append(")");
            }
        }
    }
    else
    {
        out->append(" (unknown)");
    }
    out->append("\n");
}

}  // namespace claire
