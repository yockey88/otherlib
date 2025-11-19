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
    uint16_t code_section_offset = 0;
    uint16_t data_table_offset = 0;
    uint16_t data_section_offset = 0;
    uint16_t num_instructions = 0;
  };
  static_assert(sizeof(program_header) == 8, "Program header size must be 8 bytes");

  struct ocmd_file_header {
    char file_signature[4] = { 'O', 'C', 'M', 'D' };
    uint8_t file_version_major = OCMD_FILE_FORMAT_VERSION_MAJOR;
    uint8_t file_version_minor = OCMD_FILE_FORMAT_VERSION_MINOR;
    uint8_t reserved[10] = { 0 };
    program_header prog_header = {};
  };
#pragma pack(pop)
  static_assert(sizeof(ocmd_file_header) == 24, "OCMD file header size must be 24 bytes");

}  // namespace other

#endif  // OTHERLIB_VM_COMMAND_FILES_OCMD_HEADERS_HPP