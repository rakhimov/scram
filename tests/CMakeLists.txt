set(BUILD_SHARED_LIBS FALSE)  # No need to share with other executables.

# Exclude Debug warnings from the framework.
set(CURRENT_DEBUG_FLAGS "${CMAKE_CXX_FLAGS_DEBUG}")
set(CMAKE_CXX_FLAGS_DEBUG "-g")

set(GTEST_DIR googletest/googletest)

add_subdirectory("${GTEST_DIR}" EXCLUDE_FROM_ALL)

set(CMAKE_CXX_FLAGS_DEBUG "${CURRENT_DEBUG_FLAGS}")  # Restore the original flags.

include_directories(SYSTEM "${GTEST_DIR}/include")

# Include the project headers.
include_directories("${CMAKE_SOURCE_DIR}/src")
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(SCRAM_CORE_TEST_SOURCE
  linear_map_tests.cc
  xml_stream_tests.cc
  settings_tests.cc
  config_tests.cc
  element_tests.cc
  event_tests.cc
  expression_tests.cc
  extern_function_tests.cc
  ccf_group_tests.cc
  fault_tree_tests.cc
  alignment_tests.cc
  pdag_tests.cc
  initializer_tests.cc
  risk_analysis_tests.cc
  serialization_tests.cc
  bench_core_tests.cc
  bench_two_train_tests.cc
  bench_lift_tests.cc
  bench_ne574_tests.cc
  bench_theatre_tests.cc
  bench_three_motor_tests.cc
  bench_200_event_tests.cc
  bench_small_tree_tests.cc
  bench_bscu_tests.cc
  bench_chinese_tree_tests.cc
  bench_baobab1_tests.cc
  bench_baobab2_tests.cc
  bench_CEA9601_tests.cc
  bench_hipps_tests.cc
  bench_attack.cc
  bench_gas_leak.cc
  performance_tests.cc
  )

# Make tests switch and run in install directory.
configure_file(scram_unit_test_driver.cc.in scram_unit_test_driver.cc @ONLY)

add_executable(scram_tests ${CMAKE_CURRENT_BINARY_DIR}/scram_unit_test_driver.cc ${SCRAM_CORE_TEST_SOURCE})

target_link_libraries(scram_tests ${LIBS} scramcore gtest)

install(
  TARGETS scram_tests
  RUNTIME DESTINATION bin
  COMPONENT testing
  )

# Copy test input files into the shared directory for testing.
install(DIRECTORY
  input
  DESTINATION share/scram
  COMPONENT testing
  )

# Create dummy library to test dynamic loading and extern function.
add_library(scram_dummy_extern SHARED scram_dummy_extern.cc)
if(WIN32)
  set(CMAKE_SHARED_LIBRARY_PREFIX "")
endif()
install(
  TARGETS scram_dummy_extern
  DESTINATION lib/scram/test
  COMPONENT testing
  )
