/**
 * \file scripting/execution_nodes/transform_nodes.hpp
 **/
#ifndef OTHERLIB_SCRIPTING_EXECUTION_NODES_TRANSFORM_NODES_HPP
#define OTHERLIB_SCRIPTING_EXECUTION_NODES_TRANSFORM_NODES_HPP

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "scripting/execution_node.hpp"

namespace other {

  struct transform_source_node : public execution_node_impl<0, 3> {  // <0, 4> {
    glm::vec3 local_position = {};
    glm::quat local_rotation = {};
    glm::vec3 local_scale = { 1.f, 1.f, 1.f };

    transform_source_node() = default;
    transform_source_node(const glm::vec3& position)
        : local_position(position) {}
    transform_source_node(const glm::vec3& position, const glm::vec3& rotation)
        : local_position(position), local_rotation(rotation) {}
    transform_source_node(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale)
        : local_position(position), local_rotation(rotation), local_scale(scale) {}

    virtual ~transform_source_node() = default;
    void execute_node() override;
  };

  struct transform_sink_node : public execution_node_impl<3, 0> {
    glm::vec3 position = {};
    glm::quat rotation = {};
    glm::vec3 scale = { 1.f, 1.f, 1.f };

    transform_sink_node() = default;

    virtual ~transform_sink_node() = default;

    void execute_node() override;
  };

  struct constant_velocity_node : public execution_node_impl<1, 1> {
    glm::vec3 velocity = {};

    constant_velocity_node() = default;
    constant_velocity_node(const glm::vec3& vel)
        : velocity(vel) {}

    virtual ~constant_velocity_node() = default;

    void execute_node() override;
  };

  struct constant_angular_velocity_node : public execution_node_impl<1, 1> {
    glm::vec3 axis = {};
    float speed = 0.f;

    constant_angular_velocity_node() = default;
    constant_angular_velocity_node(const glm::vec3& axis, float speed = 90.f)
        : axis(axis), speed(speed) {}

    virtual ~constant_angular_velocity_node() = default;

    void execute_node() override;
  };

}  // namespace other

#endif  // OTHERLIB_SCRIPTING_EXECUTION_NODES_TRANSFORM_NODES_HPP