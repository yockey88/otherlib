/**
 * \file event/event.hpp
 **/
#ifndef OTHER_CORE_EVENT_EVENT_HPP
#define OTHER_CORE_EVENT_EVENT_HPP

#include <functional>

#include "core/defines.hpp"
#include "core/timer.hpp"
#include "core/value.hpp"

namespace other {

  struct event {
    natural_t id = 0;
    std::string name;

    value data;

    microsecond duration = microsecond::zero();
    bool recurring = false;

    using handler = std::function<void(const value&)>;
  };

}  // namespace other

#endif  // OTHER_CORE_EVENT_EVENT_HPP