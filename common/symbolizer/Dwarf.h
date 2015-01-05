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

// DWARF record parser

#ifndef _CLAIRE_COMMON_SYMBOLIZER_DWARF_H_
#define _CLAIRE_COMMON_SYMBOLIZER_DWARF_H_

#include <boost/variant.hpp>

#include <claire/common/symbolizer/Elf.h>

namespace claire {

///
/// DWARF record parser.
///
/// We only implement enough DWARF functionality to convert from PC address
/// to file and line number information.
///
/// This means (although they're not part of the public API of this class), we
/// can parse Debug Information Entries (DIEs), abbreviations, attributes (of
/// all forms), and we can interpret bytecode for the line number VM.
///
/// We can interpret DWARF records of version 2, 3, or 4, although we don't
/// actually support many of the version 4 features (such as VLIW, multiple
/// operations per instruction)
///
/// Note that the DWARF record parser does not allocate heap memory at all
/// during normal operation (it might in the error case, as throwing exceptions
/// uses the heap).  This is on purpose: you can use the parser from
/// memory-constrained situations (such as an exception handler for
/// std::out_of_memory)  If it weren't for this requirement, some things would
/// be much simpler: the Path class would be unnecessary and would be replaced
/// with a std::string; the list of file names in the line number VM would be
/// kept as a vector of strings instead of re-executing the program to look for
/// DW_LNE_define_file instructions, etc.
///
class Dwarf
{
    // Note that Dwarf uses (and returns) StringPiece a lot.
    // The StringPieces point within sections in the ELF file, and so will
    // be live for as long as the passed-in ElfFile is live.
public:
    /// Create a DWARF parser around an ELF file.
    explicit Dwarf(const ElfFile* elf);

    ///
    /// Represent a file path a s collection of three parts (base directory,
    /// subdirectory, and file).
    ///
    class Path
    {
    public:
        Path() {}

        Path(StringPiece base_dir, StringPiece sub_dir,
             StringPiece file);

        StringPiece base_dir() const { return base_dir_; };
        StringPiece sub_dir() const { return sub_dir_; }
        StringPiece file() const { return file_; }

        size_t size() const;

        ///
        /// Copy the Path to a buffer of size bufSize.
        ///
        /// toBuffer behaves like snprintf: It will always null-terminate the
        /// buffer (so it will copy at most bufSize-1 bytes), and it will return
        /// the number of bytes that would have been written if there had been
        /// enough room, so, if toBuffer returns a value >= bufSize, the output
        /// was truncated.
        ///
        size_t ToBuffer(char* buf, size_t buf_size) const;

        void ToString(std::string& dest) const;
        std::string ToString() const
        {
            std::string s;
            ToString(s);
            return s;
        }

        // TODO(tudorb): Implement operator==, operator!=; not as easy as it
        // seems as the same path can be represented in multiple ways
    private:
        StringPiece base_dir_;
        StringPiece sub_dir_;
        StringPiece file_;
    };

    struct LocationInfo
    {
        LocationInfo()
            : has_main_file(false),
              has_file_and_line(false),
              line(0) { }

        bool has_main_file;
        Path main_file;

        bool has_file_and_line;
        Path file;
        uint64_t line;
    };

    /// Find the file and line number information corresponding to address
    bool FindAddress(uintptr_t address, LocationInfo* info) const;

private:
    void Init();

    const ElfFile* elf_;

    // DWARF section made up of chunks, each prefixed with a length header.
    // The length indicates whether the chunk is DWARF-32 or DWARF-64, which
    // guides interpretation of "section offset" records.
    // (yes, DWARF-32 and DWARF-64 sections may coexist in the same file)
    class Section
    {
    public:
        Section() : is_64_bits_(false) {}

        explicit Section(StringPiece d);

        // Return next chunk, if any; the 4- or 12-byte length was already
        // parsed and isn't part of the chunk.
        bool Next(StringPiece& chunk);

        // Is the current chunk 64 bit?
        bool is_64_bits() const { return is_64_bits_; }

