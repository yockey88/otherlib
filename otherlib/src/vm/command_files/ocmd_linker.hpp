/**
 * \file vm/command_files/ocmd_linker.hpp
 **/
#ifndef OTHERLIB_VM_COMMAND_FILES_OCMD_LINKER_HPP
#define OTHERLIB_VM_COMMAND_FILES_OCMD_LINKER_HPP

#include "core/scope.hpp"

#include "vm/command_files/ocmd_headers.hpp"
#include "vm/command_files/ocmd_program.hpp"
#include "vm/command_files/symbol_resolver.hpp"

namespace other {

  class diagnostic_engine;

  class ocmd_linker {
   public:
    ocmd_linker(const ocmd_program& code)
        : code(code) {}
    ~ocmd_linker() = default;

    std::vector<uint8_t> link(scope<symbol_resolver> resolver, diagnostic_engine* diag);

    constexpr static inline size_t kHeaderAddressOffset = sizeof(ocmd_file_header);
    inline uint16_t normalize_address(size_t address) const {
      return address + kHeaderAddressOffset;
    }

   private:
    diagnostic_engine* diagnostics;
    ocmd_program code;

    struct symbol_address {
      std::string name;
      uint16_t local_address;
    };
    std::vector<symbol_address> compiler_symbol_local_addresses;

    inline uint16_t get_code_section_offset() const {
      return sizeof(ocmd_file_header);
    }
    inline uint16_t get_current_linking_address(const std::span<const uint8_t> binary) const {
      return static_cast<uint16_t>(binary.size());
    }
    uint16_t calculate_code_section_offset(size_t index) const;
    uint16_t globablize_offset(uint16_t offset) const;

    void register_symbols(scope<symbol_resolver>& resolver);

    void write_code(scope<symbol_resolver>& resolver, ostd::vector<uint8_t>& binary);
    // void write_generated_code(scope<symbol_resolver>& resolver, ostd::vector<uint8_t>& binary, std::span<const uint8_t> generated_code);
    void write_data_sections(scope<symbol_resolver>& resolver, ostd::vector<uint8_t>& binary);

    void do_final_linking(scope<symbol_resolver>& resolver, ostd::vector<uint8_t>& binary);

    ostd::vector<uint8_t> create_compiler_generated_symbols(scope<symbol_resolver>& resolver);
    void rewrite_instructions(scope<symbol_resolver>& resolver);
  };

}  // namespace other

#endif  // OTHERLIB_VM_COMMAND_FILES_OCMD_LINKER_HPP