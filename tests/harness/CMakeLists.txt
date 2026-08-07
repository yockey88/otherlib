## test harness driver: hosts long-running engine scenarios (soak today; room for
##  fuzzing/replay/stress scenarios later). built alongside other_tests but is its
##  own executable because scenarios need the full driver boot path, which the unit
##  test binary deliberately stubs out.
add_static_driver(other_soak
  src/harness_driver.cpp
  src/soak_scenario.cpp
  src/network_scenario.cpp
  src/memory_sampler.cpp
)
target_include_directories(other_soak PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
