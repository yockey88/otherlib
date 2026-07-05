/**
 * \file vm/default_symbol_resolver.hpp
 **/
#ifndef OTHERLIB_VM_DEFAULT_SYMBOL_RESOLVER_HPP
#define OTHERLIB_VM_DEFAULT_SYMBOL_RESOLVER_HPP

#include "vm/command_files/symbol_resolver.hpp"
#include "vm/operand.hpp"

namespace other {

  class default_symbol_resolver : public symbol_resolver {
   public:
    default_symbol_resolver() = default;
    ~default_symbol_resolver() override = default;

    void register_symbol(const std::string_view symbol, std::span<const uint8_t> invocation_thunk = {}) override;
    void attach_code_label(const std::string_view label, uint16_t address) override;
    void attach_data_symbol(const std::string_view symbol, uint16_t address) override;
    invocation_thunk resolve_symbol(const std::string_view symbol) override;

   private:
    struct symbol_info {
      std::string name;
      operand_kind kind;
      uint16_t address;
      ostd::vector<uint8_t> invocation_thunk;
    };
    std::unordered_map<natural_t, symbol_info> symbols;
  };

}  // namespace other

#endif  // OTHERLIB_VM_DEFAULT_SYMBOL_RESOLVER_HPP
