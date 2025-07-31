/**
 * \file dotnet/dotnet_attribute.hpp
 **/
#ifndef OTHER_SCRIPTING_DOTNET_DOTNET_ATTRIBUTE_HPP
#define OTHER_SCRIPTING_DOTNET_DOTNET_ATTRIBUTE_HPP

#include <string>

namespace other {

  class dotnet_host;
  class dotnet_type;

  class dotnet_attribute {
   public:
    dotnet_attribute(dotnet_host* host, int32_t type_id, int32_t dotnet_id)
        : host(host), type_dotnet_id(type_id), dotnet_id(dotnet_id) {}
    ~dotnet_attribute() {}

    std::string name() const;

    dotnet_type* type = nullptr;

   private:
    dotnet_host* host = nullptr;

    int32_t type_dotnet_id = -1;
    int32_t dotnet_id = 0;
  };

}  // namespace other

#endif  // OTHER_SCRIPTING_DOTNET_DOTNET_ATTRIBUTE_HPP