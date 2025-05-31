/**
 * \file memory/memory_sandbox.cpp
 **/
#include "memory_sandbox.hpp"

#include "core/defines.hpp"
#include "core/memory_pool.hpp"
#include "core/ref.hpp"

#include "simulation/scene_object.hpp"

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

void memory_sandbox::on_initialize() {
  CORE_LOG_INFO("Memory sandbox initialized.");
}

void memory_sandbox::run() {
  CORE_LOG_INFO("Running memory sandbox...");

  other::ref<test_ref> natural_value = other::make_ref<test_ref>();
  natural_value->print();
  {
    other::ref<test_ref> copied_value = natural_value;
    copied_value->print();
  }

  ref<value_storage> value_storage = make_ref<value_storage_impl<natural_t>>(42);
  CORE_LOG_INFO("Value storage created with size: {}", value_storage->size());
  CORE_LOG_INFO("Value storage type: {}", value_storage->val_type());

  natural_t* value_ptr = value_storage->unchecked_ptr_unwrap<natural_t>();
  CORE_LOG_INFO("Value pointer: {}", (void*)value_ptr);

  natural_t& value_ref = value_storage->unchecked_unwrap<natural_t>();
  CORE_LOG_INFO("Value reference: {}", value_ref);

  // other::value v1 = other::value{ other::natural_t{ 42 } };
  // other::natural_t n1 = v1;
  // CORE_LOG_INFO("Value: {}", n1);
}

void memory_sandbox::on_shutdown() {
  CORE_LOG_INFO("Memory sandbox shutdown.");
}