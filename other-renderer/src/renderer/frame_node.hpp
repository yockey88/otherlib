/**
 * \file renderer/frame_node.hpp
 **/
#ifndef OTHER_RENDERER_RENDERER_FRAME_NODE_HPP
#define OTHER_RENDERER_RENDERER_FRAME_NODE_HPP

#include "core/defines.hpp"

#include "renderer/render_pass.hpp"

namespace other {

  class renderer;

  struct frame_node {
    natural_t id;
    render_pass* pass = nullptr;

    std::map<natural_t, render_pass::buffer_resource> input_buffers;
    std::map<natural_t, render_pass::buffer_resource> output_buffers;
    std::map<natural_t, render_pass::texture_resource> input_textures;
    std::map<natural_t, render_pass::texture_resource> output_textures;

    void start_pass(renderer* renderer_ptr) const;
    void end_pass(renderer* renderer_ptr) const;

    bool operator==(const frame_node& other) const { return id == other.id && pass == other.pass; }
  };

}  // namespace other

#endif  // OTHER_RENDERER_RENDERER_FRAME_NODE_HPP