/**
 * \file gpu_resource/texture_importer.hpp
 *
 * load_texture_data = pure cpu decode (any thread); upload_texture_data = gpu upload (main thread only)
 **/
#ifndef OTHER_RENDERER_GPU_RESOURCE_TEXTURE_IMPORTER_HPP
#define OTHER_RENDERER_GPU_RESOURCE_TEXTURE_IMPORTER_HPP

#include "core/defines.hpp"

#include "gpu_resource/renderer_resource.hpp"

namespace other {
  namespace texture_importer {

    struct texture_data {
      int32_t width = 0;
      int32_t height = 0;
      /// channel count of the source image; pixels are always expanded to rgba8
      int32_t source_channels = 0;
      ostd::vector<uint8_t> pixels = {};

      bool valid() const { return width > 0 && height > 0 && !pixels.empty(); }
    };

    texture_data load_texture_data(const filepath& file_path);
    resource_handle upload_texture_data(const std::string& name, const texture_data& data);

  }  // namespace texture_importer
}  // namespace other

#endif  // OTHER_RENDERER_GPU_RESOURCE_TEXTURE_IMPORTER_HPP
