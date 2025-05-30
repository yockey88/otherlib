/**
 * \file memory/memory_sandbox.cpp
 **/
#include "memory_sandbox.hpp"

#include "core/arena_allocator.hpp"

void memory_sandbox::on_initialize() {
  CORE_LOG_INFO("Memory sandbox initialized.");
}

void memory_sandbox::run() {
  CORE_LOG_INFO("Running memory sandbox...");

  // Example usage of arena allocator
  other::arena_allocator<int> allocator;
  int* arr = allocator.allocate(10);
  for (int i = 0; i < 10; ++i) {
    arr[i] = i * i;
  }

  CORE_LOG_INFO("Allocated array: ");
  for (int i = 0; i < 10; ++i) {
    CORE_LOG_INFO("arr[{}] = {}", i, arr[i]);
  }

  allocator.free(arr);
}

void memory_sandbox::on_shutdown() {
  CORE_LOG_INFO("Memory sandbox shutdown.");
}