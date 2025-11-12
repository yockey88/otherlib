/**
 * \file scripting/execution_node.hpp
 **/
#ifndef OTHERLIB_SCRIPTING_EXECUTION_NODE_HPP
#define OTHERLIB_SCRIPTING_EXECUTION_NODE_HPP

#include <cstdint>
#include <type_traits>

#include "core/defines.hpp"
#include "core/logger.hpp"

#include "vm/other_device.hpp"
#include "vm/vm_memory.hpp"

namespace other {

  struct pin {
    struct address {
      natural_t node_id = 0;
      uint8_t pin_index = 0;
      constexpr auto operator<=>(const address& other) const = default;
    };
  };

  struct execution_link {
    pin::address from;
    pin::address to;
  };

  constexpr static size_t kExecutionNodeRegisterBitSize = other_command_device::kRegisterBitSize * 2;

  template <typename T>
  concept fits_in_node_register = std::is_trivially_copyable_v<T> && (sizeof(T) <= kExecutionNodeRegisterBitSize / 8);

  struct execution_graph;

  struct execution_node {
    natural_t id = 0;

    virtual ~execution_node() = default;

    virtual natural_t get_num_inputs() const = 0;
    virtual natural_t get_num_outputs() const = 0;

    virtual value_type get_input_type(natural_t index) const = 0;
    virtual value_type get_output_type(natural_t index) const = 0;

    template <typename T>
      requires fits_in_node_register<T>
    void write_input(const natural_t index, const T& value) {
      natural_t input_index = get_this_node_input_index(index);
      write_register<T>(input_index, value);
    }

    template <typename T>
      requires fits_in_node_register<T>
    const T& read_input(const natural_t index) const {
      natural_t input_index = get_this_node_input_index(index);
      return read_register<T>(input_index);
    }

    template <typename T>
      requires fits_in_node_register<T>
    const T& read_output(const natural_t index) const {
      natural_t output_index = get_this_node_output_index(index);
      return read_register<T>(output_index);
    }

    void read_inputs_from_predecessors(execution_graph* graph);

    virtual void execute_node() = 0;
    // virtual void compile_node() = 0;

   protected:
    template <typename T>
      requires fits_in_node_register<T>
    const T& read_register(natural_t index) const {
      return *reinterpret_cast<const T*>(get_register_memory(index));
    }

    template <typename T>
      requires fits_in_node_register<T>
    void write_register(natural_t index, const T& value) {
      *(T*)get_register_memory(index) = value;
    }

    inline natural_t get_this_node_input_index(natural_t index) const { return index; }
    inline natural_t get_this_node_output_index(natural_t index) const { return get_num_inputs() + index; }

    virtual void* get_register_memory(natural_t /*index*/) = 0;
    virtual const void* get_register_memory(natural_t /*index*/) const = 0;
    virtual register_t<other_command_device::kRegisterBitSize * 2>& raw_register(natural_t index) = 0;
  };

  template <size_t IS, size_t OS>
  struct execution_node_impl : public execution_node {
    constexpr static size_t kMaxNodeArity = 4;
    static_assert(IS <= kMaxNodeArity, "Input arity exceeds maximum node arity");
    static_assert(OS <= kMaxNodeArity, "Output arity exceeds maximum node arity");

    /// need to fit a glm::vec4 in a register
    constexpr static size_t kRegisterByteSize = kExecutionNodeRegisterBitSize / 8;
    constexpr static size_t kNodeArity = IS + OS;
    constexpr static size_t kInputArity = IS;
    constexpr static size_t kOutputArity = OS;

    register_t<kExecutionNodeRegisterBitSize> registers[kNodeArity];
    value_type register_types[kNodeArity];

    virtual ~execution_node_impl() = default;

    natural_t get_num_inputs() const override { return kInputArity; }
    natural_t get_num_outputs() const override { return kOutputArity; }

