/**
 * \file script/script_object.hpp
 **/
#ifndef OTHER_SCRIPTING_SCRIPT_SCRIPT_OBJECT_HPPP
#define OTHER_SCRIPTING_SCRIPT_SCRIPT_OBJECT_HPPP

#include "core/defines.hpp"

namespace other {

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