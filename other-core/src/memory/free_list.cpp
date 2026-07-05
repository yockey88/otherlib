/**
 * \file memory/free_list.cpp
 **/
#include "memory/free_list.hpp"

#include "memory/page.hpp"

namespace other {

  void free_list::push(uint8_t bin, void* block) {
    OTHER_ASSERT(bin < kNumBins, "free_list bin {} out of range.", bin);
    OTHER_ASSERT(block != nullptr, "free_list::push called with a null block.");
    OTHER_ASSERT((reinterpret_cast<uintptr_t>(block) & (page::kAlignment - 1)) == 0,
                 "free_list::push block {:p} is not {}-aligned — not a block start.", block, page::kAlignment);

    auto* node = static_cast<free_node*>(block);
    // #ifdef OTHER_MEMORY_DEBUG
    //     OTHER_ASSERT(node->free_magic != kFreeMagic,
    //                  "Double-free: block {:p} already carries the free-list stamp.", block);
    //     /// poison the payload so dangling reads crash loudly; the node header stays intact
    //     std::memset(static_cast<uint8_t*>(block) + sizeof(free_node), 0xDD,
    //                 bin_block_size(bin) - sizeof(free_node));
    //     node->free_magic = kFreeMagic;
    // #endif
    node->next = bins[bin];
    bins[bin] = node;
    counts[bin]++;
  }

  void* free_list::pop(uint8_t bin) {
    OTHER_ASSERT(bin < kNumBins, "free_list bin {} out of range.", bin);
    free_node* node = bins[bin];
    if (node == nullptr) {
      return nullptr;
    }
    // #ifdef OTHER_MEMORY_DEBUG
    //     /// a clobbered stamp here means something wrote through a dangling pointer into a freed
    //     /// block — caught at recycle time, at the exact block, before the corruption spreads
    //     OTHER_ASSERT(node->free_magic == kFreeMagic,
    //                  "Use-after-free write detected: head of bin {} ({:p}) lost its free-list stamp.", bin, static_cast<void*>(node));
    //     node->free_magic = 0;
    // #endif
    bins[bin] = node->next;
    counts[bin]--;
    return node;
  }

  size_t free_list::free_blocks(size_t bin) const {
    OTHER_ASSERT(bin < kNumBins, "free_list bin {} out of range.", bin);
    return counts[bin];
  }

  size_t free_list::idle_bytes() const {
    size_t total = 0;
    for (size_t bin = 0; bin < kNumBins; bin++) {
      total += counts[bin] * bin_block_size(bin);
    }
    return total;
  }

}  // namespace other