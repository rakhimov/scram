###############################################################################
#################################### begin google test ########################
###############################################################################

# Be sure to clear these each time
SET( GENERATED_TEST_LIST "" CACHE INTERNAL "" )
SET( GENERATED_FILTER_LIST "" CACHE INTERNAL "" )
SET( GENERATED_TEST_SOURCE_LIST "" CACHE INTERNAL "" )

# The gtest include file directories.
SET(GTEST_INCLUDE_DIRS 
  "${CMAKE_CURRENT_SOURCE_DIR}/GoogleTest"
  )

# The gtest library directories.
SET(GTEST_LIBRARY_DIRS "${CMAKE_CURRENT_BINARY_DIR}/lib")

# Alerts CMake to any tests we need to worry about
INCLUDE_DIRECTORIES(${GTEST_INCLUDE_DIRS})

FIND_PACKAGE(Threads)
IF(CMAKE_USE_PTHREADS_INIT)  # The pthreads library is available.
  SET(cxx_base_flags "${cxx_base_flags} -DGTEST_HAS_PTHREAD=1")
ENDIF()

# Build libgtest
SET( GTest
  "${CMAKE_CURRENT_SOURCE_DIR}/GoogleTest/gtest/gtest-all.cc"
  )

INCLUDE_DIRECTORIES("${CMAKE_CURRENT_SOURCE_DIR}")

ADD_LIBRARY(gtest ${GTest})

IF (CMAKE_USE_PTHREADS_INIT)
  TARGET_LINK_LIBRARIES(gtest ${CMAKE_THREAD_LIBS_INIT})
ENDIF()

SET_TARGET_PROPERTIES(gtest
  PROPERTIES INSTALL_NAME_DIR "${CMAKE_INSTALL_PREFIX}/lib"
  INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib"
)

INSTALL(TARGETS gtest
  LIBRARY DESTINATION lib
  COMPONENT testing
  )

INSTALL(FILES GoogleTest/gtest/gtest.h
  DESTINATION include/gtest
  COMPONENT testing
  )

###############################################################################
##################################### end google test #########################
###############################################################################

###############################################################################
################################### begin scram testing ######################
###############################################################################

INCLUDE_DIRECTORIES(${SCRAM_CORE_INCLUDE_DIRS})

ADD_SUBDIRECTORY(test_modules)

# To add a new file, just add it to this list.  Any GoogleTests inside will be automatically
# added to ctest.
set ( SCRAM_CORE_TEST_SOURCE ${SCRAM_CORE_TEST_SOURCE}
  # --
  )

ADD_EXECUTABLE( scram_unit_tests
  scram_unit_test_driver.cc 
  ${SCRAM_CORE_TEST_SOURCE}
  )

TARGET_LINK_LIBRARIES( scram_unit_tests dl 
  ${LIBS} 
  scramcore 
  gtest
  )

INSTALL(TARGETS scram_unit_tests
  RUNTIME DESTINATION bin
  COMPONENT testing
  )

FILE(GLOB header_files "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
INSTALL(FILES ${header_files}
  DESTINATION include/scram
  COMPONENT testing
  )

# read tests after building the driver, and add them to ctest
set( tgt "scram_unit_tests" )
add_custom_command(TARGET ${tgt}
  POST_BUILD
  COMMAND python "${CMAKE_CURRENT_SOURCE_DIR}/generate_test_macros.py"
          "--executable=${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${tgt}"
          "--output=${SCRAM_BINARY_DIR}/CTestTestfile.cmake"
  COMMENT "adding tests from ${tgt}"
  DEPENDS
  VERBATIM
  )

###############################################################################
#################################### end scram testing #######################
###############################################################################
