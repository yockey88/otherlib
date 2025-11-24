/**
 * \file core/timer.hpp
 **/
#ifndef OTHERLIB_CORE_TIMER_HPP
#define OTHERLIB_CORE_TIMER_HPP

#include <thread>

#include "core/defines.hpp"

namespace other {

  using microseconds = std::chrono::microseconds;
  using milliseconds = std::chrono::milliseconds;
  using seconds = std::chrono::seconds;
  using fseconds = std::chrono::duration<float>;

  /// 10,000 ticks per second
  using tick_conversion = std::ratio<1, 10000>;
  using tick_duration = std::chrono::duration<natural_t, tick_conversion>;

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

  template <integer_t FPS = 60>
  struct frame_rate_enforcer {
    /// ms
    static constexpr natural_t kFrameDuration = 1000 / FPS;

    frame_rate_enforcer() {}

    void wait() {
      auto now = std::chrono::steady_clock::now();
      if (last_frame_time.time_since_epoch().count() == 0) {
        last_frame_time = now;
        return;
      }

      auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_frame_time).count();
      if (elapsed < kFrameDuration) {
        std::this_thread::sleep_for(std::chrono::milliseconds(kFrameDuration - elapsed));
      }
      last_frame_time = std::chrono::steady_clock::now();
    }

   private:
    std::chrono::time_point<std::chrono::steady_clock> last_frame_time;
  };

}  // namespace other

#endif  // OTHERLIB_CORE_TIMER_HPP