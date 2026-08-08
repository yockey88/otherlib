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

  /// member of arena_test so it can reach arena::kAllocationTag through the test friendship
  void arena_test::verify_allocation_header(void* ptr, size_t size) {
    auto* header = static_cast<alloc_header*>(ptr) - 1;
    EXPECT_EQ(header->magic, arena::kAllocationTag) << "Allocation header magic is corrupt for " << ptr;
    EXPECT_EQ(header->user_size, size) << "Allocation header does not record the requested size.";
    EXPECT_EQ(header->bin, free_list::bin_index(size + arena::kHeaderSize)) << "Allocation header records the wrong bin.";
  }

  void arena_test::test_memory_boundaries(void* ptr, size_t size) {
    uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);

    arena* a = subsystem<arena>::get();
    page* page = a->get_current_page();
    EXPECT_NE(page, nullptr) << "Current page is null during memory boundary test.";

    /// free-list hits may come from older pages (suite shares one arena), so the
    ///  page-span property is only checkable for pointers the current page served
    const uintptr_t page_begin = reinterpret_cast<uintptr_t>(page->data());
    const uintptr_t page_end = page_begin + page::kPageSize;
    if (addr < page_begin || addr >= page_end) {
      return;
    }
    EXPECT_GE(addr, page_begin + arena::kHeaderSize) << "Pointer is inside the first block's header region.";
    EXPECT_LE(addr + size, page_end) << "Pointer payload exceeds page end address.";
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
    // TODO: allocate blocks per sizes pattern, fill w/ unique pattern, verify intact
    // TODO: test deallocation in various orders (LIFO, FIFO, random)
  }

  void arena_test::simulate_memory_pressure() {
    // TODO: allocate until pages are exhausted, verify OOM handling
    // TODO: test arena behavior under extreme memory pressure
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
    /// used_memory is a live gauge (decremented on free), so the only valid invariant
    ///  is against the blocks this test currently holds
    ASSERT_GE(a->used_memory, total_block_bytes) << "Total allocated memory does not match recorded allocations.";

    ASSERT_LE(a->allocated_memory, arena_storage::kMaxMemoryAllowed) << "Allocated memory exceeds maximum allowed limit.";
    ASSERT_LE(a->page_allocation_cursor, a->storage.kMaxPages) << "Page allocation cursor exceeds maximum number pages.";
    ASSERT_NE(a->get_current_page(), nullptr) << "Current page is null after verification.";

    auto* current_page = a->get_current_page();
    /// allocations may be served from recycled free-list bins rather than the bump
    ///  cursor, so the cursor carries no relation to this test's live bytes
    ASSERT_GE(current_page->cursor, 0) << "Current page cursor is negative.";
    ASSERT_LT(current_page->cursor, page::kPageSize) << "Current page cursor exceeds page size limit.";
  }

  /// allocation source (bump cursor vs recycled bin) depends on prior tests, so this checks
  ///  the allocation contract (header, counters, usability, recycling), not placement
  TEST_F(arena_test, basic_allocation) {
    arena* a = subsystem<arena>::get();
    ASSERT_NE(a, nullptr);

    const arena::stats before = a->get_stats();

    void* ptr = a->allocate(kTestBlockSize);
    ASSERT_NE(ptr, nullptr) << "Allocation failed for size " << kTestBlockSize;
    verify_alignment(ptr, kAlignment);
    verify_allocation_header(ptr, kTestBlockSize);

    uint64_t& value = *static_cast<uint64_t*>(ptr);
    value = 0xDEADBEEF;  // Fill with a test pattern
    EXPECT_EQ(value, 0xDEADBEEF);

    /// counter deltas are exact here (single-threaded, no other arena users between snapshots)
    const arena::stats allocated = a->get_stats();
    EXPECT_EQ(allocated.total_allocations, before.total_allocations + 1);
    EXPECT_EQ(allocated.live_allocations, before.live_allocations + 1);
    EXPECT_EQ(allocated.requested_memory, before.requested_memory + kTestBlockSize);
    EXPECT_EQ(allocated.used_memory, before.used_memory + expected_block_bytes(kTestBlockSize));

    a->free(ptr, kTestBlockSize);
    const arena::stats freed = a->get_stats();
    EXPECT_EQ(freed.total_allocations, before.total_allocations + 1) << "total_allocations is cumulative and must survive the free.";
    EXPECT_EQ(freed.live_allocations, before.live_allocations);
    EXPECT_EQ(freed.used_memory, before.used_memory);

    /// the free-list is LIFO, so the freed block sits at the head of its bin and the next
    ///  same-size allocation must recycle it
    void* recycled = a->allocate(kTestBlockSize);
    EXPECT_EQ(recycled, ptr) << "Freed block was not recycled for a same-bin allocation.";
    a->free(recycled, kTestBlockSize);
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

  /// regression: request_region used to lock arena_mutex and then call allocate(),
  ///  which locks it again — this test would hang instead of passing
  TEST_F(arena_test, request_region_returns_usable_memory) {
    arena* a = subsystem<arena>::get();
    ASSERT_NE(a, nullptr);

    void* region = a->request_region(kLargeBlockSize, kAlignment);
    ASSERT_NE(region, nullptr);
    verify_alignment(region, kAlignment);

    std::memset(region, 0xAB, kLargeBlockSize);
    a->free_region(region);
  }

}  // namespace other
