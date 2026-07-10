/**
 * \file dotnet/dotnet_method.hpp
 **/
#ifndef OTHER_SCRIPTING_DOTNET_DOTNET_METHOD_HPP
#define OTHER_SCRIPTING_DOTNET_DOTNET_METHOD_HPP

#include <cstdint>
#include <string>

#include "dotnet/dotnet_attribute.hpp"

namespace other {

  class dotnet_host;
  class dotnet_type;

  class dotnet_method {
   public:
    dotnet_method(dotnet_host* host, dotnet_type* type, int32_t method_id)
        : dotnet_id(method_id), host(host), type(type) {}
    ~dotnet_method() {}

    dotnet_type* get_return_type();

    void initialize_method();

    bool is_static() const;

    bool has_attribute(const std::string_view attr_name) const;
    void get_attribute(const std::string_view attr_name, const std::string_view field_name, void* out) const;
    ostd::vector<std::string> get_attribute_names() const;

    std::string name() const;

    int32_t dotnet_id = -1;

   private:
    dotnet_host* host = nullptr;
    dotnet_type* type = nullptr;
    dotnet_type* return_type = nullptr;

    ostd::vector<dotnet_attribute> attributes;
  };

}  // namespace other

#endif  // OTHER_SCRIPTING_DOTNET_DOTNET_METHOD_HPP