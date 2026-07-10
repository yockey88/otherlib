/**
 * \file tests/arena/arena_test.cpp
 */
#include "arena_test.hpp"

#include <random>

namespace other {

  static constexpr size_t expected_block_bytes(size_t user_size) {
    return free_list::bin_block_size(free_list::bin_index(user_size + arena::kHeaderSize));
  }

  namespace {

    // compile time tests
    constexpr size_t kTestBlockSize = 64;
    constexpr size_t kLargeBlockSize = 1024;
    static_assert(expected_block_bytes(kTestBlockSize) == 128, "64 + 16-byte header rounds to the 128 bin.");
    static_assert(expected_block_bytes(kLargeBlockSize) == 2048, "1024 + header rounds to the 2048 bin.");

    std::vector<size_t> generate_random_sizes(size_t count, size_t min_size, size_t max_size) {
      std::random_device rd;
      std::mt19937 gen{ rd() };
      std::uniform_int_distribution<size_t> dis(min_size, max_size);

      std::vector<size_t> sizes;
      sizes.reserve(count);
      for (size_t i = 0; i < count; ++i) {
        sizes.push_back(dis(gen));
      }
      return sizes;
    }

  }  // anonymous namespace

  void arena_test::print_current_page() {
    arena* a = subsystem<arena>::get();
    page* current_page = a->get_current_page();

    std::stringstream ss;
    if (current_page) {
      uint8_t* page_start = static_cast<uint8_t*>(current_page->data());
      for (size_t i = 0; i < current_page->cursor; ++i) {
        ss << std::format("0x{:02x} ", page_start[i]);
      }
      std::cout << "Current page data: " << ss.str() << std::endl;
    } else {
      std::cout << "No current page allocated." << std::endl;
    }
  }

  void arena_test::verify_alignment(void* ptr, size_t alignment) {
    uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
    EXPECT_EQ(addr % alignment, 0) << "Pointer is not aligned to " << alignment << " bytes: " << addr;
  }

  void arena_test::test_memory_boundaries(void* ptr, size_t size) {
    uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);

