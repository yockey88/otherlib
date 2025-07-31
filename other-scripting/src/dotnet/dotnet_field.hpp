/**
 * \file dotnet/dotnet_field.hpp
 **/
#ifndef OTHER_SCRIPTING_DOTNET_DOTNET_FIELD_HPP
#define OTHER_SCRIPTING_DOTNET_DOTNET_FIELD_HPP

#include <cstdint>
#include <string>

#include "core/defines.hpp"
#include "core/logger.hpp"

namespace other {

  class dotnet_host;
  class dotnet_type;

  class dotnet_field {
   public:
    struct storage {
      value_type stored_type;
      uint8_t* data = nullptr;
      size_t size = 0;

      template <typename FT>
        requires std::is_copy_constructible_v<FT>
      FT get_as() const {
        OTHER_ASSERT(data != nullptr, "Field data is null");
        OTHER_ASSERT(size == sizeof(FT), "Field data size is not equal to requested type size");
        return *reinterpret_cast<const FT*>(data);
      }
    };
    dotnet_field(dotnet_host* host, dotnet_type* type, int32_t dotnet_id, bool is_property = false)
        : host(host), type(type), dotnet_id(dotnet_id), flags{ is_property } {}
    ~dotnet_field() {}

    inline bool is_property() const {
      return flags.is_property;
    }

    std::string name() const;

   private:
    dotnet_host* host = nullptr;
    dotnet_type* type = nullptr;
    dotnet_type* field_type = nullptr;

    int32_t dotnet_id = 0;

    struct field_flags {
      bool is_property = false;
    } flags;
  };

}  // namespace other

#endif  // OTHER_SCRIPTING_DOTNET_DOTNET_FIELD_HPP