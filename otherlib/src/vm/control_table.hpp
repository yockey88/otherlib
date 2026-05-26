/**
 * \file vm/control_table.hpp
 **/
#ifndef OTHERLIB_VM_CONTROL_TABLE_HPP
#define OTHERLIB_VM_CONTROL_TABLE_HPP

#include <array>
#include <string_view>

namespace other {

  struct other_command_device;
  using other_command_executor = void (*)(other_command_device*);
  using other_command_table = std::array<other_command_executor, 16>;

  enum control_tables {
    OTHER_CONTROL_TABLE_V000 = 0,
    OTHER_CONTROL_TABLE_DEBUGGER_V000,
    OTHER_CONTROL_TABLE_DECOMPILER_V000,

    NUM_CONTROL_TABLES,
    INVALID_CONTROL_TABLE = NUM_CONTROL_TABLES,
  };

  using namespace std::literals::string_view_literals;
  struct control_table_tag {
    std::string_view name;
    control_tables table;
    constexpr control_table_tag(std::string_view n, control_tables t)
        : name(n), table(t) {}
  };

  static constexpr std::array<control_table_tag, NUM_CONTROL_TABLES> kControlTableNames = {
    control_table_tag{ "V000"sv, OTHER_CONTROL_TABLE_V000 },
    control_table_tag{ "DEBUGGER-V000"sv, OTHER_CONTROL_TABLE_DEBUGGER_V000 },
    control_table_tag{ "DECOMPILER-V000"sv, OTHER_CONTROL_TABLE_DECOMPILER_V000 }
  };

  void load_builtin_control_table(other_command_device* device, control_tables table);

}  // namespace other

#endif  // OTHERLIB_VM_CONTROL_TABLE_HPP