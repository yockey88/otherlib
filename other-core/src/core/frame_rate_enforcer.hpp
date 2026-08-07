/**
 * \file core/frame_rate_enforcer.hpp
 **/
#ifndef OTHERLIB_CORE_FRAME_RATE_ENFORCER_HPP
#define OTHERLIB_CORE_FRAME_RATE_ENFORCER_HPP

#include <thread>

#include "core/profiler.hpp"
#include "core/time.hpp"

namespace other {

  template <integer_t FPS = 60>
  struct frame_rate_enforcer {
    /// ms
    static constexpr natural_t kFrameDuration = 1000 / FPS;

    frame_rate_enforcer() {}

    void wait() {
      PROFILE_SECTION("frame_rate_enforcer::wait");
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

#endif  // OTHERLIB_CORE_FRAME_RATE_ENFORCER_HPP