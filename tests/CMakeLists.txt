set(BUILD_SHARED_LIBS FALSE)  # No need to share with other executables.

# Exclude Debug warnings from the framework.
set(CURRENT_DEBUG_FLAGS "${CMAKE_CXX_FLAGS_DEBUG}")
set(CMAKE_CXX_FLAGS_DEBUG "-g")

set(GTEST_DIR  "${CMAKE_CURRENT_SOURCE_DIR}/googletest/googletest")

add_subdirectory("${GTEST_DIR}" EXCLUDE_FROM_ALL)

set(CMAKE_CXX_FLAGS_DEBUG "${CURRENT_DEBUG_FLAGS}")  # Restore the original flags.

include_directories(SYSTEM "${GTEST_DIR}/include")

# Include the project headers.
include_directories("${SCRAM_SOURCE_DIR}")

set(SCRAM_CORE_TEST_SOURCE
  "${CMAKE_CURRENT_SOURCE_DIR}/linear_map_tests.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/xml_stream_tests.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/settings_tests.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/config_tests.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/element_tests.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/event_tests.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/expression_tests.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/ccf_group_tests.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/fault_tree_tests.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/pdag_tests.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/initializer_tests.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/risk_analysis_tests.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/bench_core_tests.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/bench_two_train_tests.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/bench_lift_tests.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/bench_ne574_tests.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/bench_theatre_tests.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/bench_three_motor_tests.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/bench_200_event_tests.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/bench_small_tree_tests.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/bench_bscu_tests.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/bench_chinese_tree_tests.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/bench_baobab1_tests.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/bench_baobab2_tests.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/bench_CEA9601_tests.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/bench_hipps_tests.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/performance_tests.cc"
  )

# Make tests switch and run in install directory.
configure_file(scram_unit_test_driver.cc.in
  "${CMAKE_CURRENT_SOURCE_DIR}/scram_unit_test_driver.cc"
  @ONLY
  )

add_executable(scram_tests
  scram_unit_test_driver.cc
  ${SCRAM_CORE_TEST_SOURCE}
  )

target_link_libraries(scram_tests
  ${LIBS}
  scramcore
  gtest
  )

install(
  TARGETS scram_tests
  RUNTIME DESTINATION bin
  COMPONENT testing
  )

# Copy test input files into the shared directory for testing.
install(DIRECTORY
  "${CMAKE_CURRENT_SOURCE_DIR}/input"
  DESTINATION share/scram
  COMPONENT testing
  )
