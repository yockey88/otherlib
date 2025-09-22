/**
 * \file terminal/command_executor.hpp
 **/
#ifndef OTHER_CORE_COMMAND_COMMAND_EXECUTOR_HPP
#define OTHER_CORE_COMMAND_COMMAND_EXECUTOR_HPP

#include "command/command.hpp"
#include "core/defines.hpp"

namespace other {

  // class Terminal;

  class command_executor {
   public:
    command_executor() {}
    ~command_executor() = default;

    exit_code execute(command_block& block);

   private:
    // Terminal* terminal = nullptr;

    command_block* current_block = nullptr;
    const command* current_command = nullptr;

    exit_code execute_command(const command& command);

    exit_code handle_control(uint8_t value);
    exit_code handle_file(uint8_t value);
    // void HandleModule(uint8_t value);
    // void HandleNet(uint8_t value);
    // void HandleDebug(uint8_t value);
    // void handle_other(uint8_t value);

    exit_code print_help();
    // void handle_call();
    // void handle_lua_call();

    exit_code handle_ls();
    exit_code handle_pwd();
    exit_code handle_source();
    exit_code handle_mount();

    // void handle_load();

    // void handle_listen();
    // void handle_connect();

    // void handle_echo();

    // void push_error_message(const std::string_view message);

    // template <typename T>
    // T GetArgument() {
    //   OE_ASSERT(current_block != nullptr, "Current block is null!");
    //   OE_ASSERT(!current_block->argument_queue.empty(), "Argument queue is empty!");

    //   address_t addr = current_block->argument_queue.front();
    //   current_block->argument_queue.pop();

    //   Registers& registers = Arena::GetRegisters();
    //   T value;
    //   {
    //     ValueReference val = registers.Get(addr);
    //     OE_ASSERT(val.value.GetType() == ValueType::STRING, "Argument is not a string");

    //     std::string str_val = val.value.Get<std::string>();
    //     try {
    //       if constexpr (std::is_same_v<T, std::string>) {
    //         value = str_val;
    //       } else if constexpr (std::is_same_v<T, uint8_t>) {
    //         value = static_cast<uint8_t>(std::stoi(str_val));
    //       } else if constexpr (std::is_same_v<T, uint16_t>) {
    //         value = static_cast<uint16_t>(std::stoi(str_val));
    //       } else if constexpr (std::is_same_v<T, uint32_t>) {
    //         value = static_cast<uint32_t>(std::stoi(str_val));
    //       } else if constexpr (std::is_same_v<T, uint64_t>) {
    //         value = static_cast<uint64_t>(std::stoull(str_val));
    //       } else {
    //         OE_ASSERT(false, "Invalid argument type");
    //       }
    //     } catch (const std::exception& e) {
    //       PushErrorMessage(fmtstr("Failed to parse argument : {}", e.what()));
    //       return T{};
    //     }

    //     registers.Clear(addr);
    //   }

    //   return value;
    // }
  };

}  // namespace other

#endif  // OTHER_CORE_COMMAND_COMMAND_EXECUTOR_HPP