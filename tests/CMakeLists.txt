# Be sure to clear these each time
set(GENERATED_TEST_LIST "" CACHE INTERNAL "")
set(GENERATED_FILTER_LIST "" CACHE INTERNAL "")
set(GENERATED_TEST_SOURCE_LIST "" CACHE INTERNAL "")

# The gtest include file directories.
set(GTEST_INCLUDE_DIRS
  "${CMAKE_CURRENT_SOURCE_DIR}/GoogleTest/include"
  )

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/GoogleTest")

# Alerts CMake to any tests we need to worry about
include_directories(${GTEST_INCLUDE_DIRS})

find_package(Threads)
if(CMAKE_USE_PTHREADS_INIT)  # The pthreads library is available.
  set(cxx_base_flags "${cxx_base_flags} -DGTEST_HAS_PTHREAD=1")
endif()

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")

include_directories(${SCRAM_CORE_INCLUDE_DIRS})

# To add a new file, just add it to this list.
# Any GoogleTests inside will be automatically added to ctest.
set(SCRAM_CORE_TEST_SOURCE ${SCRAM_CORE_TEST_SOURCE}
  "${CMAKE_CURRENT_SOURCE_DIR}/random_tests.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/xml_parser_tests.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/settings_tests.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/config_tests.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/element_tests.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/event_tests.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/expression_tests.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/ccf_group_tests.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/fault_tree_tests.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/boolean_graph_tests.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/preprocessor_tests.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/fault_tree_analysis_tests.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/probability_analysis_tests.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/uncertainty_analysis_tests.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/initializer_tests.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/risk_analysis_tests.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/grapher_tests.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/bench_core_tests.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/bench_two_train_tests.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/bench_lift_tests.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/bench_ne574_tests.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/bench_theatre_tests.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/bench_three_motor_tests.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/bench_200_event_tests.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/bench_small_tree_tests.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/bench_chinese_tree_tests.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/bench_baobab1_tests.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/bench_baobab2_tests.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/bench_CEA9601_tests.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/performance_tests.cc"
  )

# Make tests to switch and run in build directory
configure_file(scram_unit_test_driver.cc.in
  "${CMAKE_CURRENT_SOURCE_DIR}/scram_unit_test_driver.cc"
  @ONLY
  )

add_executable(scram_tests
  scram_unit_test_driver.cc
  ${SCRAM_CORE_TEST_SOURCE}
  )

target_link_libraries(scram_tests dl
  ${LIBS}
  scramcore
  gtest
  )

install(TARGETS scram_tests
  RUNTIME DESTINATION bin
  COMPONENT testing
  )

file(GLOB header_files "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
install(FILES ${header_files}
  DESTINATION include/scram
  COMPONENT testing
  )

# Read tests after building the driver, and add them to ctest.
set(tgt "scram_tests")
add_custom_command(TARGET ${tgt}
  POST_BUILD
  COMMAND python "${CMAKE_CURRENT_SOURCE_DIR}/generate_gtest_macros.py"
  "--executable=${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${tgt}"
  "--output=${SCRAM_BINARY_DIR}/CTestTestfile.cmake"
  COMMENT "adding tests from ${tgt}"
  DEPENDS
  VERBATIM
  )

# Copy test input files into the shared directory for testing.
install(DIRECTORY
  "${CMAKE_CURRENT_SOURCE_DIR}/input"
  DESTINATION share/scram
  COMPONENT testing
  )
