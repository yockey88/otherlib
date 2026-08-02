/**
 * \file tests/core/filesystem_tests.cpp
 *
 * contract under test: the engine-path query surface of file_system
 *  (open / file_exists / get_file, plus directory::get_file(hash)) resolves
 *  mount://file.ext and mount://sub/dir/file.ext paths against cold caches.
 **/
#include "core/filesystem_tests.hpp"

#include "core/fnv.hpp"
#include "event/event_system.hpp"

namespace other {

  namespace {

    file_system* mounted_fs(event_system& events, const filepath& root, std::string_view mount_name) {
      auto* fs = subsystem<file_system>::get();
      fs->initialize_file_events(events);
      fs->mount_directory(mount_name, root);
      return fs;
    }

  }  // namespace

  TEST_F(filesystem_tests, open_top_level_file) {
    asio::io_context io;
    event_system events{ io };
    auto* fs = mounted_fs(events, temp_root, kMountName);

    ref<file_handle> file = fs->open("fstest://root-file.txt");
    ASSERT_NE(file, nullptr);
    ASSERT_EQ(file->name(), "root-file");
    ASSERT_EQ(file->extension(), ".txt");
  }

  TEST_F(filesystem_tests, open_nested_file_on_cold_cache) {
    asio::io_context io;
    event_system events{ io };
    auto* fs = mounted_fs(events, temp_root, kMountName);

    /// mounting does not index subdirectories; open must still resolve through the disk
    ref<file_handle> nested = fs->open("fstest://sub/nested.txt");
    ASSERT_NE(nested, nullptr);
    ASSERT_EQ(nested->name(), "nested");

    ref<file_handle> leaf = fs->open("fstest://sub/deep/leaf.lua");
    ASSERT_NE(leaf, nullptr);
    ASSERT_EQ(leaf->extension(), ".lua");
  }

  TEST_F(filesystem_tests, open_failure_paths_return_null) {
    asio::io_context io;
    event_system events{ io };
    auto* fs = mounted_fs(events, temp_root, kMountName);

    ASSERT_EQ(fs->open("fstest://does-not-exist.txt"), nullptr);
    ASSERT_EQ(fs->open("fstest://sub/does-not-exist.txt"), nullptr);
    ASSERT_EQ(fs->open("nomount://root-file.txt"), nullptr);
    ASSERT_EQ(fs->open("no-separator-path.txt"), nullptr);
  }

  TEST_F(filesystem_tests, file_exists_for_engine_paths) {
    asio::io_context io;
    event_system events{ io };
    auto* fs = mounted_fs(events, temp_root, kMountName);

    ASSERT_TRUE(fs->file_exists("fstest://root-file.txt"));
    ASSERT_TRUE(fs->file_exists("fstest://sub/nested.txt"));
    ASSERT_TRUE(fs->file_exists("fstest://sub/deep/leaf.lua"));

    ASSERT_FALSE(fs->file_exists("fstest://missing.txt"));
    ASSERT_FALSE(fs->file_exists("fstest://sub/missing.txt"));
    ASSERT_FALSE(fs->file_exists("nomount://root-file.txt"));
  }

  TEST_F(filesystem_tests, get_file_registers_and_caches) {
    asio::io_context io;
    event_system events{ io };
    auto* fs = mounted_fs(events, temp_root, kMountName);

    ref<file_handle> first = fs->get_file("fstest://sub/nested.txt");
    ASSERT_NE(first, nullptr);

    /// the first lookup registers the file; the second must hit the cache
    ref<file_handle> second = fs->get_file("fstest://sub/nested.txt");
    ASSERT_NE(second, nullptr);
    ASSERT_EQ(first.raw_ptr(), second.raw_ptr());
  }

  TEST_F(filesystem_tests, directory_get_file_by_hash_scans_disk) {
    asio::io_context io;
    event_system events{ io };
    auto* fs = mounted_fs(events, temp_root, kMountName);

    ref<directory> mount = fs->get_mount(kMountName);
    ASSERT_NE(mount, nullptr);

    /// file_handle::hash() is FNV of the absolute path string
    filepath abs = std::filesystem::absolute(temp_root / "root-file.txt");
    natural_t hash = FNV(abs.string());

    ref<file_handle> file = mount->get_file(hash);
    ASSERT_NE(file, nullptr);
    ASSERT_EQ(file->hash(), hash);

    /// a miss must return null, not crash
    ASSERT_EQ(mount->get_file(FNV("this-path-does-not-exist")), nullptr);
  }

}  // namespace other
