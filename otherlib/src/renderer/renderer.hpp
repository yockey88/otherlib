/**
 * \file renderer/renderer.hpp
 **/
#ifndef OTHER_RENDERER_RENDERER_HPP
#define OTHER_RENDERER_RENDERER_HPP

#include <SDL3/SDL.h>
#include <glm/glm.hpp>

#include "core/logger.hpp"
#include "renderer/renderer_backend.hpp"
#include "renderer/renderer_resource.hpp"

namespace other {

  class renderer_backend;

  class renderer {
   public:
    void begin_frame();
    void end_frame();

    void set_clear_color(const glm::vec4& color);

    resource_handle create_resource(resource_type type);

    template <typename T>
    T& bind_resource(const resource_handle& handle, const std::string& name) {
      auto* r = rendering();

      uint64_t name_hash = FNV(name);
      if (r->api()->resource_has_name(handle, name_hash)) {
        CORE_LOG_ERROR("Resource with name '{}' already exists.", name);
        return *r->rendering_api_instance->bind_resource_as<T>(handle);
      }

      r->api()->set_resource_name(handle, name);
      return *r->api()->bind_resource_as<T>(handle);
    }

   private:
    renderer_backend* rendering();
  };

}  // namespace other

#endif  // OTHER_RENDERER_RENDERER_HPP