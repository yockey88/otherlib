/**
 * \file asset/asset.hpp
 **/
#ifndef OTHER_SCENE_ASSET_ASSET_HPP
#define OTHER_SCENE_ASSET_ASSET_HPP

#include "core/defines.hpp"

namespace other {

  struct asset {
    enum type {
      TEXTURE = 0,
      MODEL,
      SCRIPT,
      AUDIO,

      EMPTY,
      NUM_ASSET_TYPES = EMPTY,
    };

    type asset_type = type::EMPTY;

    natural_t id = 0;
    filepath path = "";
  };

}  // namespace other

#endif  // OTHER_SCENE_ASSET_ASSET_HPP