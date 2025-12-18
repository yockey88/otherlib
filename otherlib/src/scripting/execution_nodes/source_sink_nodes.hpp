/**
 * \file scripting/execution_nodes/source_sink_nodes.hpp
 **/
#ifndef OTHERLIB_SCRIPTING_EXECUTION_NODES_SOURCE_SINK_NODES_HPP
#define OTHERLIB_SCRIPTING_EXECUTION_NODES_SOURCE_SINK_NODES_HPP

#include "scripting/execution_node.hpp"

namespace other {

  template <typename T>
    requires std::is_integral_v<T> || std::is_floating_point_v<T>
  struct source_node : public execution_node_impl<0, 1> {
    T value = {};

    source_node() = default;
    source_node(T val) : value(val) {}

    virtual ~source_node() = default;
    void execute_node() override {
      write_output<0>(value);
    }
  };

  template <typename T>
    requires std::is_integral_v<T> || std::is_floating_point_v<T>
  struct sink_node : public execution_node_impl<1, 0> {
    T value = {};

    virtual ~sink_node() = default;
    void execute_node() override {
      value = read_input<0, T>();
    }
  };

  struct vec2_source_node : public execution_node_impl<0, 1> {
    glm::vec2 value = {};

    vec2_source_node() = default;
    vec2_source_node(const glm::vec2& val) : value(val) {}

    virtual ~vec2_source_node() = default;
    void execute_node() override {
      write_output<0>(value);
    }
  };

  struct vec2_sink_node : public execution_node_impl<1, 0> {
    glm::vec2 value = {};

    virtual ~vec2_sink_node() = default;
    void execute_node() override {
      value = read_input<0, glm::vec2>();
    }
  };

  struct vec3_source_node : public execution_node_impl<0, 1> {
    glm::vec3 value = {};

    vec3_source_node() = default;
    vec3_source_node(const glm::vec3& val) : value(val) {}
    virtual ~vec3_source_node() = default;

    void execute_node() override {
      write_output<0>(value);
    }
  };

  struct vec3_sink_node : public execution_node_impl<1, 0> {
    glm::vec3 value = {};

    virtual ~vec3_sink_node() = default;
    void execute_node() override {
      value = read_input<0, glm::vec3>();
    }
  };

  struct vec4_source_node : public execution_node_impl<0, 1> {
    glm::vec4 value = {};

    vec4_source_node() = default;
    vec4_source_node(const glm::vec4& val) : value(val) {}

    virtual ~vec4_source_node() = default;
    void execute_node() override {
      write_output<0>(value);
    }
  };

  struct vec4_sink_node : public execution_node_impl<1, 0> {
    glm::vec4 value = {};

    virtual ~vec4_sink_node() = default;
    void execute_node() override {
      value = read_input<0, glm::vec4>();
    }
  };

}  // namespace other

#endif  // OTHERLIB_SCRIPTING_EXECUTION_NODES_SOURCE_SINK_NODES_HPP