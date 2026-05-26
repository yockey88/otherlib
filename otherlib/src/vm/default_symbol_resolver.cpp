/**
 * \file vm/default_symbol_resolver.cpp
 **/
#include "vm/default_symbol_resolver.hpp"

#include "vm/command_files/compiler_error.hpp"

namespace other {

  void default_symbol_resolver::register_symbol(const std::string_view symbol, std::span<const uint8_t> invocation_thunk) {
    if (auto itr = symbols.find(FNV(symbol)); itr != symbols.end()) {
      throw ocmd_linking_error(std::format("Symbol '{}' is already registered as a data symbol", symbol));
    }
    auto [itr, inserted] = symbols.emplace(FNV(symbol), symbol_info{ .address = 0, .invocation_thunk = std::vector<uint8_t>(invocation_thunk.begin(), invocation_thunk.end()) });
    if (!inserted) {
      throw ocmd_linking_error(std::format("Symbol '{}' is already registered as a code label", symbol));
    }
    itr->second.name = symbol;
  }

  void default_symbol_resolver::attach_code_label(const std::string_view label, uint16_t address) {
    if (auto itr = symbols.find(FNV(label)); itr != symbols.end()) {
      if (itr->second.address != 0 && itr->second.address != address) {
        throw ocmd_linking_error(std::format("Code label '{}' is already attached to address 0x{:04X}, cannot attach to different address 0x{:04X}", label, itr->second.address, address));
      }

      itr->second.address = address;
      itr->second.kind = operand_kind::CODE_LABEL;
    } else {
      auto [itr2, inserted] = symbols.emplace(FNV(label), symbol_info{ .address = address });
      if (!inserted) {
        throw ocmd_linking_error(std::format("Symbol '{}' is already registered as a code label", label));
      }
      itr2->second.kind = operand_kind::CODE_LABEL;
    }
  }

  void default_symbol_resolver::attach_data_symbol(const std::string_view symbol, uint16_t address) {
    if (auto itr = symbols.find(FNV(symbol)); itr != symbols.end()) {
      if (itr->second.address != 0 && itr->second.address != address) {
        throw ocmd_linking_error(std::format("Data symbol '{}' is already attached to address 0x{:04X}, cannot attach to different address 0x{:04X}", symbol, itr->second.address, address));
      }

      itr->second.address = address;
      itr->second.kind = operand_kind::DATA_SYMBOL;
    } else {
      auto [itr2, inserted] = symbols.emplace(FNV(symbol), symbol_info{ .address = address });
      if (!inserted) {
        throw ocmd_linking_error(std::format("Symbol '{}' is already registered as a data symbol", symbol));
      }
      itr2->second.kind = operand_kind::DATA_SYMBOL;
    }
  }

  invocation_thunk default_symbol_resolver::resolve_symbol(const std::string_view symbol) {
    const auto symbol_hash = FNV(symbol);
    if (auto itr = symbols.find(symbol_hash); itr != symbols.end()) {
      return invocation_thunk{ .final_address = itr->second.address, .resolution_code = itr->second.invocation_thunk };
    } else if (auto itr = symbols.find(symbol_hash); itr != symbols.end()) {
      return invocation_thunk{ .final_address = itr->second.address, .resolution_code = itr->second.invocation_thunk };
    } else {
      return invocation_thunk{ .final_address = 0, .resolution_code = {} };
    }
  }

}  // namespace other