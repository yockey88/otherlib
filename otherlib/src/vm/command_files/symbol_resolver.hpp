/**
 * \file vm/command_files/symbol_resolver.hpp
 **/
#ifndef OTHERLIB_VM_COMMAND_FILES_SYMBOL_RESOLVER_HPP
#define OTHERLIB_VM_COMMAND_FILES_SYMBOL_RESOLVER_HPP

#include "core/interfaces.hpp"

namespace other {

  struct invocation_thunk {
    uint16_t final_address;
    std::vector<uint8_t> resolution_code;
  };

  class OTHER_API symbol_resolver {
    OTHER_ENVIRONMENT_INTERFACE("VM", "SymbolResolver");

   public:
    virtual ~symbol_resolver() = default;

    virtual void register_symbol(const std::string_view symbol, std::span<const uint8_t> invocation_thunk = {}) = 0;
    virtual void attach_code_label(const std::string_view label, uint16_t address) = 0;
    virtual void attach_data_symbol(const std::string_view symbol, uint16_t address) = 0;
    virtual invocation_thunk resolve_symbol(const std::string_view symbol) = 0;
  };

}  // namespace other

#endif  // OTHERLIB_VM_COMMAND_FILES_SYMBOL_RESOLVER_HPP