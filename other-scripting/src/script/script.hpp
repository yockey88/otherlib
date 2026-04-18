/**
 * \file script/script.hpp
 **/
#ifndef OTHER_SCRIPTING_SCRIPT_SCRIPT_HPP
#define OTHER_SCRIPTING_SCRIPT_SCRIPT_HPP

#include "core/memory_pool.hpp"
#include "core/ref.hpp"

#include "script/script_object.hpp"

namespace other {

  struct script_object;

  enum class script_language {
    SCRIPT_TYPE_DOTNET = 0,
    SCRIPT_TYPE_PYTHON,
    SCRIPT_TYPE_LUA,
    // SCRIPT_TYPE_JAVASCRIPT,

    SCRIPT_TYPE_MAX,
    INVALID_SCRIPT_TYPE = SCRIPT_TYPE_MAX
  };
  constexpr static size_t kNumScriptTypes = static_cast<size_t>(script_language::SCRIPT_TYPE_MAX);

  class script {
   public:
    script(const std::string_view name)
        : object_name(name) {}
    ~script() = default;

   private:
    struct script_object_binding_data {
      script_object* object = nullptr;
    };

    std::string object_name;
    std::array<std::vector<script_object_binding_data>, kNumScriptTypes> bindings;
  };

}  // namespace other

#endif  // OTHER_SCRIPTING_SCRIPT_SCRIPT_HPP