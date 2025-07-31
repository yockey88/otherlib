/**
 * \file dotnet/dotnet_method.hpp
 **/
#ifndef OTHER_SCRIPTING_DOTNET_DOTNET_METHOD_HPP
#define OTHER_SCRIPTING_DOTNET_DOTNET_METHOD_HPP

#include <cstdint>
#include <string>

namespace other {

  class dotnet_host;
  class dotnet_type;

  class dotnet_method {
   public:
    dotnet_method(dotnet_host* host, dotnet_type* type, int32_t method_id)
        : host(host), type(type), dotnet_id(method_id) {}
    ~dotnet_method() {}

    dotnet_type* get_return_type();

    std::string name();

   private:
    dotnet_host* host = nullptr;
    dotnet_type* type = nullptr;
    dotnet_type* return_type = nullptr;

    int32_t dotnet_id = -1;
  };

}  // namespace other

#endif  // OTHER_SCRIPTING_DOTNET_DOTNET_METHOD_HPP