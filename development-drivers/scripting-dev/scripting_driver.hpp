// /**
//  * \file scripting-dev/scripting_driver.hpp
//  **/
#ifndef OTHER_SCRIPTING_DRIVER_HPP
#define OTHER_SCRIPTING_DRIVER_HPP

#include <dotnet/hostfxr.h>

#include "core/defines.hpp"

#include "dotnet/host.hpp"
#include "scene/scene.hpp"

#include "driver/driver.hpp"

namespace other {

  struct mouse_state {
    glm::vec2 position = { 0, 0 };
    glm::vec2 delta = { 0, 0 };
  };

  class OTHER_CLASS scripting_driver : public driver {
   public:
    scripting_driver(const config_table& config)
        : driver(config) {}
    virtual ~scripting_driver() = default;

    void on_initialize() override;
    void run() override;
    void on_shutdown() override;

   private:
    dotnet_host dotnet;
    scene active_scene;

    natural_t object_id = 0;
    ref<assembly> other_assembly = nullptr;
    ref<assembly> testing_assembly = nullptr;

    void on_event(SDL_Event* event) override;
  };

}  // namespace other

OTHER_DRIVER(other::scripting_driver)

#endif  // OTHER_SCRIPTING_DRIVER_HPP