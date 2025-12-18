/**
 * \file scene/scene_network_context.hpp
 **/
#ifndef OTHER_SCENE_SCENE_SCENE_NETWORK_CONTEXT_HPP
#define OTHER_SCENE_SCENE_SCENE_NETWORK_CONTEXT_HPP

#include <set>

#include "core/defines.hpp"

namespace other {

  class scene_network_context {
   public:
    scene_network_context() = default;
    ~scene_network_context() = default;

    void add_remote_session(integer_t session_id);
    bool is_synchronizing() const;

   private:
    struct {
      bool synchronizing = false;
    } state;

    std::set<integer_t> remote_sessions;
  };

}  // namespace other

#endif  // OTHER_SCENE_SCENE_SCENE_NETWORK_CONTEXT_HPP