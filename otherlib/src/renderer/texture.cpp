/**
 * \file renderer/texture.cpp
 **/
#include "renderer/texture.hpp"

#include "core/logger.hpp"
#include "renderer/renderer_backend.hpp"

namespace other {

  texture& texture::bind(uint32_t slot) {
    subsystem<renderer_backend>::get()->api()->bind_texture_resource(handle(), slot);
    return *this;
  }

  void texture::unbind(uint32_t slot) {
    subsystem<renderer_backend>::get()->api()->unbind_texture_resource(handle(), slot);
  }

  texture& texture::set_type(tex_type type) {
    texture_type = type;
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

  texture& texture::set_data(const std::vector<uint8_t>& data) {
    if (data.empty()) {
      CORE_LOG_ERROR("Invalid texture data: data vector is empty.");
      return *this;
    }

    this->data.clear();
    this->data.resize(data.size());
    std::ranges::copy(data, this->data.begin());
    return *this;
  }

  texture& texture::set_data(const uint8_t* data, size_t size) {
    if (!data || size == 0) {
      CORE_LOG_ERROR("Invalid texture data: data pointer is null or size is zero.");
      return *this;
    }

    set_data(std::vector<uint8_t>(data, data + size));
    return *this;
  }

  void texture::finalize_texture() {
    if (size.x <= 0 || size.y <= 0) {
      CORE_LOG_ERROR("Texture size must be set before finalizing.");
      return;
    }

    if (texture_format == format::NUM_FORMATS) {
      CORE_LOG_ERROR("Texture format must be set before finalizing.");
      return;
    }

    uint8_t* data = this->data.empty() ? nullptr : this->data.data();
    size_t data_size = data == nullptr ? 0 : this->data.size();
    subsystem<renderer_backend>::get()->api()->upload_texture(handle(), texture_type, texture_format, size, data, data_size);
  }

  void texture::finalize_image(uint32_t idx, bool writable) {
    finalize_texture();

    bind();
    subsystem<renderer_backend>::get()->api()->bind_texture_as_image(handle(), idx, writable);
    unbind();
  }

}  // namespace other