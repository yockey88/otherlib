/**
 * \file renderer/frame_binding_registry.hpp
 **/
#ifndef OTHER_RENDERER_RENDERER_FRAME_BINDING_REGISTRY_HPP
#define OTHER_RENDERER_RENDERER_FRAME_BINDING_REGISTRY_HPP

#include "gpu_resource/renderer_resource.hpp"
#include "renderer/resource_tag.hpp"

namespace other {

  struct render_data;
  class render_pipeline;

  using per_frame_producer_fn = std::function<void(render_pipeline&, const render_data&, resource_handle)>;
  using per_draw_producer_fn = std::function<void(const render_data&, size_t, std::span<uint8_t>)>;
  using per_instance_producer_fn = std::function<void(const render_data&, size_t, size_t, std::span<uint8_t>)>;

  class frame_binding_registry {
   public:
    void register_per_frame(resource_tag, per_frame_producer_fn);
    void register_per_draw(resource_tag, per_draw_producer_fn);
    void register_per_instance(resource_tag, per_instance_producer_fn);

    per_frame_producer_fn find_per_frame(resource_tag) const;
    per_draw_producer_fn find_per_draw(resource_tag) const;
    per_instance_producer_fn find_per_instance(resource_tag) const;

   private:
    ostd::map<resource_tag, per_frame_producer_fn> per_frame_binders;
    ostd::map<resource_tag, per_draw_producer_fn> per_draw_binders;
    ostd::map<resource_tag, per_instance_producer_fn> per_instance_binders;
  };

}  // namespace other

#endif  // OTHER_RENDERER_RENDERER_FRAME_BINDING_REGISTRY_HPP