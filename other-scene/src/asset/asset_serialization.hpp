/**
 * \file asset/asset_serialization.hpp
 **/
#ifndef OTHER_SCENE_ASSET_ASSET_SERIALIZATION_HPP
#define OTHER_SCENE_ASSET_ASSET_SERIALIZATION_HPP

#include "core/defines.hpp"

#include "asset/asset_handler.hpp"

namespace other {
  namespace serialization {

    void write_asset_to_bytes(const asset_handler& handler, natural_t asset_id, ostd::vector<uint8_t>& out_bytes);

  }  // namespace serialization
}  // namespace other

#endif  // OTHER_SCENE_ASSET_ASSET_SERIALIZATION_HPP