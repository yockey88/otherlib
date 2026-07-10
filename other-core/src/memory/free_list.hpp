/**
 * \file memory/free_list.hpp
 **/
#ifndef OTHER_CORE_MEMORY_FREE_LIST_HPP
#define OTHER_CORE_MEMORY_FREE_LIST_HPP

namespace other {

  class free_list {
   public:
    static constexpr size_t kMinBlockLog2 = 5;
    static constexpr size_t kMinBlockSize = size_t{ 1 } << kMinBlockLog2;
    static constexpr size_t kNumBins = 20;

    constexpr static size_t bin_index(size_t total_size) {
      const size_t clamped = total_size < 32 ? 32 : total_size;
      return static_cast<size_t>(std::bit_width(clamped - 1)) - kMinBlockLog2;
    }
    constexpr static size_t bin_block_size(size_t bin) {
      return size_t{ 1 } << (bin + kMinBlockLog2);
    }

    void push(uint8_t bin_idx, void* block);
    void* pop(uint8_t bin_idx);

    size_t free_blocks(size_t bin) const;
    size_t idle_bytes() const;

    void cleanup();

   private:
    struct free_node {
      free_node* next;
    };

    free_node* bins[kNumBins] = {};
    size_t counts[kNumBins] = {};
  };

}  // namespace other

#endif  // OTHER_CORE_MEMORY_FREE_LIST_HPP