/**
 * \file script/script_object.hpp
 **/
#ifndef OTHER_SCRIPTING_SCRIPT_SCRIPT_OBJECT_HPPP
#define OTHER_SCRIPTING_SCRIPT_SCRIPT_OBJECT_HPPP

#include "core/defines.hpp"

namespace other {

  class script_object {
   public:
    script_object() = default;
    ~script_object() = default;

    integer_t id = -1;

   private:
  };

}  // namespace other

#endif  // OTHER_SCRIPTING_SCRIPT_SCRIPT_OBJECT_HPPP