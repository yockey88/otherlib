/**
 * \file scripting/execution_nodes/linear_algebra_nodes.cpp
 **/
#include "scripting/execution_nodes/linear_algebra_nodes.hpp"

namespace other {

  void add_vec2_node::execute_node() {
    glm::vec2 a = read_input<0, glm::vec2>();
    glm::vec2 b = read_input<1, glm::vec2>();
    glm::vec2 result = a + b;
    write_output<0>(result);
  }

  void add_vec3_node::execute_node() {
    glm::vec3 a = read_input<0, glm::vec3>();
    glm::vec3 b = read_input<1, glm::vec3>();
    glm::vec3 result = a + b;
    write_output<0>(result);
  }

  void add_vec4_node::execute_node() {
    glm::vec4 a = read_input<0, glm::vec4>();
    glm::vec4 b = read_input<1, glm::vec4>();
    glm::vec4 result = a + b;
    write_output<0>(result);
  }

}  // namespace other