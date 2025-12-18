/**
 * \file scripting/execution_nodes/transform_nodes.cpp
 **/
#include "scripting/execution_nodes/transform_nodes.hpp"

#include "glm/fwd.hpp"

namespace other {

  void transform_source_node::execute_node() {
    write_output<0>(local_position);
    write_output<1>(local_rotation);
    write_output<2>(local_scale);
  }

  void transform_sink_node::execute_node() {
    position = read_input<0, glm::vec3>();
    rotation = read_input<1, glm::quat>();
    scale = read_input<2, glm::vec3>();
  }

  void constant_velocity_node::execute_node() {
    glm::vec3 current_position = read_input<0, glm::vec3>();
    glm::vec3 new_position = current_position + velocity;
    write_output<0>(new_position);
  }

  void constant_angular_velocity_node::execute_node() {
    glm::quat current_rotation = read_input<0, glm::quat>();

    float radians_per_second = glm::radians(speed);
    float radians_per_frame = radians_per_second * get_delta_time();
    glm::quat new_rotation = glm::rotate(current_rotation, radians_per_frame, axis);

    write_output<0>(new_rotation);
  }

}  // namespace other