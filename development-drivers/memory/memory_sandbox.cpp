/**
 * \file memory/memory_sandbox.cpp
 **/
#include "memory_sandbox.hpp"

#include "core/defines.hpp"
#include "core/memory_pool.hpp"
#include "core/ref.hpp"

// #include "simulation/scene_object.hpp"

using other::natural_t;

using other::make_ref;
using other::ref;

using other::value_storage;
using other::value_storage_impl;

namespace {

  struct test_ref : public other::ref_counted {
    test_ref() {
      CORE_LOG_INFO("test_ref created.");
    }

    ~test_ref() {
      CORE_LOG_INFO("test_ref destroyed.");
    }

    void print() const {
      CORE_LOG_INFO("test_ref print called.");
    }
  };

}  // namespace

void memory_sandbox::on_initialize(const other::command_line& cmd) {
  CORE_LOG_INFO("Memory sandbox initialized.");
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

void memory_sandbox::run() {
  PROFILE_SECTION("memory_sandbox::run");

  constexpr size_t alloc_size = 1024;
  void* mem = nullptr;
  {
    PROFILE_SECTION("memory_sandbox::run--malloc-test");
    {
      PROFILE_SECTION("memory_sandbox::run--malloc-test-malloc");
      mem = malloc(alloc_size);
    }
    {
      PROFILE_SECTION("memory_sandbox::run--malloc-test-free");
      free(mem);
    }
  }

  mem = nullptr;
  {
    PROFILE_SECTION("memory_sandbox::run--new-test");
    {
      PROFILE_SECTION("memory_sandbox::run--new-test-new");
      mem = new char[alloc_size];
    }
    {
      PROFILE_SECTION("memory_sandbox::run--new-test-delete");
      delete[] static_cast<char*>(mem);
    }
  }

  mem = nullptr;
  {
    PROFILE_SECTION("memory_sandbox::run--arena-test");
    other::arena* arena_instance = other::subsystem<other::arena>::get();
    {
      PROFILE_SECTION("memory_sandbox::run--arena-test-arena");
      mem = arena_instance->allocate(alloc_size);
    }
    {
      PROFILE_SECTION("memory_sandbox::run--arena-test-free");
      arena_instance->free(mem, alloc_size);
    }
  }

  {
    PROFILE_SECTION("memory_sandbox::run--lots-of-malloc");
    std::vector<void*> allocations(100);
    {
      PROFILE_SECTION("memory_sandbox::run--lots-of-malloc-allocations");
      for (size_t i = 0; i < allocations.size(); ++i) {
        PROFILE_SECTION("memory_sandbox::run--lots-of-malloc-allocations--malloc");
        allocations[i] = malloc(alloc_size);
      }
    }
    {
      PROFILE_SECTION("memory_sandbox::run--lots-of-malloc-frees");
      for (void* ptr : allocations) {
        PROFILE_SECTION("memory_sandbox::run--lots-of-malloc-frees--free");
        free(ptr);
      }
    }
  }

  // {
  //   PROFILE_SECTION("memory_sandbox::run--lots-of-new");
  //   std::vector<void*> allocations(100);
  //   {
  //     PROFILE_SECTION("memory_sandbox::run--lots-of-new-allocations");
  //     for (size_t i = 0; i < allocations.size(); ++i) {
  //       PROFILE_SECTION("memory_sandbox::run--lots-of-new-allocations--new");
  //       allocations[i] = new char[alloc_size];
  //     }
  //   }
  //   {
  //     PROFILE_SECTION("memory_sandbox::run--lots-of-new-deletes");
  //     for (void* ptr : allocations) {
  //       PROFILE_SECTION("memory_sandbox::run--lots-of-new-deletes--delete");
  //       delete[] static_cast<char*>(ptr);
  //     }
  //   }
  // }

  {
    PROFILE_SECTION("memory_sandbox::run--arena-lots-of-allocations");
    std::vector<void*> allocations(100);
    other::arena* arena_instance = other::subsystem<other::arena>::get();
    {
      PROFILE_SECTION("memory_sandbox::run--arena-lots-of-allocations-arena");
      for (size_t i = 0; i < allocations.size(); ++i) {
        allocations[i] = arena_instance->allocate(alloc_size);
      }
    }
    {
      PROFILE_SECTION("memory_sandbox::run--arena-lots-of-allocations-frees");
      for (void* ptr : allocations) {
        arena_instance->free(ptr, alloc_size);
      }
    }
  }
}

void memory_sandbox::on_shutdown() {
  CORE_LOG_INFO("Memory sandbox shutdown.");
}
