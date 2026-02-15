/**
 * \file audio/audio_engine.hpp
 **/
#ifndef OTHER_AUDIO_AUDIO_AUDIO_ENGINE_HPP
#define OTHER_AUDIO_AUDIO_AUDIO_ENGINE_HPP

#include <miniaudio/miniaudio.h>

#include "core/subsystem.hpp"

namespace other {

  class audio_engine : public subsystem<audio_engine> {
   public:
    audio_engine() = default;
    ~audio_engine() override = default;

    void initialize();
    void shutdown();

   private:
    ma_engine engine;
  };

}  // namespace other

#endif  // OTHER_AUDIO_AUDIO_AUDIO_ENGINE_HPP