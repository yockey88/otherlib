/**
 * \file vm/command_files/ocmd_headers.hpp
 **/
#ifndef OTHERLIB_VM_COMMAND_FILES_OCMD_HEADERS_HPP
#define OTHERLIB_VM_COMMAND_FILES_OCMD_HEADERS_HPP

#include <cstdint>

#include "vm/ocmd_file_format_version.hpp"

namespace other {

#pragma pack(push, 1)
  struct program_header {
    // could be pure data section file (a 'library')
    uint8_t has_code_flag = 0;
    uint16_t code_section_offset = 0;
    uint16_t data_section_offset = 0;
    uint16_t data_table_offset = 0;
    uint16_t num_instructions = 0;
    uint16_t entry_point_address = 0;
  };
  static_assert(sizeof(program_header) == 11, "Program header size must be 11 bytes");

  struct ocmd_file_header {
    char file_signature[4] = { 'O', 'C', 'M', 'D' };
    uint8_t file_version_major = OCMD_FILE_FORMAT_VERSION_MAJOR;
    uint8_t file_version_minor = OCMD_FILE_FORMAT_VERSION_MINOR;
    uint8_t file_version_patch = OCMD_FILE_FORMAT_VERSION_PATCH;
    program_header prog_header = {};
    uint8_t reserved[14] = { 0 };
  };
#pragma pack(pop)
  static_assert(sizeof(ocmd_file_header) == 32, "OCMD file header size must be 32 bytes");

}  // namespace other

#endif  // OTHERLIB_VM_COMMAND_FILES_OCMD_HEADERS_HPP