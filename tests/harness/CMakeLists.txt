## test harness driver: hosts long-running scenarios (soak, fuzz, stress, replay); its own
##  executable because it needs the full driver boot path, which other_tests stubs out
add_static_driver(other_soak
  src/harness_driver.cpp
  src/soak_scenario.cpp
  src/network_scenario.cpp
  src/fuzz_scenario.cpp
  src/stress_scenario.cpp
  src/memory_sampler.cpp
)
target_include_directories(other_soak PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
