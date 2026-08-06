/**
 * \file tests/audio/audio_scene_tests.cpp
 *
 * desired-state reconciliation against a headless scene + pump-mode engine:
 * play/stop, completion writeback, hot-reload rebinding, entity deletion, and
 * the document-level scene -> audio manifest edge
 **/
#include <gtest/gtest.h>

#include "audio/audio_environment.hpp"

#include "object/audio_listener_component.hpp"
#include "object/audio_source_component.hpp"
#include "scene/scene.hpp"
#include "serialization/component_codec.hpp"

#include "driver/systems/audio_system.hpp"

#include "audio/audio_test_fixtures.hpp"
#include "other_test.hpp"

namespace other {

  namespace {

    /// tests key the registry directly by asset id — identity mapping
    const asset_hash_fn kIdentityHash = [](natural_t asset_id) -> natural_t { return asset_id; };

    constexpr natural_t kClipId = 0xC11A;

    audio_config pump_config() {
      audio_config config{};
      config.force_pump_mode = true;
      return config;
    }

  }  // namespace

  class audio_scene_tests : public other_test {
   protected:
    /// scene construction needs the physics/scripting side of the headless profile
    bool script_and_physics() const override { return true; }
  };

  TEST_F(audio_scene_tests, autoplay_source_sounds_only_while_playing) {
    audio_environment env;
    ASSERT_TRUE(env.initialize(pump_config()));
    env.add_clip(kClipId, make_test_clip(0.05f));

    scene s("Audio Reconcile");
    audio_reconcile_state state;
    scene_object& emitter = s.create_object("Emitter");
    audio_source_component source = {};
    source.clip_asset_id = kClipId;
    source.playing = true;
    source.looping = true;
    source.spatial = false;
    s.add_component<audio_source_component>(&emitter, std::move(source));

    /// edit mode: desired state reconciles toward silence
    reconcile_scene_audio(&s, &env, kIdentityHash, 1.0 / 60.0, state);
    EXPECT_EQ(env.live_voice_count(), 0u);

    s.play();
    reconcile_scene_audio(&s, &env, kIdentityHash, 1.0 / 60.0, state);
    EXPECT_EQ(env.live_voice_count(), 1u);
    audio_source_component* live = s.try_get_component<audio_source_component>(emitter.id);
    ASSERT_NE(live, nullptr);
    EXPECT_NE(live->voice, 0u);
    EXPECT_EQ(live->bound_clip_id, kClipId);

    /// stop() tears down and rebuilds from the snapshot; the rebuilt component has
    ///  no voice and the next diff clears the orphan
    s.stop();
    reconcile_scene_audio(&s, &env, kIdentityHash, 1.0 / 60.0, state);
    EXPECT_EQ(env.live_voice_count(), 0u);

    /// the fake asset id cannot survive the snapshot's id->path->id conversion in a
    ///  headless test (no asset system) — reassign it, as an editor edit would
    scene_object* revived = s.find_object(std::string_view{ "Emitter" });
    ASSERT_NE(revived, nullptr);
    audio_source_component* restored = s.try_get_component<audio_source_component>(revived->id);
    ASSERT_NE(restored, nullptr);
    restored->clip_asset_id = kClipId;

    /// play -> stop -> play works because nothing audio-specific entered the snapshot
    s.play();
    reconcile_scene_audio(&s, &env, kIdentityHash, 1.0 / 60.0, state);
    EXPECT_EQ(env.live_voice_count(), 1u);

    s.stop();
    reconcile_scene_audio(&s, &env, kIdentityHash, 1.0 / 60.0, state);
    env.remove_clip(kClipId);
    env.shutdown();
  }

  TEST_F(audio_scene_tests, finished_voice_writes_playing_back) {
    audio_environment env;
    ASSERT_TRUE(env.initialize(pump_config()));
    env.add_clip(kClipId, make_test_clip(0.1f, 48000));

    scene s("Audio Completion");
    audio_reconcile_state state;
    scene_object& emitter = s.create_object("OneShotEmitter");
    audio_source_component source = {};
    source.clip_asset_id = kClipId;
    source.playing = true;
    source.looping = false;
    source.spatial = false;
    s.add_component<audio_source_component>(&emitter, std::move(source));

    s.play();
    reconcile_scene_audio(&s, &env, kIdentityHash, 1.0 / 60.0, state);
    EXPECT_EQ(env.live_voice_count(), 1u);

    env.pump(0.25);  /// deterministic: run the 0.1s clip out
    reconcile_scene_audio(&s, &env, kIdentityHash, 1.0 / 60.0, state);

    audio_source_component* live = s.try_get_component<audio_source_component>(emitter.id);
    ASSERT_NE(live, nullptr);
    /// the pollable completion signal for scripts
    EXPECT_FALSE(live->playing);
    EXPECT_EQ(live->voice, 0u);
    EXPECT_EQ(env.live_voice_count(), 0u);

    s.stop();
    env.remove_clip(kClipId);
    env.shutdown();
  }

