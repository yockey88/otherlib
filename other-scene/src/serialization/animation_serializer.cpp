/**
 * \file serialization/animation_serializer.cpp
 **/
#include "serialization/animation_serializer.hpp"

#include <array>
#include <fstream>

#include "serialization/scene_field_codec.hpp"

namespace other {
  namespace serialization {

    namespace {

      constexpr std::array<uint8_t, 4> kMagic = { 'O', 'A', 'N', 'M' };
      constexpr uint16_t kFormatVersion = 1;

      template <typename T>
      void write_keys(const ostd::vector<keyframe<T>>& keys, ostd::vector<uint8_t>& out) {
        field_codec::write_raw<uint32_t>(static_cast<uint32_t>(keys.size()), out);
        for (const keyframe<T>& key : keys) {
          field_codec::write_raw<float>(key.t, out);
          field_codec::write_raw<T>(key.value, out);
        }
      }

      template <typename T>
      bool read_keys(std::span<const uint8_t> bytes, size_t& offset, ostd::vector<keyframe<T>>& out) {
        uint32_t count = 0;
        if (!field_codec::read_raw(bytes, offset, count)) {
          return false;
        }
        /// a corrupt count must fail here, before it can size an allocation
        constexpr size_t kKeySize = sizeof(float) + sizeof(T);
        if (count > (bytes.size() - offset) / kKeySize) {
          return false;
        }
        out.reserve(count);
        for (uint32_t i = 0; i < count; ++i) {
          keyframe<T>& key = out.emplace_back();
          if (!field_codec::read_raw(bytes, offset, key.t) || !field_codec::read_raw(bytes, offset, key.value)) {
            return false;
          }
        }
        return true;
      }

    }  // namespace

    ostd::vector<uint8_t> serialize_animation_clip(const animation_clip& clip) {
      ostd::vector<uint8_t> out;
      out.insert(out.end(), kMagic.begin(), kMagic.end());
      field_codec::write_raw<uint16_t>(kFormatVersion, out);
      field_codec::write_raw<uint16_t>(0, out);  // flags

      field_codec::write_sized_string(clip.name, out);
      field_codec::write_raw<float>(clip.duration, out);
      field_codec::write_raw<uint32_t>(static_cast<uint32_t>(clip.joint_tracks.size()), out);
      for (const joint_track& track : clip.joint_tracks) {
        field_codec::write_raw<natural_t>(track.joint_name_hash, out);
        field_codec::write_sized_string(track.joint_name, out);
        write_keys(track.position_keyframes, out);
        write_keys(track.rotation_keyframes, out);
        write_keys(track.scale_keyframes, out);
      }
      return out;
    }

    clip_parse_result parse_animation_clip(std::span<const uint8_t> bytes) {
      size_t offset = 0;

      std::array<uint8_t, 4> magic{};
      if (!field_codec::read_raw(bytes, offset, magic) || magic != kMagic) {
        return { .error = "not an .oanim clip (bad magic)" };
      }

      uint16_t format = 0;
      uint16_t flags = 0;
      if (!field_codec::read_raw(bytes, offset, format) || !field_codec::read_raw(bytes, offset, flags)) {
        return { .error = "truncated .oanim header" };
      }
      if (format != kFormatVersion) {
        return { .error = std::format("unsupported .oanim format {} (this build reads format {})", format, kFormatVersion) };
      }

      animation_clip clip;
      uint32_t track_count = 0;
      if (!field_codec::read_sized_string(bytes, offset, clip.name) ||
          !field_codec::read_raw(bytes, offset, clip.duration) ||
          !field_codec::read_raw(bytes, offset, track_count)) {
        return { .error = "truncated .oanim clip header" };
      }

      for (uint32_t i = 0; i < track_count; ++i) {
        joint_track& track = clip.joint_tracks.emplace_back();
        if (!field_codec::read_raw(bytes, offset, track.joint_name_hash) ||
            !field_codec::read_sized_string(bytes, offset, track.joint_name) ||
            !read_keys(bytes, offset, track.position_keyframes) ||
            !read_keys(bytes, offset, track.rotation_keyframes) ||
            !read_keys(bytes, offset, track.scale_keyframes)) {
          return { .error = std::format("truncated .oanim track {} of {}", i, track_count) };
        }
      }

      if (offset != bytes.size()) {
        return { .error = std::format("{} trailing bytes after the .oanim payload", bytes.size() - offset) };
      }

      return { .clip = std::move(clip) };
    }

    clip_parse_result load_animation_clip(const filepath& path) {
      std::ifstream in(path, std::ios::binary | std::ios::ate);
      if (!in) {
        return { .error = std::format("could not open '{}'", path.string()) };
      }

      const std::streamsize size = in.tellg();
      in.seekg(0, std::ios::beg);

      ostd::vector<uint8_t> bytes;
      bytes.resize(static_cast<size_t>(size));
      if (size > 0 && !in.read(reinterpret_cast<char*>(bytes.data()), size)) {
        return { .error = std::format("failed to read '{}'", path.string()) };
      }

      return parse_animation_clip(bytes);
    }

  }  // namespace serialization
}  // namespace other
