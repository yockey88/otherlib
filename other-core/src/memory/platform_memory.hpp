/**
 * \file memory/platform_memory.hpp
 **/
#ifndef OTHER_CORE_MEMORY_PLATFORM_MEMORY_HPP
#define OTHER_CORE_MEMORY_PLATFORM_MEMORY_HPP

#include "core/logger.hpp"
#include "memory/page.hpp"

namespace other {

  class platform_memory {
   public:
    static constexpr size_t kMaxPages = 64;
    static constexpr size_t kReservation = kMaxPages * page::kPageSize;

    void initialize();
    void shutdown();

    uint8_t* create_new_page();

    size_t committed_pages() const { return next_page; }

   private:
    std::mutex platform_memory_mutex;

    void* base_ptr = nullptr;
    size_t next_page = 0;

    inline uint8_t* memory_array() {
      return static_cast<uint8_t*>(base_ptr == nullptr ? nullptr : base_ptr);
    }
    inline uint8_t* page_at(size_t page_index) {
      return memory_array() + (page_index * page::kPageSize);
    }
  };

}  // namespace other

#endif  // OTHER_CORE_MEMORY_PLATFORM_MEMORY_HPP