/**
 * \file tools/tool.hpp
 **/
#ifndef OTHERLIB_TOOLS_TOOL_HPP
#define OTHERLIB_TOOLS_TOOL_HPP

#include <string_view>

#include "core/defines.hpp"

#include "dotnet/dotnet_object.hpp"
#include "script/script_object.hpp"

namespace other {

  struct script_object;

  class tool {
   public:
    tool(const std::string& type_name);
    virtual ~tool();

    const std::string_view get_type_name() const { return type_name; }

    template <typename... Args>
    void invoke_tool_method(const std::string_view method_name, Args&&... args) const {
      if (build_tool_obj == nullptr || build_tool_obj->dotnet_object == nullptr) {
        CORE_LOG_ERROR("Cannot invoke method '{}' on tool of type '{}': .NET object is not initialized.", method_name, type_name);
        return;
      }
      build_tool_obj->dotnet_object->invoke<>(method_name, std::forward<Args>(args)...);
    }

    template <typename R, typename... Args>
    R invoke_tool_method(const std::string_view method_name, Args&&... args) const {
      if (build_tool_obj == nullptr || build_tool_obj->dotnet_object == nullptr) {
        CORE_LOG_ERROR("Cannot invoke method '{}' on tool of type '{}': .NET object is not initialized.", method_name, type_name);
        if constexpr (std::is_same_v<R, void>) {
          return;
        } else {
          return R{};
        }
      }
      return build_tool_obj->dotnet_object->invoke<R>(method_name, std::forward<Args>(args)...);
    }

   protected:
    inline script_object* get_tool_script_object() const;

   private:
    std::string type_name;

    integer_t build_tool_obj_id = -1;
    script_object* build_tool_obj = nullptr;

    void initialize();
    void shutdown();
  };

}  // namespace other

#endif  // OTHERLIB_TOOLS_TOOL_HPP