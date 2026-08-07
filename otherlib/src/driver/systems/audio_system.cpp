/**
 * \file driver/systems/audio_system.cpp
 **/
#include "driver/systems/audio_system.hpp"

#include <glm/gtc/quaternion.hpp>

#include "audio/audio_environment.hpp"

#include "object/audio_listener_component.hpp"
#include "object/audio_source_component.hpp"
#include "scene/scene.hpp"

#include "driver/systems/asset_system.hpp"
#include "driver/systems/scene_system.hpp"

namespace other {

  namespace {

    /// explicit active listener -> "main-camera" tagged object -> origin
    void resolve_listener(scene* active_scene, audio_environment* env, double dt) {
      PROFILE_SECTION("resolve_listener");
      if (active_scene != nullptr) {
        bool found = false;
        active_scene->each_component<audio_listener_component>([&](const scene::object_handle& handle, audio_listener_component& listener) {
          if (found || !listener.active) {
            return;
          }
          found = true;

          const glm::mat4 world = active_scene->get_world_transform(handle.id);
          const glm::vec3 position = glm::vec3(world[3]);
          const glm::quat rotation = glm::quat_cast(glm::mat3(world));
          const glm::vec3 velocity = dt > 0.0 ? (position - listener.prev_position) / static_cast<float>(dt) : glm::vec3(0.f);
          listener.prev_position = position;
          env->set_listener(position, rotation, velocity);
        });
        if (found) {
          return;
        }

        if (const camera* primary = active_scene->get_primary_camera(); primary != nullptr) {
          const glm::vec3 direction = glm::normalize(primary->direction);
          env->set_listener(primary->position, glm::quatLookAt(direction, primary->up()), glm::vec3(0.f));
          return;
        }
      }
      env->set_listener(glm::vec3(0.f), glm::quat(1.f, 0.f, 0.f, 0.f), glm::vec3(0.f));
    }

  }  // namespace

