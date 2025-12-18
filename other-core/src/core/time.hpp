/**
 * \file core/time.hpp
 **/
#ifndef OTHER_CORE_CORE_TIME_HPP
#define OTHER_CORE_CORE_TIME_HPP

#include <chrono>

#include "core/defines.hpp"

namespace other {

  using microseconds = std::chrono::microseconds;
  using milliseconds = std::chrono::milliseconds;
  using seconds = std::chrono::seconds;
  using fseconds = std::chrono::duration<float>;

  /// 10,000 ticks per second
  using tick_conversion = std::ratio<1, 10000>;
  using tick_duration = std::chrono::duration<natural_t, tick_conversion>;

  using delta_time_duration = fseconds;
  using frame_time_point = std::chrono::time_point<std::chrono::steady_clock, delta_time_duration>;

  using sys_clock = std::chrono::system_clock;
  using steady_clock = std::chrono::steady_clock;
  using highres_clock = std::chrono::high_resolution_clock;
  using steady_timepoint = std::chrono::time_point<steady_clock>;
  using system_timepoint = std::chrono::time_point<sys_clock>;
  using highres_timepoint = std::chrono::time_point<highres_clock>;

  template <typename D>
  decltype(auto) duration_cast(auto from_duration) {
    return std::chrono::duration_cast<D>(from_duration);
  }

}  // namespace other

#endif  // OTHER_CORE_CORE_TIME_HPP