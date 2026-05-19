/**
 * \file audio/audio_engine.cpp
 **/
#include "audio/audio_engine.hpp"

#include "core/enum_formatter.hpp"
#include "core/logger.hpp"
#include "core/subsystem.hpp"

namespace other {

  void audio_engine::initialize() {
    PROFILE_SECTION("audio_engine::initialize");

    ma_resource_manager_config config = ma_resource_manager_config_init();
    config.decodedFormat = ma_format_f32;
    // Use the same number of channels as the source file.
    config.decodedChannels = 0;
    config.decodedSampleRate = 48000;

    const bool res_manager_success = ma_resource_manager_init(&config, &resource_manager) == MA_SUCCESS;
    OTHER_ASSERT(res_manager_success, "Failed to initialize audio resource manager");

    const bool init_success = ma_context_init(NULL, 0, NULL, &context) == MA_SUCCESS;
    OTHER_ASSERT(init_success, "Failed to initialize audio context");
  }

  void audio_engine::shutdown() {
    PROFILE_SECTION("audio_engine::shutdown");
    ma_context_uninit(&context);
  }

  // void audio_engine::enumerate_and_load_audio_devices() {
  // }
  // CORE_LOG_TRACE("[AUDIO] Enumerating audio devices");
  // ma_uint32 playback_count;
  // ma_uint32 capture_count;

  // ma_device_info* playback_infos;
  // ma_device_info* capture_infos;

  // if (ma_context_get_devices(&context, &playback_infos, &playback_count, &capture_infos, &capture_count) != MA_SUCCESS) {
  //   CORE_LOG_ERROR("Failed to get audio devices");
  //   return;
  // }
  // CORE_LOG_DEBUG("[AUDIO] Found {} playback devices and {} capture devices", playback_count, capture_count);

}  // namespace other