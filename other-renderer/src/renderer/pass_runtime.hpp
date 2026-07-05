/**
 * \file renderer/pass_runtime.hpp
 **/
#ifndef OTHER_RENDERER_RENDERER_PASS_RUNTIME_HPP
#define OTHER_RENDERER_RENDERER_PASS_RUNTIME_HPP

#include <cstdint>
#include <vector>

#include "renderer/pipeline_definition.hpp"
#include "renderer/resource_tag.hpp"

namespace other {

  struct per_pass_binding_state {
    ostd::vector<resource_handle> per_frame_handles;
    struct draw_ring {
      resource_handle ring_buffer;  // GPU-side
      uint8_t* cpu_staging;         // CPU-side
      uint32_t capacity;
      uint32_t element_size;
      uint32_t stride;  // element_size aligned up to the required alignment for the buffer type
      uint32_t head;    // bytes consumed this frame
      uint32_t binding_point;
      uint32_t set;  // needed for some rendering apis, gl ignores, vk uses, dx12 uses sort of..., etc.
    };
    ostd::vector<draw_ring> per_draw_rings;
  };

  struct pass_runtime {
    natural_t pass_id;
    const pipeline_pass_definition* def;
    per_pass_binding_state state;
  };

}  // namespace other

#endif  // OTHER_RENDERER_RENDERER_PASS_RUNTIME_HPP