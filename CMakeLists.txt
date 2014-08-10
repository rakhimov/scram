cmake_minimum_required(VERSION 2.8)
# in-source build prevention
IF(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
    message(STATUS "Scram requires an out-of-source build.")
    message(STATUS "Please remove these files from ${CMAKE_BINARY_DIR} first:")
    message(STATUS "CMakeCache.txt")
    message(STATUS "CMakeFiles")
    message(STATUS "Once these files are removed, create a separate directory")
    message(STATUS "and run CMake from there")
    message(FATAL_ERROR "in-source build detected")
ENDIF()

project(SCRAM)

############################ begin cmake configuration ###################

# set project directories
set(PROJECT_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(SCRAM_SOURCE_DIR "${PROJECT_SOURCE_DIR}/src")
set(SCRAM_TEST_DIR "${PROJECT_SOURCE_DIR}/tests")

# This makes all the libraries build as SHARED
SET(BUILD_SHARED_LIBS true)

# Setup build locations.
if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${SCRAM_BINARY_DIR}/bin")
endif()
if(NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${SCRAM_BINARY_DIR}/lib")
endif()
if(NOT CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${SCRAM_BINARY_DIR}/lib")
endif()

set(SCRAM_EXECUTABLE_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

# use, i.e. don't skip the full RPATH for the build tree
set(CMAKE_SKIP_BUILD_RPATH  FALSE)

# when building, don't use the install RPATH already
# (but later on when installing)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)

set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")

# add the automatically determined parts of the RPATH
# which point to directories outside the build tree to the install RPATH
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# set configuration for all library builds
IF (WIN32)
    set(suffix ".dll")
ELSEIF(APPLE)
    set(CMAKE_SHARED_LINKER_FLAGS "-undefined dynamic_lookup")
    set(suffix ".dylib")
ELSE()
    set(suffix ".so")
ENDIF()

# enable testing, must be at top-level cmake file
OPTION( USE_TESTING "Build testing" ON )
IF( USE_TESTING )
    enable_testing()
    INCLUDE(CTest)
ENDIF()

############################ end cmake configuration ###################

############################ begin find libraries ######################

# Include the boost header files and the program_options library
# Please be sure to use Boost rather than BOOST.
# Capitalization matters on some platforms
set(Boost_USE_STATIC_LIBS       OFF)
set(Boost_USE_STATIC_RUNTIME    OFF)
# SET(Boost_USE_MULTITHREADED    OFF)
find_package(Boost COMPONENTS
  program_options filesystem system random
  REQUIRED)
message("--    Boost Root: ${Boost_ROOT}")
message("--    Boost Include directory: ${Boost_INCLUDE_DIR}")
message("--    Boost Library directories: ${Boost_LIBRARY_DIRS}")
set(LIBS ${LIBS} ${Boost_PROGRAM_OPTIONS_LIBRARY})
message("--    Boost Program Options location: ${Boost_PROGRAM_OPTIONS_LIBRARY}")
set(LIBS ${LIBS} ${Boost_SYSTEM_LIBRARY})
message("--    Boost System location: ${Boost_SYSTEM_LIBRARY}")
set(LIBS ${LIBS} ${Boost_FILESYSTEM_LIBRARY})
message("--    Boost Filesystem location: ${Boost_FILESYSTEM_LIBRARY}")

########################## end of find libraries ########################

########################## begin includes ###############################

# include all the directories we just found
include_directories("${Boost_INCLUDE_DIR}")
include_directories("${SCRAM_SOURCE_DIR}")

add_subdirectory("${SCRAM_SOURCE_DIR}")
add_subdirectory("${SCRAM_TEST_DIR}")
