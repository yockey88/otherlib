/**
 * \file dotnet/dotnet_type.hpp
 **/
#ifndef OTHER_ENGINE_SCRIPTING_DOTNET_DOTNET_TYPE_HPP
#define OTHER_ENGINE_SCRIPTING_DOTNET_DOTNET_TYPE_HPP

#include <cstdint>
#include <string>
#include <vector>

#include "dotnet/dotnet_attribute.hpp"
#include "dotnet/dotnet_field.hpp"
#include "dotnet/dotnet_method.hpp"
#include "dotnet/interop_interface.hpp"
#include "dotnet/types.hpp"

namespace other {

  class dotnet_host;
  class dotnet_object;

  class dotnet_type {
   public:
    dotnet_type(dotnet_host* host, int32_t type_id)
        : dotnet_id(type_id), host(host) {}
    ~dotnet_type() {}

    void initialize_type_interface();
    std::string full_name() const;
    std::string namespace_name() const;
    std::string class_name() const;

    bool has_attribute(const std::string_view attr_name) const;
    ostd::vector<std::string> get_attribute_names() const;

    template <typename T>
    T get_attribute(const std::string_view attr_name, const std::string_view field_name) {
      OTHER_ASSERT(host != nullptr, "dotnet_host is null");
      T val{};
      if (std::is_pointer_v<T>) {
        get_attribute_object(attr_name, field_name, (void*)val);
      } else {
        get_attribute_object(attr_name, field_name, &val);
      }
      return val;
    }

    bool has_field(const std::string_view field_name) const;
    bool is_field_property(const std::string_view field_name) const;

    bool has_method(const std::string_view method_name) const;
    // dotnet_method* get_method(int32_t method_id);

    dotnet_object* instantiate_object(const std::string_view name, const void** argv, const managed_type* arg_ts, size_t argc);
    void destroy_object(dotnet_object* obj);

    const ostd::vector<dotnet_method>& get_methods() {
      if (!type_interface_initialized) {
        initialize_type_interface();
      }
      return dotnet_methods;
    }
    const ostd::vector<dotnet_field>& get_fields() {
      if (!type_interface_initialized) {
        initialize_type_interface();
      }
      return dotnet_fields;
    }

    int32_t dotnet_id = -1;

   private:
    dotnet_host* host = nullptr;

    ostd::vector<dotnet_method> dotnet_methods = {};
    ostd::vector<dotnet_field> dotnet_fields = {};
    ostd::vector<dotnet_attribute> dotnet_attributes = {};

    bool type_interface_initialized = false;

    void fill_out_type_information(ostd::vector<int32_t>& dotnet_ids, get_type_information fn);

    void get_attribute_object(const std::string_view name, const std::string_view field_name, void* out) const;
  };

}  // namespace other

#endif  // OTHER_ENGINE_SCRIPTING_DOTNET_DOTNET_TYPE_HPP