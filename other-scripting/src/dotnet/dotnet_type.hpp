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
#include "dotnet/dotnet_object.hpp"
#include "dotnet/types.hpp"

#include "interop_interface.hpp"

namespace other {

  class dotnet_host;

  class dotnet_type {
   public:
    dotnet_type(dotnet_host* host, int32_t type_id)
        : dotnet_id(type_id), host(host) {}
    ~dotnet_type() {}

    void initialize_type_interface();
    std::string full_name() const;

    bool has_attribute(const std::string_view attr_name) const;
    std::vector<std::string> get_attribute_names() const;

    bool has_field(const std::string_view field_name) const;
    bool is_field_property(const std::string_view field_name) const;

    // dotnet_method* get_method(int32_t method_id);

    dotnet_object* instantiate_object(const std::string_view name, const void** argv, const managed_type* arg_ts, size_t argc);
    void destroy_object(dotnet_object* obj);

    int32_t dotnet_id = -1;

   private:
    dotnet_host* host = nullptr;

    std::vector<dotnet_method> dotnet_methods = {};
    std::vector<dotnet_field> dotnet_fields = {};

    struct attribute_data {
      int32_t type_dotnet_id = -1;
      dotnet_attribute attribute;
    };
    std::vector<attribute_data> dotnet_attributes = {};

    bool type_interface_initialized = false;

    void fill_out_type_information(std::vector<int32_t>& dotnet_ids, get_type_information fn);
  };

}  // namespace other

#endif  // OTHER_ENGINE_SCRIPTING_DOTNET_DOTNET_TYPE_HPP