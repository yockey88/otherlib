/**
 * \file renderer/frame_binding_definition.hpp
 **/
#ifndef OTHER_RENDERER_FRAME_BINDING_DEFINITION_HPP
#define OTHER_RENDERER_FRAME_BINDING_DEFINITION_HPP

#include "renderer/resource_tag.hpp"

namespace other {

  enum class binding_scope : uint8_t {
    PER_PIPELINE = 0,   // persistent state, uploaded once on initialization/reload
    PER_FRAME = 1,      // upload once per frame
    PER_PASS = 2,       // upload once per pass, indexed by pass index
    PER_DRAW_CALL = 3,  // upload once per draw call in a frame, indexed by draw call index
    PER_INSTANCE = 4,   // upload once per instance in a draw call, indexed by draw call index and instance index
  };

  enum class binding_type : uint8_t {
    UNKNOWN = 0,
    UNIFORM_BUFFER,
    STORAGE_BUFFER,
    TEXTURE_2D,
    STORAGE_IMAGE,
    TEXTURE_ARRAY,
    DRAW_INDIRECT_BUFFER,
  };

  struct frame_binding_definition {
    std::string name;
    resource_tag tag = resource_tag::none();

    binding_scope scope;
    binding_type type;

    uint32_t binding = 0;
    uint32_t set = 0;

    // for arrays, number of elements in array, for non-arrays should be 1
    uint32_t array_size = 1;
    uint32_t element_size = 0;  // required for PER_DRAW_CALL/PER_INSTANCE
  };

}  // namespace other

#endif  // OTHER_RENDERER_FRAME_BINDING_DEFINITION_HPP