/**
 * \file renderer/pass_executor_interop.hpp
 **/
#ifndef OTHER_RENDERER_RENDERER_PASS_EXECUTOR_INTEROP_HPP
#define OTHER_RENDERER_RENDERER_PASS_EXECUTOR_INTEROP_HPP

#include "renderer/render_graph.hpp"

namespace other {

  class renderer;
  class render_pipeline;

  struct pass_invocation_interop {
    renderer* renderer_ptr;
    render_graph::node* node;
    render_pipeline* pipeline;
    const uint8_t* params;
    uint32_t params_size;
    uint32_t reserved = 0;
  };

}  // namespace other

#endif  // OTHER_RENDERER_RENDERER_PASS_EXECUTOR_INTEROP_HPP