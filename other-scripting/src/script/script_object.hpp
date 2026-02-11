/**
 * \file script/script_object.hpp
 **/
#ifndef OTHER_SCRIPTING_SCRIPT_SCRIPT_OBJECT_HPPP
#define OTHER_SCRIPTING_SCRIPT_SCRIPT_OBJECT_HPPP

#include <refl/refl.hpp>

#include "core/defines.hpp"

namespace other {
  namespace attr {

    struct script_object_field : refl::attr::usage::field {
      std::string_view display_name;
      bool editable = true;
      script_object_field() = default;
      explicit constexpr script_object_field(const std::string_view display_name, bool editable = true)
          : display_name(display_name), editable(editable) {}
    };

  }  // namespace attr

  class dotnet_object;
  class python_object;

  struct script_object {
    script_object() = default;
    ~script_object() = default;

    std::string name;
    integer_t id = -1;

    dotnet_object* dotnet_object = nullptr;
    python_object* python_object = nullptr;
  };

}  // namespace other

#endif  // OTHER_SCRIPTING_SCRIPT_SCRIPT_OBJECT_HPPP