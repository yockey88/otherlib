/**
 * \file audio/audio_engine.cpp
 **/
#include "audio/audio_engine.hpp"

#include "core/logger.hpp"
#include "core/subsystem.hpp"

namespace other {

  void audio_engine::initialize() {
    PROFILE_SECTION("audio_engine::initialize");
    ma_result result = ma_engine_init(nullptr, &engine);
    if (result != MA_SUCCESS) {
      CORE_LOG_ERROR("Failed to initialize audio engine: error code {}", result);
      return;
    }
  }

  void audio_engine::shutdown() {
    PROFILE_SECTION("audio_engine::shutdown");
    ma_engine_uninit(&engine);
  }

}  // namespace other