/**
 * \file scripting/execution_nodes/arithmetic_nodes.hpp
 **/
#ifndef OTHERLIB_SCRIPTING_EXECUTION_NODES_ARITHMETIC_NODES_HPP
#define OTHERLIB_SCRIPTING_EXECUTION_NODES_ARITHMETIC_NODES_HPP

#include "scripting/execution_node.hpp"

namespace other {

  template <typename T>
    requires std::is_integral_v<T> || std::is_floating_point_v<T>
  struct addition_node : public execution_node_impl<2, 1> {
    virtual ~addition_node() = default;

    void execute_node() override {
      T a = read_input<0, T>();
      T b = read_input<1, T>();
      T result = a + b;  // Default to addition
      write_output<0>(result);
    }
  };

  template <typename T>
    requires std::is_integral_v<T> || std::is_floating_point_v<T>
  struct subtraction_node : public execution_node_impl<2, 1> {
    virtual ~subtraction_node() = default;

    void execute_node() override {
      T a = read_input<0, T>();
      T b = read_input<1, T>();
      T result = a - b;  // Default to subtraction
      write_output<0>(result);
    }
  };

  template <typename T>
    requires std::is_integral_v<T> || std::is_floating_point_v<T>
  struct multiplication_node : public execution_node_impl<2, 1> {
    virtual ~multiplication_node() = default;

    void execute_node() override {
      T a = read_input<0, T>();
      T b = read_input<1, T>();
      T result = a * b;  // Default to multiplication
      write_output<0>(result);
    }
  };

  template <typename T>
    requires std::is_integral_v<T> || std::is_floating_point_v<T>
  struct division_node : public execution_node_impl<2, 1> {
    virtual ~division_node() = default;

    void execute_node() override {
      T a = read_input<0, T>();
      T b = read_input<1, T>();
      if (b == static_cast<T>(0)) {
        CORE_LOG_ERROR("Division by zero error in division_node.");
        return;
      }
      T result = a / b;  // Default to division
      write_output<0>(result);
    }
  };

}  // namespace other

#endif  // OTHERLIB_SCRIPTING_EXECUTION_NODES_ARITHMETIC_NODES_HPP