    private:
        // Yes, 32- and 64- bit sections may coexist.  Yikes!
        bool is_64_bits_;
        StringPiece data_;
    };

    // Abbreviation for a Debugging Information Entry.
    struct DIEAbbreviation
    {
        uint64_t code;
        uint64_t tag;
        bool has_children;

        struct Attribute
        {
            uint64_t name;
            uint64_t form;
        };

        StringPiece attributes;
    };

    // Interpreter for the line number bytecode VM
    class LineNumberVM
    {
    public:
        LineNumberVM(StringPiece data,
                     StringPiece compilation_directory);

        bool FindAddress(uintptr_t address, Path& file, uint64_t& line);

    private:
        void Init();
        void Reset();

        // Execute until we commit one new row to the line number matrix
        bool Next(StringPiece& program);

        enum StepResult
        {
            CONTINUE,  // Continue feeding opcodes
            COMMIT,    // Commit new <address, file, line> tuple
            END,       // End of sequence
        };

        // Execute one opcode
        StepResult Step(StringPiece& program);

        struct FileName
        {
            StringPiece relative_name;
            // 0 = current compilation directory
            // otherwise, 1-based index in the list of include directories
            uint64_t directory_index;
        };
        // Read one FileName object, advance sp
        static bool ReadFileName(StringPiece& sp, FileName& fn);

        // Get file name at given index; may be in the initial table
        // (fileNames_) or defined using DW_LNE_define_file (and we reexecute
        // enough of the program to find it, if so)
        FileName GetFileName(uint64_t index) const;

        // Get include directory at given index
        StringPiece GetIncludeDirectory(uint64_t index) const;

        // Execute opcodes until finding a DW_LNE_define_file and return true;
        // return file at the end.
        bool NextDefineFile(StringPiece& program, FileName& fn) const;

        // Initialization
        bool is_64_bits_;
        StringPiece data_;
        StringPiece compilation_directory_;

        // Header
        uint16_t version_;
        uint8_t min_length_;
        bool default_is_stmt_;
        int8_t line_base_;
        uint8_t line_range_;
        uint8_t opcode_base_;
        const uint8_t* standard_opcode_lengths_;

        StringPiece include_directories_;
        size_t include_directory_count_;

        StringPiece file_names_;
        size_t file_name_count_;

        // State machine registers
        uint64_t address_;
        uint64_t file_;
        uint64_t line_;
        uint64_t column_;
        bool is_stmt_;
        bool basic_block_;
        bool end_sequence_;
        bool prologue_end_;
        bool epilogue_begin_;
        uint64_t isa_;
        uint64_t discriminator_;
    };

    // Read an abbreviation from a StringPiece, return true if at end; advance sp
    static bool ReadAbbreviation(StringPiece& sp, DIEAbbreviation& abbr);

    // Get abbreviation corresponding to a code, in the chunk starting at
    // offset in the .debug_abbrev section
    DIEAbbreviation GetAbbreviation(uint64_t code, uint64_t offset) const;

    // Read one attribute <name, form> pair, advance sp; returns <0, 0> at end.
    static DIEAbbreviation::Attribute ReadAttribute(StringPiece& s);

    // Read one attribute value, advance sp
    typedef boost::variant<uint64_t, StringPiece> AttributeValue;
    AttributeValue ReadAttributeValue(StringPiece& s,
                                      uint64_t form,
                                      bool is_64_bits) const;

    // Get an ELF section by name, return true if found
    bool GetSection(const char* name, StringPiece* section) const;

    // Get a string from the .debug_str section
    StringPiece GetStringFromStringSection(uint64_t offset) const;

    StringPiece info_;       // .debug_info
    StringPiece abbrev_;     // .debug_abbrev
    StringPiece aranges_;    // .debug_aranges
    StringPiece line_;       // .debug_line
    StringPiece strings_;    // .debug_str
};

} // namespace claire

#endif // _CLAIRE_COMMON_SYMBOLIZER_DWARF_H_
