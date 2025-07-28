/**
 * \file script/scripting_environment.hpp
 **/
#ifndef OTHER_SCRIPTING_SCRIPT_SCRIPTING_ENVIRONMENT_HPP
#define OTHER_SCRIPTING_SCRIPT_SCRIPTING_ENVIRONMENT_HPP

#include "core/memory_pool.hpp"
#include "core/ref.hpp"
#include "core/subsystem.hpp"

#include "script/script_object.hpp"

#include "dotnet/host.hpp"

namespace other {

  class scripting_environment : public subsystem<scripting_environment> {
   public:
    scripting_environment() = default;
    virtual ~scripting_environment() = default;

    void initialize_script_environment();
    void shutdown_script_environment();

    integer_t create_object(const std::string_view name);
    void destroy_object(integer_t id);

    script_object* get_object(integer_t id);

    ref<assembly> load_dotnet_module(const std::string_view module_path);
    void unload_dotnet_module(ref<assembly> module_id);

    constexpr static inline size_t kMaxScriptObjects = memory_pool<script_object>::kMaxObjects;

   private:
    struct live_script_object {
      size_t index = 0;
      script_object* object = nullptr;
    };

    assembly_context* dotnet_load_context = nullptr;
    dotnet_host dotnet;

    std::array<live_script_object, kMaxScriptObjects> live_objects = {};
    ref<memory_pool<script_object>> script_object_pool = nullptr;
  };

}  // namespace other

OTHER_SUBSYSTEM(other::scripting_environment);

#endif  // OTHER_SCRIPTING_SCRIPT_SCRIPTING_ENVIRONMENT_HPP