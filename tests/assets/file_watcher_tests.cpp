/**
 * \file tests/assets/file_watcher_tests.cpp
 *
 * contract under test: file watchers debounce by mtime but keep firing across
 *  repeated modifications; directory watchers diff their subtree per poll
 *  (CREATED/DELETED) and domain filters prune excluded paths — the mechanism
 *  that keeps build output from re-entering the resolver.
 **/
#include <fstream>

#include <asio/asio.hpp>
#include <gtest/gtest.h>

#include "event/event_system.hpp"
#include "file/file_watcher.hpp"
#include "file/glob.hpp"

#include "other_test.hpp"

namespace other {

  class file_watcher_tests : public other_test {
   protected:
    asio::io_context io;
    filepath temp_root;
    std::unique_ptr<event_system> events;
    ostd::vector<file_event> received;

    void SetUp() override {
      other_test::SetUp();

      temp_root = std::filesystem::temp_directory_path() / "other-watcher-tests";
      std::filesystem::remove_all(temp_root);
      std::filesystem::create_directories(temp_root);

      events = std::make_unique<event_system>(io);
      events->register_event("filesystem.watch-event");
      events->add_listener("filesystem.watch-event", [this](const value& data) {
        received.push_back(static_cast<file_event>(data));
      });
    }

    void TearDown() override {
      other_test::TearDown();
      events = nullptr;
      std::error_code ec;
      std::filesystem::remove_all(temp_root, ec);
    }

    static void write_file(const filepath& path, std::string_view contents) {
      std::ofstream out(path);
      out << contents;
    }

    /// advances the file's mtime far past the 100ms debounce window without sleeping
    static void bump_write_time(const filepath& path, std::chrono::milliseconds forward) {
      const auto t = std::filesystem::last_write_time(path);
      std::filesystem::last_write_time(path, t + forward);
    }

    /// templated because file_event's member 'type' shadows the nested enum name
    //  in qualified type position; expression uses (file_event::type::CREATED) still resolve
    template <typename ET>
    size_t count_events(ET type_value) const {
      return std::ranges::count_if(received, [type_value](const file_event& e) { return e.type == type_value; });
    }
  };

  /// regression for the reloads-once class: every distinct modification past the
  /// debounce window must produce its own MODIFIED event
  TEST_F(file_watcher_tests, repeated_modifications_keep_firing) {
    const filepath target = temp_root / "watched.cs";
    write_file(target, "one");

    scope<file_watcher> watcher = file_watcher::make_file_watcher(*events, target);
    watcher->poll();
    ASSERT_EQ(count_events(file_event::type::MODIFIED), 0u);

    write_file(target, "two");
    bump_write_time(target, std::chrono::milliseconds(500));
    watcher->poll();
    EXPECT_EQ(count_events(file_event::type::MODIFIED), 1u);

    write_file(target, "three");
    bump_write_time(target, std::chrono::milliseconds(1000));
    watcher->poll();
    EXPECT_EQ(count_events(file_event::type::MODIFIED), 2u);
  }

  TEST_F(file_watcher_tests, debounce_swallows_rapid_writes) {
    const filepath target = temp_root / "watched.cs";
    write_file(target, "one");

    scope<file_watcher> watcher = file_watcher::make_file_watcher(*events, target);
    write_file(target, "two");
    bump_write_time(target, std::chrono::milliseconds(500));
    watcher->poll();
    ASSERT_EQ(count_events(file_event::type::MODIFIED), 1u);

    /// a write landing inside the 100ms window after the last accepted one is deferred
    write_file(target, "three");
    watcher->poll();
    EXPECT_EQ(count_events(file_event::type::MODIFIED), 1u);

    /// once the window passes, the pending change surfaces. the bump is measured from
    //  the fresh write's real mtime, so push far enough to clear the stored (future)
    //  timestamp from the first accepted write regardless of test wall-clock speed
    bump_write_time(target, std::chrono::milliseconds(2000));
    watcher->poll();
    EXPECT_EQ(count_events(file_event::type::MODIFIED), 2u);
  }

  TEST_F(file_watcher_tests, subtree_diff_emits_created_deleted) {
    std::filesystem::create_directories(temp_root / "scripts");
    write_file(temp_root / "scripts" / "existing.cs", "class E {}");

    scope<file_watcher> watcher = file_watcher::make_directory_watcher(*events, temp_root, file_watcher::watch_mode::RECURSIVE);
    watcher->poll();
    ASSERT_TRUE(received.empty());  /// baseline seeded in the ctor

    write_file(temp_root / "scripts" / "added.cs", "class A {}");
    watcher->poll();
    ASSERT_EQ(count_events(file_event::type::CREATED), 1u);
    EXPECT_EQ(received.back().path.filename().string(), "added.cs");

    std::filesystem::remove(temp_root / "scripts" / "existing.cs");
    watcher->poll();
    EXPECT_EQ(count_events(file_event::type::DELETED), 1u);

    /// steady state: no spurious events
    watcher->poll();
    EXPECT_EQ(received.size(), 2u);
  }

  TEST_F(file_watcher_tests, filter_prunes_excluded_subtrees) {
    std::filesystem::create_directories(temp_root / "scripts");
    std::filesystem::create_directories(temp_root / "bin" / "Debug");

    glob_set filter;
    filter.include("**/*.cs");
    filter.exclude({ "bin/**", "obj/**", ".*/**" });

    scope<file_watcher> watcher = file_watcher::make_directory_watcher(*events, temp_root, file_watcher::watch_mode::RECURSIVE);
    watcher->set_filter(&filter);
    watcher->poll();
    ASSERT_TRUE(received.empty());

    /// a build writing into bin/ must never surface — the rebuild-loop tripwire
    write_file(temp_root / "bin" / "Debug" / "Fake.dll", "binary");
    write_file(temp_root / "bin" / "Debug" / "generated.cs", "class G {}");
    watcher->poll();
    EXPECT_TRUE(received.empty());

    /// non-matching extensions are not the domain's business either
    write_file(temp_root / "scripts" / "notes.txt", "text");
    watcher->poll();
    EXPECT_TRUE(received.empty());

    write_file(temp_root / "scripts" / "real.cs", "class R {}");
    watcher->poll();
    EXPECT_EQ(count_events(file_event::type::CREATED), 1u);
  }

  TEST_F(file_watcher_tests, set_filter_rebaselines_silently) {
    std::filesystem::create_directories(temp_root / "bin");
    write_file(temp_root / "bin" / "stale.cs", "class S {}");
    write_file(temp_root / "kept.cs", "class K {}");

    /// the unfiltered baseline saw bin/stale.cs; installing the filter must not
    /// surface it as DELETED on the next poll
    scope<file_watcher> watcher = file_watcher::make_directory_watcher(*events, temp_root, file_watcher::watch_mode::RECURSIVE);
    glob_set filter;
    filter.include("**/*.cs");
    filter.exclude({ "bin/**" });
    watcher->set_filter(&filter);

    watcher->poll();
    EXPECT_TRUE(received.empty());
  }

}  // namespace other
