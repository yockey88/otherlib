/**
 * \file renderer/draw_buffer.hpp
 **/
#ifndef OTHER_RENDERER_RENDERER_DRAW_BUFFER_HPP
#define OTHER_RENDERER_RENDERER_DRAW_BUFFER_HPP

#include <string>

#include "core/arena_buffer.hpp"

namespace other {

  enum class draw_op_type : uint16_t {
    BEGIN_PASS = 0,
    END_PASS,

    BIND_PIPELINE_STATE,
    BIND_RESOURCES,

    SET_CONSTANTS,
    SET_DYNAMIC_OFFSETS,

    DRAW,
    DRAW_INDEXED,
    DRAW_INDIRECT,

    DISPATCH,
    DISPATCH_INDIRECT,

    COPY_BUFFER,
    COPY_IMAGE,

    BLIT_IMAGE,

    CLEAR_IMAGE,
    CLEAR_BUFFER,

    BARRIER,

    PUSH_DEBUG_MARKER,
    POP_DEBUG_MARKER,
  };

  struct draw_op {
    draw_op_type type;
    uint16_t pass_index;
    uint32_t payload_offset;
  };

  struct draw_buffer {
    ostd::vector<draw_op> draw_ops;
    arena_buffer payload_buffer;

    // ostd::vector<pass_state> pass_states;
    // resource_state_table resource_states;

    template <typename T>
    T& append(draw_op_type type, uint16_t pass_index) {
    }

    void clear();
  };

}  // namespace other

#endif  // OTHER_RENDERER_RENDERER_DRAW_BUFFER_HPP