  void reconcile_scene_audio(scene* active_scene, audio_environment* env, const asset_hash_fn& hash_of_asset, double dt, audio_reconcile_state& state) {
    OTHER_ASSERT(env != nullptr, "reconcile_scene_audio requires an audio environment");
    PROFILE_SECTION("reconcile_scene_audio");
    if (!env->is_initialized()) {
      state.bound.clear();
      return;
    }

    env->recycle_finished_one_shots();
    resolve_listener(active_scene, env, dt);

    if (active_scene == nullptr) {
      /// scene gone (unload/teardown): every bound voice's owner went with it
      for (const auto& [id, voice] : state.bound) {
        env->stop_voice(voice);
      }
      state.bound.clear();
      return;
    }

    const bool gameplay_on = active_scene->is_playing();
    ostd::map<natural_t, bool> seen;

    active_scene->each_component<audio_source_component>([&](const scene::object_handle& handle, audio_source_component& source) {
      seen[handle.id] = true;

      /// tree-id reuse guard (high-water pool wrap): a stale entry under this id
      ///  belongs to a destroyed owner — end its voice before this component binds
      if (const auto it = state.bound.find(handle.id); it != state.bound.end() && it->second != source.voice) {
        if (env->voice_alive(it->second)) {
          env->stop_voice(it->second);
        }
        state.bound.erase(it);
        source.voice = 0;
      }

      /// handles die under their holders when a clip reload/unload runs
      ///  stop_voices_on — cleanse before treating the source as bound
      if (source.voice != 0 && !env->voice_alive(source.voice)) {
        state.bound.erase(handle.id);
        source.voice = 0;
      }

      const natural_t clip_hash = source.clip_asset_id != 0 ? hash_of_asset(source.clip_asset_id) : 0;
      const audio_clip* clip = clip_hash != 0 ? env->get_clip(clip_hash) : nullptr;
      const bool desired = gameplay_on && source.playing && clip != nullptr;
      const bool bound = source.voice != 0;

      /// hot reload / clip swap detection: identity is the asset id + registry revision
      const bool identity_changed =
        bound && (source.bound_clip_id != source.clip_asset_id ||
                  source.bound_clip_revision != env->clip_revision(hash_of_asset(source.bound_clip_id)));

      if (bound && (!desired || identity_changed)) {
        env->stop_voice(source.voice);
        state.bound.erase(handle.id);
        source.voice = 0;
      }

      if (source.voice == 0 && desired) {
        const glm::mat4 world = active_scene->get_world_transform(handle.id);
        const glm::vec3 position = glm::vec3(world[3]);

        voice_params params{};
        params.clip_hash = clip_hash;
        params.volume = source.volume;
        params.pitch = source.pitch;
        params.looping = source.looping;
        params.bus = static_cast<audio_bus>(std::min<uint32_t>(source.bus, static_cast<uint32_t>(audio_bus::NUM_BUSES) - 1));
        params.spatial = source.spatial;
        params.min_distance = source.min_distance;
        params.max_distance = source.max_distance;
        params.doppler_factor = source.doppler_factor;
        params.position = position;

        source.voice = env->start_voice(params);
        if (source.voice != 0) {
          source.bound_clip_id = source.clip_asset_id;
          source.bound_clip_revision = env->clip_revision(clip_hash);
          source.prev_position = position;
          state.bound[handle.id] = source.voice;
        }
        return;
      }

      if (source.voice != 0) {
        if (env->voice_finished(source.voice)) {
          /// non-looping voice ran out: free the slot and flip the desired state
          ///  back — this writeback is the pollable completion signal for scripts
          env->stop_voice(source.voice);
          state.bound.erase(handle.id);
          source.voice = 0;
          source.playing = false;
          return;
        }

        const glm::mat4 world = active_scene->get_world_transform(handle.id);
        const glm::vec3 position = glm::vec3(world[3]);

        voice_dynamics dynamics{};
        dynamics.position = position;
        dynamics.velocity = dt > 0.0 ? (position - source.prev_position) / static_cast<float>(dt) : glm::vec3(0.f);
        dynamics.volume = source.volume;
        dynamics.pitch = source.pitch;
        source.prev_position = position;
        env->update_voice(source.voice, dynamics);
      }
    });

    /// sweep: owners that vanished since last tick (entity destroyed, component
    ///  removed, play/stop teardown-rebuild) leave entries no component claimed
    for (auto it = state.bound.begin(); it != state.bound.end();) {
      if (seen.find(it->first) == seen.end()) {
        if (env->voice_alive(it->second)) {
          env->stop_voice(it->second);
        }
        it = state.bound.erase(it);
      } else {
        ++it;
      }
    }
  }

  void audio_system::initialize(driver_kernel* kernel) {
    CORE_LOG_DEBUG("Audio system initialized (environment {})",
                   subsystem<audio_environment>::inert ? "inert" : "active");
  }

  void audio_system::tick(driver_kernel* kernel, double dt) {
    PROFILE_SECTION("audio_system::tick");
    if (subsystem<audio_environment>::inert) {
      return;
    }
    audio_environment* env = subsystem<audio_environment>::get();
    if (env == nullptr || !env->is_initialized()) {
      return;
    }

    scene* active_scene = nullptr;
    if (has_sibling<scene_system>(*kernel)) {
      active_scene = sibling<scene_system>(*kernel).get_active_scene();
    }

    asset_hash_fn hash_of_asset = [](natural_t) -> natural_t { return 0; };
    if (has_sibling<asset_system>(*kernel)) {
      asset_system& assets = sibling<asset_system>(*kernel);
      hash_of_asset = [&assets](natural_t asset_id) { return assets.get_asset_hash(asset_id); };
    }

    reconcile_scene_audio(active_scene, env, hash_of_asset, dt, reconcile_state);

    /// after the diff so voices started this frame advance this frame
    if (env->pump_mode()) {
      PROFILE_SECTION("audio_system::tick--pump");
      env->pump(dt);
    }
  }

  void audio_system::shutdown(driver_kernel* kernel) {
    PROFILE_SECTION("audio_system::shutdown");
    if (subsystem<audio_environment>::inert) {
      return;
    }
    audio_environment* env = subsystem<audio_environment>::get();
    if (env == nullptr || !env->is_initialized()) {
      return;
    }

    /// voices die here, while clips are still registered; the device itself closes
    ///  later in subsystem_registry::shutdown_all (reverse dependency order)
    env->stop_all_voices();
  }

}  // namespace other
