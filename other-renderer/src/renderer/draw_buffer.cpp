/**
 * \file renderer/draw_buffer.cpp
 **/
#include "renderer/draw_buffer.hpp"

namespace other {

  void draw_buffer::clear() {
    draw_ops.clear();
    payload_buffer.release();
  }

}  // namespace other