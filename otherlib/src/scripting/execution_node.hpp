/**
 * \file scripting/execution_node.hpp
 **/
#ifndef OTHERLIB_SCRIPTING_EXECUTION_NODE_HPP
#define OTHERLIB_SCRIPTING_EXECUTION_NODE_HPP

#include <cstdint>
#include <type_traits>

#include "core/defines.hpp"
#include "core/logger.hpp"
#include "core/value.hpp"

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

  struct execution_graph;

  struct execution_node {
    natural_t id = 0;
    float delta_time = 1.f / 60.f;

    virtual ~execution_node() = default;

    virtual natural_t get_num_inputs() const = 0;
    virtual natural_t get_num_outputs() const = 0;

    virtual value_type get_input_type(natural_t index) const = 0;
    virtual value_type get_output_type(natural_t index) const = 0;

    virtual value& get_value_of_register(natural_t index) = 0;
    virtual const value& get_value_of_register(natural_t index) const = 0;

    template <typename T>
    void write_input(const natural_t index, const T& value) {
      natural_t input_index = get_this_node_input_index(index);
      write_register<T>(input_index, value);
    }

    template <typename T>
    T read_input(const natural_t index) const {
      natural_t input_index = get_this_node_input_index(index);
      return read_register<T>(input_index);
    }

    template <typename T>
    T read_output(const natural_t index) const {
      natural_t output_index = get_this_node_output_index(index);
      return read_register<T>(output_index);
    }

    void read_inputs_from_predecessors(execution_graph* graph);

    virtual void execute_node() = 0;
    /// vector of opcodes representing compiled version of this node
    // virtual natural_t compile_node(std::vector<uint32_t>& curr_program) = 0;

   protected:
    template <typename T>
    T read_register(natural_t index) const {
      const value& val = get_value_of_register(index);
      if (val.type() != get_value_type<T>()) {
        CORE_LOG_ERROR("Type mismatch when reading register at index {}: expected type {}, got type {}", index, get_value_type<T>(), val.type());
        return T{};
      } else {
        return (const T&)val;
      }
    }

    template <typename T>
    void write_register(natural_t index, const T& value) {
      get_value_of_register(index) = value;
    }

    float get_delta_time() const { return delta_time; }

    inline natural_t get_this_node_input_index(natural_t index) const { return index; }
    inline natural_t get_this_node_output_index(natural_t index) const { return get_num_inputs() + index; }
  };

  template <size_t IS, size_t OS>
  struct execution_node_impl : public execution_node {
    constexpr static size_t kMaxNodeArity = 4;
    static_assert(IS <= kMaxNodeArity, "Input arity exceeds maximum node arity");
    static_assert(OS <= kMaxNodeArity, "Output arity exceeds maximum node arity");

    constexpr static size_t kNodeArity = IS + OS;
    constexpr static size_t kInputArity = IS;
    constexpr static size_t kOutputArity = OS;

    value registers[kNodeArity];

    virtual ~execution_node_impl() = default;

    natural_t get_num_inputs() const override { return kInputArity; }
    natural_t get_num_outputs() const override { return kOutputArity; }

    value_type get_input_type(natural_t index) const override {
      OTHER_ASSERT(index < kInputArity, "Input index out of bounds in get_input_type");
      natural_t reg_indx = get_this_node_input_index(index);
      return registers[reg_indx].type();
    }

    value_type get_output_type(natural_t index) const override {
      OTHER_ASSERT(kInputArity + index < kOutputArity, "Output index out of bounds in get_output_type");
      natural_t reg_indx = get_this_node_output_index(index);
      return registers[reg_indx].type();
    }

    const value& get_value_of_register(natural_t index) const override {
      OTHER_ASSERT(index < kNodeArity, "Register index [{}] out of bounds in execution_node_impl", index);
      return registers[index];
    }

    value& get_value_of_register(natural_t index) override {
      OTHER_ASSERT(index < kNodeArity, "Register index [{}] out of bounds in execution_node_impl", index);
      return registers[index];
    }

    template <size_t I, typename T>
    void write_input(const T& value) {
      size_t input_reg_idx = get_this_node_input_index(I);
      registers[input_reg_idx] = value;
    }

    template <size_t I, typename T>
    const T& read_input() const {
      static_assert(I < kInputArity, "Input index out of bounds");
      size_t input_reg_idx = get_this_node_input_index(I);
      return (const T&)registers[input_reg_idx];
    }

    template <size_t I, typename T>
    const T& read_output() const {
      static_assert(kInputArity + I < kNodeArity, "Output index out of bounds");
      size_t output_reg_idx = get_this_node_output_index(I);
      return (const T&)registers[output_reg_idx];
    }

   protected:
    template <size_t I, typename T>
    void write_output(const T& value) {
      static_assert(kInputArity + I < kNodeArity, "Output index out of bounds");
      size_t output_reg_idx = get_this_node_output_index(I);
      registers[output_reg_idx] = value;
    }
  };

}  // namespace other

#endif  // OTHERLIB_SCRIPTING_EXECUTION_NODE_HPP