/**
 * \file dotnet/dotnet_field.hpp
 **/
#ifndef OTHER_SCRIPTING_DOTNET_DOTNET_FIELD_HPP
#define OTHER_SCRIPTING_DOTNET_DOTNET_FIELD_HPP

#include <cstdint>
#include <string>

#include "core/defines.hpp"
#include "core/logger.hpp"
#include "serialization/reflection.hpp"

#include "dotnet/native_string.hpp"

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
        OTHER_ASSERT(stored_type == get_value_type<FT>(), "Field type mismatch: expected {}, got {}", get_value_type<FT>(), stored_type);

        if constexpr (is_stringlike_type<FT>) {
          return std::string(reinterpret_cast<const char*>(data), size - 1);
        } else {
          return *reinterpret_cast<const FT*>(data);
        }
      }

      template <typename FT>
        requires std::is_copy_constructible_v<FT>
      void set_as(const FT& value) {
        OTHER_ASSERT(data != nullptr, "Field data is null");
        OTHER_ASSERT(stored_type == get_value_type<FT>(), "Field type mismatch: expected {}, got {}", get_value_type<FT>(), stored_type);

        if constexpr (is_stringlike_type<FT>) {
          copy_string_to_storage(value);
        } else {
          *reinterpret_cast<FT*>(data) = value;
        }
      }

      void load_from_bytes(const uint8_t* data, uint64_t size);

     private:
      void copy_string_to_storage(const std::string& value);
    };
    dotnet_field(dotnet_host* host, dotnet_type* type, int32_t dotnet_id, bool is_property = false)
        : host(host), type(type), dotnet_id(dotnet_id), flags{ is_property } {}
    ~dotnet_field() {}

    void initialize_field();

    inline bool is_property() const {
      return flags.is_property;
    }

    std::string name() const;

    value_type get_type() const;

   private:
    dotnet_host* host = nullptr;
    dotnet_type* type = nullptr;

    int32_t dotnet_id = 0;

    value_type valtype = value_type::EMPTY_TYPE;

    struct field_flags {
      bool is_property = false;
    } flags;
  };

}  // namespace other

#endif  // OTHER_SCRIPTING_DOTNET_DOTNET_FIELD_HPP