/**
 * \file audio_device/audio_device.cpp
 **/
#include "audio_device/audio_device.hpp"

namespace other {

  /**
    device ex:
      ma_device_config config = ma_device_config_init(ma_device_type_playback);
      config.playback.format   = ma_format_f32;   // Set to ma_format_unknown to use the device's native format.
      config.playback.channels = 2;               // Set to 0 to use the device's native channel count.
      config.sampleRate        = 48000;           // Set to 0 to use the device's native sample rate.
      config.dataCallback      = data_callback;   // This function will be called when miniaudio needs more data.
      config.pUserData         = pMyCustomData;   // Can be accessed from the device object (device.pUserData).

      ma_device device;
      if (ma_device_init(NULL, &config, &device) != MA_SUCCESS) {
          return -1;  // Failed to initialize the device.
      }

      ma_device_start(&device);     // The device is sleeping by default so you'll need to start it manually.

      // Do something here. Probably your program's main loop.

      ma_device_uninit(&device);

    device:
      ma_device_init()
      ma_device_init_ex()
      ma_device_uninit()
      ma_device_start()
      ma_device_stop()

    device type:
      ma_device_type_playback- Write to output buffer, leave input buffer untouched.
      ma_device_type_capture- Read from input buffer, leave output buffer untouched.
      ma_device_type_duplex- Read from input buffer, write to output buffer.
      ma_device_type_loopback- Read from input buffer, leave output buffer untouched.

    formats:
      ma_format_f32
      ma_format_s16
      ma_format_s24
      ma_format_s32
      ma_format_u8
  */

  audio_device::audio_device() {
    // ma_device_config config = ma_device_config_init(ma_device_type_playback);
    // config.playback.pDeviceID = &playback_infos[chosenPlaybackDeviceIndex].id;
    // config.playback.format = MY_FORMAT;
    // config.playback.channels = MY_CHANNEL_COUNT;
    // config.sampleRate = MY_SAMPLE_RATE;
    // config.dataCallback = data_callback;
    // config.pUserData = pMyCustomData;

    // ma_device device;
    // if (ma_device_init(&context, &config, &device) != MA_SUCCESS) {
    //   // Error
    // }
  }

  audio_device::~audio_device() {
    // ma_device_uninit(&device);
  }

}  // namespace other