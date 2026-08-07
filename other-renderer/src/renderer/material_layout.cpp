/**
 * \file renderer/material_layout.cpp
 **/
#include "renderer/material_layout.hpp"

#include <cstring>

#include "core/fnv.hpp"
#include "core/logger.hpp"
#include "core/profiler.hpp"

namespace other {

  namespace {

    struct kind_traits {
      uint32_t size = 0;
      uint32_t alignment = 0;
    };

    kind_traits std430_traits(material_value::kind kind) {
      switch (kind) {
        case material_value::kind::F32:
        case material_value::kind::I32:
        case material_value::kind::B32: return { 4, 4 };
        case material_value::kind::VEC2: return { 8, 8 };
        case material_value::kind::VEC3: return { 12, 16 };
        case material_value::kind::VEC4: return { 16, 16 };
        default:
          OTHER_ASSERT(false, "unknown material value kind {}", static_cast<int>(kind));
          return {};
      }
    }

    uint32_t align_up(uint32_t v, uint32_t alignment) {
      return (v + alignment - 1) & ~(alignment - 1);
    }

    void write_value(const material_value& value, material_value::kind kind, std::span<uint8_t> out, uint32_t offset) {
      const kind_traits traits = std430_traits(kind);
      OTHER_ASSERT(offset + traits.size <= out.size(), "material pack write out of bounds (offset {} size {} blob {})", offset, traits.size, out.size());
      std::memcpy(out.data() + offset, &value.data.x, traits.size);
    }

  }  // namespace

  void material_layout::finalize() {
    OTHER_ASSERT(!params.empty(), "material layout declares no params — a material binding without a block is meaningless");
    OTHER_ASSERT(instance_capacity > 0, "material layout instance capacity must be > 0");
    PROFILE_SECTION("material_layout::finalize");

    uint32_t cursor = 0;
    uint32_t max_alignment = 4;
    base_color_offset.reset();
    for (auto& p : params) {
      OTHER_ASSERT(!p.name.empty(), "material layout param with empty name");
      p.name_hash = FNV(p.name);
      OTHER_ASSERT(p.default_value.value_kind == p.kind, "material layout param '{}': default value kind does not match declared type", p.name);

      const kind_traits traits = std430_traits(p.kind);
      p.offset = align_up(cursor, traits.alignment);
      cursor = p.offset + traits.size;
      max_alignment = std::max(max_alignment, traits.alignment);

      if (p.kind == material_value::kind::VEC4 && p.name == "base_color") {
        base_color_offset = p.offset;
      }
    }
    element_size = align_up(cursor, max_alignment);

    for (size_t i = 0; i < params.size(); ++i) {
      for (size_t j = i + 1; j < params.size(); ++j) {
        OTHER_ASSERT(params[i].name_hash != params[j].name_hash, "material layout declares param '{}' twice", params[i].name);
      }
    }
    for (auto& slot : texture_slots) {
      OTHER_ASSERT(!slot.name.empty() && !slot.uniform.empty(), "material layout texture slot needs both a name and a uniform");
      slot.name_hash = FNV(slot.name);
    }
    for (size_t i = 0; i < texture_slots.size(); ++i) {
      for (size_t j = i + 1; j < texture_slots.size(); ++j) {
        OTHER_ASSERT(texture_slots[i].name_hash != texture_slots[j].name_hash, "material layout declares texture slot '{}' twice", texture_slots[i].name);
        OTHER_ASSERT(texture_slots[i].unit != texture_slots[j].unit, "material layout texture slots '{}' and '{}' share unit {}", texture_slots[i].name, texture_slots[j].name, texture_slots[i].unit);
      }
    }
  }

  void material_layout::pack(const material* mat, std::span<uint8_t> out, const pack_warning_fn& warn) const {
    OTHER_ASSERT(element_size > 0, "material layout was not finalized before pack");
    OTHER_ASSERT(out.size() >= element_size, "material pack target is smaller than the layout element ({} < {})", out.size(), element_size);
    PROFILE_SECTION("material_layout::pack");
    std::memset(out.data(), 0, element_size);

    for (const param& p : params) {
      const material_value* value = &p.default_value;
      if (mat != nullptr) {
        if (const auto it = mat->params.find(p.name_hash); it != mat->params.end()) {
          if (it->second.value_kind == p.kind) {
            value = &it->second;
          } else if (warn != nullptr) {
            warn(p.name, "value shape does not match the layout's declared type; using the layout default");
          }
        }
      }
      write_value(*value, p.kind, out, p.offset);
    }

    if (mat != nullptr && warn != nullptr) {
      for (const auto& [hash, value] : mat->params) {
        const bool known = std::ranges::find(params, hash, &param::name_hash) != params.end();
        if (!known) {
          const auto name_it = mat->param_names.find(hash);
          warn(name_it != mat->param_names.end() ? std::string_view{ name_it->second } : std::string_view{ "<unnamed>" },
               "not declared by the active pipeline's material layout; ignored");
        }
      }
    }
  }

  void material_layout::fold_base_color_tint(std::span<uint8_t> element, const glm::vec4& tint) const {
    if (!base_color_offset.has_value() || tint == glm::vec4(1.f)) {
      return;
    }
    OTHER_ASSERT(*base_color_offset + sizeof(glm::vec4) <= element.size(), "tint fold target out of bounds");

    glm::vec4 base;
    std::memcpy(&base, element.data() + *base_color_offset, sizeof(glm::vec4));
    base *= tint;
    std::memcpy(element.data() + *base_color_offset, &base, sizeof(glm::vec4));
  }

}  // namespace other
