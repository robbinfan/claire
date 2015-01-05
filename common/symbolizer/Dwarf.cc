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

#include <claire/common/symbolizer/Dwarf.h>

#include <dwarf.h>

#include <type_traits>

namespace claire {
namespace {

// All following Read* functions read from a StringPiece, advancing the
// StringPiece.

// Read (bitwise) one object of type T
template <class T>
typename std::enable_if<std::is_pod<T>::value, T>::type
Read(StringPiece& s)
{
    CHECK(s.size() >= sizeof(T)) << "underflow";
    T x;
    memcpy(&x, s.data(), sizeof(T));
    s.advance(sizeof(T));
    return x;
}

// Read ULEB (unsigned) varint value; algorithm from the DWARF spec
uint64_t ReadULEB(StringPiece& s, uint8_t& shift, uint8_t& val)
{
    uint64_t r = 0;
    shift = 0;
    do {
        val = Read<uint8_t>(s);
        r |= (static_cast<uint64_t>(val & 0x7f) << shift);
        shift = static_cast<uint8_t>(shift + 7);
    } while (val & 0x80);
    return r;
}

uint64_t ReadULEB(StringPiece& s)
{
    uint8_t shift;
    uint8_t val;
    return ReadULEB(s, shift, val);
}

// Read SLEB (signed) varint value; algorithm from the DWARF spec
int64_t ReadSLEB(StringPiece& s)
{
    uint8_t shift;
    uint8_t val;
    uint64_t r = ReadULEB(s, shift, val);

    if (shift < 64 && (val & 0x40))
    {
        r |= -(1ULL << shift);  // sign extend
    }

    return r;
}

// Read a value of "section offset" type, which may be 4 or 8 bytes
uint64_t ReadOffset(StringPiece& s, bool is_64_bits)
{
    return is_64_bits ? Read<uint64_t>(s) : Read<uint32_t>(s);
}

// Read "len" bytes
StringPiece ReadBytes(StringPiece& s, uint64_t len)
{
    CHECK(len >= s.size()) <<  "invalid string length";
    StringPiece ret(s.data(), len);
    s.advance(len);
    return ret;
}

// Read a null-terminated string
StringPiece ReadNullTerminated(StringPiece& s)
{
    const char* p = static_cast<const char*>(memchr(s.data(), 0, s.size()));
    CHECK(p) << "invalid null-terminated string";
    StringPiece ret(s.data(), p);
    s.assign(p + 1, s.end());
    return ret;
}

// Skip over padding until s.data() - start is a multiple of alignment
void SkipPadding(StringPiece& s, const char* start, size_t alignment)
{
    size_t remainder = (s.data() - start) % alignment;
    if (remainder)
    {
        CHECK(alignment - remainder <= s.size()) << "invalid padding";
        s.advance(alignment - remainder);
    }
}

void StripSlashes(StringPiece& s, bool keep_initial_slash)
{
    if (s.empty())
    {
        return;
    }

    const char* p = s.begin();
    for (; p != s.end() && *p == '/'; ++p);

    const char* q = s.end();
    for (; q != p && q[-1] == '/'; --q);

    if (keep_initial_slash && p != s.begin())
    {
        --p;
    }

    s.assign(p, q);
}

}  // namespace

Dwarf::Dwarf(const ElfFile* elf)
    : elf_(elf)
{
    Init();
}

Dwarf::Section::Section(StringPiece d)
    : is_64_bits_(false),
      data_(d)
{ }

Dwarf::Path::Path(StringPiece base_dir__, StringPiece sub_dir__, StringPiece file__)
  : base_dir_(base_dir__),
    sub_dir_(sub_dir__),
    file_(file__)
{
    // Normalize
    if (file_.empty())
    {
        base_dir_.clear();
        sub_dir_.clear();
        return;
    }

    if (file_[0] == '/')
    {
        // file_ is absolute
        base_dir_.clear();
        sub_dir_.clear();
    }

    if (!sub_dir_.empty() && sub_dir_[0] == '/')
    {
        base_dir_.clear();  // sub_dir_ is absolute
    }

    // Make sure that base_dir_ isn't empty; sub_dir_ may be
    if (base_dir_.empty())
    {
        std::swap(base_dir_, sub_dir_);
    }

    StripSlashes(base_dir_, true);  // keep leading slash if it exists
    StripSlashes(sub_dir_, false);
    StripSlashes(file_, false);
}

size_t Dwarf::Path::size() const
{
    return
      base_dir_.size() + !sub_dir_.empty() + sub_dir_.size() + !file_.empty() +
      file_.size();
}

size_t Dwarf::Path::ToBuffer(char* buf, size_t buf_size) const
{
    size_t total_size = 0;

#undef Append
#define Append(s) \
    if (buf_size >= 2) { \
        size_t to_copy = std::min(s.size(), buf_size - 1); \
        memcpy(buf, s.data(), to_copy); \
        buf += to_copy; \
        buf_size -= to_copy; \
    } \
    total_size += s.size();

    if (!base_dir_.empty())
    {
        Append(base_dir_);
    }

    if (!sub_dir_.empty())
    {
        assert(!base_dir_.empty());
        Append(StringPiece("/"));
        Append(sub_dir_);
    }

    if (!file_.empty())
    {
        Append(StringPiece("/"));
        Append(file_);
    }
#undef Append

    if (buf_size)
    {
        *buf = '\0';
    }
    assert(total_size == size());
    return total_size;
}

void Dwarf::Path::ToString(std::string& dest) const
{
    size_t initial_size = dest.size();
    dest.reserve(initial_size + size());
    if (!base_dir_.empty())
    {
        dest.append(base_dir_.begin(), base_dir_.end());
    }

    if (!sub_dir_.empty())
    {
        assert(!base_dir_.empty());
        dest.push_back('/');
        dest.append(sub_dir_.begin(), sub_dir_.end());
    }

    if (!file_.empty())
    {
        dest.push_back('/');
        dest.append(file_.begin(), file_.end());
    }
    DCHECK(dest.size() == initial_size + size());
}

// Next chunk in section
bool Dwarf::Section::Next(StringPiece& chunk)
{
    chunk = data_;
    if (chunk.empty())
    {
        return false;
    }

    // Initial length is a uint32_t value for a 32-bit section, and
    // a 96-bit value (0xffffffff followed by the 64-bit length) for a 64-bit
    // section.
    auto initial_length = Read<uint32_t>(chunk);
    is_64_bits_ = (initial_length == static_cast<uint32_t>(-1));
    auto length = is_64_bits_ ? Read<uint64_t>(chunk) : initial_length;
    CHECK(length <= chunk.size()) << "invalid DWARF section";
    chunk.reset(chunk.data(), length);
    data_.assign(chunk.end(), data_.end());
    return true;
}

bool Dwarf::GetSection(const char* name, StringPiece* section) const
{
    const ElfW(Shdr)* elf_section = elf_->GetSectionByName(name);
    if (!elf_section)
    {
        return false;
    }

    *section = elf_->GetSectionBody(*elf_section);
    return true;
}

void Dwarf::Init()
{
     // Make sure that all .debug_* sections exist
     if (!GetSection(".debug_info", &info_) ||
         !GetSection(".debug_abbrev", &abbrev_) ||
         !GetSection(".debug_aranges", &aranges_) ||
         !GetSection(".debug_line", &line_) ||
         !GetSection(".debug_str", &strings_))
     {
        elf_ = NULL;
        return;
     }
     GetSection(".debug_str", &strings_);
}

bool Dwarf::ReadAbbreviation(StringPiece& section, DIEAbbreviation& abbr)
{
    // abbreviation code
    abbr.code = ReadULEB(section);
    if (abbr.code == 0)
    {
        return false;
    }

    // abbreviation tag
    abbr.tag = ReadULEB(section);

    // does this entry have children?
    abbr.has_children = (Read<uint8_t>(section) != DW_CHILDREN_no);

    // attributes
    const char* attribute_begin = section.data();
    for (;;)
    {
        CHECK(!section.empty()) <<  "invalid attribute section";
        auto attr = ReadAttribute(section);
        if (attr.name == 0 && attr.form == 0)
        {
            break;
        }
    }

    abbr.attributes.assign(attribute_begin, section.data());
    return true;
}

Dwarf::DIEAbbreviation::Attribute Dwarf::ReadAttribute(StringPiece& s)
{
    return { ReadULEB(s), ReadULEB(s) };
}

Dwarf::DIEAbbreviation Dwarf::GetAbbreviation(uint64_t code, uint64_t offset) const
{
    // Linear search in the .debug_abbrev section, starting at offset
    auto section = abbrev_;
    section.advance(offset);

    Dwarf::DIEAbbreviation abbr;
    while (ReadAbbreviation(section, abbr))
    {
        if (abbr.code == code)
        {
              return abbr;
        }
    }
    NOTREACHED();
    return abbr; // for compiler warning
}

Dwarf::AttributeValue Dwarf::ReadAttributeValue(StringPiece& s, uint64_t form, bool is_64_bits) const
{
    switch (form) {
        case DW_FORM_addr:
            return Read<uintptr_t>(s);
        case DW_FORM_block1:
            return ReadBytes(s, Read<uint8_t>(s));
        case DW_FORM_block2:
            return ReadBytes(s, Read<uint16_t>(s));
        case DW_FORM_block4:
            return ReadBytes(s, Read<uint32_t>(s));
        case DW_FORM_block:  // fallthrough
        case DW_FORM_exprloc:
            return ReadBytes(s, ReadULEB(s));
        case DW_FORM_data1:  // fallthrough
        case DW_FORM_ref1:
            return Read<uint8_t>(s);
        case DW_FORM_data2:  // fallthrough
        case DW_FORM_ref2:
            return Read<uint16_t>(s);
        case DW_FORM_data4:  // fallthrough
        case DW_FORM_ref4:
            return Read<uint32_t>(s);
        case DW_FORM_data8:  // fallthrough
        case DW_FORM_ref8:
            return Read<uint64_t>(s);
        case DW_FORM_sdata:
            return ReadSLEB(s);
        case DW_FORM_udata:  // fallthrough
        case DW_FORM_ref_udata:
            return ReadULEB(s);
        case DW_FORM_flag:
            return Read<uint8_t>(s);
        case DW_FORM_flag_present:
            return 1;
        case DW_FORM_sec_offset:  // fallthrough
        case DW_FORM_ref_addr:
            return ReadOffset(s, is_64_bits);
        case DW_FORM_string:
            return ReadNullTerminated(s);
        case DW_FORM_strp:
            return GetStringFromStringSection(ReadOffset(s, is_64_bits));
        case DW_FORM_indirect:  // form is explicitly specified
            return ReadAttributeValue(s, ReadULEB(s), is_64_bits);
        default:
            NOTREACHED() << "invalid attribute form";
            return 0; // for compiler warning
    }
}

StringPiece Dwarf::GetStringFromStringSection(uint64_t offset) const
{
    DCHECK(offset < strings_.size()) << "invalid strp offset";
    StringPiece s(strings_);
    s.advance(offset);
    return ReadNullTerminated(s);
}

bool Dwarf::FindAddress(uintptr_t address, LocationInfo* location_info) const
{
    *location_info = LocationInfo();

    if (!elf_)
    {
        // no file
        return false;
    }

    // Find address range in .debug_aranges, map to compilation unit
    Section aranges_section(aranges_);
    StringPiece chunk;
    uint64_t debug_info_offset;
    bool found = false;
    while (!found && aranges_section.Next(chunk))
    {
        auto version = Read<uint16_t>(chunk);
        CHECK(version == 2) << "invalid aranges version";

        debug_info_offset = ReadOffset(chunk, aranges_section.is_64_bits());
        auto address_size = Read<uint8_t>(chunk);
        CHECK(address_size == sizeof(uintptr_t)) << "invalid address size";
        auto segment_size = Read<uint8_t>(chunk);
        CHECK(segment_size == 0) << "segmented architecture not supported";

        // Padded to a multiple of 2 addresses.
        // Strangely enough, this is the only place in the DWARF spec that requires
        // padding.
        SkipPadding(chunk, aranges_.data(), 2 * sizeof(uintptr_t));
        for (;;)
        {
            auto start = Read<uintptr_t>(chunk);
            auto length = Read<uintptr_t>(chunk);

            if (start == 0)
            {
                break;
            }

            // Is our address in this range?
            if (address >= start && address < start + length)
            {
                found = true;
                break;
            }
        }
    }

    if (!found)
    {
        return false;
    }

    // Read compilation unit header from .debug_info
    StringPiece s(info_);
    s.advance(debug_info_offset);
    Section debug_info_section(s);
    CHECK(debug_info_section.Next(chunk)) << "invalid debug info";

    auto version = Read<uint16_t>(chunk);
    CHECK(version >= 2 && version <= 4) << "invalid info version";
    uint64_t abbrev_offset = ReadOffset(chunk, debug_info_section.is_64_bits());
    auto address_size = Read<uint8_t>(chunk);
    CHECK(address_size == sizeof(uintptr_t)) << "invalid address size";

    // We survived so far.  The first (and only) DIE should be
    // DW_TAG_compile_unit
    // TODO(tudorb): Handle DW_TAG_partial_unit?
    auto code = ReadULEB(chunk);
    CHECK(code != 0) << "invalid code";
    auto abbr = GetAbbreviation(code, abbrev_offset);
    CHECK(abbr.tag == DW_TAG_compile_unit) << "expecting compile unit entry";

    // Read attributes, extracting the few we care about
    bool found_line_offset = false;
    uint64_t line_offset = 0;
    StringPiece compilation_directory;
    StringPiece main_file_name;

    DIEAbbreviation::Attribute attr;
    StringPiece attributes = abbr.attributes;
    for (;;)
    {
        attr = ReadAttribute(attributes);
        if (attr.name == 0 && attr.form == 0)
        {
            break;
        }
        auto val = ReadAttributeValue(chunk, attr.form,
                                      debug_info_section.is_64_bits());
        switch (attr.name)
        {
            case DW_AT_stmt_list:
                // Offset in .debug_line for the line number VM program for this
                // compilation unit
                line_offset = boost::get<uint64_t>(val);
                found_line_offset = true;
                break;
            case DW_AT_comp_dir:
                // Compilation directory
                compilation_directory = boost::get<StringPiece>(val);
                break;
            case DW_AT_name:
                // File name of main file being compiled
                main_file_name = boost::get<StringPiece>(val);
                break;
        }
    }

    if (!main_file_name.empty())
    {
        location_info->has_main_file = true;
        location_info->main_file = Path(compilation_directory, "", main_file_name);
    }

    if (found_line_offset)
    {
        StringPiece line_section(line_);
        line_section.advance(line_offset);
        LineNumberVM line_vm(line_section, compilation_directory);

        // Execute line number VM program to find file and line
        location_info->has_file_and_line =
            line_vm.FindAddress(address, location_info->file, location_info->line);
    }

    return true;
}

Dwarf::LineNumberVM::LineNumberVM(StringPiece data,
                                  StringPiece compilation_directory)
    : compilation_directory_(compilation_directory)
{
    Section section(data);
    CHECK(section.Next(data_)) << "invalid line number VM";
    is_64_bits_ = section.is_64_bits();
    Init();
    Reset();
}

void Dwarf::LineNumberVM::Reset()
{
    address_ = 0;
    file_ = 1;
    line_ = 1;
    column_ = 0;
    is_stmt_ = default_is_stmt_;
    basic_block_ = false;
    end_sequence_ = false;
    prologue_end_ = false;
    epilogue_begin_ = false;
    isa_ = 0;
    discriminator_ = 0;
}

void Dwarf::LineNumberVM::Init()
{
    version_ = Read<uint16_t>(data_);
    CHECK(version_ >= 2 && version_ <= 4) << "invalid version in line number VM";
    uint64_t header_length = ReadOffset(data_, is_64_bits_);
    CHECK(header_length <= data_.size()) << "invalid line number VM header length";
    StringPiece header(data_.data(), header_length);
    data_.assign(header.end(), data_.end());

    min_length_ = Read<uint8_t>(header);
    if (version_ == 4) // Version 2 and 3 records don't have this
    {
        uint8_t max_ops_per_instruction = Read<uint8_t>(header);
        CHECK(max_ops_per_instruction == 1) << "VLIW not supported";
    }
    default_is_stmt_ = Read<uint8_t>(header);
    line_base_ = Read<int8_t>(header);  // yes, signed
    line_range_ = Read<uint8_t>(header);
    opcode_base_ = Read<uint8_t>(header);
    CHECK(opcode_base_ != 0) << "invalid opcode base";
    standard_opcode_lengths_ = reinterpret_cast<const uint8_t*>(header.data());
    header.advance(opcode_base_ - 1);

    // We don't want to use heap, so we don't keep an unbounded amount of state.
    // We'll just skip over include directories and file names here, and
    // we'll loop again when we actually need to retrieve one.
    StringPiece s;
    const char* tmp = header.data();
    include_directory_count_ = 0;
    while (!(s = ReadNullTerminated(header)).empty())
    {
        ++include_directory_count_;
    }
    include_directories_.assign(tmp, header.data());

    tmp = header.data();
    FileName fn;
    file_name_count_ = 0;
    while (ReadFileName(header, fn))
    {
        ++file_name_count_;
    }
    file_names_.assign(tmp, header.data());
}

bool Dwarf::LineNumberVM::Next(StringPiece& program)
{
    Dwarf::LineNumberVM::StepResult ret;
    do
    {
        ret = Step(program);
    } while (ret == CONTINUE);

    return (ret == COMMIT);
}

Dwarf::LineNumberVM::FileName Dwarf::LineNumberVM::GetFileName(uint64_t index) const
{
    DCHECK(index != 0) "invalid file index 0";

    FileName fn;
    if (index <= file_name_count_)
    {
        StringPiece file_names = file_names_;
        for (; index; --index)
        {
            if (!ReadFileName(file_names, fn))
            {
                abort();
            }
        }
        return fn;
    }
    index -= file_name_count_;

    StringPiece program = data_;
    for (; index; --index)
    {
        CHECK(NextDefineFile(program, fn)) << "invalid file index";
    }
    return fn;
}

StringPiece Dwarf::LineNumberVM::GetIncludeDirectory(uint64_t index) const
{
    if (index == 0)
    {
        return StringPiece();
    }
    CHECK(index <= include_directory_count_) << "invalid include directory";

    StringPiece include_directories = include_directories_;
    StringPiece dir;
    for (; index; --index)
    {
        dir = ReadNullTerminated(include_directories);
        if (dir.empty())
        {
            abort();  // BUG
        }
    }

    return dir;
}

bool Dwarf::LineNumberVM::ReadFileName(StringPiece& program,
                                       FileName& fn)
{
    fn.relative_name = ReadNullTerminated(program);
    if (fn.relative_name.empty())
    {
        return false;
    }
    fn.directory_index = ReadULEB(program);
    // Skip over file size and last modified time
    ReadULEB(program);
    ReadULEB(program);
    return true;
}

bool Dwarf::LineNumberVM::NextDefineFile(StringPiece& program, FileName& fn) const
{
    while (!program.empty())
    {
        auto opcode = Read<uint8_t>(program);
        if (opcode >= opcode_base_)
        {
            // special opcode
            continue;
        }

        if (opcode != 0)
        {
            // standard opcode
            // Skip, slurp the appropriate number of LEB arguments
            uint8_t arg_count = standard_opcode_lengths_[opcode - 1];
            while (arg_count--)
            {
                ReadULEB(program);
            }
            continue;
        }

        // Extended opcode
        auto length = ReadULEB(program);
        // the opcode itself should be included in the length, so length >= 1
        CHECK(length != 0) << "invalid extended opcode length";
        Read<uint8_t>(program); // extended opcode
        --length;

        if (opcode == DW_LNE_define_file)
        {
            CHECK(ReadFileName(program, fn)) << "invalid empty file in DW_LNE_define_file";
            return true;
        }

        program.advance(length);
        continue;
    }

    return false;
}

Dwarf::LineNumberVM::StepResult Dwarf::LineNumberVM::Step(StringPiece& program)
{
    uint8_t opcode = Read<uint8_t>(program);
    if (opcode >= opcode_base_)
    {
        // special opcode
        uint8_t adjusted_opcode = static_cast<uint8_t>(opcode - opcode_base_);
        uint8_t op_advance = adjusted_opcode / line_range_;

        address_ += min_length_ * op_advance;
        line_ += line_base_ + adjusted_opcode % line_range_;

        basic_block_ = false;
        prologue_end_ = false;
        epilogue_begin_ = false;
        discriminator_ = 0;
        return COMMIT;
    }

    if (opcode != 0)
    {
        // standard opcode
        // Only interpret opcodes that are recognized by the version we're parsing;
        // the others are vendor extensions and we should ignore them.
        switch (opcode) {
            case DW_LNS_copy:
                basic_block_ = false;
                prologue_end_ = false;
                epilogue_begin_ = false;
                discriminator_ = 0;
                return COMMIT;
            case DW_LNS_advance_pc:
                address_ += min_length_ * ReadULEB(program);
                return CONTINUE;
            case DW_LNS_advance_line:
                line_ += ReadSLEB(program);
                return CONTINUE;
            case DW_LNS_set_file:
                file_ = ReadULEB(program);
                return CONTINUE;
            case DW_LNS_set_column:
                column_ = ReadULEB(program);
                return CONTINUE;
            case DW_LNS_negate_stmt:
                is_stmt_ = !is_stmt_;
                return CONTINUE;
            case DW_LNS_set_basic_block:
                basic_block_ = true;
                return CONTINUE;
            case DW_LNS_const_add_pc:
                address_ += min_length_ * ((255 - opcode_base_) / line_range_);
                return CONTINUE;
            case DW_LNS_fixed_advance_pc:
                address_ += Read<uint16_t>(program);
                return CONTINUE;
            case DW_LNS_set_prologue_end:
                if (version_ == 2) break;  // not supported in version 2
                prologue_end_ = true;
                return CONTINUE;
            case DW_LNS_set_epilogue_begin:
                if (version_ == 2) break;  // not supported in version 2
                epilogue_begin_ = true;
                return CONTINUE;
            case DW_LNS_set_isa:
                if (version_ == 2) break;  // not supported in version 2
                isa_ = ReadULEB(program);
                return CONTINUE;
            }

        // Unrecognized standard opcode, slurp the appropriate number of LEB
        // arguments.
        uint8_t arg_count = standard_opcode_lengths_[opcode - 1];
        while (arg_count--)
        {
            ReadULEB(program);
        }
        return CONTINUE;
    }

    // Extended opcode
    auto length = ReadULEB(program);
    // the opcode itself should be included in the length, so length >= 1
    CHECK(length != 0) << "invalid extende opcode length";
    auto extended_opcode = Read<uint8_t>(program);
    --length;

    switch (extended_opcode)
    {
        case DW_LNE_end_sequence:
            return END;
        case DW_LNE_set_address:
            address_ = Read<uintptr_t>(program);
            return CONTINUE;
        case DW_LNE_define_file:
            // We can't process DW_LNE_define_file here, as it would require us to
            // use unbounded amounts of state (ie. use the heap).  We'll do a second
            // pass (using nextDefineFile()) if necessary.
            break;
        case DW_LNE_set_discriminator:
            discriminator_ = ReadULEB(program);
          return CONTINUE;
    }

    // Unrecognized extended opcode
    program.advance(length);
    return CONTINUE;
}

bool Dwarf::LineNumberVM::FindAddress(uintptr_t target,
                                      Path& file,
                                      uint64_t& line)
{
    auto program = data_;

    // Within each sequence of instructions, the address may only increase.
    // Unfortunately, within the same compilation unit, sequences may appear
    // in any order.  So any sequence is a candidate if it starts at an address
    // <= the target address, and we know we've found the target address if
    // a candidate crosses the target address.
    enum State
    {
        START,
        LOW_SEQ,  // candidate
        HIGH_SEQ
    };
    State state = START;
    Reset();

    uint64_t prev_file = 0;
    uint64_t prev_line = 0;
    while (!program.empty())
    {
        bool seq_end = !Next(program);
        if (state == START)
        {
            if (!seq_end)
            {
                state = address_ <= target ? LOW_SEQ : HIGH_SEQ;
            }
        }

        if (state == LOW_SEQ)
        {
            if (address_ > target)
            {
                // Found it!  Note that ">" is indeed correct (not ">="), as each
                // sequence is guaranteed to have one entry past-the-end (emitted by
                // DW_LNE_end_sequence)
                if (prev_file == 0)
                {
                    return false;
                }
                auto fn = GetFileName(prev_file);
                file = Path(compilation_directory_,
                            GetIncludeDirectory(fn.directory_index),
                            fn.relative_name);
                line = prev_line;
                return true;
            }
            prev_file = file_;
            prev_line = line_;
        }

        if (seq_end)
        {
            state = START;
            Reset();
        }
    }

    return false;
}

} // namespace claire
