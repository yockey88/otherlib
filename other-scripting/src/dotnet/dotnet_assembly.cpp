/**
 * \file dotnet/dotnet_assembly.cpp
 **/
#include "dotnet/dotnet_assembly.hpp"

#include "core/fnv.hpp"
#include "core/logger.hpp"

#include "dotnet/dotnet_type.hpp"
#include "dotnet/host.hpp"
#include "dotnet/native_string.hpp"
#include "dotnet/type_cache.hpp"

namespace other {

  void assembly::cache_types(type_cache* cache, const std::vector<int32_t>& dotnet_type_ids) {
    for (auto id : dotnet_type_ids) {
      types.emplace_back(cache->cache_type(host, id));
    }

    CORE_LOG_DEBUG("Loaded [{}] types from assembly {}", types.size(), handle);
  }

  bool assembly::has_method(const std::string_view type_name, const std::string_view method_name) const {
    auto* t = host->get_type_cache()->get_type(type_name);
    if (t == nullptr) {
      return false;
    }

    return t->has_method(method_name);
  }

  ref<assembly> assembly_context::load_assembly(const std::string_view path) {
    OTHER_ASSERT(host != nullptr, "DotNet host is not initialized.");
    OTHER_ASSERT(!path.empty(), "Assembly path cannot be empty.");

    PROFILE_SECTION("assembly_context::load-assembly");

    filepath asm_path{ path };
    if (std::filesystem::exists(asm_path) && !std::filesystem::is_regular_file(asm_path)) {
      CORE_LOG_ERROR("Provided path '{}' is not a regular file.", asm_path.string());
      return nullptr;
    }

    natural_t assembly_handle = FNV(asm_path.string());
    {
      auto itr = assemblies.find(assembly_handle);
      if (itr != assemblies.end()) {
        CORE_LOG_ERROR("Assembly with handle {} already loaded.", assembly_handle);
        return nullptr;
      }
    }

    std::string name = asm_path.filename().stem().string();
    auto [itr, inserted] = assemblies.insert({ assembly_handle, make_ref<assembly>(name, assembly_handle, host) });
    OTHER_ASSERT(inserted, "Failed to insert assembly into context map");

    auto asm_ref = itr->second;
    CORE_LOG_INFO("Loading assembly [{}:{}] from path: {}", asm_ref->get_handle(), asm_ref->get_name(), asm_path.string());

    native_string native_path = native_string::new_str(asm_path.string());
    asm_ref->dotnet_id = host->interop().load_managed_assembly(this->dotnet_id, native_path);
    if (asm_ref->dotnet_id == -1) {
      CORE_LOG_ERROR("Failed to load assembly from path: {}", asm_path.string());
    }

    asm_ref->load_status = host->interop().get_last_load_status();
    if (asm_ref->load_status == assembly_load_status::SUCCESS) {
      auto asm_name = host->interop().get_assembly_name(asm_ref->dotnet_id);
      asm_ref->dotnet_name = asm_name;
      native_string::free_str(asm_name);

      int32_t type_counter = 0;
      host->interop().get_assembly_types(asm_ref->dotnet_id, nullptr, &type_counter);

      std::vector<int32_t> type_ids;
      type_ids.resize(type_counter);
      host->interop().get_assembly_types(asm_ref->dotnet_id, type_ids.data(), &type_counter);

      auto* types = host->get_type_cache();
      OTHER_ASSERT(types != nullptr, "Failed to get type cache!");

      asm_ref->cache_types(types, type_ids);
      CORE_LOG_INFO("Loaded assembly [{}:{}]", itr->second->get_handle(), itr->second->get_name());
    } else {
      CORE_LOG_ERROR("Failed to load assembly [{}:{}] | status: {}", asm_ref->get_handle(), asm_ref->get_name(), asm_ref->load_status);
    }

    native_string::free_str(native_path);
    return itr->second;
  }

  ref<assembly> assembly_context::get_assembly_by_name(const std::string_view name) {
    CORE_LOG_TRACE("Searching for assembly by name: {}", name);
    for (const auto& [id, asm_ref] : assemblies) {
      if (asm_ref->get_name() == name) {
        CORE_LOG_TRACE("Found assembly [{}:{}]", asm_ref->get_handle(), asm_ref->get_name());
        return asm_ref;
      }
    }
    CORE_LOG_ERROR("Assembly with name '{}' not found in context [{}:{}]", name, handle, this->name);
    return nullptr;
  }

  void assembly_context::unload_assembly(natural_t assembly_id) {
    auto itr = assemblies.find(assembly_id);
    if (itr != assemblies.end()) {
      CORE_LOG_INFO("Unloading assembly [{}:{}]", itr->second->get_handle(), itr->second->get_name());

      /// remove types from type cache
      for (auto* type : itr->second->types) {
        host->get_type_cache()->remove_type(type->dotnet_id);
      }

      host->interop().unload_managed_assembly(itr->second->dotnet_id);
      assemblies.erase(itr);
    } else {
      CORE_LOG_ERROR("Failed to unload assembly: ID {} not found", assembly_id);
    }
  }

  void assembly_context::unload_all() {
    CORE_LOG_DEBUG("Unloading all assemblies from context [{}:{}]", handle, name);
    for (auto& [id, asm_ref] : assemblies) {
      unload_assembly(id);
    }
    assemblies.clear();
  }

}  // namespace other