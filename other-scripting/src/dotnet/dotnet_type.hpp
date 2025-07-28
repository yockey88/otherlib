/**
 * \file dotnet/dotnet_type.hpp
 **/
#ifndef OTHER_ENGINE_SCRIPTING_DOTNET_DOTNET_TYPE_HPP
#define OTHER_ENGINE_SCRIPTING_DOTNET_DOTNET_TYPE_HPP

#include <cstdint>
#include <string>
#include <vector>

namespace other {

  class dotnet_host;

  class dotnet_type {
   public:
    dotnet_type(dotnet_host* host, int32_t type_id)
        : host(host), dotnet_id(type_id) {}
    ~dotnet_type() {}

    std::string full_name();

    void initialize_type_interface();

   private:
    dotnet_host* host = nullptr;

    std::vector<int32_t> method_dotnet_ids = {};
    std::vector<int32_t> field_dotnet_ids = {};
    std::vector<int32_t> property_dotnet_ids = {};
    std::vector<int32_t> attribute_dotnet_ids = {};

    int32_t dotnet_id = -1;
    bool type_interface_initialized = false;
  };

}  // namespace other

#endif  // OTHER_ENGINE_SCRIPTING_DOTNET_DOTNET_TYPE_HPP