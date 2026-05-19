/**
 * \file audio_device/audio_device.hpp
 **/
#ifndef OTHER_AUDIO_DEVICE_AUDIO_DEVICE_HPP
#define OTHER_AUDIO_DEVICE_AUDIO_DEVICE_HPP

#include <miniaudio/miniaudio.h>

#include "core/ref_counted.hpp"

namespace other {

  class audio_device : public ref_counted {
   public:
    enum type {
      PLAYBACK,
      CAPTURE,
    };

    audio_device();
    ~audio_device();

    inline type get_device_type() const { return device_type; }

   private:
    type device_type;
    ma_device device;
  };

}  // namespace other

#endif  // OTHER_AUDIO_DEVICE_AUDIO_DEVICE_HPP