    arena* a = subsystem<arena>::get();
    page* page = a->get_current_page();
    EXPECT_NE(page, nullptr) << "Current page is null during memory boundary test.";
    EXPECT_GE(addr, reinterpret_cast<uintptr_t>(page->data()) + arena::kHeaderSize) << "Pointer is inside the first block's header region.";
    EXPECT_LE(addr + size, reinterpret_cast<uintptr_t>(page->data()) + page::kPageSize) << "Pointer payload exceeds page end address.";
  }

  void* arena_test::allocate_and_verify(size_t size) {
    void* ptr = subsystem<arena>::get()->allocate(size);
    EXPECT_NE(ptr, nullptr) << "Allocation failed for size " << size;

    allocations.push_back({ ptr, size, 0 });

    verify_alignment(ptr, kAlignment);
    test_memory_boundaries(ptr, size);
    verify_arena_state();
    return ptr;
  }

  void arena_test::test_allocation_pattern(const std::vector<size_t>& sizes) {
    // TODO: Allocate memory blocks according to sizes pattern
    // TODO: Fill each block with unique test pattern
    // TODO: Verify all patterns remain intact after all allocations
    // TODO: Test deallocation in various orders (LIFO, FIFO, random)
  }

  void arena_test::simulate_memory_pressure() {
    // TODO: Allocate memory until arena pages are exhausted
    // TODO: Verify proper handling of out-of-memory conditions
    // TODO: Test arena behavior under extreme memory pressure
  }

  void arena_test::verify_arena_state() {
    size_t total_allocated = 0;
    size_t total_block_bytes = 0;
    for (const auto& alloc : allocations) {
      total_allocated += alloc.size;
      total_block_bytes += expected_block_bytes(alloc.size);
    }

    arena* a = subsystem<arena>::get();

    /// restricted to testing >= b/c arena is used for some of the testing infrastructure
    //  that's probably a horrible idea and we should refactor but it is what it is for now
    ASSERT_GE(a->total_allocations, allocations.size()) << "Total allocations do not match recorded allocations.";
    ASSERT_GE(a->live_allocations, allocations.size()) << "Requested memory does not match recorded allocations.";
    ASSERT_GE(a->requested_memory, total_allocated) << "Requested memory does not match recorded allocations.";
    ASSERT_GE(a->used_memory, total_block_bytes) << "Total allocated memory does not match recorded allocations.";
    ASSERT_GE(a->used_memory, a->requested_memory + arena::kHeaderSize * allocations.size()) << "Total allocated memory does not match recorded allocations.";

    ASSERT_LE(a->allocated_memory, arena_storage::kMaxMemoryAllowed) << "Allocated memory exceeds maximum allowed limit.";
    ASSERT_LE(a->page_allocation_cursor, a->storage.kMaxPages) << "Page allocation cursor exceeds maximum number pages.";
    ASSERT_NE(a->get_current_page(), nullptr) << "Current page is null after verification.";

    auto* current_page = a->get_current_page();
    ASSERT_GE(current_page->cursor, 0) << "Current page cursor is negative.";
    ASSERT_GE(current_page->cursor, total_block_bytes) << "Current page cursor does not match total allocated block bytes.";
    ASSERT_LT(current_page->cursor, page::kPageSize) << "Current page cursor exceeds page size limit.";
  }

  TEST_F(arena_test, basic_allocation) {
    GTEST_SKIP()
      << "this test may need rewriting, the page may shift depending on order of execution of tests so the prealloc_cursor -> allocation_cursor check may not be valid anymore.";
    arena* a = subsystem<arena>::get();
    page* current_page = a->get_current_page();
    ASSERT_NE(current_page, nullptr) << "Current page is null after allocation.";

    size_t prealloc_cursor = current_page->cursor;
    size_t allocation_cursor = prealloc_cursor + arena::kHeaderSize;
    void* ptr = allocate_and_verify(kTestBlockSize);
    ASSERT_NE(ptr, nullptr) << "Allocation failed for size " << kTestBlockSize;
    /// make sure it is 16-byte aligned
    verify_alignment(ptr, kAlignment);

    const size_t expected_block_size = expected_block_bytes(kTestBlockSize);
    ASSERT_GE(current_page->cursor, expected_block_size) << "Current page cursor does not match allocation size.";

    uint64_t& value = *static_cast<uint64_t*>(ptr);
    value = 0xDEADBEEF;  // Fill with a test pattern

    EXPECT_EQ(current_page->get_ptr_at(allocation_cursor), ptr) << "Pointer does not match expected address in current page.";
    EXPECT_EQ(*(uint64_t*)current_page->get_ptr_at(allocation_cursor), value) << "Pointer does not match expected address in current page.";
  }

  TEST_F(arena_test, alignment_requirements) {
    void* ptr = allocate_and_verify(kTestBlockSize);
    ASSERT_NE(ptr, nullptr) << "Allocation failed for size " << kTestBlockSize;

    verify_alignment(ptr, kAlignment);
    test_memory_boundaries(ptr, kTestBlockSize);

    // Check that the pointer is aligned to the required alignment
    uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
    EXPECT_EQ(addr % kAlignment, 0) << "Pointer is not aligned to " << kAlignment << " bytes: " << addr;
  }

  TEST_F(arena_test, null_pointer_deallocation) {
    arena* a = subsystem<arena>::get();
    ASSERT_NO_THROW(a->free(nullptr, kTestBlockSize)) << "Deallocating null pointer should be perfectly safe.";
  }

  TEST_F(arena_test, arena_maintains_16byte_alignment_after_multiple_allocations) {
    const size_t num_allocations = 10;
    std::vector<void*> pointers;

    for (size_t i = 0; i < num_allocations; ++i) {
      void* ptr = allocate_and_verify(rand() % kTestBlockSize + 1);
      ASSERT_NE(ptr, nullptr) << "Allocation failed for size " << kTestBlockSize;
      pointers.push_back(ptr);
    }

    for (void* ptr : pointers) {
      verify_alignment(ptr, kAlignment);
    }
  }

  // TEST_F(arena_test, memory_pattern_integrity) {
  //   /// TODO;
  // }

  // TEST_F(arena_test, sequential_allocation_patterns) {
  //   // TODO: Test allocating many small blocks sequentially
  //   // TODO: Verify no memory fragmentation issues
  //   // TODO: Test allocation efficiency and performance
  // }

  // TEST_F(arena_test, interleaved_allocation_deallocation) {
  //   // TODO: Test interleaving allocation and deallocation operations
  //   // TODO: Verify arena handles complex allocation patterns
  //   // TODO: Test for memory fragmentation and efficiency
  // }

  // // Boundary and edge case tests
  // TEST_F(arena_test, page_boundary_allocations) {
  //   // TODO: Test allocations that cross page boundaries
  //   // TODO: Verify proper page management and allocation
  //   // TODO: Test edge cases at page size limits
  // }

  // TEST_F(arena_test, maximum_allocation_size) {
  //   // TODO: Test allocation of maximum allowed size
  //   // TODO: Verify behavior at arena capacity limits
  //   // TODO: Test allocation failure handling
  // }

  // TEST_F(arena_test, arena_exhaustion) {
  //   // TODO: Allocate memory until arena is completely full
  //   // TODO: Verify proper out-of-memory handling
  //   // TODO: Test arena recovery after memory is freed
  // }

  // // Concurrency and thread safety tests
  // TEST_F(arena_test, concurrent_allocations) {
  //   // TODO: Test allocation from multiple threads simultaneously
  //   // TODO: Verify thread safety of arena operations
  //   // TODO: Test for race conditions and data corruption
  // }

  // TEST_F(arena_test, concurrent_allocation_deallocation) {
  //   // TODO: Test mixed allocation/deallocation from multiple threads
  //   // TODO: Verify arena mutex protection works correctly
  //   // TODO: Test high-contention scenarios
  // }

  // // Performance and stress tests
  // TEST_F(arena_test, allocation_performance) {
  //   // TODO: Benchmark allocation performance
  //   // TODO: Compare with standard malloc/free performance
  //   // TODO: Verify arena provides expected performance benefits
  // }

  // TEST_F(arena_test, memory_fragmentation_resistance) {
  //   // TODO: Test arena's resistance to memory fragmentation
  //   // TODO: Simulate fragmentation-inducing allocation patterns
  //   // TODO: Verify arena can still allocate efficiently
  // }

  // TEST_F(arena_test, long_running_stress_test) {
  //   // TODO: Run extended stress test with random allocation patterns
  //   // TODO: Verify arena stability over long periods
  //   // TODO: Monitor for memory leaks or corruption over time
  // }

  // // Subsystem integration tests
  // TEST_F(arena_test, subsystem_initialization) {
  //   // TODO: Test arena subsystem initialization
  //   // TODO: Verify proper setup of subsystem storage
  //   // TODO: Test subsystem ptr() and address() methods
  // }

  // TEST_F(arena_test, subsystem_destruction) {
  //   // TODO: Test arena subsystem cleanup
  //   // TODO: Verify all allocated memory is properly freed
  //   // TODO: Test destructor behavior and resource cleanup
  // }

  // // Error condition tests
  // TEST_F(arena_test, invalid_pointer_deallocation) {
  //   // TODO: Test deallocation of pointers not allocated by arena
  //   // TODO: Verify arena can detect invalid pointers
  //   // TODO: Test error handling for corrupt pointer addresses
  // }

  // TEST_F(arena_test, arena_corruption_detection) {
  //   // TODO: Test arena's ability to detect internal corruption
  //   // TODO: Simulate various corruption scenarios
  //   // TODO: Verify arena fails safely when corrupted
  // }

  // TEST_F(arena_test, out_of_bounds_access_protection) {
  //   // TODO: Test protection against buffer overruns
  //   // TODO: Verify arena can detect out-of-bounds writes
  //   // TODO: Test guard pages or other protection mechanisms
  // }

}  // namespace other
