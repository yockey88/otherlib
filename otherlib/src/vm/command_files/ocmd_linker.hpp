/**
 * \file vm/command_files/ocmd_linker.hpp
 **/
#ifndef OTHERLIB_VM_COMMAND_FILES_OCMD_LINKER_HPP
#define OTHERLIB_VM_COMMAND_FILES_OCMD_LINKER_HPP

#include "core/scope.hpp"

#include "vm/command_files/ocmd_program.hpp"
#include "vm/command_files/symbol_resolver.hpp"

namespace other {

  class ocmd_linker {
   public:
    ocmd_linker(const ocmd_program& code)
        : code(code) {}
    ~ocmd_linker() = default;

    std::vector<uint8_t> link(scope<symbol_resolver> resolver);

   private:
    ocmd_program code;

    struct symbol_address {
      std::string name;
      uint16_t local_address;
    };
    std::vector<symbol_address> compiler_symbol_local_addresses;

    void register_symbols(scope<symbol_resolver>& resolver);

    std::vector<uint8_t> create_compiler_generated_symbols(scope<symbol_resolver>& resolver);
    void rewrite_instructions(scope<symbol_resolver>& resolver);

    void write_header(scope<symbol_resolver>& resolver, std::vector<uint8_t>& binary);
    void write_code(scope<symbol_resolver>& resolver, std::vector<uint8_t>& binary);
    void write_generated_code(scope<symbol_resolver>& resolver, std::vector<uint8_t>& binary, std::span<const uint8_t> generated_code);
    void write_data_sections(scope<symbol_resolver>& resolver, std::vector<uint8_t>& binary);

    void do_final_linking(scope<symbol_resolver>& resolver, std::vector<uint8_t>& binary);
  };

}  // namespace other

#endif  // OTHERLIB_VM_COMMAND_FILES_OCMD_LINKER_HPP