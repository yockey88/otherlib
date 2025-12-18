/**
 * \file event/event.hpp
 **/
#ifndef OTHER_CORE_EVENT_EVENT_HPP
#define OTHER_CORE_EVENT_EVENT_HPP

#include <functional>

#include "core/defines.hpp"
#include "core/time.hpp"
#include "core/value.hpp"

namespace other {

  struct event {
    natural_t id = 0;
    std::string name;

    value data;

    microseconds duration = microseconds::zero();
    bool recurring = false;

    using handler = std::function<void(const value&)>;
  };

}  // namespace other

#endif  // OTHER_CORE_EVENT_EVENT_HPP