/**
 * \file gpu_resource/texture.cpp
 **/
#include "gpu_resource/texture.hpp"

#include "core/logger.hpp"
#include "core/profiler.hpp"

#include "renderer/renderer_backend.hpp"

namespace other {

  resource_handle texture::create(const std::string& name, tex_type type, format frmt, uint32_t width, uint32_t height) {
    PROFILE_SECTION("texture::create");
    resource_handle handle = subsystem<renderer_backend>::get()->api()->create_resource(name, resource_type::TEXTURE);
    if (handle.id == 0) {
      CORE_LOG_ERROR("Failed to create texture resource with name: {}", name);
      return { 0, resource_type::EMPTY };
    }

    auto& text = (*subsystem<renderer_backend>::get()->api()->get_resource_as<texture>(handle))
                   .set_type(type)
                   .set_format(frmt)
                   .set_size(width, height)
                   .set_filter(texture::filter::LINEAR, texture::filter::LINEAR)
                   .set_wrap_mode(texture::wrap::CLAMP_TO_EDGE, texture::wrap::CLAMP_TO_EDGE);

    text.finalize_texture();

    return handle;
  }

  resource_handle texture::create(const std::string& name, tex_type type, format frmt, const std::pair<filter, filter>& filters, const std::tuple<wrap, wrap, wrap>& wraps, uint32_t mip_levels, bool generate_mips, uint32_t width, uint32_t height) {
    PROFILE_SECTION("texture::create");
    resource_handle handle = subsystem<renderer_backend>::get()->api()->create_resource(name, resource_type::TEXTURE);
    if (handle.id == 0) {
      CORE_LOG_ERROR("Failed to create texture resource with name: {}", name);
      return { 0, resource_type::EMPTY };
    }

    auto& text = (*subsystem<renderer_backend>::get()->api()->get_resource_as<texture>(handle))
                   .set_type(type)
                   .set_format(frmt)
                   .set_size(width, height)
                   .set_filter(filters.first, filters.second)
                   .set_wrap_mode(std::get<0>(wraps), std::get<1>(wraps), std::get<2>(wraps))
                   .set_mip_levels(mip_levels)
                   .set_generate_mips(generate_mips);

    text.finalize_texture();

    return handle;
  }

  resource_handle texture::create3d(const std::string& name, format frmt, const glm::vec3& dimensions) {
    PROFILE_SECTION("texture::create3d");
    resource_handle handle = subsystem<renderer_backend>::get()->api()->create_resource(name, resource_type::TEXTURE);
    if (handle.id == 0) {
      CORE_LOG_ERROR("Failed to create 3D texture resource with name: {}", name);
      return { 0, resource_type::EMPTY };
    }

    auto& text = (*subsystem<renderer_backend>::get()->api()->get_resource_as<texture>(handle))
                   .set_type(texture::tex_type::TEXTURE_3D)
                   .set_format(frmt)
                   .set_dimensions(dimensions)
                   .set_filter(texture::filter::LINEAR, texture::filter::LINEAR)
                   .set_wrap_mode(texture::wrap::CLAMP_TO_EDGE, texture::wrap::CLAMP_TO_EDGE, texture::wrap::CLAMP_TO_EDGE);

    text.finalize_texture();

    return handle;
  }

  resource_handle texture::create3d(const std::string& name, format frmt, const std::pair<filter, filter>& filters, const std::tuple<wrap, wrap, wrap>& wraps, uint32_t mip_levels, bool generate_mips, const glm::vec3& dimensions) {
    PROFILE_SECTION("texture::create3d");
    resource_handle handle = subsystem<renderer_backend>::get()->api()->create_resource(name, resource_type::TEXTURE);
    if (handle.id == 0) {
      CORE_LOG_ERROR("Failed to create 3D texture resource with name: {}", name);
      return { 0, resource_type::EMPTY };
    }

    auto& text = (*subsystem<renderer_backend>::get()->api()->get_resource_as<texture>(handle))
                   .set_type(texture::tex_type::TEXTURE_3D)
                   .set_format(frmt)
                   .set_dimensions(dimensions)
                   .set_filter(filters.first, filters.second)
                   .set_wrap_mode(std::get<0>(wraps), std::get<1>(wraps), std::get<2>(wraps))
                   .set_mip_levels(mip_levels)
                   .set_generate_mips(generate_mips);

    text.finalize_texture();

    return handle;
  }

  void texture::destroy_texture(resource_handle handle) {
    subsystem<renderer_backend>::get()->api()->destroy_resource(handle);
  }

