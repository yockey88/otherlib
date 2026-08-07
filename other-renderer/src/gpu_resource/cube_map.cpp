/**
 * \file gpu_resource/cube_map.cpp
 **/
#include "gpu_resource/cube_map.hpp"

#include "core/profiler.hpp"

#include "renderer/renderer_backend.hpp"

namespace other {

  resource_handle cube_map::create(const std::string& name, texture::format format, uint32_t width, uint32_t height) {
    PROFILE_SECTION("cube_map::create");
    resource_handle handle = subsystem<renderer_backend>::get()->api()->create_resource(name, resource_type::CUBEMAP);
    if (handle.id == 0) {
      CORE_LOG_ERROR("Failed to create texture resource with name: {}", name);
      return { 0, resource_type::EMPTY };
    }

    auto& text = (*subsystem<renderer_backend>::get()->api()->get_resource_as<cube_map>(handle))
                   .set_format(format)
                   .set_face_size(width, height)
                   .set_filter(texture::filter::LINEAR, texture::filter::LINEAR)
                   .set_wrap_mode(texture::wrap::CLAMP_TO_EDGE, texture::wrap::CLAMP_TO_EDGE);

    std::ranges::fill(text.faces, ostd::vector<uint8_t>());
    text.finalize_cube_map();

    return handle;
  }

  cube_map& cube_map::bind(uint32_t slot) {
    subsystem<renderer_backend>::get()->api()->bind_texture_resource(handle(), slot);
    return *this;
  }

  cube_map& cube_map::set_face_size(uint32_t width, uint32_t height) {
    if (width == 0 || height == 0) {
      CORE_LOG_ERROR("Invalid texture face size: width and height must be greater than zero.");
    } else {
      face_size = { width, height };
    }
    return *this;
  }

  cube_map& cube_map::set_format(texture::format frmt) {
    if (frmt >= texture::format::NUM_FORMATS) {
      CORE_LOG_ERROR("Invalid texture format: out of range.");
    } else {
      texture_format = frmt;
    }
    return *this;
  }

  cube_map& cube_map::set_filter(texture::filter min_filter, texture::filter mag_filter) {
    if (min_filter >= texture::filter::NUM_FILTERS || mag_filter >= texture::filter::NUM_FILTERS) {
      CORE_LOG_ERROR("Invalid texture filter: out of range.");
    } else {
      this->min_filter = min_filter;
      this->mag_filter = mag_filter;

      subsystem<renderer_backend>::get()->api()->set_texture_filter(handle(), min_filter, mag_filter);
    }
    return *this;
  }

  cube_map& cube_map::set_wrap_mode(texture::wrap wrap_s, texture::wrap wrap_t, texture::wrap wrap_r) {
    if (wrap_s >= texture::wrap::NUM_WRAP_MODES || wrap_t >= texture::wrap::NUM_WRAP_MODES || wrap_r >= texture::wrap::NUM_WRAP_MODES) {
      CORE_LOG_ERROR("Invalid texture wrap mode: out of range.");
    } else {
      this->wrap_s = wrap_s;
      this->wrap_t = wrap_t;
      this->wrap_r = wrap_r;

      subsystem<renderer_backend>::get()->api()->set_texture_wrap_mode(handle(), wrap_s, wrap_t, wrap_r);
    }
    return *this;
  }

  cube_map& cube_map::set_data(face face_idx, const std::span<const uint8_t> data) {
    PROFILE_SECTION("cube_map::set_data");
    if (data.empty()) {
      CORE_LOG_ERROR("Invalid cube map data: data vector is empty.");
      return *this;
    }

    if (face_idx >= kCubeFaces) {
      CORE_LOG_ERROR("Invalid cube map face index: out of range.");
      return *this;
    }

    faces[face_idx].clear();
    faces[face_idx].resize(data.size());
    std::ranges::copy(data, faces[face_idx].begin());
    return *this;
  }

  cube_map& cube_map::set_data(face face_idx, const uint8_t* data, size_t size) {
    PROFILE_SECTION("cube_map::set_data");
    if (data == nullptr || size == 0) {
      CORE_LOG_ERROR("Invalid cube map data: data pointer is null or size is zero.");
      return *this;
    }

    if (face_idx >= kCubeFaces) {
      CORE_LOG_ERROR("Invalid cube map face index: out of range.");
      return *this;
    }

    faces[face_idx].clear();
    faces[face_idx].resize(size);
    std::memcpy(faces[face_idx].data(), data, size);
    return *this;
  }

  void cube_map::unbind(uint32_t slot) {
  }

  namespace {

    template <typename T>
      requires std::is_enum_v<T>
    inline void increment_enum(T& value) {
      value = static_cast<T>(static_cast<std::underlying_type_t<T>>(value) + 1);
    }

    texture::tex_type get_face_texture_type(cube_map::face face_idx) {
      switch (face_idx) {
        case cube_map::face::POSITIVE_X:
          return texture::tex_type::TEXTURE_CUBE_FACE_POSITIVE_X;
        case cube_map::face::NEGATIVE_X:
          return texture::tex_type::TEXTURE_CUBE_FACE_NEGATIVE_X;
        case cube_map::face::POSITIVE_Y:
          return texture::tex_type::TEXTURE_CUBE_FACE_POSITIVE_Y;
        case cube_map::face::NEGATIVE_Y:
          return texture::tex_type::TEXTURE_CUBE_FACE_NEGATIVE_Y;
        case cube_map::face::POSITIVE_Z:
          return texture::tex_type::TEXTURE_CUBE_FACE_POSITIVE_Z;
        case cube_map::face::NEGATIVE_Z:
          return texture::tex_type::TEXTURE_CUBE_FACE_NEGATIVE_Z;
        default:
          OTHER_ASSERT(false, "Invalid cube map face index.");
      }
    }

  }  // namespace

  void cube_map::finalize_cube_map() {
    PROFILE_SECTION("cube_map::finalize_cube_map");
    if (face_size.x == 0 || face_size.y == 0) {
      CORE_LOG_ERROR("Cube map not finalized: face size is zero.");
      return;
    }

    if (texture_format >= texture::format::NUM_FORMATS) {
      CORE_LOG_ERROR("Cube map not finalized: invalid texture format.");
      return;
    }

    for (size_t i = 0; i < NUM_FACES; ++i) {
      face f = static_cast<face>(i);

      uint8_t* data = faces[f].empty() ? nullptr : faces[f].data();
      size_t data_size = data == nullptr ? 0 : faces[f].size();
      subsystem<renderer_backend>::get()->api()->upload_texture(handle(), get_face_texture_type(f), texture_format, 1, false, face_size, 1, data, data_size);
    }
  }

}  // namespace other