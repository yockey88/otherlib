/**
 * \file dotnet/dotnet_assembly.hpp
 **/
#ifndef OTHER_SCRIPTING_DOTNET_DOTNET_ASSEMBLY_HPP
#define OTHER_SCRIPTING_DOTNET_DOTNET_ASSEMBLY_HPP

#include <cstdint>
#include <map>
#include <string>

#include <dotnet/nethost.h>

#include "core/ref.hpp"

namespace other {

  enum class assembly_load_status {
    UNKNOWN_ERROR = -1,
    SUCCESS = 0,
    ASSEMBLY_NOT_FOUND = 1,
    ASSEMBLY_LOAD_FAILED = 2,
    ASSEMBLY_ALREADY_LOADED = 3,
    ASSEMBLY_UNLOAD_FAILED = 4
  };

  struct native_function_call {
    const char_t* name;
    void* native_function;
  };

  class dotnet_host;
  class dotnet_type;
  class type_cache;

  class assembly : public ref_counted {
   public:
    assembly(const std::string_view name, natural_t handle, dotnet_host* host)
        : name(name), handle(handle), host(host) {}

    void cache_types(type_cache* cache, const std::vector<int32_t>& dotnet_type_ids);

    natural_t get_handle() const {
      return handle;
    }
    const std::string& get_name() const {
      return name;
    }

    int32_t dotnet_id = -1;
    std::string dotnet_name;
    assembly_load_status load_status = assembly_load_status::UNKNOWN_ERROR;

   private:
    friend class assembly_context;

    std::string name;
    natural_t handle = 0;

    dotnet_host* host = nullptr;

    std::vector<dotnet_type*> types;

    // AssemblyLoadStatus load_status = AssemblyLoadStatus::UNKNOWN_ERROR;
    // std::vector<dostring> internal_call_names = {};
    // std::vector<InternalCall> internal_calls = {};
    // std::vector<Type*> types = {};

    void add_call(const std::string& name, void* fn);
  };

  class dotnet_host;

  class assembly_context {
   public:
    assembly_context(const std::string_view name, natural_t handle, dotnet_host* host)
        : handle(handle), name(name), host(host) {}

    ref<assembly> load_assembly(const std::string_view path);
    ref<assembly> get_assembly_by_name(const std::string_view name);
    void unload_assembly(natural_t assembly_id);
    void unload_all();

    natural_t get_handle() const {
      return handle;
    }
    const std::string& get_name() const {
      return name;
    }
    size_t num_assemblies() const {
      return assemblies.size();
    }

    int32_t dotnet_id = -1;

   private:
    natural_t handle = 0;
    std::string name;

    dotnet_host* host = nullptr;

    std::map<natural_t, ref<assembly>> assemblies{};
  };

}  // namespace other

#endif  // OTHER_SCRIPTING_DOTNET_DOTNET_ASSEMBLY_HPP