    value_type get_input_type(natural_t index) const override {
      OTHER_ASSERT(index < kInputArity, "Input index out of bounds in get_input_type");
      natural_t reg_indx = get_this_node_input_index(index);
      return register_types[reg_indx];
    }

    value_type get_output_type(natural_t index) const override {
      OTHER_ASSERT(kInputArity + index < kOutputArity, "Output index out of bounds in get_output_type");
      natural_t reg_indx = get_this_node_output_index(index);
      return register_types[reg_indx];
    }

    template <size_t I, typename T>
      requires fits_in_node_register<T>
    void write_input(const T& value) {
      static_assert(I < kInputArity, "Input index out of bounds");
      size_t input_reg_idx = I;

      std::vector<uint8_t> reg_bytes;
      reg_bytes.resize(sizeof(uint64_t), 0);

      const uint8_t* value_bytes = reinterpret_cast<const uint8_t*>(&value);
      auto bytes = std::span(value_bytes, sizeof(T));
      std::ranges::copy(bytes, reg_bytes.begin());

      registers[input_reg_idx] = (register_t<kExecutionNodeRegisterBitSize>)(*reinterpret_cast<register_t<kExecutionNodeRegisterBitSize>*>(reg_bytes.data()));
      register_types[input_reg_idx] = get_value_type<T>();
    }

    template <size_t I, typename T>
      requires fits_in_node_register<T>
    const T& read_input() const {
      static_assert(I < kInputArity, "Input index out of bounds");
      size_t input_reg_idx = get_this_node_input_index(I);
      return read_register<T>(input_reg_idx);
    }

    template <size_t I, typename T>
      requires fits_in_node_register<T>
    const T& read_output() const {
      static_assert(kInputArity + I < kNodeArity, "Output index out of bounds");
      size_t output_reg_idx = get_this_node_output_index(I);
      return read_register<T>(output_reg_idx);
    }

   protected:
    template <size_t I, typename T>
      requires fits_in_node_register<T>
    void write_output(const T& value) {
      static_assert(kInputArity + I < kNodeArity, "Output index out of bounds");
      size_t output_reg_idx = get_this_node_output_index(I);

      std::vector<uint8_t> reg_bytes;
      reg_bytes.resize(sizeof(kRegisterByteSize), 0);

      const uint8_t* value_bytes = reinterpret_cast<const uint8_t*>(&value);
      auto bytes = std::span(value_bytes, sizeof(kRegisterByteSize));
      std::ranges::copy(bytes, reg_bytes.begin());

      registers[output_reg_idx] = (register_t<kExecutionNodeRegisterBitSize>)(*reinterpret_cast<register_t<kExecutionNodeRegisterBitSize>*>(reg_bytes.data()));
      register_types[output_reg_idx] = get_value_type<T>();
    }

    template <typename T>
      requires fits_in_node_register<T>
    const T& read_register(natural_t idx) const {
      OTHER_ASSERT(idx < kNodeArity, "Register index [{}] out of bounds in execution_node_impl", idx);
      return *reinterpret_cast<const T*>(reinterpret_cast<const void*>(&registers[idx]));
    }

    void* get_register_memory(natural_t index) override {
      OTHER_ASSERT(index < kNodeArity, "Register index [{}] out of bounds in execution_node_impl", index);
      void* reg_ptr = static_cast<void*>(&registers[index]);
      return reinterpret_cast<void*>(reg_ptr);
    }

    const void* get_register_memory(natural_t index) const override {
      OTHER_ASSERT(index < kNodeArity, "Register index [{}] out of bounds in execution_node_impl", index);
      const void* reg_ptr = static_cast<const void*>(&registers[index]);
      return reinterpret_cast<const void*>(reg_ptr);
    }

    register_t<kExecutionNodeRegisterBitSize>& raw_register(natural_t index) override {
      OTHER_ASSERT(index < kNodeArity, "Register index [{}] out of bounds in execution_node_impl", index);
      return registers[index];
    }
  };

}  // namespace other

#endif  // OTHERLIB_SCRIPTING_EXECUTION_NODE_HPP