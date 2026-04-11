/**
 * \file scripting/dotnet_bindings/driver_bindings.cpp
 **/
#include "scripting/dotnet_bindings/driver_bindings.hpp"

#include "core/logger.hpp"

#include "driver/driver.hpp"

namespace other {
  namespace bindings {
    namespace {
      static driver* s_driver = nullptr;
    }  // namespace
  }  // namespace bindings
  namespace detail {

    driver* get_dotnet_native_driver_unchecked() {
      return bindings::s_driver;
    }

    driver* get_dotnet_native_driver() {
      OTHER_ASSERT(bindings::s_driver != nullptr, "Driver pointer is null.");
      return bindings::s_driver;
    }

    void set_dotnet_native_driver(driver* drv) {
      OTHER_ASSERT(drv != nullptr, "Driver pointer is null.");
      bindings::s_driver = drv;
    }

  }  // namespace detail
  namespace bindings {

    int32_t native_driver_get_state() {
      OTHER_ASSERT(s_driver != nullptr, "Driver pointer is null.");
      return static_cast<int32_t>(s_driver->current_driver_state());
    }

    void native_driver_request_shutdown() {
      OTHER_ASSERT(s_driver != nullptr, "Driver pointer is null.");
      s_driver->request_shutdown();
    }

    native_string native_driver_get_project_name() {
      OTHER_ASSERT(s_driver != nullptr, "Driver pointer is null.");
      std::string project_name = s_driver->get_project_name();
      return native_string::new_str(project_name);
    }

  }  // namespace bindings
}  // namespace other