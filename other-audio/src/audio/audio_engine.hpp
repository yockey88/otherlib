/**
 * \file audio/audio_engine.hpp
 **/
#ifndef OTHER_AUDIO_AUDIO_AUDIO_ENGINE_HPP
#define OTHER_AUDIO_AUDIO_AUDIO_ENGINE_HPP

#include <miniaudio/miniaudio.h>

#include "core/ref.hpp"
#include "core/subsystem.hpp"

#include "audio_device/audio_device.hpp"

namespace other {

  class audio_engine : public subsystem<audio_engine> {
   public:
    audio_engine() = default;
    ~audio_engine() override = default;

    void initialize();
    void shutdown();

   private:
    ma_resource_manager resource_manager;
    ma_context context;

    // natural_t next_device_id = 1;
    // natural_t generate_device_id() {
    //   return next_device_id++;
    // }
  };

}  // namespace other

OTHER_DEPENDENT_SUBSYSTEM(
  other::audio_engine,
  subsystem_profile::kArena,
  subsystem_profile::kLogger
);

#endif  // OTHER_AUDIO_AUDIO_AUDIO_ENGINE_HPP