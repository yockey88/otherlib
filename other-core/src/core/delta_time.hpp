/**
 * \file core/delta_time.hpp
 **/
#ifndef OTHER_CORE_CORE_DELTA_TIME_HPP
#define OTHER_CORE_CORE_DELTA_TIME_HPP

#include "core/time.hpp"

namespace other {

  struct delta_time {
    steady_clock::time_point last_time = steady_clock::now();
    steady_clock::time_point current_time = steady_clock::now();

    delta_time_duration frame_duration;

    double get_no_update() {
      return frame_duration.count();
    }

    operator double() {
      current_time = steady_clock::now();
      frame_duration = current_time - last_time;
      last_time = current_time;
      return frame_duration.count();
    }
  };

}  // namespace other

#endif  // OTHERLIB_CORE_DELTA_TIME_HPP