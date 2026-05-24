/**
 * \file renderer/resource_tag_registry.hpp
 **/
#ifndef OTHER_RENDERER_RENDERER_RESOURCE_TAG_REGISTRY_HPP
#define OTHER_RENDERER_RENDERER_RESOURCE_TAG_REGISTRY_HPP

#include "gpu_resource/renderer_resource.hpp"
#include "renderer/resource_tag.hpp"

namespace other {

  struct render_data;
  class render_pipeline;

  using resource_tag_binder_fn_t = std::function<void(render_pipeline&, const render_data&, resource_handle)>;
  class resource_tag_registry {
   public:
    void register_binder(resource_tag tag, resource_tag_binder_fn_t resource_binder);
    resource_tag_binder_fn_t find(resource_tag tag) const;

   private:
    std::map<resource_tag, resource_tag_binder_fn_t> binders;
  };

}  // namespace other

#endif  // OTHER_RENDERER_RENDERER_RESOURCE_TAG_REGISTRY_HPP