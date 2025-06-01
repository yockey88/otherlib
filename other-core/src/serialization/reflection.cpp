/**
 * \file serialization/reflection.cpp
 **/
#include "serialization/reflection.hpp"

namespace other {

  std::string reflection_data::member::get_name() const {
    if (display_name.has_value()) {
      return *display_name;
    }
    return name;
  }

};  // namespace other