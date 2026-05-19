/**
 * \file object/audio_component.hpp
 **/
#ifndef OTHER_SCENE_AUDIO_COMPONENT_HPP
#define OTHER_SCENE_AUDIO_COMPONENT_HPP

#include "serialization/reflection.hpp"

#include "object/component.hpp"

#include "asset/asset.hpp"

namespace other {

  struct audio_component : public component {
    natural_t audio_asset_id = 0;

    audio_component()
        : component(component::AUDIO) {}

    void play();
    void stop();
  };

}  // namespace other

OTHER_REFLECT(
  other::audio_component,
  field(audio_asset_id, other::attr::serializable("Audio"), other::attr::asset_identifier_field(other::asset::AUDIO))
)

#endif  // OTHER_SCENE_AUDIO_COMPONENT_HPP