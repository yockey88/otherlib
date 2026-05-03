/**
 * \file thread/messages.cpp
 **/
#include "thread/messages.hpp"

#include "core/arena.hpp"
#include "core/logger.hpp"

namespace other {

  // std::span<const uint8_t> other_message_spec::as_buffer() {
  //   if (data_ptr == nullptr || data_size == 0) {
  //     on_initialize_data();
  //   }
  //   return std::span<const uint8_t>(data_ptr, data_size);
  // }

  // uint8_t* other_message_spec::get_data_ptr() {
  //   OTHER_ASSERT(data_ptr != nullptr, "Data pointer is not initialized in other_message_spec.");
  //   return const_cast<uint8_t*>(data_ptr);
  // }

  // void other_message_spec::initialize_data_ptr(const uint8_t* ptr, size_t size) {
  //   void* mem = arena::allocate(size);
  //   auto bytes = std::span<const uint8_t>(ptr, size);
  //   std::memcpy(mem, bytes.data(), size);

  //   data_ptr = reinterpret_cast<const uint8_t*>(mem);
  //   data_size = size;
  // }

}  // namespace other