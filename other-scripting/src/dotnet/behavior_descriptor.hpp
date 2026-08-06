/**
 * \file dotnet/behavior_descriptor.hpp
 **/
#ifndef OTHER_SCRIPTING_DOTNET_BEHAVIOR_DESCRIPTOR_HPP
#define OTHER_SCRIPTING_DOTNET_BEHAVIOR_DESCRIPTOR_HPP

#include <cstdint>
#include <string>
#include <vector>

#include "core/defines.hpp"

namespace other {

  /// \note must stay in sync with Other.Core.BehaviorFieldFlags on the C# side.
  enum class behavior_display_flags : uint32_t {
    NONE = 0,
    READ_ONLY = 1 << 0,
    HAS_RANGE = 1 << 1,
    HAS_TOOLTIP = 1 << 2,
    COLOR_FIELD = 1 << 3,
    IS_GROUP_START = 1 << 4,
    HAS_SEPARATOR = 1 << 5,
    SERIALIZABLE = 1 << 6,
  };

  constexpr behavior_display_flags operator|(behavior_display_flags a, behavior_display_flags b) {
    return static_cast<behavior_display_flags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
  }
  constexpr behavior_display_flags operator&(behavior_display_flags a, behavior_display_flags b) {
    return static_cast<behavior_display_flags>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
  }
  constexpr bool has_flag(behavior_display_flags flags, behavior_display_flags flag) {
    return (static_cast<uint32_t>(flags) & static_cast<uint32_t>(flag)) != 0;
  }

  /// \note must stay in sync with C# struct and layout of C# Other.Core.NativeBehaviorFieldDescriptor
  struct native_behavior_field_descriptor {
    uint8_t field_value_type;
    uint32_t flags;
    float range_min;
    float range_max;
  };
  static_assert(sizeof(native_behavior_field_descriptor) <= 16, "descriptor must be small and blittable");

  struct behavior_field_descriptor {
    std::string field_name;    ///< raw C# field name
    std::string display_name;  ///< [InspectorField(DisplayName=...)] or field_name
    std::string tooltip;       ///< [InspectorField(Tooltip=...)]
    std::string group_name;    ///< [InspectorGroup("...")]

    value_type type = value_type::EMPTY_TYPE;
    behavior_display_flags flags = behavior_display_flags::NONE;

    float range_min = 0.f;
    float range_max = 0.f;
    opt<float> speed;

    /// index in parent behavior
    int32_t field_index = -1;
  };

  struct behavior_descriptor {
    std::string full_type_name;
    std::string display_name;
    int32_t behavior_index = -1;

    /// the behavior's own script object — field reads/writes go through it by name
    integer_t script_object_id = -1;

    ostd::vector<behavior_field_descriptor> fields;
  };

  struct behavior_snapshot {
    ostd::vector<behavior_descriptor> behaviors;
    bool valid = false;
  };

}  // namespace other

#endif  // OTHER_SCRIPTING_DOTNET_BEHAVIOR_DESCRIPTOR_HPP