  texture& texture::bind(uint32_t slot) {
    subsystem<renderer_backend>::get()->api()->bind_texture_resource(handle(), slot);
    return *this;
  }

  texture& texture::bind_image(uint32_t index, uint32_t level, bool layered, int32_t layer, format frmt, access_flags flags) {
    subsystem<renderer_backend>::get()->api()->bind_image(handle(), index, level, layered, layer, frmt, flags);
    return *this;
  }

  void texture::unbind(uint32_t slot) {
    subsystem<renderer_backend>::get()->api()->unbind_texture_resource(handle(), slot);
  }

  texture& texture::set_type(tex_type type) {
    texture_type = type;
    return *this;
  }

  texture& texture::set_mip_levels(uint32_t mip_levels) {
    if (mip_levels == 0) {
      CORE_LOG_ERROR("Invalid mip levels: must be greater than zero.");
    } else {
      this->mip_levels = mip_levels;
    }
    return *this;
  }

  texture& texture::set_generate_mips(bool generate_mips) {
    this->generate_mips = generate_mips;
    return *this;
  }

  texture& texture::set_size(uint32_t width, uint32_t height) {
    if (width == 0 || height == 0) {
      CORE_LOG_ERROR("Invalid texture size: width and height must be greater than zero.");
    } else {
      size = { width, height };
    }
    return *this;
  }

  texture& texture::set_dimensions(const glm::vec3& dimensions) {
    if (dimensions.x <= 0 || dimensions.y <= 0 || dimensions.z <= 0) {
      CORE_LOG_ERROR("Invalid texture dimensions: all dimensions must be greater than zero.");
    } else {
      size = { static_cast<uint32_t>(dimensions.x), static_cast<uint32_t>(dimensions.y) };
      depth = static_cast<uint32_t>(dimensions.z);
    }
    set_depth(static_cast<uint32_t>(dimensions.z));
    return *this;
  }

  texture& texture::set_depth(uint32_t depth) {
    if (depth == 0) {
      CORE_LOG_ERROR("Invalid texture depth: must be greater than zero.");
    } else {
      this->depth = depth;
    }
    return *this;
  }

  texture& texture::set_format(format frmt) {
    if (frmt >= format::NUM_FORMATS) {
      CORE_LOG_ERROR("Invalid texture format: out of range.");
    } else {
      texture_format = frmt;
    }
    return *this;
  }

  texture& texture::set_filter(filter min_filter, filter mag_filter) {
    if (min_filter >= filter::NUM_FILTERS || mag_filter >= filter::NUM_FILTERS) {
      CORE_LOG_ERROR("Invalid texture filter: out of range.");
    } else {
      this->min_filter = min_filter;
      this->mag_filter = mag_filter;

      subsystem<renderer_backend>::get()->api()->set_texture_filter(handle(), min_filter, mag_filter);
    }
    return *this;
  }

  texture& texture::set_wrap_mode(wrap wrap_s, wrap wrap_t, wrap wrap_r) {
    if (wrap_s >= wrap::NUM_WRAP_MODES || wrap_t >= wrap::NUM_WRAP_MODES || wrap_r >= wrap::NUM_WRAP_MODES) {
      CORE_LOG_ERROR("Invalid texture wrap mode: out of range.");
    } else {
      this->wrap_s = wrap_s;
      this->wrap_t = wrap_t;
      this->wrap_r = wrap_r;

      subsystem<renderer_backend>::get()->api()->set_texture_wrap_mode(handle(), wrap_s, wrap_t, wrap_r);
    }
    return *this;
  }

  texture& texture::set_data(void* data, size_t size) {
    if (!data || size == 0) {
      CORE_LOG_ERROR("Invalid texture data: data pointer is null or size is zero.");
      return *this;
    }

    this->data = data;
    this->data_size = size;
    return *this;
  }

  void texture::finalize_texture() {
    PROFILE_SECTION("texture::finalize_texture");
    if (size.x <= 0 || size.y <= 0) {
      CORE_LOG_ERROR("Texture size must be set before finalizing.");
      return;
    }

    if (texture_format == format::NUM_FORMATS) {
      CORE_LOG_ERROR("Texture format must be set before finalizing.");
      return;
    }

    subsystem<renderer_backend>::get()->api()->upload_texture(handle(), get_type(), get_format(), mip_levels, generate_mips, size, depth, data, data_size);
  }

  ImTextureID texture::get_imgui_texture_id() {
    return (ImTextureID)(uintptr_t)subsystem<renderer_backend>::get()->api()->get_texture_gpu_resource(handle());
  }

}  // namespace other