  TEST_F(audio_scene_tests, clip_reload_rebinds_voice_same_tick) {
    audio_environment env;
    ASSERT_TRUE(env.initialize(pump_config()));
    env.add_clip(kClipId, make_test_clip(0.05f));

    scene s("Audio Reload");
    audio_reconcile_state state;
    scene_object& emitter = s.create_object("Emitter");
    audio_source_component source = {};
    source.clip_asset_id = kClipId;
    source.playing = true;
    source.looping = true;
    source.spatial = false;
    s.add_component<audio_source_component>(&emitter, std::move(source));

    s.play();
    reconcile_scene_audio(&s, &env, kIdentityHash, 1.0 / 60.0, state);
    audio_source_component* live = s.try_get_component<audio_source_component>(emitter.id);
    ASSERT_NE(live, nullptr);
    const voice_id first_voice = live->voice;
    ASSERT_NE(first_voice, 0u);
    EXPECT_EQ(live->bound_clip_revision, 1u);

    /// hot reload: unload-then-load bumps the revision; the loader stops voices first
    env.stop_voices_on(kClipId);
    env.remove_clip(kClipId);
    env.add_clip(kClipId, make_test_clip(0.08f));

    reconcile_scene_audio(&s, &env, kIdentityHash, 1.0 / 60.0, state);
    live = s.try_get_component<audio_source_component>(emitter.id);
    ASSERT_NE(live, nullptr);
    EXPECT_NE(live->voice, 0u);
    EXPECT_NE(live->voice, first_voice);
    EXPECT_EQ(live->bound_clip_revision, 2u);
    EXPECT_EQ(env.live_voice_count(), 1u);

    s.stop();
    reconcile_scene_audio(&s, &env, kIdentityHash, 1.0 / 60.0, state);
    env.remove_clip(kClipId);
    env.shutdown();
  }

  TEST_F(audio_scene_tests, destroyed_entity_voice_is_swept) {
    audio_environment env;
    ASSERT_TRUE(env.initialize(pump_config()));
    env.add_clip(kClipId, make_test_clip(0.05f));

    scene s("Audio Deletion");
    audio_reconcile_state state;
    scene_object& emitter = s.create_object("Doomed");
    audio_source_component source = {};
    source.clip_asset_id = kClipId;
    source.playing = true;
    source.looping = true;
    source.spatial = false;
    s.add_component<audio_source_component>(&emitter, std::move(source));

    s.play();
    reconcile_scene_audio(&s, &env, kIdentityHash, 1.0 / 60.0, state);
    audio_source_component* live = s.try_get_component<audio_source_component>(emitter.id);
    ASSERT_NE(live, nullptr);
    const voice_id voice = live->voice;
    ASSERT_NE(voice, 0u);

    /// component (and entity) gone: nothing owns the voice anymore; the loader's
    ///  stop_voices_on covers clip unload, and scene teardown covers the rest —
    ///  here the component is removed while the scene lives, so the diff never
    ///  sees the source again and the voice must not leak past cleanup
    s.remove_component<audio_source_component>(emitter.id);
    reconcile_scene_audio(&s, &env, kIdentityHash, 1.0 / 60.0, state);
    EXPECT_EQ(env.live_voice_count(), 0u);

    s.stop();
    env.remove_clip(kClipId);
    env.shutdown();
  }

  TEST_F(audio_scene_tests, listener_and_one_shots_smoke) {
    audio_environment env;
    ASSERT_TRUE(env.initialize(pump_config()));
    env.add_clip(kClipId, make_test_clip(0.05f));

    scene s("Audio Listener");
    audio_reconcile_state state;
    scene_object& ears = s.create_object("Ears");
    s.add_component<audio_listener_component>(&ears);

    s.play();
    /// explicit listener resolves without incident; fallback paths (main-camera,
    ///  origin) are exercised by every other test in this file
    reconcile_scene_audio(&s, &env, kIdentityHash, 1.0 / 60.0, state);

    voice_params one_shot{};
    one_shot.clip_hash = kClipId;
    one_shot.spatial = true;
    one_shot.position = { 1.f, 0.f, 0.f };
    env.play_one_shot(one_shot);
    EXPECT_EQ(env.live_voice_count(), 1u);

    env.pump(0.2);  /// run it out
    env.recycle_finished_one_shots();
    EXPECT_EQ(env.live_voice_count(), 0u);

    s.stop();
    env.remove_clip(kClipId);
    env.shutdown();
  }

  TEST_F(audio_scene_tests, scene_manifest_collects_audio_clip_refs) {
    /// document-level, engine-free: an authored audio-source table produces an
    ///  AUDIO edge for the resolver's scene manifest
    const serialization::component_codec* codec = serialization::find_component_codec(FNV("audio-source"));
    ASSERT_NE(codec, nullptr);
    ASSERT_NE(codec->payload_from_toml, nullptr);
    ASSERT_NE(codec->collect_asset_refs, nullptr);

    toml::table table = toml::parse(R"(
clip_asset_id = "assets/audio/thruster_loop.wav"
playing = true
looping = true
)");
    ostd::vector<std::string> warnings;
    const ostd::vector<uint8_t> payload = codec->payload_from_toml(table, warnings);
    EXPECT_TRUE(warnings.empty());
    ASSERT_FALSE(payload.empty());

    ostd::vector<serialization::component_asset_ref> refs;
    codec->collect_asset_refs(payload, refs);
    ASSERT_EQ(refs.size(), 1u);
    EXPECT_EQ(refs[0].path, "assets/audio/thruster_loop.wav");
    EXPECT_EQ(refs[0].type, asset::AUDIO);
  }

}  // namespace other
