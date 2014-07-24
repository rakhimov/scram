# Be sure to clear these each time
set(GENERATED_TEST_LIST "" CACHE INTERNAL "")
set(GENERATED_FILTER_LIST "" CACHE INTERNAL "")
set(GENERATED_TEST_SOURCE_LIST "" CACHE INTERNAL "")

# The gtest include file directories.
set(GTEST_INCLUDE_DIRS
  "${CMAKE_CURRENT_SOURCE_DIR}/GoogleTest"
  )

# The gtest library directories.
set(GTEST_LIBRARY_DIRS "${CMAKE_CURRENT_BINARY_DIR}/lib")

# Alerts CMake to any tests we need to worry about
include_directories(${GTEST_INCLUDE_DIRS})

find_package(Threads)
if(CMAKE_USE_PTHREADS_INIT)  # The pthreads library is available.
  set(cxx_base_flags "${cxx_base_flags} -DGTEST_HAS_PTHREAD=1")
endif()

# Build libgtest
set(GTest
  "${CMAKE_CURRENT_SOURCE_DIR}/GoogleTest/gtest/gtest-all.cc"
  )

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")

add_library(gtest ${GTest})

if(CMAKE_USE_PTHREADS_INIT)
  target_link_libraries(gtest ${CMAKE_THREAD_LIBS_INIT})
endif()

set_target_properties(gtest
  PROPERTIES INSTALL_NAME_DIR "${CMAKE_INSTALL_PREFIX}/lib"
  INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib"
  )

install(TARGETS gtest
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  COMPONENT testing
  )

install(FILES GoogleTest/gtest/gtest.h
  DESTINATION include/gtest
  COMPONENT testing
  )


include_directories(${SCRAM_CORE_INCLUDE_DIRS})

# To add a new file, just add it to this list.  Any GoogleTests inside will be automatically
# added to ctest.
set(SCRAM_CORE_TEST_SOURCE ${SCRAM_CORE_TEST_SOURCE}
  "${CMAKE_CURRENT_SOURCE_DIR}/event_tests.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/superset_tests.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/fault_tree_input_tests.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/fault_tree_tests.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/bench_abc_tests.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/bench_abbc_tests.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/bench_not_gate_tests.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/bench_xor_tests.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/bench_lift_tests.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/bench_ne574_tests.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/bench_two_train_tests.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/bench_three_levels_tests.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/bench_trans_tests.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/bench_theatre_tests.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/bench_three_motor_tests.cc"
  )

# Make tests to switch and run in build directory
configure_file(scram_unit_test_driver.cc.in
  "${CMAKE_CURRENT_SOURCE_DIR}/scram_unit_test_driver.cc"
  @ONLY
  )

add_executable(scram_unit_tests
  scram_unit_test_driver.cc
  ${SCRAM_CORE_TEST_SOURCE}
  )

target_link_libraries(scram_unit_tests dl
  ${LIBS}
  scramcore
  gtest
  )

install(TARGETS scram_unit_tests
  RUNTIME DESTINATION bin
  COMPONENT testing
  )

file(GLOB header_files "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
install(FILES ${header_files}
  DESTINATION include/scram
  COMPONENT testing
  )

# read tests after building the driver, and add them to ctest
set(tgt "scram_unit_tests")
add_custom_command(TARGET ${tgt}
  POST_BUILD
  COMMAND python "${CMAKE_CURRENT_SOURCE_DIR}/generate_gtest_macros.py"
  "--executable=${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${tgt}"
  "--output=${SCRAM_BINARY_DIR}/CTestTestfile.cmake"
  COMMENT "adding tests from ${tgt}"
  DEPENDS
  VERBATIM
  )

# copy input files into build directory for testing
add_custom_command(TARGET ${tgt}
  POST_BUILD
  COMMAND cp "-r"
  "${CMAKE_CURRENT_SOURCE_DIR}/input"
  "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/"
  COMMENT "copying input files for unit testing"
  DEPENDS
  VERBATIM
  )
