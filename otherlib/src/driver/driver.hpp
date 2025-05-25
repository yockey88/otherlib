/**
 * \file driver/driver.hpp
 **/
#ifndef OTHER_DRIVER_DRIVER_HPP
#define OTHER_DRIVER_DRIVER_HPP

#include "core/config_table.hpp"
#include "core/defines.hpp"

#include "plugin/plugin.hpp"

namespace other {

  class OTHER_CLASS driver {
   public:
    driver(const config_table& config)
        : config(config) {}
    virtual ~driver() = default;

    virtual void initialize() = 0;
    virtual void run() = 0;
    virtual void shutdown() = 0;

   protected:
    const config_table& configuration() const {
      return config;
    }

   private:
    config_table config;
  };

#ifndef DRIVER_NEW
  #define DRIVER_NEW(name, config) new name(*config)
#endif
#ifndef DRIVER_DELETE
  #define DRIVER_DELETE(instance) delete instance
#endif

#define OTHER_DRIVER(name)                                                                                       \
  OTHER_PLUGIN(pbrt_sandbox)                                                                                     \
  OTHER_API other::driver* create_driver(const other::config_table* config) { return DRIVER_NEW(name, config); } \
  OTHER_API void destroy_driver(other::driver* instance) { DRIVER_DELETE(instance); }

}  // namespace other

#endif  // OTHER_DRIVER_DRIVER_HPP