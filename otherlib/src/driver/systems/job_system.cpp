/**
 * \file driver/systems/job_system.cpp
 **/
#include "driver/systems/job_system.hpp"

namespace other {

  void job_system::initialize(driver_kernel* kernel) {
  }

  void job_system::tick(driver_kernel* kernel, double dt) {
    for (auto it = live_coroutines.begin(); it != live_coroutines.end();) {
      it->handle();
      if (it->handle.coro_handle.done()) {
        it->handle.coro_handle.destroy();
        it = live_coroutines.erase(it);
      } else {
        ++it;
      }
    }
  }

  void job_system::shutdown(driver_kernel* kernel) {
    live_coroutines.clear();
  }

  void job_system::post_coroutine(task coro) {
    live_coroutines.push_back({ .handle = std::move(coro) });
  }

}  // namespace other