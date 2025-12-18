/**
 * \file scene/scene_network_context.cpp
 **/
#include "scene/scene_network_context.hpp"

namespace other {

  void scene_network_context::add_remote_session(integer_t session_id) {
    if (!remote_sessions.contains(session_id)) {
      state.synchronizing = true;
    }

    remote_sessions.insert(session_id);
  }

  bool scene_network_context::is_synchronizing() const {
    return state.synchronizing;
  }

}  // namespace other