/**
 * \file gpu_resource/texture_importer.cpp
 **/
#include "gpu_resource/texture_importer.hpp"

#include <filesystem>

#include "core/logger.hpp"

#include "gpu_resource/texture.hpp"
#include "renderer/renderer_backend.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

namespace other {
  namespace texture_importer {

    texture_data load_texture_data(const filepath& file_path) {
      PROFILE_SECTION("texture_importer::load_texture_data");

      texture_data data;
      if (!std::filesystem::exists(file_path)) {
        CORE_LOG_ERROR("Texture file does not exist: {}", file_path.string());
        return data;
      }

      int32_t width = 0;
      int32_t height = 0;
      int32_t channels = 0;
      /// force rgba so the upload format is uniform regardless of source encoding
      stbi_uc* pixels = stbi_load(file_path.string().c_str(), &width, &height, &channels, STBI_rgb_alpha);
      if (pixels == nullptr) {
        CORE_LOG_ERROR("Failed to decode texture '{}': {}", file_path.string(), stbi_failure_reason());
        return data;
      }

      data.width = width;
      data.height = height;
      data.source_channels = channels;
      data.pixels.assign(pixels, pixels + static_cast<size_t>(width) * static_cast<size_t>(height) * 4);
      stbi_image_free(pixels);
      return data;
    }

    resource_handle upload_texture_data(const std::string& name, const texture_data& data) {
      PROFILE_SECTION("texture_importer::upload_texture_data");
      OTHER_ASSERT(data.valid(), "upload_texture_data called with invalid texture data for '{}'", name);

      auto* renderer = subsystem<renderer_backend>::get();
      OTHER_ASSERT(renderer != nullptr, "Renderer backend subsystem is not available in upload_texture_data");

      resource_handle handle = renderer->api()->create_resource(name, resource_type::TEXTURE);
      if (handle.id == 0) {
        CORE_LOG_ERROR("Failed to create texture resource with name: {}", name);
        return { 0, resource_type::EMPTY };
      }

      texture& text = *renderer->api()->get_resource_as<texture>(handle);
      text.set_type(texture::tex_type::TEXTURE_2D)
        .set_format(texture::format::RGBA8)
        .set_size(static_cast<uint32_t>(data.width), static_cast<uint32_t>(data.height))
        .set_filter(texture::filter::LINEAR, texture::filter::LINEAR)
        .set_wrap_mode(texture::wrap::REPEAT, texture::wrap::REPEAT)
        .set_data(const_cast<uint8_t*>(data.pixels.data()), data.pixels.size());
      text.finalize_texture();

      /// the pixel buffer belongs to the caller; never leave a dangling pointer behind
      text.data = nullptr;
      text.data_size = 0;

      return handle;
    }

  }  // namespace texture_importer
}  // namespace other
